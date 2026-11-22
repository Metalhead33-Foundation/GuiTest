/**
 * @file VkExecutorResources.cpp
 * @brief Buffer, texture, image, sampler and framebuffer handlers for the Vulkan executor.
 *
 * This bring-up version implements 2D resources; cube maps, 3D textures, arrays,
 * multisampled images, compressed uploads, compute dispatches and indirect draws
 * report `HalStatusCode::UnsupportedBaseline` with a named message until they are
 * ported.
 */
#include "VkCommandExecutor.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace Vk {

namespace {

/** @brief Vulkan image aspect mask for a Euphemy format. */
VkImageAspectFlags aspectMask(Kld::TextureFormat format) noexcept
{
	switch (format) {
		case Kld::TextureFormat::DEPTH24_STENCIL8:
		case Kld::TextureFormat::DEPTH32F_STENCIL8:
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		case Kld::TextureFormat::DEPTH_COMPONENT16:
		case Kld::TextureFormat::DEPTH_COMPONENT24:
		case Kld::TextureFormat::DEPTH_COMPONENT32:
		case Kld::TextureFormat::DEPTH_COMPONENT32F:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		case Kld::TextureFormat::STENCIL_INDEX8:
			return VK_IMAGE_ASPECT_STENCIL_BIT;
		default:
			return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

} // namespace

Kld::HalStatus CommandExecutor::createBuffer(const Kld::OpCreateBufferObject& op)
{
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "buffer handle is zero or already in use");
	}
	VkBufferObject object;
	object.size = op.size;
	VkBufferCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = op.size;
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
	if (typeIndex == UINT32_MAX) {
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
	op.callback(static_cast<std::uint8_t*>(object->mapped) + op.offset, size, op.userData);
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
	barrier.subresourceRange = { object.aspect, 0, object.levels, 0, 1 };
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
	object.aspect = aspectMask(object.euphemyFormat);
	VkImageCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.format = object.format;
	info.extent = { object.width, object.height, 1 };
	info.mipLevels = object.levels;
	info.arrayLayers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
		| VK_IMAGE_USAGE_SAMPLED_BIT;
	if (object.aspect != VK_IMAGE_ASPECT_COLOR_BIT) {
		info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	} else {
		info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (vkCreateImage(context_.device(), &info, nullptr, &object.image) != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::AllocationFailed, "vkCreateImage failed");
	}
	VkMemoryRequirements requirements {};
	vkGetImageMemoryRequirements(context_.device(), object.image, &requirements);
	const std::uint32_t typeIndex = findMemoryType(requirements.memoryTypeBits,
						       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (typeIndex == UINT32_MAX) {
		vkDestroyImage(context_.device(), object.image, nullptr);
		object.image = VK_NULL_HANDLE;
		return fail(Kld::HalStatusCode::AllocationFailed, "no device-local memory type for the image");
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
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = object.format;
	viewInfo.subresourceRange = { object.aspect, 0, object.levels, 0, 1 };
	if (vkCreateImageView(context_.device(), &viewInfo, nullptr, &object.view) != VK_SUCCESS) {
		vkDestroyImage(context_.device(), object.image, nullptr);
		vkFreeMemory(context_.device(), object.memory, nullptr);
		object = VkTextureObject {};
		return fail(Kld::HalStatusCode::BackendError, "vkCreateImageView failed");
	}
	return Kld::HalOk;
}

}
