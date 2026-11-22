/**
 * @file VkExecutorResources.cpp
 * @brief Buffer, texture, image, sampler and framebuffer handlers for the Vulkan executor.
 *
 * @section kaldivk_resources_layout Image layout
 *
 * Every image the executor owns lives in `VK_IMAGE_LAYOUT_GENERAL` for its whole
 * life. `GENERAL` is the only layout that is simultaneously valid as a colour or
 * depth/stencil attachment, as a sampled image, as a storage image and as a copy
 * source/destination, so a single transition per image after creation removes an
 * entire class of per-command layout bookkeeping. The cost is some loss of
 * optimal-layout tiling on some drivers; a production backend should track the
 * portable `Kld::ResourceState` per subresource instead.
 *
 * @section kaldivk_resources_upload Uploads
 *
 * Uploads go through a temporary `HOST_VISIBLE` staging buffer recorded into the
 * same command buffer. The buffer is retired through the executor's deferred list,
 * so it is destroyed only after the queue is idle.
 */
#include "VkCommandExecutor.hpp"
#include <algorithm>
#include <cstring>
#include <map>
#include <vector>

namespace Vk {

namespace {

/** @brief Size of mip level @p level on an axis of @p size texels. */
std::uint32_t mipExtent(std::uint32_t size, unsigned level) noexcept
{
	const std::uint32_t shifted = level >= 32 ? 0u : size >> level;
	return shifted == 0 ? 1u : shifted;
}

/** @brief Number of mip levels a full chain of @p size texels has. */
std::uint32_t fullMipCount(std::uint32_t size) noexcept
{
	std::uint32_t levels = 1;
	while (size > 1) {
		size >>= 1;
		++levels;
	}
	return levels;
}

/** @brief Bytes of one block of a block-compressed format, or @c 0. */
std::uint32_t blockByteSize(Kld::TextureFormat format) noexcept
{
	switch (format) {
		case Kld::TextureFormat::B_DXT1: return 8;
		case Kld::TextureFormat::B_DXT3:
		case Kld::TextureFormat::B_DXT5: return 16;
		default: return 0;
	}
}

/** @brief Vulkan sample count for a Kaldi sample count. */
VkSampleCountFlagBits sampleCount(std::uint8_t samples) noexcept
{
	switch (samples) {
		case 2: return VK_SAMPLE_COUNT_2_BIT;
		case 4: return VK_SAMPLE_COUNT_4_BIT;
		case 8: return VK_SAMPLE_COUNT_8_BIT;
		case 16: return VK_SAMPLE_COUNT_16_BIT;
		case 32: return VK_SAMPLE_COUNT_32_BIT;
		case 64: return VK_SAMPLE_COUNT_64_BIT;
		default: return VK_SAMPLE_COUNT_1_BIT;
	}
}

/** @brief Rounds @p value up to the next multiple of @p alignment. */
std::uint32_t alignUp(std::uint32_t value, std::uint32_t alignment) noexcept
{
	if (alignment == 0) {
		return value;
	}
	return ((value + alignment - 1) / alignment) * alignment;
}

/** @brief Destroys the Vulkan objects of a partially or fully built image. */
void destroyImageObjects(VkDevice device, VkTextureObject& object) noexcept
{
	if (object.view != VK_NULL_HANDLE) {
		vkDestroyImageView(device, object.view, nullptr);
	}
	if (object.image != VK_NULL_HANDLE) {
		vkDestroyImage(device, object.image, nullptr);
	}
	if (object.memory != VK_NULL_HANDLE) {
		vkFreeMemory(device, object.memory, nullptr);
	}
	object = VkTextureObject {};
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Buffers
// ─────────────────────────────────────────────────────────────────────────────

Kld::HalStatus CommandExecutor::createHostBuffer(std::uint32_t size, VkBuffer& buffer,
						 VkDeviceMemory& memory, void*& mapped, bool& coherent)
{
	VkBufferCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = size == 0 ? 4 : size;
	info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(context_.device(), &info, nullptr, &buffer) != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::AllocationFailed, "vkCreateBuffer failed");
	}
	VkMemoryRequirements requirements {};
	vkGetBufferMemoryRequirements(context_.device(), buffer, &requirements);
	std::uint32_t typeIndex = findMemoryType(requirements.memoryTypeBits,
						 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
							 | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	coherent = typeIndex != UINT32_MAX;
	if (!coherent) {
		typeIndex = findMemoryType(requirements.memoryTypeBits,
					   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	}
	if (typeIndex == UINT32_MAX) {
		vkDestroyBuffer(context_.device(), buffer, nullptr);
		buffer = VK_NULL_HANDLE;
		return fail(Kld::HalStatusCode::AllocationFailed,
			    "no host-visible memory type for a staging buffer");
	}
	VkMemoryAllocateInfo allocate {};
	allocate.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate.allocationSize = requirements.size;
	allocate.memoryTypeIndex = typeIndex;
	if (vkAllocateMemory(context_.device(), &allocate, nullptr, &memory) != VK_SUCCESS) {
		vkDestroyBuffer(context_.device(), buffer, nullptr);
		buffer = VK_NULL_HANDLE;
		return fail(Kld::HalStatusCode::AllocationFailed,
			    "vkAllocateMemory failed for a staging buffer");
	}
	if (vkBindBufferMemory(context_.device(), buffer, memory, 0) != VK_SUCCESS
	    || vkMapMemory(context_.device(), memory, 0, VK_WHOLE_SIZE, 0, &mapped) != VK_SUCCESS) {
		destroyBufferObjects(buffer, memory, mapped);
		return fail(Kld::HalStatusCode::BackendError, "the staging buffer could not be mapped");
	}
	return Kld::HalOk;
}

void CommandExecutor::destroyBufferObjects(VkBuffer& buffer, VkDeviceMemory& memory,
					   void*& mapped) noexcept
{
	if (mapped != nullptr) {
		vkUnmapMemory(context_.device(), memory);
		mapped = nullptr;
	}
	if (buffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(context_.device(), buffer, nullptr);
		buffer = VK_NULL_HANDLE;
	}
	if (memory != VK_NULL_HANDLE) {
		vkFreeMemory(context_.device(), memory, nullptr);
		memory = VK_NULL_HANDLE;
	}
}

Kld::HalStatus CommandExecutor::createBuffer(const Kld::OpCreateBufferObject& op)
{
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "buffer handle is zero or already in use");
	}
	VkBufferObject object;
	object.size = op.size;
	VkBufferCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = op.size == 0 ? 4 : op.size;
	info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if ((op.policy & Kld::BUFFER_USAGE_VERTEX) != 0) info.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if ((op.policy & Kld::BUFFER_USAGE_INDEX) != 0) info.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if ((op.policy & Kld::BUFFER_USAGE_UNIFORM) != 0) info.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if ((op.policy & Kld::BUFFER_USAGE_STORAGE) != 0) info.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if ((op.policy & Kld::BUFFER_USAGE_INDIRECT) != 0) info.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(context_.device(), &info, nullptr, &object.buffer) != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::AllocationFailed, "vkCreateBuffer failed");
	}
	VkMemoryRequirements requirements {};
	vkGetBufferMemoryRequirements(context_.device(), object.buffer, &requirements);
	std::uint32_t typeIndex = findMemoryType(requirements.memoryTypeBits,
						 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
							 | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	object.coherent = typeIndex != UINT32_MAX;
	if (!object.coherent) {
		typeIndex = findMemoryType(requirements.memoryTypeBits,
					   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	}
	if (typeIndex == UINT32_MAX) {
		vkDestroyBuffer(context_.device(), object.buffer, nullptr);
		return fail(Kld::HalStatusCode::AllocationFailed, "no host-visible memory type for the buffer");
	}
	VkMemoryAllocateInfo allocate {};
	allocate.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate.allocationSize = requirements.size;
	allocate.memoryTypeIndex = typeIndex;
	if (vkAllocateMemory(context_.device(), &allocate, nullptr, &object.memory) != VK_SUCCESS) {
		vkDestroyBuffer(context_.device(), object.buffer, nullptr);
		return fail(Kld::HalStatusCode::AllocationFailed, "vkAllocateMemory failed for a buffer");
	}
	vkBindBufferMemory(context_.device(), object.buffer, object.memory, 0);
	if (vkMapMemory(context_.device(), object.memory, 0, VK_WHOLE_SIZE, 0, &object.mapped)
	    != VK_SUCCESS) {
		object.mapped = nullptr;
	}
	if (object.mapped != nullptr && op.data != nullptr) {
		std::memcpy(object.mapped, op.data, op.size);
	}
	buffers_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::destroyBuffer(const Kld::OpDestroy& op)
{
	const auto found = buffers_.find(op.id);
	if (found == buffers_.end()) {
		return failInvalidHandle("buffer", op.id);
	}
	DeferredDestroy deferred {};
	deferred.buffer_ = found->second.buffer;
	deferred.bufferMemory_ = found->second.memory;
	if (found->second.mapped != nullptr) {
		vkUnmapMemory(context_.device(), found->second.memory);
		found->second.mapped = nullptr;
	}
	deferred_.push_back(deferred);
	buffers_.erase(found);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::resizeBuffer(const Kld::OpResizeBufferObject& op)
{
	VkBufferObject* object = buffer(op.id);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.id);
	}
	if (object->size == op.size) {
		return Kld::HalOk;
	}
	DeferredDestroy deferred {};
	deferred.buffer_ = object->buffer;
	deferred.bufferMemory_ = object->memory;
	if (object->mapped != nullptr) {
		vkUnmapMemory(context_.device(), object->memory);
		object->mapped = nullptr;
	}
	deferred_.push_back(deferred);
	buffers_.erase(op.id);
	// The HAL allows undefined contents after a resize.
	return createBuffer(Kld::OpCreateBufferObject { op.id, op.size, 0, nullptr });
}

Kld::HalStatus CommandExecutor::updateBuffer(const Kld::OpUpdateBufferObject& op)
{
	VkBufferObject* object = buffer(op.id);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.id);
	}
	if (op.data == nullptr || op.offset > object->size || op.size > object->size - op.offset) {
		return fail(Kld::HalStatusCode::InvalidCommand, "buffer update range is outside the buffer");
	}
	if (object->mapped == nullptr) {
		return fail(Kld::HalStatusCode::BackendError, "the buffer is not host visible");
	}
	std::memcpy(static_cast<std::uint8_t*>(object->mapped) + op.offset, op.data, op.size);
	if (!object->coherent) {
		VkMappedMemoryRange range {};
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = object->memory;
		range.size = VK_WHOLE_SIZE;
		vkFlushMappedMemoryRanges(context_.device(), 1, &range);
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::mapBuffer(const Kld::OpMapBufferObject& op)
{
	VkBufferObject* object = buffer(op.id);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.id);
	}
	const std::uint32_t size = op.size != 0 ? op.size : object->size - std::min(op.offset, object->size);
	if (op.offset > object->size || size > object->size - op.offset || op.callback == nullptr) {
		return fail(Kld::HalStatusCode::InvalidCommand, "buffer map range or callback is invalid");
	}
	if (object->mapped == nullptr) {
		return fail(Kld::HalStatusCode::BackendError, "the buffer is not host visible");
	}
	VkMappedMemoryRange range {};
	range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range.memory = object->memory;
	range.size = VK_WHOLE_SIZE;
	if (op.access != Kld::BufferMapAccess::WriteOnly) {
		// The mapping is persistent, but the GPU may have written the buffer since
		// the last submission; make those writes visible before handing it out.
		const Kld::HalStatus status = flushForHostAccess();
		if (!status.ok()) {
			return status;
		}
		vkInvalidateMappedMemoryRanges(context_.device(), 1, &range);
	}
	op.callback(static_cast<std::uint8_t*>(object->mapped) + op.offset, size, op.userData);
	if (op.access != Kld::BufferMapAccess::ReadOnly && !object->coherent) {
		vkFlushMappedMemoryRanges(context_.device(), 1, &range);
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createVertexArray(const Kld::OpCreateVertexArrayObject& op)
{
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "vertex array handle is zero or already in use");
	}
	// Vulkan has no vertex array objects; vertex input lives in the pipeline and
	// the buffer bindings. The handle is accepted so portable streams work.
	vertexArrays_.emplace(op.id, 0);
	return Kld::HalOk;
}

