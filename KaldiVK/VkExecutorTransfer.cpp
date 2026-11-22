/**
 * @file VkExecutorTransfer.cpp
 * @brief Query, copy and readback handlers for the Vulkan executor.
 *
 * @section kaldivk_transfer_sync Readback and the synchronous submission model
 *
 * Copies are recorded into the command buffer like any other command. A readback
 * therefore submits and waits first (`flushForHostAccess()`), then maps the
 * host-visible buffer and hands the bytes to the callback. In a frames-in-flight
 * design the wait would move to a fence and the readback would be deferred; the
 * portable contract only promises that the callback runs before `process()`
 * returns, so the simple version is conforming.
 */
#include "VkCommandExecutor.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace Vk {

namespace {

/** @brief Size of mip level @p level on an axis of @p size texels. */
std::uint32_t mipExtent(std::uint32_t size, unsigned level) noexcept
{
	const std::uint32_t shifted = level >= 32 ? 0u : size >> level;
	return shifted == 0 ? 1u : shifted;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────────────────────

Kld::HalStatus CommandExecutor::createQueryPool(const Kld::OpCreateQueryPool& op)
{
	if (op.queryCount == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a query pool needs at least one slot");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "query pool handle is zero or already in use");
	}
	VkQueryPoolCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	info.queryType = op.type == Kld::QueryType::Timestamp ? VK_QUERY_TYPE_TIMESTAMP
							      : VK_QUERY_TYPE_OCCLUSION;
	info.queryCount = op.queryCount;
	VkQueryPoolObject object;
	object.type = op.type;
	object.count = op.queryCount;
	if (vkCreateQueryPool(context_.device(), &info, nullptr, &object.pool) != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::AllocationFailed, "vkCreateQueryPool failed");
	}
	queryPools_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::destroyQueryPool(const Kld::OpDestroyQueryPool& op)
{
	const auto found = queryPools_.find(op.id);
	if (found == queryPools_.end()) {
		return failInvalidHandle("query pool", op.id);
	}
	DeferredDestroy deferred {};
	deferred.queryPool_ = found->second.pool;
	defer(deferred);
	queryPools_.erase(found);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::resetQueryPool(const Kld::OpResetQueryPool& op)
{
	const VkQueryPoolObject* pool = queryPool(op.id);
	if (pool == nullptr) {
		return failInvalidHandle("query pool", op.id);
	}
	if (op.firstQuery > pool->count || op.queryCount > pool->count - op.firstQuery) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the query range is outside the pool");
	}
	// Vulkan 1.0 resets queries through the command buffer, never from the host.
	endActiveRenderPass();
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	vkCmdResetQueryPool(commandBuffer_, pool->pool, op.firstQuery, op.queryCount);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::beginQuery(const Kld::OpBeginQuery& op)
{
	const VkQueryPoolObject* pool = queryPool(op.id);
	if (pool == nullptr) {
		return failInvalidHandle("query pool", op.id);
	}
	if (pool->type != Kld::QueryType::Occlusion) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "only occlusion queries use begin/end; timestamps use WriteTimestamp");
	}
	if (op.query >= pool->count) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the query index is outside the pool");
	}
	Kld::HalStatus status = ensureRenderPass(boundFramebuffer_);
	if (!status.ok()) {
		return status;
	}
	vkCmdBeginQuery(commandBuffer_, pool->pool, op.query, 0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::endQuery(const Kld::OpEndQuery& op)
{
	const VkQueryPoolObject* pool = queryPool(op.id);
	if (pool == nullptr) {
		return failInvalidHandle("query pool", op.id);
	}
	if (pool->type != Kld::QueryType::Occlusion) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "only occlusion queries use begin/end; timestamps use WriteTimestamp");
	}
	if (op.query >= pool->count) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the query index is outside the pool");
	}
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	vkCmdEndQuery(commandBuffer_, pool->pool, op.query);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::writeTimestamp(const Kld::OpWriteTimestamp& op)
{
	const VkQueryPoolObject* pool = queryPool(op.id);
	if (pool == nullptr) {
		return failInvalidHandle("query pool", op.id);
	}
	if (pool->type != Kld::QueryType::Timestamp) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "the pool is not a timestamp pool");
	}
	if (op.query >= pool->count) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the query index is outside the pool");
	}
	// Timestamps are written outside a render pass so any stage mask is valid.
	endActiveRenderPass();
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	vkCmdWriteTimestamp(commandBuffer_,
			   static_cast<VkPipelineStageFlagBits>(toVkPipelineStageFlags(op.pipelineStage)),
			   pool->pool, op.query);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::readQueryResults(const Kld::OpReadQueryResults& op)
{
	const VkQueryPoolObject* pool = queryPool(op.queryPool);
	if (pool == nullptr) {
		return failInvalidHandle("query pool", op.queryPool);
	}
	if (op.callback == nullptr) {
		return fail(Kld::HalStatusCode::InvalidCommand, "ReadQueryResults needs a callback");
	}
	if (op.queryCount == 0 || op.firstQuery > pool->count
	    || op.queryCount > pool->count - op.firstQuery) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the query range is outside the pool");
	}
	const Kld::HalStatus status = flushForHostAccess();
	if (!status.ok()) {
		return status;
	}
	std::vector<std::uint64_t> values(op.queryCount, 0);
	const VkResult result = vkGetQueryPoolResults(
		context_.device(), pool->pool, op.firstQuery, op.queryCount,
		values.size() * sizeof(std::uint64_t), values.data(), sizeof(std::uint64_t),
		VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
	if (result != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::BackendError, "vkGetQueryPoolResults failed");
	}
	op.callback(values.data(), op.queryCount, op.userData);
	return Kld::HalOk;
}

