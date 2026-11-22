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

/** @brief Rounds @p value up to a multiple of 4, the copy alignment Vulkan wants. */
std::uint32_t alignUp4(std::uint32_t value) noexcept
{
	return (value + 3u) & ~3u;
}

/** @brief Bytes per texel of one aspect of @p format, or @c 0 when it has none. */
std::uint32_t aspectTexelSize(VkFormat format, bool stencil) noexcept
{
	if (stencil) {
		// `S8_UINT` and the stencil aspect of every combined format are one byte.
		return 1;
	}
	switch (format) {
		case VK_FORMAT_D16_UNORM: return 2;
		case VK_FORMAT_X8_D24_UNORM_PACK32:
		case VK_FORMAT_D32_SFLOAT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT: return 4;
		default: return 0;
	}
}

/**
 * @brief Meaningful bytes of the depth component in one source texel.
 *
 * `X8_D24` and the depth half of `D24S8` keep 24 bits in a 32-bit texel, so the
 * padding byte must not leak into the portable value.
 */
std::uint32_t depthComponentBytes(VkFormat format) noexcept
{
	switch (format) {
		case VK_FORMAT_D16_UNORM: return 2;
		case VK_FORMAT_X8_D24_UNORM_PACK32:
		case VK_FORMAT_D24_UNORM_S8_UINT: return 3;
		default: return 4;
	}
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
		// Depth and stencil live in separate aspects; Vulkan copies one at a time.
		return readDepthStencilImage2D(op, *object);
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

Kld::HalStatus CommandExecutor::readDepthStencilImage2D(const Kld::OpReadImage2D& op,
							VkTextureObject& object)
{
	const bool hasDepth = (object.aspect & VK_IMAGE_ASPECT_DEPTH_BIT) != 0;
	const bool hasStencil = (object.aspect & VK_IMAGE_ASPECT_STENCIL_BIT) != 0;
	const std::uint32_t depthTexel = hasDepth ? aspectTexelSize(object.format, false) : 0;
	if (hasDepth && depthTexel == 0) {
		return failUnsupported("the depth format has no known texel size");
	}
	const std::uint32_t portableTexel =
		static_cast<std::uint32_t>(pixelByteSize(object.euphemyFormat));
	if (portableTexel == 0) {
		return failUnsupported("the image format has no known texel size");
	}
	// Not every depth format is a legal copy source, and the ones that are not report
	// it through the format properties instead of producing garbage.
	VkFormatProperties properties {};
	vkGetPhysicalDeviceFormatProperties(context_.physicalDevice(), object.format, &properties);
	if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT) == 0) {
		return failUnsupported("the depth/stencil format cannot be copied on this device");
	}
	// Vulkan requires every row pitch of a buffer image copy to be a multiple of 4.
	const std::uint32_t portablePitch = alignUp4(op.width * portableTexel);
	const std::uint32_t depthPitch = hasDepth ? alignUp4(op.width * depthTexel) : 0;
	const std::uint32_t stencilPitch = hasStencil ? alignUp4(op.width) : 0;
	endActiveRenderPass();
	Kld::HalStatus status = ensureImageLayout(object);
	if (status.ok()) {
		status = ensureRecording();
	}
	if (!status.ok()) {
		return status;
	}
	// One slice per plane, so each copy starts at an aligned offset.
	StagingSlice depthSlice;
	StagingSlice stencilSlice;
	if (hasDepth) {
		status = requestStaging(depthPitch * op.height, depthSlice);
		if (!status.ok()) {
			return status;
		}
	}
	if (hasStencil) {
		status = requestStaging(stencilPitch * op.height, stencilSlice);
		if (!status.ok()) {
			return status;
		}
	}
	VkBufferImageCopy base {};
	base.bufferImageHeight = op.height;
	base.imageSubresource.mipLevel = op.mipLevel;
	base.imageSubresource.baseArrayLayer = object.imageType == VK_IMAGE_TYPE_3D ? 0 : op.layer;
	base.imageSubresource.layerCount = 1;
	base.imageOffset = { op.x, op.y, object.imageType == VK_IMAGE_TYPE_3D ? op.layer : 0 };
	base.imageExtent = { op.width, op.height, 1 };
	if (hasDepth) {
		VkBufferImageCopy region = base;
		region.bufferOffset = depthSlice.offset;
		region.bufferRowLength = depthPitch / depthTexel;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		vkCmdCopyImageToBuffer(commandBuffer_, object.image, VK_IMAGE_LAYOUT_GENERAL,
				       depthSlice.buffer, 1, &region);
	}
	if (hasStencil) {
		VkBufferImageCopy region = base;
		region.bufferOffset = stencilSlice.offset;
		region.bufferRowLength = stencilPitch;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
		vkCmdCopyImageToBuffer(commandBuffer_, object.image, VK_IMAGE_LAYOUT_GENERAL,
				       stencilSlice.buffer, 1, &region);
	}
	status = flushForHostAccess();
	if (!status.ok()) {
		return status;
	}
	if (hasDepth) {
		invalidateStaging(depthSlice);
	}
	if (hasStencil) {
		invalidateStaging(stencilSlice);
	}
	// Interleave the planes into the portable texel: depth bytes first, then the
	// stencil byte (when the format has one). Padding bytes stay zero, so an `X8_D24`
	// source never leaks its undefined byte into a 32-bit depth value.
	const auto* depthPlane = hasDepth ? static_cast<const std::uint8_t*>(depthSlice.mapped)
					  : nullptr;
	const auto* stencilPlane = hasStencil ? static_cast<const std::uint8_t*>(stencilSlice.mapped)
					      : nullptr;
	const std::uint32_t sourceDepthBytes = hasDepth ? depthComponentBytes(object.format) : 0;
	const std::uint32_t packedDepthBytes =
		hasStencil ? portableTexel - 1 : std::min(portableTexel, sourceDepthBytes);
	const std::uint32_t copyDepthBytes = std::min(packedDepthBytes, sourceDepthBytes);
	std::vector<std::uint8_t> packed(static_cast<std::size_t>(portablePitch) * op.height, 0);
	for (std::uint32_t row = 0; row < op.height; ++row) {
		std::uint8_t* destination = packed.data() + static_cast<std::size_t>(row) * portablePitch;
		for (std::uint32_t column = 0; column < op.width; ++column) {
			std::uint8_t* pixel = destination + static_cast<std::size_t>(column) * portableTexel;
			if (hasDepth) {
				std::memcpy(pixel,
					    depthPlane + static_cast<std::size_t>(row) * depthPitch
						    + static_cast<std::size_t>(column) * depthTexel,
					    copyDepthBytes);
			}
			if (hasStencil) {
				pixel[portableTexel - 1] =
					stencilPlane[static_cast<std::size_t>(row) * stencilPitch + column];
			}
		}
	}
	op.callback(packed.data(), static_cast<std::uint32_t>(packed.size()), op.width, op.height,
		    portablePitch, op.userData);
	return Kld::HalOk;
}

}