// ─────────────────────────────────────────────────────────────────────────────
// Image storage, uploads, mips and blits
// ─────────────────────────────────────────────────────────────────────────────

Kld::HalStatus CommandExecutor::ensureImageLayout(VkTextureObject& object)
{
	if (object.layout == VK_IMAGE_LAYOUT_GENERAL) {
		return Kld::HalOk;
	}
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	VkImageMemoryBarrier barrier {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.oldLayout = object.layout;
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = object.image;
	barrier.subresourceRange = { object.aspect, 0, object.levels, 0, object.layers };
	vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
			     &barrier);
	object.layout = VK_IMAGE_LAYOUT_GENERAL;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createImageStorage(VkTextureObject& object, VkImageUsageFlags usage)
{
	if (object.format == VK_FORMAT_UNDEFINED || object.width == 0 || object.height == 0) {
		return failUnsupported("the texture format or extent cannot be expressed in Vulkan");
	}
	if (object.levels == 0) {
		object.levels = 1;
	}
	const std::uint32_t depth = object.imageType == VK_IMAGE_TYPE_3D ? object.depth : 1;
	const std::uint32_t largest = std::max({ object.width, object.height, depth });
	if (object.levels > fullMipCount(largest)) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the mip level count exceeds the image dimensions");
	}
	if (object.imageType == VK_IMAGE_TYPE_3D) {
		// A 3D image has no array layers; Kaldi's "layer" is the z slice.
		object.layers = 1;
	}
	object.aspect = toVkAspectMask(object.euphemyFormat);
	VkImageCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.imageType = object.imageType;
	info.format = object.format;
	info.extent = { object.width, object.height, depth };
	info.mipLevels = object.levels;
	info.arrayLayers = object.layers;
	info.samples = object.samples;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
		| VK_IMAGE_USAGE_SAMPLED_BIT;
	if ((object.aspect & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0) {
		info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	} else {
		info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (object.cube) {
		info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}
	if (vkCreateImage(context_.device(), &info, nullptr, &object.image) != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::AllocationFailed, "vkCreateImage failed");
	}
	object.usage = info.usage;
	VkMemoryRequirements requirements {};
	vkGetImageMemoryRequirements(context_.device(), object.image, &requirements);
	std::uint32_t typeIndex = findMemoryType(requirements.memoryTypeBits,
						 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (typeIndex == UINT32_MAX) {
		typeIndex = findMemoryType(requirements.memoryTypeBits, 0);
	}
	if (typeIndex == UINT32_MAX) {
		vkDestroyImage(context_.device(), object.image, nullptr);
		object.image = VK_NULL_HANDLE;
		return fail(Kld::HalStatusCode::AllocationFailed, "no memory type for the image");
	}
	VkMemoryAllocateInfo allocate {};
	allocate.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate.allocationSize = requirements.size;
	allocate.memoryTypeIndex = typeIndex;
	if (vkAllocateMemory(context_.device(), &allocate, nullptr, &object.memory) != VK_SUCCESS) {
		vkDestroyImage(context_.device(), object.image, nullptr);
		object.image = VK_NULL_HANDLE;
		return fail(Kld::HalStatusCode::AllocationFailed, "vkAllocateMemory failed for an image");
	}
	vkBindImageMemory(context_.device(), object.image, object.memory, 0);
	VkImageViewCreateInfo viewInfo {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = object.image;
	viewInfo.viewType = object.viewType;
	viewInfo.format = object.format;
	viewInfo.subresourceRange = { object.aspect, 0, object.levels, 0, object.layers };
	if (vkCreateImageView(context_.device(), &viewInfo, nullptr, &object.view) != VK_SUCCESS) {
		vkDestroyImage(context_.device(), object.image, nullptr);
		vkFreeMemory(context_.device(), object.memory, nullptr);
		object = VkTextureObject {};
		return fail(Kld::HalStatusCode::BackendError, "vkCreateImageView failed");
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::uploadImageLevel(VkTextureObject& object, std::uint8_t level,
						 std::uint16_t x, std::uint16_t y,
						 std::uint16_t layer, std::uint16_t width,
						 std::uint16_t height, const void* data,
						 std::uint32_t rowPitch, std::uint32_t byteSize)
{
	if (data == nullptr || width == 0 || height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the upload has no pixel data or extent");
	}
	if (level >= object.levels) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the mip level is outside the image");
	}
	const std::uint32_t levelWidth = mipExtent(object.width, level);
	const std::uint32_t levelHeight = mipExtent(object.height, level);
	if (static_cast<std::uint32_t>(x) + width > levelWidth
	    || static_cast<std::uint32_t>(y) + height > levelHeight) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the upload region is outside the mip level");
	}
	const std::uint32_t maxLayer = object.imageType == VK_IMAGE_TYPE_3D ? object.depth
									 : object.layers;
	if (static_cast<std::uint32_t>(layer) >= maxLayer) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the upload layer is out of range");
	}
	const std::uint32_t block = blockByteSize(object.euphemyFormat);
	const std::uint32_t texel = block != 0 ? 0
					      : static_cast<std::uint32_t>(pixelByteSize(object.euphemyFormat));
	if (block == 0 && texel == 0) {
		return failUnsupported("the image format has no known texel size");
	}
	const std::uint32_t rowTexels = block != 0 ? alignUp(width, 4) / 4 : width;
	const std::uint32_t rowStride = rowPitch != 0 ? rowPitch
						      : rowTexels * (block != 0 ? block : texel);
	const std::uint32_t rows = block != 0 ? alignUp(height, 4) / 4 : height;
	const std::uint32_t payload = byteSize != 0 ? byteSize : rowStride * rows;
	Kld::HalStatus status = ensureImageLayout(object);
	if (!status.ok()) {
		return status;
	}
	status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	VkBuffer staging = VK_NULL_HANDLE;
	VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
	void* mapped = nullptr;
	bool coherent = true;
	status = createHostBuffer(payload, staging, stagingMemory, mapped, coherent);
	if (!status.ok()) {
		return status;
	}
	std::memcpy(mapped, data, payload);
	if (!coherent) {
		VkMappedMemoryRange range {};
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = stagingMemory;
		range.size = VK_WHOLE_SIZE;
		vkFlushMappedMemoryRanges(context_.device(), 1, &range);
	}
	VkBufferImageCopy copy {};
	copy.bufferOffset = 0;
	copy.bufferRowLength = rowStride / (block != 0 ? block : texel);
	copy.bufferImageHeight = rows;
	copy.imageSubresource.aspectMask = object.aspect;
	copy.imageSubresource.mipLevel = level;
	copy.imageSubresource.baseArrayLayer = object.imageType == VK_IMAGE_TYPE_3D ? 0 : layer;
	copy.imageSubresource.layerCount = 1;
	copy.imageOffset = { x, y, object.imageType == VK_IMAGE_TYPE_3D ? layer : 0 };
	copy.imageExtent = { width, height, 1 };
	vkCmdCopyBufferToImage(commandBuffer_, staging, object.image, VK_IMAGE_LAYOUT_GENERAL, 1,
			       &copy);
	// Retire the staging buffer only after the copy has executed.
	DeferredDestroy deferred {};
	deferred.buffer_ = staging;
	deferred.bufferMemory_ = stagingMemory;
	vkUnmapMemory(context_.device(), stagingMemory);
	deferred_.push_back(deferred);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::uploadImage(VkTextureObject& object, const void* data,
					    std::uint32_t byteSize, std::uint32_t rowPitch,
					    std::uint32_t levelWidth, std::uint32_t levelHeight)
{
	return uploadImageLevel(object, 0, 0, 0, 0, static_cast<std::uint16_t>(levelWidth),
				static_cast<std::uint16_t>(levelHeight), data, rowPitch, byteSize);
}

Kld::HalStatus CommandExecutor::generateMips(VkTextureObject& object)
{
	if (object.levels <= 1) {
		return Kld::HalOk;
	}
	if ((object.aspect & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0) {
		return failUnsupported("mip generation is not implemented for depth/stencil images");
	}
	if (object.compressed) {
		return failUnsupported("mip generation is not implemented for block-compressed images");
	}
	Kld::HalStatus status = ensureImageLayout(object);
	if (!status.ok()) {
		return status;
	}
	status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	const std::uint32_t slices = object.imageType == VK_IMAGE_TYPE_3D ? object.depth
									 : object.layers;
	for (std::uint32_t level = 1; level < object.levels; ++level) {
		// The previous blit may still be reading the source level; order the two.
		VkImageMemoryBarrier barrier {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = object.image;
		barrier.subresourceRange = { object.aspect, 0, object.levels, 0, object.layers };
		vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
				     &barrier);
		for (std::uint32_t slice = 0; slice < slices; ++slice) {
			VkImageBlit blit {};
			blit.srcSubresource = { object.aspect, level - 1, slice, 1 };
			blit.srcOffsets[1] = {
				static_cast<std::int32_t>(mipExtent(object.width, level - 1)),
				static_cast<std::int32_t>(mipExtent(object.height, level - 1)),
				static_cast<std::int32_t>(mipExtent(object.depth, level - 1))
			};
			blit.dstSubresource = { object.aspect, level, slice, 1 };
			blit.dstOffsets[1] = {
				static_cast<std::int32_t>(mipExtent(object.width, level)),
				static_cast<std::int32_t>(mipExtent(object.height, level)),
				static_cast<std::int32_t>(mipExtent(object.depth, level))
			};
			vkCmdBlitImage(commandBuffer_, object.image, VK_IMAGE_LAYOUT_GENERAL, object.image,
				       VK_IMAGE_LAYOUT_GENERAL, 1, &blit, VK_FILTER_LINEAR);
		}
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::blitImages(VkTextureObject& source, std::uint16_t sourceX,
					    std::uint16_t sourceY, VkTextureObject& destination,
					    std::uint16_t destinationX, std::uint16_t destinationY,
					    std::uint16_t width, std::uint16_t height)
{
	if (source.aspect != VK_IMAGE_ASPECT_COLOR_BIT
	    || destination.aspect != VK_IMAGE_ASPECT_COLOR_BIT) {
		return failUnsupported("blitting depth/stencil images is not implemented");
	}
	if (source.samples != destination.samples) {
		if (source.samples == VK_SAMPLE_COUNT_1_BIT
		    || destination.samples != VK_SAMPLE_COUNT_1_BIT) {
			return failUnsupported("a blit can only resolve a multisampled source");
		}
	} else if (source.compressed != destination.compressed) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "a blit needs matching compressed or uncompressed storage");
	}
	if (width == 0 || height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a blit needs a non-zero extent");
	}
	Kld::HalStatus status = ensureImageLayout(source);
	if (!status.ok()) {
		return status;
	}
	status = ensureImageLayout(destination);
	if (!status.ok()) {
		return status;
	}
	status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	const std::uint32_t sourceWidth = mipExtent(source.width, 0);
	const std::uint32_t sourceHeight = mipExtent(source.height, 0);
	const std::uint32_t destinationWidth = mipExtent(destination.width, 0);
	const std::uint32_t destinationHeight = mipExtent(destination.height, 0);
	if (static_cast<std::uint32_t>(sourceX) + width > sourceWidth
	    || static_cast<std::uint32_t>(sourceY) + height > sourceHeight
	    || static_cast<std::uint32_t>(destinationX) + width > destinationWidth
	    || static_cast<std::uint32_t>(destinationY) + height > destinationHeight) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the blit region is outside an image");
	}
	VkImageMemoryBarrier barrier {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = destination.image;
	barrier.subresourceRange = { destination.aspect, 0, destination.levels, 0,
				     destination.layers };
	vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
			     &barrier);
	VkImageBlit blit {};
	blit.srcSubresource = { source.aspect, 0, 0, 1 };
	blit.srcOffsets[0] = { sourceX, sourceY, 0 };
	blit.srcOffsets[1] = { static_cast<std::int32_t>(sourceX + width),
			       static_cast<std::int32_t>(sourceY + height), 1 };
	blit.dstSubresource = { destination.aspect, 0, 0, 1 };
	blit.dstOffsets[0] = { destinationX, destinationY, 0 };
	blit.dstOffsets[1] = { static_cast<std::int32_t>(destinationX + width),
			       static_cast<std::int32_t>(destinationY + height), 1 };
	// NEAREST reproduces the GL `glBlitFramebuffer`/`glCopyImageSubData` semantics
	// the portable commands describe; a resolve with different sample counts still
	// resolves exactly because only the sample count changes.
	vkCmdBlitImage(commandBuffer_, source.image, VK_IMAGE_LAYOUT_GENERAL, destination.image,
		       VK_IMAGE_LAYOUT_GENERAL, 1, &blit, VK_FILTER_NEAREST);
	return Kld::HalOk;
}

// ─────────────────────────────────────────────────────────────────────────────
// Sampled textures
// ─────────────────────────────────────────────────────────────────────────────

Kld::HalStatus CommandExecutor::createTexture1D(const Kld::OpCreateTexture1D& op)
{
	const VkFormat format = translateToVkFormat(op.format);
	if (format == VK_FORMAT_UNDEFINED || isCompressedFormat(op.format)) {
		return failUnsupported("the requested 1D texture format cannot be expressed in Vulkan");
	}
	if (op.width == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a 1D texture needs a non-zero width");
	}
	if (capabilities_.maxTexture1DSize != 0 && op.width > capabilities_.maxTexture1DSize) {
		return failUnsupported("the texture exceeds the probed 1D texture limit");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "texture handle is zero or already in use");
	}
	VkTextureObject object;
	object.format = format;
	object.euphemyFormat = op.format;
	object.imageType = VK_IMAGE_TYPE_1D;
	object.viewType = VK_IMAGE_VIEW_TYPE_1D;
	object.width = op.width;
	object.height = 1;
	Kld::HalStatus status = createImageStorage(object, VK_IMAGE_USAGE_SAMPLED_BIT);
	if (!status.ok()) {
		return status;
	}
	if (op.pixels != nullptr) {
		const std::uint32_t pitch = op.width * static_cast<std::uint32_t>(pixelByteSize(op.format));
		status = uploadImage(object, op.pixels, 0, pitch, op.width, 1);
	}
	if (!status.ok()) {
		destroyImageObjects(context_.device(), object);
		return status;
	}
	textures_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createTexture2D(const Kld::OpCreateTexture2D& op)
{
	const VkFormat format = translateToVkFormat(op.format);
	if (format == VK_FORMAT_UNDEFINED) {
		return failUnsupported("the requested texture format cannot be expressed in Vulkan");
	}
	if (isCompressedFormat(op.format)) {
		return failUnsupported("use CreateCompressedTexture2D for block-compressed formats");
	}
	if (op.width == 0 || op.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a 2D texture needs non-zero extents");
	}
	if (capabilities_.maxTexture2DSize != 0
	    && (op.width > capabilities_.maxTexture2DSize || op.height > capabilities_.maxTexture2DSize)) {
		return failUnsupported("the texture exceeds the probed 2D texture limit");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "texture handle is zero or already in use");
	}
	VkTextureObject object;
	object.format = format;
	object.euphemyFormat = op.format;
	object.imageType = VK_IMAGE_TYPE_2D;
	object.viewType = VK_IMAGE_VIEW_TYPE_2D;
	object.width = op.width;
	object.height = op.height;
	object.levels = op.mipLevels > 0 ? op.mipLevels : 1;
	Kld::HalStatus status = createImageStorage(object, VK_IMAGE_USAGE_SAMPLED_BIT);
	if (!status.ok()) {
		return status;
	}
	if (op.pixels != nullptr) {
		const std::uint32_t pitch = op.width * static_cast<std::uint32_t>(pixelByteSize(op.format));
		status = uploadImage(object, op.pixels, 0, pitch, op.width, op.height);
	}
	if (status.ok() && op.generateMipmaps && object.levels > 1) {
		status = generateMips(object);
	}
	if (!status.ok()) {
		destroyImageObjects(context_.device(), object);
		return status;
	}
	textures_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createCompressedTexture2D(const Kld::OpCreateCompressedTexture2D& op)
{
	const VkFormat format = translateToVkFormat(op.format);
	if (format == VK_FORMAT_UNDEFINED || !isCompressedFormat(op.format)) {
		return failUnsupported("the requested format is not a supported block-compressed format");
	}
	if (op.width == 0 || op.height == 0 || op.mipCount == 0 || op.mips == nullptr) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "a compressed texture needs extents, a mip count and a mip array");
	}
	if (op.layout.blockWidth == 0 || op.layout.blockHeight == 0
	    || op.layout.bytesPerBlock == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the compressed block layout is incomplete");
	}
	if ((op.width % 4) != 0 || (op.height % 4) != 0) {
		// Vulkan requires compressed texel-block alignment for both the image and
		// every copy extent; OpenGL tolerates partial edge blocks, Vulkan does not.
		return failUnsupported("Vulkan needs block-compressed extents that are multiples of 4");
	}
	if (capabilities_.maxTexture2DSize != 0
	    && (op.width > capabilities_.maxTexture2DSize || op.height > capabilities_.maxTexture2DSize)) {
		return failUnsupported("the texture exceeds the probed 2D texture limit");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "texture handle is zero or already in use");
	}
	VkTextureObject object;
	object.format = format;
	object.euphemyFormat = op.format;
	object.imageType = VK_IMAGE_TYPE_2D;
	object.viewType = VK_IMAGE_VIEW_TYPE_2D;
	object.width = op.width;
	object.height = op.height;
	object.levels = op.mipCount;
	object.compressed = true;
	Kld::HalStatus status = createImageStorage(object, VK_IMAGE_USAGE_SAMPLED_BIT);
	if (!status.ok()) {
		return status;
	}
	for (std::uint8_t level = 0; level < op.mipCount && status.ok(); ++level) {
		const Kld::TextureMipData& mip = op.mips[level];
		if (mip.data == nullptr || mip.byteSize == 0) {
			status = fail(Kld::HalStatusCode::InvalidCommand, "a compressed mip has no payload");
			break;
		}
		const std::uint32_t blocks = alignUp(mip.width, 4) / 4;
		status = uploadImageLevel(object, level, 0, 0, 0, mip.width, mip.height, mip.data,
					  blocks * op.layout.bytesPerBlock, mip.byteSize);
	}
	if (!status.ok()) {
		destroyImageObjects(context_.device(), object);
		return status;
	}
	textures_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createTextureFromDecodeTarget(
	const Kld::OpCreateTexture2DFromDecodeTarget& op)
{
	if (op.source == nullptr || op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the decode target or texture handle is invalid");
	}
	const std::pmr::vector<Euph::Media::Image::Frame>& frames = op.source->getFrames();
	if (frames.empty()) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the decode target has no frames");
	}
	const Kld::TextureFormat sourceFormat = op.source->getFormat();
	const VkFormat format = translateToVkFormat(sourceFormat);
	if (format == VK_FORMAT_UNDEFINED) {
		return failUnsupported("the decoded image format cannot be expressed in Vulkan");
	}
	const bool compressed = isCompressedFormat(sourceFormat);
	if (compressed && (frames.front().width % 4) != 0) {
		return failUnsupported("Vulkan needs block-compressed extents that are multiples of 4");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "texture handle is zero or already in use");
	}
	std::uint8_t levels = static_cast<std::uint8_t>(std::min<std::size_t>(frames.size(), 255));
	if (op.generateMipmaps && levels < 1) {
		levels = 1;
	}
	VkTextureObject object;
	object.format = format;
	object.euphemyFormat = sourceFormat;
	object.imageType = VK_IMAGE_TYPE_2D;
	object.viewType = VK_IMAGE_VIEW_TYPE_2D;
	object.width = frames.front().width;
	object.height = frames.front().height;
	object.levels = levels;
	object.compressed = compressed;
	Kld::HalStatus status = createImageStorage(object, VK_IMAGE_USAGE_SAMPLED_BIT);
	if (!status.ok()) {
		return status;
	}
	for (std::uint8_t level = 0; level < levels && status.ok(); ++level) {
		const Euph::Media::Image::Frame& frame = frames[level];
		status = uploadImageLevel(object, level, 0, 0, 0,
					  static_cast<std::uint16_t>(frame.width),
					  static_cast<std::uint16_t>(frame.height), frame.data.data(),
					  frame.stride, static_cast<std::uint32_t>(frame.data.size()));
	}
	if (status.ok() && op.generateMipmaps && op.mipLevels > levels) {
		status = failUnsupported("generating extra mips for a decode target is not implemented");
	}
	if (!status.ok()) {
		destroyImageObjects(context_.device(), object);
		return status;
	}
	textures_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createTexture3D(const Kld::OpCreateTexture3D& op)
{
	const VkFormat format = translateToVkFormat(op.format);
	if (format == VK_FORMAT_UNDEFINED || isCompressedFormat(op.format)) {
		return failUnsupported("the requested 3D texture format cannot be expressed in Vulkan");
	}
	if (op.width == 0 || op.height == 0 || op.depth == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a 3D texture needs non-zero extents");
	}
	if (capabilities_.maxTexture3DSize != 0
	    && (op.width > capabilities_.maxTexture3DSize
		|| op.height > capabilities_.maxTexture3DSize
		|| op.depth > capabilities_.maxTexture3DSize)) {
		return failUnsupported("the texture exceeds the probed 3D texture limit");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "texture handle is zero or already in use");
	}
	VkTextureObject object;
	object.format = format;
	object.euphemyFormat = op.format;
	object.imageType = VK_IMAGE_TYPE_3D;
	object.viewType = VK_IMAGE_VIEW_TYPE_3D;
	object.width = op.width;
	object.height = op.height;
	object.depth = op.depth;
	object.layers = 1;
	if (op.generateMipmaps) {
		object.levels = fullMipCount(std::max({ op.width, op.height, op.depth }));
	}
	Kld::HalStatus status = createImageStorage(object, VK_IMAGE_USAGE_SAMPLED_BIT);
	if (!status.ok()) {
		return status;
	}
	if (op.pixels != nullptr) {
		const std::uint32_t texel = static_cast<std::uint32_t>(pixelByteSize(op.format));
		status = uploadImageLevel(object, 0, 0, 0, 0, op.width, op.height, op.pixels,
					  op.width * texel, op.width * op.height * texel * op.depth);
	}
	if (status.ok() && op.generateMipmaps && object.levels > 1) {
		status = generateMips(object);
	}
	if (!status.ok()) {
		destroyImageObjects(context_.device(), object);
		return status;
	}
	textures_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createTextureCube(const Kld::OpCreateTextureCube& op)
{
	const VkFormat format = translateToVkFormat(op.format);
	if (format == VK_FORMAT_UNDEFINED || isCompressedFormat(op.format)) {
		return failUnsupported("the requested cube-map format cannot be expressed in Vulkan");
	}
	if (op.size == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a cube map needs a non-zero face size");
	}
	if (capabilities_.maxCubeMapSize != 0 && op.size > capabilities_.maxCubeMapSize) {
		return failUnsupported("the cube map exceeds the probed limit");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "texture handle is zero or already in use");
	}
	VkTextureObject object;
	object.format = format;
	object.euphemyFormat = op.format;
	object.imageType = VK_IMAGE_TYPE_2D;
	object.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	object.width = op.size;
	object.height = op.size;
	object.layers = 6;
	object.levels = op.mipLevels > 0 ? op.mipLevels : 1;
	object.cube = true;
	Kld::HalStatus status = createImageStorage(object, VK_IMAGE_USAGE_SAMPLED_BIT);
	if (!status.ok()) {
		return status;
	}
	if (op.generateMipmaps && object.levels > 1) {
		status = generateMips(object);
	}
	if (!status.ok()) {
		destroyImageObjects(context_.device(), object);
		return status;
	}
	textures_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createTexture2DArray(const Kld::OpCreateTexture2DArray& op)
{
	const VkFormat format = translateToVkFormat(op.format);
	if (format == VK_FORMAT_UNDEFINED || isCompressedFormat(op.format)) {
		return failUnsupported("the requested array format cannot be expressed in Vulkan");
	}
	if (op.width == 0 || op.height == 0 || op.layers == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a texture array needs extents and layers");
	}
	if (capabilities_.maxTextureArrayLayers != 0 && op.layers > capabilities_.maxTextureArrayLayers) {
		return failUnsupported("the array exceeds the probed layer limit");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "texture handle is zero or already in use");
	}
	VkTextureObject object;
	object.format = format;
	object.euphemyFormat = op.format;
	object.imageType = VK_IMAGE_TYPE_2D;
	object.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	object.width = op.width;
	object.height = op.height;
	object.layers = op.layers;
	object.levels = op.mipLevels > 0 ? op.mipLevels : 1;
	Kld::HalStatus status = createImageStorage(object, VK_IMAGE_USAGE_SAMPLED_BIT);
	if (!status.ok()) {
		return status;
	}
	if (op.pixels != nullptr) {
		const std::uint32_t texel = static_cast<std::uint32_t>(pixelByteSize(op.format));
		for (std::uint16_t layer = 0; layer < op.layers && status.ok(); ++layer) {
			const auto* slice = static_cast<const std::uint8_t*>(op.pixels)
				+ static_cast<std::size_t>(layer) * op.width * op.height * texel;
			status = uploadImageLevel(object, 0, 0, 0, layer, op.width, op.height, slice,
						  op.width * texel, op.width * op.height * texel);
		}
	}
	if (status.ok() && op.generateMipmaps && object.levels > 1) {
		status = generateMips(object);
	}
	if (!status.ok()) {
		destroyImageObjects(context_.device(), object);
		return status;
	}
	textures_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::destroyTexture(const Kld::OpDestroy& op)
{
	const auto found = textures_.find(op.id);
	if (found == textures_.end()) {
		return failInvalidHandle("texture", op.id);
	}
	DeferredDestroy deferred {};
	deferred.view_ = found->second.view;
	deferred.image_ = found->second.image;
	deferred.imageMemory_ = found->second.memory;
	deferred_.push_back(deferred);
	textures_.erase(found);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::resizeTexture2D(const Kld::OpResizeTexture2D& op)
{
	VkTextureObject* object = texture(op.id);
	if (object == nullptr) {
		return failInvalidHandle("texture", op.id);
	}
	if (object->imageType != VK_IMAGE_TYPE_2D || object->cube) {
		return fail(Kld::HalStatusCode::InvalidCommand, "ResizeTexture2D requires a 2D texture");
	}
	const VkFormat format = translateToVkFormat(op.format);
	if (format == VK_FORMAT_UNDEFINED || isCompressedFormat(op.format)) {
		return failUnsupported("the requested resize format cannot be expressed in Vulkan");
	}
	if (op.width == 0 || op.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a 2D texture needs non-zero extents");
	}
	if (capabilities_.maxTexture2DSize != 0
	    && (op.width > capabilities_.maxTexture2DSize || op.height > capabilities_.maxTexture2DSize)) {
		return failUnsupported("the texture exceeds the probed 2D texture limit");
	}
	const Kld::HalStatus status = ensureImageLayout(*object);
	if (!status.ok()) {
		return status;
	}
	DeferredDestroy deferred {};
	deferred.view_ = object->view;
	deferred.image_ = object->image;
	deferred.imageMemory_ = object->memory;
	deferred_.push_back(deferred);
	const VkImageUsageFlags usage = object->usage;
	object->view = VK_NULL_HANDLE;
	object->image = VK_NULL_HANDLE;
	object->memory = VK_NULL_HANDLE;
	object->layout = VK_IMAGE_LAYOUT_UNDEFINED;
	object->format = format;
	object->euphemyFormat = op.format;
	object->width = op.width;
	object->height = op.height;
	object->levels = op.mipLevels > 0 ? op.mipLevels : 1;
	Kld::HalStatus result = createImageStorage(*object, usage);
	if (result.ok() && op.generateMipmaps && object->levels > 1) {
		result = generateMips(*object);
	}
	if (!result.ok()) {
		// The old storage is already retired; drop the handle rather than leave a
		// half-built object behind.
		destroyImageObjects(context_.device(), *object);
		textures_.erase(op.id);
	}
	return result;
}

Kld::HalStatus CommandExecutor::updateTexture2D(const Kld::OpUpdateTexture2D& op)
{
	VkTextureObject* object = texture(op.id);
	if (object == nullptr) {
		return failInvalidHandle("texture", op.id);
	}
	if (object->imageType != VK_IMAGE_TYPE_2D || object->cube) {
		return fail(Kld::HalStatusCode::InvalidCommand, "UpdateTexture2D requires a 2D texture");
	}
	if (isCompressedFormat(op.format) || object->compressed) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "use the compressed update for block-compressed textures");
	}
	if (op.pixels == nullptr) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the texture update has no pixel data");
	}
	const std::uint32_t texel = static_cast<std::uint32_t>(pixelByteSize(op.format));
	return uploadImageLevel(*object, op.mipLevel, op.x, op.y, 0, op.width, op.height, op.pixels,
				op.width * texel, op.width * op.height * texel);
}