// ─────────────────────────────────────────────────────────────────────────────
// Copies
// ─────────────────────────────────────────────────────────────────────────────

Kld::HalStatus CommandExecutor::copyBuffer(const Kld::OpCopyBuffer& op)
{
	VkBufferObject* source = buffer(op.source);
	VkBufferObject* destination = buffer(op.destination);
	if (source == nullptr) {
		return failInvalidHandle("buffer", op.source);
	}
	if (destination == nullptr) {
		return failInvalidHandle("buffer", op.destination);
	}
	if (op.size == 0) {
		return Kld::HalOk;
	}
	if (op.sourceOffset > source->size || op.size > source->size - op.sourceOffset
	    || op.destinationOffset > destination->size
	    || op.size > destination->size - op.destinationOffset) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the copy range is outside a buffer");
	}
	endActiveRenderPass();
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	const VkBufferCopy region { op.sourceOffset, op.destinationOffset, op.size };
	vkCmdCopyBuffer(commandBuffer_, source->buffer, destination->buffer, 1, &region);
	transferBarrier(commandBuffer_);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::copyBufferToImage2D(const Kld::OpCopyBufferToImage2D& op)
{
	VkBufferObject* source = buffer(op.sourceBuffer);
	VkTextureObject* destination = image(op.destinationImage);
	if (source == nullptr) {
		return failInvalidHandle("buffer", op.sourceBuffer);
	}
	if (destination == nullptr) {
		return failInvalidHandle("image", op.destinationImage);
	}
	if (op.width == 0 || op.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a copy needs a non-zero extent");
	}
	if (op.mipLevel >= destination->levels) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the mip level is outside the image");
	}
	const std::uint32_t maxLayer = destination->imageType == VK_IMAGE_TYPE_3D ? destination->depth
										     : destination->layers;
	if (op.layer >= maxLayer) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the layer is outside the image");
	}
	if (static_cast<std::uint32_t>(op.x) + op.width > mipExtent(destination->width, op.mipLevel)
	    || static_cast<std::uint32_t>(op.y) + op.height > mipExtent(destination->height, op.mipLevel)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the copy region is outside the image");
	}
	if (op.bufferOffset > source->size) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the buffer offset is outside the buffer");
	}
	const std::uint32_t texel = static_cast<std::uint32_t>(pixelByteSize(destination->euphemyFormat));
	if (texel == 0) {
		return failUnsupported("the image format has no known texel size");
	}
	endActiveRenderPass();
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	VkBufferImageCopy region {};
	region.bufferOffset = op.bufferOffset;
	region.bufferRowLength = op.bufferRowPitch != 0 ? op.bufferRowPitch / texel : 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = destination->aspect;
	region.imageSubresource.mipLevel = op.mipLevel;
	region.imageSubresource.baseArrayLayer = destination->imageType == VK_IMAGE_TYPE_3D ? 0
												 : op.layer;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { op.x, op.y, destination->imageType == VK_IMAGE_TYPE_3D ? op.layer : 0 };
	region.imageExtent = { op.width, op.height, 1 };
	vkCmdCopyBufferToImage(commandBuffer_, source->buffer, destination->image,
			       VK_IMAGE_LAYOUT_GENERAL, 1, &region);
	transferBarrier(commandBuffer_);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::copyImage2DToBuffer(const Kld::OpCopyImage2DToBuffer& op)
{
	VkTextureObject* source = image(op.sourceImage);
	VkBufferObject* destination = buffer(op.destinationBuffer);
	if (source == nullptr) {
		return failInvalidHandle("image", op.sourceImage);
	}
	if (destination == nullptr) {
		return failInvalidHandle("buffer", op.destinationBuffer);
	}
	if (op.width == 0 || op.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a copy needs a non-zero extent");
	}
	if (op.mipLevel >= source->levels) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the mip level is outside the image");
	}
	const std::uint32_t maxLayer = source->imageType == VK_IMAGE_TYPE_3D ? source->depth
									     : source->layers;
	if (op.layer >= maxLayer) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the layer is outside the image");
	}
	if (static_cast<std::uint32_t>(op.x) + op.width > mipExtent(source->width, op.mipLevel)
	    || static_cast<std::uint32_t>(op.y) + op.height > mipExtent(source->height, op.mipLevel)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the copy region is outside the image");
	}
	if (op.bufferOffset > destination->size) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the buffer offset is outside the buffer");
	}
	const std::uint32_t texel = static_cast<std::uint32_t>(pixelByteSize(source->euphemyFormat));
	if (texel == 0) {
		return failUnsupported("the image format has no known texel size");
	}
	endActiveRenderPass();
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	VkBufferImageCopy region {};
	region.bufferOffset = op.bufferOffset;
	region.bufferRowLength = op.bufferRowPitch != 0 ? op.bufferRowPitch / texel : 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = source->aspect;
	region.imageSubresource.mipLevel = op.mipLevel;
	region.imageSubresource.baseArrayLayer = source->imageType == VK_IMAGE_TYPE_3D ? 0 : op.layer;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { op.x, op.y, source->imageType == VK_IMAGE_TYPE_3D ? op.layer : 0 };
	region.imageExtent = { op.width, op.height, 1 };
	vkCmdCopyImageToBuffer(commandBuffer_, source->image, VK_IMAGE_LAYOUT_GENERAL,
			       destination->buffer, 1, &region);
	transferBarrier(commandBuffer_);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::copyImage2D(const Kld::OpCopyImage2D& op)
{
	VkTextureObject* source = image(op.sourceImage);
	VkTextureObject* destination = image(op.destinationImage);
	if (source == nullptr) {
		return failInvalidHandle("image", op.sourceImage);
	}
	if (destination == nullptr) {
		return failInvalidHandle("image", op.destinationImage);
	}
	if (source->format != destination->format) {
		return failUnsupported("an image copy needs matching source and destination formats");
	}
	if (op.width == 0 || op.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a copy needs a non-zero extent");
	}
	if (op.sourceMipLevel >= source->levels || op.destinationMipLevel >= destination->levels) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a mip level is outside an image");
	}
	if (op.sourceLayer >= source->layers || op.destinationLayer >= destination->layers) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a layer is outside an image");
	}
	if (static_cast<std::uint32_t>(op.sourceX) + op.width
		    > mipExtent(source->width, op.sourceMipLevel)
	    || static_cast<std::uint32_t>(op.sourceY) + op.height
		    > mipExtent(source->height, op.sourceMipLevel)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the copy region is outside the source");
	}
	if (static_cast<std::uint32_t>(op.destinationX) + op.width
		    > mipExtent(destination->width, op.destinationMipLevel)
	    || static_cast<std::uint32_t>(op.destinationY) + op.height
		    > mipExtent(destination->height, op.destinationMipLevel)) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the copy region is outside the destination");
	}
	endActiveRenderPass();
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	VkImageCopy region {};
	region.srcSubresource = { source->aspect, op.sourceMipLevel, op.sourceLayer, 1 };
	region.srcOffset = { op.sourceX, op.sourceY, 0 };
	region.dstSubresource = { destination->aspect, op.destinationMipLevel, op.destinationLayer, 1 };
	region.dstOffset = { op.destinationX, op.destinationY, 0 };
	region.extent = { op.width, op.height, 1 };
	vkCmdCopyImage(commandBuffer_, source->image, VK_IMAGE_LAYOUT_GENERAL, destination->image,
		       VK_IMAGE_LAYOUT_GENERAL, 1, &region);
	transferBarrier(commandBuffer_);
	return Kld::HalOk;
}

