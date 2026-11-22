/**
 * @file VkUtil.cpp
 * @brief Implements the Vulkan translation tables declared in VkUtil.hpp.
 *
 * Every `VK_FORMAT_*` token used here is part of the Vulkan 1.0 core header
 * shipped as `glad/include/glad/vulkan.h`; the unit test suite can only verify the
 * table's internal consistency, not driver behaviour.
 */
#include "VkUtil.hpp"

namespace Vk {

namespace {

using Euph::Media::Image::Format;

/** @brief Returns the attribute element count encoded in a Kaldi primitive type. */
constexpr unsigned elementCount(Kld::PrimitiveType type) noexcept
{
	return static_cast<unsigned>(type) & static_cast<unsigned>(Kld::PrimitiveType::SIZE_MASK);
}

/** @brief Returns the scalar type encoded in a Kaldi primitive type. */
constexpr unsigned scalarType(Kld::PrimitiveType type) noexcept
{
	return static_cast<unsigned>(type) & static_cast<unsigned>(Kld::PrimitiveType::TYPE_MASK);
}

/** @brief Returns the byte size of one scalar of a Kaldi primitive type. */
constexpr unsigned scalarSize(Kld::PrimitiveType type) noexcept
{
	switch (scalarType(type)) {
		case Kld::PrimitiveType::TYPE_HALF_FLOAT: return 2;
		case Kld::PrimitiveType::TYPE_FLOAT: return 4;
		case Kld::PrimitiveType::TYPE_DOUBLE: return 8;
		case Kld::PrimitiveType::TYPE_UBYTE:
		case Kld::PrimitiveType::TYPE_SBYTE: return 1;
		case Kld::PrimitiveType::TYPE_USHORT:
		case Kld::PrimitiveType::TYPE_SSHORT: return 2;
		case Kld::PrimitiveType::TYPE_UINT:
		case Kld::PrimitiveType::TYPE_SINT: return 4;
		default: return 0;
	}
}

/** @brief Selects one of the 1/2/3/4-component formats of a family. */
constexpr VkFormat pickFormat(unsigned count, VkFormat one, VkFormat two, VkFormat three, VkFormat four) noexcept
{
	switch (count) {
		case 1: return one;
		case 2: return two;
		case 3: return three;
		case 4: return four;
		default: return VK_FORMAT_UNDEFINED;
	}
}

} // namespace

bool isCompressedFormat(Format format) noexcept
{
	switch (format) {
		case Format::B_DXT1:
		case Format::B_DXT3:
		case Format::B_DXT5: return true;
		default: return false;
	}
}

bool isDepthFormat(Format format) noexcept
{
	switch (format) {
		case Format::DEPTH_COMPONENT16:
		case Format::DEPTH_COMPONENT24:
		case Format::DEPTH_COMPONENT32:
		case Format::DEPTH_COMPONENT32F:
		case Format::DEPTH24_STENCIL8:
		case Format::DEPTH32F_STENCIL8: return true;
		default: return false;
	}
}

bool hasStencilComponent(Format format) noexcept
{
	switch (format) {
		case Format::DEPTH24_STENCIL8:
		case Format::DEPTH32F_STENCIL8:
		case Format::STENCIL_INDEX8: return true;
		default: return false;
	}
}

VkImageAspectFlags toVkAspectMask(Format format) noexcept
{
	if (hasStencilComponent(format)) {
		if (format == Format::STENCIL_INDEX8) {
			return VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	if (isDepthFormat(format)) {
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	return VK_IMAGE_ASPECT_COLOR_BIT;
}

VkFormat translateToVkFormat(Format format) noexcept
{
	switch (format) {
		// ── Single channel ───────────────────────────────────────────────────
		case Format::R8U: return VK_FORMAT_R8_UNORM;
		case Format::R8S: return VK_FORMAT_R8_SNORM;
		case Format::R16U: return VK_FORMAT_R16_UNORM;
		case Format::R16S: return VK_FORMAT_R16_SNORM;
		case Format::R16F: return VK_FORMAT_R16_SFLOAT;
		case Format::R32U: return VK_FORMAT_R32_UINT;
		case Format::R32S: return VK_FORMAT_R32_SINT;
		case Format::R32F: return VK_FORMAT_R32_SFLOAT;
		case Format::R64F: return VK_FORMAT_R64_SFLOAT;
		// ── Two channel ──────────────────────────────────────────────────────
		case Format::RG8U: return VK_FORMAT_R8G8_UNORM;
		case Format::RG8S: return VK_FORMAT_R8G8_SNORM;
		case Format::RG16U: return VK_FORMAT_R16G16_UNORM;
		case Format::RG16S: return VK_FORMAT_R16G16_SNORM;
		case Format::RG16F: return VK_FORMAT_R16G16_SFLOAT;
		case Format::RG32U: return VK_FORMAT_R32G32_UINT;
		case Format::RG32S: return VK_FORMAT_R32G32_SINT;
		case Format::RG32F: return VK_FORMAT_R32G32_SFLOAT;
		case Format::RG64F: return VK_FORMAT_R64G64_SFLOAT;
		// ── Three channel ────────────────────────────────────────────────────
		case Format::RGB8U: return VK_FORMAT_R8G8B8_UNORM;
		case Format::RGB8S: return VK_FORMAT_R8G8B8_SNORM;
		case Format::RGB16U: return VK_FORMAT_R16G16B16_UNORM;
		case Format::RGB16S: return VK_FORMAT_R16G16B16_SNORM;
		case Format::RGB16F: return VK_FORMAT_R16G16B16_SFLOAT;
		case Format::RGB32U: return VK_FORMAT_R32G32B32_UINT;
		case Format::RGB32S: return VK_FORMAT_R32G32B32_SINT;
		case Format::RGB32F: return VK_FORMAT_R32G32B32_SFLOAT;
		case Format::RGB64F: return VK_FORMAT_R64G64B64_SFLOAT;
		// ── Three channel, BGR memory order ──────────────────────────────────
		case Format::BGR8U: return VK_FORMAT_B8G8R8_UNORM;
		case Format::BGR8S: return VK_FORMAT_B8G8R8_SNORM;
		case Format::BGR16U:
		case Format::BGR16S:
		case Format::BGR16F:
		case Format::BGR32U:
		case Format::BGR32S:
		case Format::BGR32F:
		case Format::BGR64F: return VK_FORMAT_UNDEFINED; // Vulkan only has BGR8 variants
		// ── Four channel, RGBA order ─────────────────────────────────────────
		case Format::RGBA8U: return VK_FORMAT_R8G8B8A8_UNORM;
		case Format::RGBA8S: return VK_FORMAT_R8G8B8A8_SNORM;
		case Format::RGBA16U: return VK_FORMAT_R16G16B16A16_UNORM;
		case Format::RGBA16S: return VK_FORMAT_R16G16B16A16_SNORM;
		case Format::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case Format::RGBA32U: return VK_FORMAT_R32G32B32A32_UINT;
		case Format::RGBA32S: return VK_FORMAT_R32G32B32A32_SINT;
		case Format::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case Format::RGBA64F: return VK_FORMAT_R64G64B64A64_SFLOAT;
		// ── Four channel, BGRA memory order ──────────────────────────────────
		case Format::BGRA8U: return VK_FORMAT_B8G8R8A8_UNORM;
		case Format::BGRA8S:
		case Format::BGRA16U:
		case Format::BGRA16S:
		case Format::BGRA16F:
		case Format::BGRA32U:
		case Format::BGRA32S:
		case Format::BGRA32F:
		case Format::BGRA64F: return VK_FORMAT_UNDEFINED;
		// ── Four channel, ARGB memory order ──────────────────────────────────
		case Format::ARGB8U:
		case Format::ARGB8S:
		case Format::ARGB16U:
		case Format::ARGB16S:
		case Format::ARGB16F:
		case Format::ARGB32U:
		case Format::ARGB32S:
		case Format::ARGB32F:
		case Format::ARGB64F: return VK_FORMAT_UNDEFINED; // no ARGB layout in Vulkan
		// ── Special types ────────────────────────────────────────────────────
		case Format::INDEXED: return VK_FORMAT_UNDEFINED;
		case Format::RGB565: return VK_FORMAT_UNDEFINED;  // only B5G6R5 exists
		case Format::RGB555: return VK_FORMAT_UNDEFINED;  // only B5G5R5A1 exists
		case Format::RGB444: return VK_FORMAT_UNDEFINED;  // only B4G4R4A4 exists
		case Format::RGBA444: return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
		case Format::DEPTH_COMPONENT16: return VK_FORMAT_D16_UNORM;
		case Format::DEPTH_COMPONENT24: return VK_FORMAT_X8_D24_UNORM_PACK32;
		case Format::DEPTH_COMPONENT32: return VK_FORMAT_X8_D24_UNORM_PACK32; // no 32-bit UNORM depth
		case Format::DEPTH_COMPONENT32F: return VK_FORMAT_D32_SFLOAT;
		case Format::DEPTH24_STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
		case Format::DEPTH32F_STENCIL8: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case Format::STENCIL_INDEX8: return VK_FORMAT_S8_UINT;
		// ── Block compression ────────────────────────────────────────────────
		case Format::B_DXT1: return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
		case Format::B_DXT3: return VK_FORMAT_BC2_UNORM_BLOCK;
		case Format::B_DXT5: return VK_FORMAT_BC3_UNORM_BLOCK;
		default: return VK_FORMAT_UNDEFINED;
	}
}

VkFormat translateToVkFormat(Kld::PrimitiveType type) noexcept
{
	if (scalarSize(type) == 0 || elementCount(type) == 0) {
		return VK_FORMAT_UNDEFINED;
	}
	const unsigned count = elementCount(type);
	const bool normalized = (type & Kld::PrimitiveType::IS_NORMALIZED) != 0;
	switch (scalarType(type)) {
		case Kld::PrimitiveType::TYPE_FLOAT:
			return pickFormat(count, VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT,
							  VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT);
		case Kld::PrimitiveType::TYPE_HALF_FLOAT:
			return pickFormat(count, VK_FORMAT_R16_SFLOAT, VK_FORMAT_R16G16_SFLOAT,
							  VK_FORMAT_R16G16B16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT);
		case Kld::PrimitiveType::TYPE_DOUBLE:
			return pickFormat(count, VK_FORMAT_R64_SFLOAT, VK_FORMAT_R64G64_SFLOAT,
							  VK_FORMAT_R64G64B64_SFLOAT, VK_FORMAT_R64G64B64A64_SFLOAT);
		case Kld::PrimitiveType::TYPE_UBYTE:
			if (normalized) {
				return pickFormat(count, VK_FORMAT_R8_UNORM, VK_FORMAT_R8G8_UNORM,
								  VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R8G8B8A8_UNORM);
			}
			return pickFormat(count, VK_FORMAT_R8_UINT, VK_FORMAT_R8G8_UINT,
							  VK_FORMAT_R8G8B8_UINT, VK_FORMAT_R8G8B8A8_UINT);
		case Kld::PrimitiveType::TYPE_SBYTE:
			if (normalized) {
				return pickFormat(count, VK_FORMAT_R8_SNORM, VK_FORMAT_R8G8_SNORM,
								  VK_FORMAT_R8G8B8_SNORM, VK_FORMAT_R8G8B8A8_SNORM);
			}
			return pickFormat(count, VK_FORMAT_R8_SINT, VK_FORMAT_R8G8_SINT,
							  VK_FORMAT_R8G8B8_SINT, VK_FORMAT_R8G8B8A8_SINT);
		case Kld::PrimitiveType::TYPE_USHORT:
			if (normalized) {
				return pickFormat(count, VK_FORMAT_R16_UNORM, VK_FORMAT_R16G16_UNORM,
								  VK_FORMAT_R16G16B16_UNORM, VK_FORMAT_R16G16B16A16_UNORM);
			}
			return pickFormat(count, VK_FORMAT_R16_UINT, VK_FORMAT_R16G16_UINT,
							  VK_FORMAT_R16G16B16_UINT, VK_FORMAT_R16G16B16A16_UINT);
		case Kld::PrimitiveType::TYPE_SSHORT:
			if (normalized) {
				return pickFormat(count, VK_FORMAT_R16_SNORM, VK_FORMAT_R16G16_SNORM,
								  VK_FORMAT_R16G16B16_SNORM, VK_FORMAT_R16G16B16A16_SNORM);
			}
			return pickFormat(count, VK_FORMAT_R16_SINT, VK_FORMAT_R16G16_SINT,
							  VK_FORMAT_R16G16B16_SINT, VK_FORMAT_R16G16B16A16_SINT);
		case Kld::PrimitiveType::TYPE_UINT:
			if (normalized) {
				return VK_FORMAT_UNDEFINED; // no normalized 32-bit vertex formats
			}
			return pickFormat(count, VK_FORMAT_R32_UINT, VK_FORMAT_R32G32_UINT,
							  VK_FORMAT_R32G32B32_UINT, VK_FORMAT_R32G32B32A32_UINT);
		case Kld::PrimitiveType::TYPE_SINT:
			if (normalized) {
				return VK_FORMAT_UNDEFINED;
			}
			return pickFormat(count, VK_FORMAT_R32_SINT, VK_FORMAT_R32G32_SINT,
							  VK_FORMAT_R32G32B32_SINT, VK_FORMAT_R32G32B32A32_SINT);
		default:
			return VK_FORMAT_UNDEFINED;
	}
}

uint32_t vertexFormatByteSize(Kld::PrimitiveType type) noexcept
{
	const unsigned count = elementCount(type);
	const unsigned size = scalarSize(type);
	if (count == 0 || size == 0) {
		return 0;
	}
	return static_cast<uint32_t>(count * size);
}

VkVertexInputBindingDescription toVkVertexBinding(const Kld::VertexDescriptor& descriptor) noexcept
{
	VkVertexInputBindingDescription binding{};
	binding.binding = 0;
	binding.stride = static_cast<uint32_t>(descriptor.stride);
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return binding;
}

std::vector<VkVertexInputAttributeDescription> toVkVertexAttributes(const Kld::VertexDescriptor& descriptor)
{
	std::vector<VkVertexInputAttributeDescription> attributes;
	attributes.reserve(descriptor.descriptors.size());
	for (std::size_t location = 0; location < descriptor.descriptors.size(); ++location) {
		const Kld::AttributeDescriptor& attribute = descriptor.descriptors[location];
		VkVertexInputAttributeDescription translated{};
		translated.location = static_cast<uint32_t>(location);
		translated.binding = 0;
		translated.format = translateToVkFormat(attribute.type);
		translated.offset = static_cast<uint32_t>(attribute.offset);
		attributes.push_back(translated);
	}
	return attributes;
}

VkImageType toVkImageType(Kld::TextureBindingType type) noexcept
{
	switch (type) {
		case Kld::TextureBindingType::Texture1D: return VK_IMAGE_TYPE_1D;
		case Kld::TextureBindingType::Texture3D: return VK_IMAGE_TYPE_3D;
		case Kld::TextureBindingType::Texture2D:
		case Kld::TextureBindingType::ArrayTexture2D:
		case Kld::TextureBindingType::TextureCube: return VK_IMAGE_TYPE_2D;
	}
	return VK_IMAGE_TYPE_2D;
}

VkImageViewType toVkImageViewType(Kld::TextureBindingType type) noexcept
{
	switch (type) {
		case Kld::TextureBindingType::Texture1D: return VK_IMAGE_VIEW_TYPE_1D;
		case Kld::TextureBindingType::Texture3D: return VK_IMAGE_VIEW_TYPE_3D;
		case Kld::TextureBindingType::ArrayTexture2D: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		case Kld::TextureBindingType::TextureCube: return VK_IMAGE_VIEW_TYPE_CUBE;
		case Kld::TextureBindingType::Texture2D: return VK_IMAGE_VIEW_TYPE_2D;
	}
	return VK_IMAGE_VIEW_TYPE_2D;
}

VkImageUsageFlags toVkImageUsage(uint32_t imageUsage) noexcept
{
	VkImageUsageFlags flags = 0;
	if (imageUsage & Kld::IMAGE_USAGE_COLOR_ATTACHMENT) {
		flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	if (imageUsage & Kld::IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT) {
		flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	if (imageUsage & Kld::IMAGE_USAGE_SAMPLED) {
		flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	}
	if (imageUsage & Kld::IMAGE_USAGE_TRANSFER_SRC) {
		flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	if (imageUsage & Kld::IMAGE_USAGE_TRANSFER_DST) {
		flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	if (imageUsage & Kld::IMAGE_USAGE_STORAGE) {
		flags |= VK_IMAGE_USAGE_STORAGE_BIT;
	}
	// RESOLVE_SRC/RESOLVE_DST describe how an image is used inside a render pass;
	// Vulkan expresses that through the colour/depth-stencil attachment usage plus
	// the resolve attachment, so no dedicated usage bit exists.
	return flags;
}

VkFilter toVkFilter(Kld::SamplerFiltering filtering) noexcept
{
	switch (filtering) {
		case Kld::SamplerFiltering::NEAREST: return VK_FILTER_NEAREST;
		case Kld::SamplerFiltering::LINEAR: return VK_FILTER_LINEAR;
		case Kld::SamplerFiltering::NEAREST_MIPMAP_NEAREST: return VK_FILTER_NEAREST;
		case Kld::SamplerFiltering::LINEAR_MIPMAP_NEAREST: return VK_FILTER_LINEAR;
		case Kld::SamplerFiltering::NEAREST_MIPMAP_LINEAR: return VK_FILTER_NEAREST;
		case Kld::SamplerFiltering::LINEAR_MIPMAP_LINEAR: return VK_FILTER_LINEAR;
	}
	return VK_FILTER_LINEAR;
}

VkSamplerMipmapMode toVkMipmapMode(Kld::SamplerFiltering filtering) noexcept
{
	switch (filtering) {
		case Kld::SamplerFiltering::NEAREST_MIPMAP_LINEAR:
		case Kld::SamplerFiltering::LINEAR_MIPMAP_LINEAR: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		default: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}
}

VkSamplerAddressMode toVkAddressMode(Kld::SamplerWrapping wrapping) noexcept
{
	switch (wrapping) {
		case Kld::SamplerWrapping::CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case Kld::SamplerWrapping::CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case Kld::SamplerWrapping::MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case Kld::SamplerWrapping::REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
	return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

VkImageLayout toVkImageLayout(Kld::ResourceState state) noexcept
{
	switch (state) {
		case Kld::ResourceState::Undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
		case Kld::ResourceState::CopySource: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case Kld::ResourceState::CopyDestination: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case Kld::ResourceState::ShaderRead: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case Kld::ResourceState::ShaderWrite: return VK_IMAGE_LAYOUT_GENERAL;
		case Kld::ResourceState::ShaderReadWrite: return VK_IMAGE_LAYOUT_GENERAL;
		case Kld::ResourceState::ColorAttachment: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case Kld::ResourceState::DepthStencilAttachment: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case Kld::ResourceState::Present: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		// Buffer-only states have no image layout of their own.
		case Kld::ResourceState::VertexBuffer:
		case Kld::ResourceState::IndexBuffer:
		case Kld::ResourceState::UniformBuffer: return VK_IMAGE_LAYOUT_UNDEFINED;
	}
	return VK_IMAGE_LAYOUT_UNDEFINED;
}

VkAccessFlags toVkAccessFlags(Kld::ResourceState state) noexcept
{
	switch (state) {
		case Kld::ResourceState::Undefined: return 0;
		case Kld::ResourceState::CopySource: return VK_ACCESS_TRANSFER_READ_BIT;
		case Kld::ResourceState::CopyDestination: return VK_ACCESS_TRANSFER_WRITE_BIT;
		case Kld::ResourceState::VertexBuffer: return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		case Kld::ResourceState::IndexBuffer: return VK_ACCESS_INDEX_READ_BIT;
		case Kld::ResourceState::UniformBuffer: return VK_ACCESS_UNIFORM_READ_BIT;
		case Kld::ResourceState::ShaderRead: return VK_ACCESS_SHADER_READ_BIT;
		case Kld::ResourceState::ShaderWrite: return VK_ACCESS_SHADER_WRITE_BIT;
		case Kld::ResourceState::ShaderReadWrite: return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		case Kld::ResourceState::ColorAttachment:
			return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		case Kld::ResourceState::DepthStencilAttachment:
			return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		case Kld::ResourceState::Present: return 0;
	}
	return 0;
}

VkPipelineStageFlags toVkPipelineStageFlags(uint32_t stageMask) noexcept
{
	VkPipelineStageFlags flags = 0;
	if (stageMask & Kld::PIPELINE_STAGE_TOP_OF_PIPE) {
		flags |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}
	if (stageMask & Kld::PIPELINE_STAGE_TRANSFER) {
		flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	if (stageMask & Kld::PIPELINE_STAGE_VERTEX_INPUT) {
		flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
	}
	if (stageMask & Kld::PIPELINE_STAGE_VERTEX_SHADER) {
		flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
	}
	if (stageMask & Kld::PIPELINE_STAGE_FRAGMENT_SHADER) {
		flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	if (stageMask & Kld::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT) {
		flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	if (stageMask & Kld::PIPELINE_STAGE_EARLY_FRAGMENT_TESTS) {
		flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	if (stageMask & Kld::PIPELINE_STAGE_LATE_FRAGMENT_TESTS) {
		flags |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	}
	if (stageMask & Kld::PIPELINE_STAGE_COMPUTE_SHADER) {
		flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
	if (stageMask & Kld::PIPELINE_STAGE_BOTTOM_OF_PIPE) {
		flags |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
	return flags;
}

VkAccessFlags toVkAccessFlags(uint32_t accessMask) noexcept
{
	VkAccessFlags flags = 0;
	if (accessMask & Kld::ACCESS_TRANSFER_READ) {
		flags |= VK_ACCESS_TRANSFER_READ_BIT;
	}
	if (accessMask & Kld::ACCESS_TRANSFER_WRITE) {
		flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	if (accessMask & Kld::ACCESS_VERTEX_ATTRIBUTE_READ) {
		flags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	}
	if (accessMask & Kld::ACCESS_INDEX_READ) {
		flags |= VK_ACCESS_INDEX_READ_BIT;
	}
	if (accessMask & Kld::ACCESS_UNIFORM_READ) {
		flags |= VK_ACCESS_UNIFORM_READ_BIT;
	}
	if (accessMask & Kld::ACCESS_SHADER_READ) {
		flags |= VK_ACCESS_SHADER_READ_BIT;
	}
	if (accessMask & Kld::ACCESS_SHADER_WRITE) {
		flags |= VK_ACCESS_SHADER_WRITE_BIT;
	}
	if (accessMask & Kld::ACCESS_COLOR_ATTACHMENT_READ) {
		flags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	}
	if (accessMask & Kld::ACCESS_COLOR_ATTACHMENT_WRITE) {
		flags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
	if (accessMask & Kld::ACCESS_DEPTH_STENCIL_READ) {
		flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	}
	if (accessMask & Kld::ACCESS_DEPTH_STENCIL_WRITE) {
		flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	if (accessMask & Kld::ACCESS_HOST_READ) {
		flags |= VK_ACCESS_HOST_READ_BIT;
	}
	if (accessMask & Kld::ACCESS_HOST_WRITE) {
		flags |= VK_ACCESS_HOST_WRITE_BIT;
	}
	return flags;
}

void transferBarrier(VkCommandBuffer commandBuffer) noexcept
{
	VkMemoryBarrier barrier {};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
			     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &barrier, 0, nullptr, 0,
			     nullptr);
}

} // namespace Vk