Kld::HalStatus CommandExecutor::updateCompressedTexture2D(
	const Kld::OpUpdateCompressedTexture2D& op)
{
	VkTextureObject* object = texture(op.id);
	if (object == nullptr) {
		return failInvalidHandle("texture", op.id);
	}
	if (!object->compressed) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "UpdateCompressedTexture2D requires a block-compressed 2D texture");
	}
	if (op.data == nullptr || op.byteSize == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the compressed update has no payload");
	}
	if ((op.x % 4) != 0 || (op.y % 4) != 0) {
		return failUnsupported("Vulkan needs block-compressed offsets that are multiples of 4");
	}
	const std::uint32_t blocks = alignUp(op.width, 4) / 4;
	return uploadImageLevel(*object, op.mipLevel, op.x, op.y, 0, op.width, op.height, op.data,
				blocks * op.layout.bytesPerBlock, op.byteSize);
}

Kld::HalStatus CommandExecutor::updateTextureCubeFace(const Kld::OpUpdateTextureCubeFace& op)
{
	VkTextureObject* object = texture(op.id);
	if (object == nullptr) {
		return failInvalidHandle("texture", op.id);
	}
	if (!object->cube) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "UpdateTextureCubeFace requires a cube texture");
	}
	if (op.pixels == nullptr || isCompressedFormat(op.format)) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the cube face update needs an uncompressed format and pixel data");
	}
	const std::uint32_t texel = static_cast<std::uint32_t>(pixelByteSize(op.format));
	return uploadImageLevel(*object, op.mipLevel, op.x, op.y,
				static_cast<std::uint16_t>(op.face), op.width, op.height, op.pixels,
				op.width * texel, op.width * op.height * texel);
}