// ─────────────────────────────────────────────────────────────────────────────
// Readback
// ─────────────────────────────────────────────────────────────────────────────

Kld::HalStatus CommandExecutor::readBuffer(const Kld::OpReadBuffer& op)
{
	VkBufferObject* object = buffer(op.buffer);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	if (op.callback == nullptr || op.offset > object->size || op.size > object->size - op.offset) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the readback range or callback is invalid");
	}
	if (!object->hostVisible) {
		// Device-local buffer: copy through the staging ring and wait, so the
		// callback sees the GPU's bytes without a mapping the buffer does not have.
		Kld::HalStatus status = ensureRecording();
		if (!status.ok()) {
			return status;
		}
		StagingSlice slice;
		status = requestStaging(op.size, slice);
		if (!status.ok()) {
			return status;
		}
		recordBufferToStaging(*object, op.offset, op.size, slice);
		status = submitAndWait();
		if (!status.ok()) {
			return status;
		}
		invalidateStaging(slice);
		op.callback(slice.mapped, op.size, op.userData);
		return Kld::HalOk;
	}
	const Kld::HalStatus status = flushForHostAccess();
	if (!status.ok()) {
		return status;
	}
	if (object->mapped == nullptr) {
		return fail(Kld::HalStatusCode::BackendError, "the buffer has no host mapping");
	}
	(void)vmaInvalidateAllocation(memory_.handle(), object->allocation, 0, VK_WHOLE_SIZE);
	op.callback(static_cast<const std::uint8_t*>(object->mapped) + op.offset, op.size,
		    op.userData);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::readImage2D(const Kld::OpReadImage2D& op)
{
	VkTextureObject* object = image(op.image);
	if (object == nullptr) {
		return failInvalidHandle("image", op.image);
	}
	if (op.callback == nullptr || op.width == 0 || op.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the readback extent or callback is invalid");
	}
	if (object->compressed) {
		return failUnsupported("readback of block-compressed images is not implemented");
	}
	if ((object->aspect & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0) {
		return failUnsupported("readback of depth/stencil images is not implemented");
	}
	if (op.mipLevel >= object->levels) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the mip level is outside the image");
	}
	const std::uint32_t maxLayer = object->imageType == VK_IMAGE_TYPE_3D ? object->depth
									     : object->layers;
	if (op.layer >= maxLayer) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the layer is outside the image");
	}
	if (static_cast<std::uint32_t>(op.x) + op.width > mipExtent(object->width, op.mipLevel)
	    || static_cast<std::uint32_t>(op.y) + op.height > mipExtent(object->height, op.mipLevel)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the readback region is outside the image");
	}
	const std::uint32_t texel = static_cast<std::uint32_t>(pixelByteSize(object->euphemyFormat));
	if (texel == 0) {
		return failUnsupported("the image format has no known texel size");
	}
	// Vulkan requires the row pitch of a buffer image copy to be a multiple of 4.
	const std::uint32_t rowPitch = ((op.width * texel) + 3u) & ~3u;
	endActiveRenderPass();
	Kld::HalStatus status = ensureImageLayout(*object);
	if (status.ok()) {
		status = ensureRecording();
	}
	if (!status.ok()) {
		return status;
	}
	StagingSlice slice;
	status = requestStaging(rowPitch * op.height, slice);
	if (!status.ok()) {
		return status;
	}
	VkBufferImageCopy region {};
	region.bufferOffset = slice.offset;
	region.bufferRowLength = op.width;
	region.bufferImageHeight = op.height;
	region.imageSubresource.aspectMask = object->aspect;
	region.imageSubresource.mipLevel = op.mipLevel;
	region.imageSubresource.baseArrayLayer = object->imageType == VK_IMAGE_TYPE_3D ? 0 : op.layer;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { op.x, op.y, object->imageType == VK_IMAGE_TYPE_3D ? op.layer : 0 };
	region.imageExtent = { op.width, op.height, 1 };
	vkCmdCopyImageToBuffer(commandBuffer_, object->image, VK_IMAGE_LAYOUT_GENERAL, slice.buffer,
			       1, &region);
	status = flushForHostAccess();
	if (!status.ok()) {
		return status;
	}
	invalidateStaging(slice);
	op.callback(slice.mapped, rowPitch * op.height, op.width, op.height, rowPitch, op.userData);
	return Kld::HalOk;
}

}