Kld::HalStatus CommandExecutor::updateTexture2DArray(const Kld::OpUpdateTexture2DArray& op)
{
	VkTextureObject* object = texture(op.id);
	if (object == nullptr) {
		return failInvalidHandle("texture", op.id);
	}
	if (object->viewType != VK_IMAGE_VIEW_TYPE_2D_ARRAY) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "UpdateTexture2DArray requires a 2D array texture");
	}
	if (op.pixels == nullptr || isCompressedFormat(op.format)) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the array update needs an uncompressed format and pixel data");
	}
	const std::uint32_t texel = static_cast<std::uint32_t>(pixelByteSize(op.format));
	return uploadImageLevel(*object, op.mipLevel, op.x, op.y, op.layer, op.width, op.height,
				op.pixels, op.width * texel, op.width * op.height * texel);
}

Kld::HalStatus CommandExecutor::blitTexture2D(const Kld::OpBlitTexture2D& op)
{
	VkTextureObject* source = texture(op.source);
	VkTextureObject* destination = texture(op.destination);
	if (source == nullptr) {
		return failInvalidHandle("texture", op.source);
	}
	if (destination == nullptr) {
		return failInvalidHandle("texture", op.destination);
	}
	return blitImages(*source, op.sourceX, op.sourceY, *destination, op.destinationX,
			  op.destinationY, op.width, op.height);
}

Kld::HalStatus CommandExecutor::generateTextureMipmaps(const Kld::OpGenerateTextureMipmaps& op)
{
	VkTextureObject* object = texture(op.id);
	if (object == nullptr) {
		return failInvalidHandle("texture", op.id);
	}
	return generateMips(*object);
}

// ─────────────────────────────────────────────────────────────────────────────
// Samplers
// ─────────────────────────────────────────────────────────────────────────────

Kld::HalStatus CommandExecutor::createSampler(const Kld::OpCreateSampler& op)
{
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "sampler handle is zero or already in use");
	}
	if (op.lodBias != 0.0f) {
		// Vulkan 1.0 has no `samplerLodBias` feature; `VkSamplerCreateInfo::mipLodBias`
		// must stay 0 unless the 1.1 feature is enabled.
		return failUnsupported("Vulkan 1.0 cannot express a per-sampler LOD bias");
	}
	if (op.maxLod < op.minLod) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the sampler's maximum LOD is below its minimum LOD");
	}
	VkSamplerCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.magFilter = toVkFilter(op.filtering);
	info.minFilter = toVkFilter(op.filtering);
	info.mipmapMode = toVkMipmapMode(op.filtering);
	info.addressModeU = toVkAddressMode(op.wrapping[0]);
	info.addressModeV = toVkAddressMode(op.wrapping[1]);
	info.addressModeW = toVkAddressMode(op.wrapping[2]);
	info.mipLodBias = 0.0f;
	info.anisotropyEnable = VK_FALSE;
	info.maxAnisotropy = 1.0f;
	if (op.maxAnisotropy > 1.0f) {
		if (!anisotropyEnabled_) {
			return failUnsupported("anisotropic filtering is not supported by this device");
		}
		info.anisotropyEnable = VK_TRUE;
		info.maxAnisotropy = std::min(op.maxAnisotropy, maxAnisotropy_);
	}
	info.compareEnable = VK_FALSE;
	info.minLod = op.minLod;
	info.maxLod = op.maxLod;
	info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	info.unnormalizedCoordinates = VK_FALSE;
	VkSamplerObject object;
	if (vkCreateSampler(context_.device(), &info, nullptr, &object.sampler) != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::AllocationFailed, "vkCreateSampler failed");
	}
	samplers_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::destroySampler(const Kld::OpDestroy& op)
{
	const auto found = samplers_.find(op.id);
	if (found == samplers_.end()) {
		return failInvalidHandle("sampler", op.id);
	}
	DeferredDestroy deferred {};
	deferred.sampler_ = found->second.sampler;
	deferred_.push_back(deferred);
	samplers_.erase(found);
	return Kld::HalOk;
}

// ─────────────────────────────────────────────────────────────────────────────
// Images
// ─────────────────────────────────────────────────────────────────────────────

Kld::HalStatus CommandExecutor::createImage2D(const Kld::OpCreateImage2D& op)
{
	const VkFormat format = translateToVkFormat(op.format);
	if (format == VK_FORMAT_UNDEFINED || isCompressedFormat(op.format)) {
		return failUnsupported("the requested image format cannot be used as an attachment");
	}
	if (op.width == 0 || op.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a 2D image needs non-zero extents");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "image handle is zero or already in use");
	}
	VkTextureObject object;
	object.format = format;
	object.euphemyFormat = op.format;
	object.imageType = VK_IMAGE_TYPE_2D;
	object.viewType = VK_IMAGE_VIEW_TYPE_2D;
	object.width = op.width;
	object.height = op.height;
	object.levels = op.mipLevels > 0 ? op.mipLevels : 1;
	Kld::HalStatus status =
		createImageStorage(object, toVkImageUsage(op.usage) | VK_IMAGE_USAGE_STORAGE_BIT);
	if (!status.ok()) {
		return status;
	}
	images_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::resizeImage2D(const Kld::OpResizeImage2D& op)
{
	VkTextureObject* object = image(op.id);
	if (object == nullptr) {
		return failInvalidHandle("image", op.id);
	}
	if (object->samples != VK_SAMPLE_COUNT_1_BIT) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "a multisampled image cannot be resized in place; destroy and recreate it");
	}
	const VkFormat format = translateToVkFormat(op.format);
	if (format == VK_FORMAT_UNDEFINED || isCompressedFormat(op.format)) {
		return failUnsupported("the requested resize format cannot be used as an attachment");
	}
	if (op.width == 0 || op.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a 2D image needs non-zero extents");
	}
	const Kld::HalStatus status = ensureImageLayout(*object);
	if (!status.ok()) {
		return status;
	}
	DeferredDestroy deferred {};
	deferred.view_ = object->view;
	deferred.image_ = object->image;
	deferred.imageMemory_ = object->memory;
	deferred_.push_back(deferred);
	const VkImageUsageFlags usage = object->usage;
	object->view = VK_NULL_HANDLE;
	object->image = VK_NULL_HANDLE;
	object->memory = VK_NULL_HANDLE;
	object->layout = VK_IMAGE_LAYOUT_UNDEFINED;
	object->format = format;
	object->euphemyFormat = op.format;
	object->width = op.width;
	object->height = op.height;
	object->levels = op.mipLevels > 0 ? op.mipLevels : 1;
	const Kld::HalStatus result = createImageStorage(*object, usage);
	if (!result.ok()) {
		destroyImageObjects(context_.device(), *object);
		images_.erase(op.id);
	}
	return result;
}

Kld::HalStatus CommandExecutor::createMultisampledImage2D(
	const Kld::OpCreateMultisampledImage2D& op)
{
	const VkFormat format = translateToVkFormat(op.format);
	if (format == VK_FORMAT_UNDEFINED || isCompressedFormat(op.format)) {
		return failUnsupported("the requested format cannot be used as a multisample attachment");
	}
	if (op.width == 0 || op.height == 0 || op.samples < 2) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "a multisampled image needs extents and at least two samples");
	}
	if (capabilities_.maxSamples != 0 && op.samples > capabilities_.maxSamples) {
		return failUnsupported("the requested sample count exceeds the probed limit");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "image handle is zero or already in use");
	}
	VkTextureObject object;
	object.format = format;
	object.euphemyFormat = op.format;
	object.imageType = VK_IMAGE_TYPE_2D;
	object.viewType = VK_IMAGE_VIEW_TYPE_2D;
	object.width = op.width;
	object.height = op.height;
	object.levels = 1;
	object.samples = sampleCount(op.samples);
	Kld::HalStatus status =
		createImageStorage(object, toVkImageUsage(op.usage) | VK_IMAGE_USAGE_STORAGE_BIT);
	if (!status.ok()) {
		return status;
	}
	images_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::updateImage2D(const Kld::OpUpdateImage2D& op)
{
	VkTextureObject* object = image(op.id);
	if (object == nullptr) {
		return failInvalidHandle("image", op.id);
	}
	if (object->samples != VK_SAMPLE_COUNT_1_BIT) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "a multisampled image cannot be updated with texel data");
	}
	if (op.pixels == nullptr || isCompressedFormat(op.format)) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the image update needs an uncompressed format and pixel data");
	}
	const std::uint32_t texel = static_cast<std::uint32_t>(pixelByteSize(op.format));
	return uploadImageLevel(*object, op.mipLevel, op.x, op.y, 0, op.width, op.height, op.pixels,
				op.width * texel, op.width * op.height * texel);
}

Kld::HalStatus CommandExecutor::blitImage2D(const Kld::OpBlitImage2D& op)
{
	VkTextureObject* source = image(op.source);
	VkTextureObject* destination = image(op.destination);
	if (source == nullptr) {
		return failInvalidHandle("image", op.source);
	}
	if (destination == nullptr) {
		return failInvalidHandle("image", op.destination);
	}
	return blitImages(*source, op.sourceX, op.sourceY, *destination, op.destinationX,
			  op.destinationY, op.width, op.height);
}

Kld::HalStatus CommandExecutor::resolveImage2D(const Kld::OpResolveImage2D& op)
{
	VkTextureObject* source = image(op.source);
	VkTextureObject* destination = image(op.destination);
	if (source == nullptr) {
		return failInvalidHandle("image", op.source);
	}
	if (destination == nullptr) {
		return failInvalidHandle("image", op.destination);
	}
	if (source->samples == VK_SAMPLE_COUNT_1_BIT) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "ResolveImage2D needs a multisampled source image");
	}
	if (source->format != destination->format) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "a resolve needs matching source and destination formats");
	}
	return blitImages(*source, op.sourceX, op.sourceY, *destination, op.destinationX,
			  op.destinationY, op.width, op.height);
}

Kld::HalStatus CommandExecutor::destroyImage(const Kld::OpDestroyImage& op)
{
	const auto found = images_.find(op.id);
	if (found == images_.end()) {
		return failInvalidHandle("image", op.id);
	}
	DeferredDestroy deferred {};
	deferred.view_ = found->second.view;
	deferred.image_ = found->second.image;
	deferred.imageMemory_ = found->second.memory;
	deferred_.push_back(deferred);
	images_.erase(found);
	return Kld::HalOk;
}

// ─────────────────────────────────────────────────────────────────────────────
// Framebuffers
// ─────────────────────────────────────────────────────────────────────────────

Kld::HalStatus CommandExecutor::createFramebuffer(const Kld::OpCreateFramebuffer& op)
{
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "framebuffer handle is zero or already in use");
	}
	if (op.attachmentCount != 0 && op.attachments == nullptr) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the attachment array is missing");
	}
	std::map<std::uint32_t, VkTextureObject*> colors;
	VkTextureObject* depth = nullptr;
	bool stencilOnly = false;
	for (std::uint32_t index = 0; index < op.attachmentCount; ++index) {
		const Kld::FramebufferAttachment& attachment = op.attachments[index];
		VkTextureObject* target = image(attachment.image);
		if (target == nullptr) {
			return failInvalidHandle("image", attachment.image);
		}
		if (target->samples != VK_SAMPLE_COUNT_1_BIT) {
			return failUnsupported(
				"framebuffers with multisampled attachments are not implemented");
		}
		if (attachment.role == Kld::FramebufferAttachmentRole::Color) {
			if (attachment.colorAttachmentIndex >= capabilities_.maxColorAttachments) {
				return fail(Kld::HalStatusCode::InvalidCommand,
					    "color attachment index exceeds the probed limit");
			}
			if (!colors.emplace(attachment.colorAttachmentIndex, target).second) {
				return fail(Kld::HalStatusCode::InvalidCommand,
					    "two attachments use the same color index");
			}
		} else {
			depth = target;
			stencilOnly = attachment.role == Kld::FramebufferAttachmentRole::Stencil;
		}
	}
	if (!colors.empty()) {
		const std::uint32_t highest = colors.rbegin()->first;
		if (highest + 1 != colors.size()) {
			// Vulkan indexes colour attachments by shader output location, so the
			// portable indices have to be contiguous starting at zero.
			return failUnsupported(
				"Vulkan needs colour attachment indices that are contiguous from zero");
		}
	}
	std::vector<VkAttachmentDescription> attachments;
	std::vector<VkAttachmentReference> colorRefs;
	attachments.reserve(colors.size() + (depth != nullptr ? 1 : 0));
	colorRefs.reserve(colors.size());
	for (const auto& entry : colors) {
		VkAttachmentDescription description {};
		description.format = entry.second->format;
		description.samples = VK_SAMPLE_COUNT_1_BIT;
		// Every image lives in GENERAL, so LOAD needs no transition and preserves
		// whatever earlier commands (including a previous render pass) wrote.
		description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		description.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
		description.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
		colorRefs.push_back(VkAttachmentReference {
			static_cast<std::uint32_t>(attachments.size()), VK_IMAGE_LAYOUT_GENERAL });
		attachments.push_back(description);
	}
	VkAttachmentReference depthRef { VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_GENERAL };
	if (depth != nullptr) {
		VkAttachmentDescription description {};
		description.format = depth->format;
		description.samples = VK_SAMPLE_COUNT_1_BIT;
		description.loadOp = stencilOnly ? VK_ATTACHMENT_LOAD_OP_DONT_CARE
						 : VK_ATTACHMENT_LOAD_OP_LOAD;
		description.storeOp = stencilOnly ? VK_ATTACHMENT_STORE_OP_DONT_CARE
						  : VK_ATTACHMENT_STORE_OP_STORE;
		description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		description.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
		description.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
		depthRef.attachment = static_cast<std::uint32_t>(attachments.size());
		attachments.push_back(description);
	}
	VkSubpassDescription subpass {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = static_cast<std::uint32_t>(colorRefs.size());
	subpass.pColorAttachments = colorRefs.empty() ? nullptr : colorRefs.data();
	subpass.pDepthStencilAttachment = depth != nullptr ? &depthRef : nullptr;
	VkSubpassDependency dependency {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
		| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
		| VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
		| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	VkRenderPassCreateInfo passInfo {};
	passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	passInfo.attachmentCount = static_cast<std::uint32_t>(attachments.size());
	passInfo.pAttachments = attachments.empty() ? nullptr : attachments.data();
	passInfo.subpassCount = 1;
	passInfo.pSubpasses = &subpass;
	passInfo.dependencyCount = 1;
	passInfo.pDependencies = &dependency;
	VkFramebufferObject object;
	object.width = op.width;
	object.height = op.height;
	object.hasDepth = depth != nullptr && !stencilOnly;
	object.hasStencil = depth != nullptr;
	if (vkCreateRenderPass(context_.device(), &passInfo, nullptr, &object.renderPass)
	    != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::BackendError, "vkCreateRenderPass failed");
	}
	std::vector<VkImageView> views;
	views.reserve(attachments.size());
	for (const auto& entry : colors) {
		views.push_back(entry.second->view);
		object.colorFormats.push_back(entry.second->format);
	}
	if (depth != nullptr) {
		views.push_back(depth->view);
		object.depthFormat = depth->format;
	}
	VkFramebufferCreateInfo frameInfo {};
	frameInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameInfo.renderPass = object.renderPass;
	frameInfo.attachmentCount = static_cast<std::uint32_t>(views.size());
	frameInfo.pAttachments = views.empty() ? nullptr : views.data();
	frameInfo.width = op.width;
	frameInfo.height = op.height;
	frameInfo.layers = 1;
	if (vkCreateFramebuffer(context_.device(), &frameInfo, nullptr, &object.framebuffer)
	    != VK_SUCCESS) {
		vkDestroyRenderPass(context_.device(), object.renderPass, nullptr);
		return fail(Kld::HalStatusCode::BackendError, "vkCreateFramebuffer failed");
	}
	VkPassInfo info;
	info.colorFormats = object.colorFormats;
	info.depthFormat = object.depthFormat;
	info.hasDepth = object.hasDepth;
	info.hasStencil = object.hasStencil;
	passInfos_[object.renderPass] = info;
	framebuffers_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::destroyFramebuffer(const Kld::OpDestroyFramebuffer& op)
{
	const auto found = framebuffers_.find(op.id);
	if (found == framebuffers_.end()) {
		return failInvalidHandle("framebuffer", op.id);
	}
	passInfos_.erase(found->second.renderPass);
	DeferredDestroy deferred {};
	deferred.framebuffer_ = found->second.framebuffer;
	deferred.renderPass_ = found->second.renderPass;
	deferred_.push_back(deferred);
	if (boundFramebuffer_ == op.id) {
		endActiveRenderPass();
		boundFramebuffer_ = Kld::DefaultFramebuffer;
	}
	framebuffers_.erase(found);
	return Kld::HalOk;
}

const VkPassInfo* CommandExecutor::currentPassInfo() const
{
	const auto found = passInfos_.find(activePass_);
	return found == passInfos_.end() ? nullptr : &found->second;
}

}
