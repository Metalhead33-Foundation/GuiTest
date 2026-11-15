/**
 * @file FormatTranslationTests.cpp
 * @brief Verifies the portable -> API format tables of the backend helpers.
 *
 * The Vulkan and Direct3D backends cannot be initialised in a headless test, but
 * their format tables are pure functions, so the mappings that the backends rely
 * on can be checked here. On top of catching typos, these tests pin the *intended*
 * answers for formats the APIs cannot express (`VK_FORMAT_UNDEFINED`,
 * `DXGI_FORMAT_UNKNOWN`), which is what callers have to handle with a CPU-side
 * conversion.
 */
#include <catch2/catch_test_macros.hpp>
#include <Kaldi/KldVertexFormat.hpp>
#include <KaldiVK/VkUtil.hpp>
#if defined(KALDID3D_FORMAT_TESTS)
#include <KaldiD3DCommon/D3DUtil.hpp>
#endif
#include <cstddef>
#include <string_view>
#include <vector>

namespace {

using Euph::Media::Image::Format;

/** @brief Builds a Kaldi attribute descriptor without repeating the field list. */
Kld::AttributeDescriptor attribute(std::string_view semantic, Kld::PrimitiveType type, uintptr_t offset)
{
	Kld::AttributeDescriptor descriptor{};
	descriptor.SemanticName = semantic;
	descriptor.SemanticIndex = 0;
	descriptor.type = type;
	descriptor.offset = offset;
	return descriptor;
}

}

TEST_CASE("VkUtil maps Euphemy formats onto Vulkan formats", "[format][vulkan]")
{
	CHECK(Vk::translateToVkFormat(Format::R8U) == VK_FORMAT_R8_UNORM);
	CHECK(Vk::translateToVkFormat(Format::R16F) == VK_FORMAT_R16_SFLOAT);
	CHECK(Vk::translateToVkFormat(Format::RG32F) == VK_FORMAT_R32G32_SFLOAT);
	CHECK(Vk::translateToVkFormat(Format::RGB8U) == VK_FORMAT_R8G8B8_UNORM);
	CHECK(Vk::translateToVkFormat(Format::RGBA8U) == VK_FORMAT_R8G8B8A8_UNORM);
	CHECK(Vk::translateToVkFormat(Format::BGRA8U) == VK_FORMAT_B8G8R8A8_UNORM);
	CHECK(Vk::translateToVkFormat(Format::B_DXT1) == VK_FORMAT_BC1_RGB_UNORM_BLOCK);
	CHECK(Vk::translateToVkFormat(Format::B_DXT5) == VK_FORMAT_BC3_UNORM_BLOCK);

	// Formats Vulkan genuinely cannot express; callers must convert them on the CPU.
	CHECK(Vk::translateToVkFormat(Format::ARGB8U) == VK_FORMAT_UNDEFINED);
	CHECK(Vk::translateToVkFormat(Format::RGB565) == VK_FORMAT_UNDEFINED);
	CHECK(Vk::translateToVkFormat(Format::INDEXED) == VK_FORMAT_UNDEFINED);
}

TEST_CASE("VkUtil reports depth and stencil aspects", "[format][vulkan]")
{
	CHECK(Vk::isDepthFormat(Format::DEPTH24_STENCIL8));
	CHECK_FALSE(Vk::isDepthFormat(Format::RGBA8U));
	CHECK(Vk::hasStencilComponent(Format::DEPTH24_STENCIL8));
	CHECK_FALSE(Vk::hasStencilComponent(Format::DEPTH_COMPONENT16));
	CHECK(Vk::toVkAspectMask(Format::RGBA8U) == VK_IMAGE_ASPECT_COLOR_BIT);
	CHECK(Vk::toVkAspectMask(Format::DEPTH_COMPONENT32F) == VK_IMAGE_ASPECT_DEPTH_BIT);
	CHECK(Vk::toVkAspectMask(Format::DEPTH32F_STENCIL8) ==
		  (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT));
}

TEST_CASE("VkUtil maps Kaldi vertex formats and builds attribute descriptions", "[format][vulkan]")
{
	CHECK(Vk::translateToVkFormat(Kld::PrimitiveType::F32x1) == VK_FORMAT_R32_SFLOAT);
	CHECK(Vk::translateToVkFormat(Kld::PrimitiveType::F32x3) == VK_FORMAT_R32G32B32_SFLOAT);
	CHECK(Vk::translateToVkFormat(Kld::PrimitiveType::UN8x4) == VK_FORMAT_R8G8B8A8_UNORM);
	CHECK(Vk::translateToVkFormat(Kld::PrimitiveType::U32x2) == VK_FORMAT_R32G32_UINT);
	CHECK(Vk::vertexFormatByteSize(Kld::PrimitiveType::F32x3) == 12);
	CHECK(Vk::vertexFormatByteSize(Kld::PrimitiveType::UN8x4) == 4);

	const std::vector<Kld::AttributeDescriptor> attributes = {
		attribute("POSITION", Kld::PrimitiveType::F32x3, 0),
		attribute("TEXCOORD", Kld::PrimitiveType::F32x2, 12),
	};
	Kld::VertexDescriptor descriptor{};
	descriptor.stride = 20;
	descriptor.descriptors = attributes;

	const VkVertexInputBindingDescription binding = Vk::toVkVertexBinding(descriptor);
	CHECK(binding.binding == 0);
	CHECK(binding.stride == 20);
	CHECK(binding.inputRate == VK_VERTEX_INPUT_RATE_VERTEX);

	const std::vector<VkVertexInputAttributeDescription> translated = Vk::toVkVertexAttributes(descriptor);
	REQUIRE(translated.size() == 2);
	CHECK(translated[0].location == 0);
	CHECK(translated[0].format == VK_FORMAT_R32G32B32_SFLOAT);
	CHECK(translated[0].offset == 0);
	CHECK(translated[1].location == 1);
	CHECK(translated[1].offset == 12);
}

TEST_CASE("VkUtil maps sampler state, usage and synchronisation metadata", "[format][vulkan]")
{
	CHECK(Vk::toVkFilter(Kld::SamplerFiltering::NEAREST) == VK_FILTER_NEAREST);
	CHECK(Vk::toVkFilter(Kld::SamplerFiltering::LINEAR_MIPMAP_LINEAR) == VK_FILTER_LINEAR);
	CHECK(Vk::toVkMipmapMode(Kld::SamplerFiltering::LINEAR_MIPMAP_LINEAR) == VK_SAMPLER_MIPMAP_MODE_LINEAR);
	CHECK(Vk::toVkMipmapMode(Kld::SamplerFiltering::LINEAR) == VK_SAMPLER_MIPMAP_MODE_NEAREST);
	CHECK(Vk::toVkAddressMode(Kld::SamplerWrapping::REPEAT) == VK_SAMPLER_ADDRESS_MODE_REPEAT);

	CHECK(Vk::toVkImageUsage(Kld::IMAGE_USAGE_SAMPLED | Kld::IMAGE_USAGE_TRANSFER_DST) ==
		  (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT));
	CHECK(Vk::toVkImageType(Kld::TextureBindingType::Texture3D) == VK_IMAGE_TYPE_3D);
	CHECK(Vk::toVkImageViewType(Kld::TextureBindingType::ArrayTexture2D) == VK_IMAGE_VIEW_TYPE_2D_ARRAY);

	CHECK(Vk::toVkImageLayout(Kld::ResourceState::ColorAttachment) == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	CHECK(Vk::toVkImageLayout(Kld::ResourceState::Present) == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	CHECK(Vk::toVkAccessFlags(Kld::ResourceState::CopySource) == VK_ACCESS_TRANSFER_READ_BIT);

	const VkPipelineStageFlags stages = Vk::toVkPipelineStageFlags(
		Kld::PIPELINE_STAGE_TRANSFER | Kld::PIPELINE_STAGE_FRAGMENT_SHADER);
	CHECK((stages & VK_PIPELINE_STAGE_TRANSFER_BIT) != 0);
	CHECK((stages & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT) != 0);
	CHECK((stages & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) == 0);
}

#if defined(KALDID3D_FORMAT_TESTS)

TEST_CASE("D3DUtil maps Euphemy formats onto DXGI formats", "[format][direct3d]")
{
	CHECK(D3D::translateToDxgiFormat(Format::R8U) == DXGI_FORMAT_R8_UNORM);
	CHECK(D3D::translateToDxgiFormat(Format::RGBA8U) == DXGI_FORMAT_R8G8B8A8_UNORM);
	CHECK(D3D::translateToDxgiFormat(Format::BGRA8U) == DXGI_FORMAT_B8G8R8A8_UNORM);
	CHECK(D3D::translateToDxgiFormat(Format::RGBA16F) == DXGI_FORMAT_R16G16B16A16_FLOAT);
	CHECK(D3D::translateToDxgiFormat(Format::DEPTH24_STENCIL8) == DXGI_FORMAT_D24_UNORM_S8_UINT);
	CHECK(D3D::translateToDxgiFormat(Format::B_DXT5) == DXGI_FORMAT_BC3_UNORM);

	// Documented gaps: DXGI has no 24-bit RGB, no R16_SNORM, no ARGB order, no
	// 8-bit stencil-only format, and its packed layouts are B-high.
	CHECK(D3D::translateToDxgiFormat(Format::RGB8U) == DXGI_FORMAT_UNKNOWN);
	CHECK(D3D::translateToDxgiFormat(Format::R16S) == DXGI_FORMAT_UNKNOWN);
	CHECK(D3D::translateToDxgiFormat(Format::ARGB8U) == DXGI_FORMAT_UNKNOWN);
	CHECK(D3D::translateToDxgiFormat(Format::STENCIL_INDEX8) == DXGI_FORMAT_UNKNOWN);
	CHECK(D3D::translateToDxgiFormat(Format::RGB565) == DXGI_FORMAT_UNKNOWN);

	CHECK(D3D::isDepthFormat(Format::DEPTH_COMPONENT32F));
	CHECK(D3D::isCompressedFormat(Format::B_DXT3));
	CHECK_FALSE(D3D::isCompressedFormat(Format::RGBA8U));
}

TEST_CASE("D3DUtil maps Kaldi vertex formats onto DXGI formats", "[format][direct3d]")
{
	CHECK(D3D::translateToDxgiFormat(Kld::PrimitiveType::F32x1) == DXGI_FORMAT_R32_FLOAT);
	CHECK(D3D::translateToDxgiFormat(Kld::PrimitiveType::F32x4) == DXGI_FORMAT_R32G32B32A32_FLOAT);
	CHECK(D3D::translateToDxgiFormat(Kld::PrimitiveType::UN8x4) == DXGI_FORMAT_R8G8B8A8_UNORM);
	CHECK(D3D::translateToDxgiFormat(Kld::PrimitiveType::U8x4) == DXGI_FORMAT_R8G8B8A8_UINT);
	CHECK(D3D::translateToDxgiFormat(Kld::PrimitiveType::SN16x2) == DXGI_FORMAT_R16G16_SNORM);
	CHECK(D3D::translateToDxgiFormat(Kld::PrimitiveType::SN16x1) == DXGI_FORMAT_UNKNOWN);
	CHECK(D3D::translateToDxgiFormat(Kld::PrimitiveType::F1x64) == DXGI_FORMAT_UNKNOWN);

	CHECK(D3D::vertexFormatByteSize(Kld::PrimitiveType::F32x4) == 16);
	CHECK(D3D::vertexFormatByteSize(Kld::PrimitiveType::INVALID) == 0);
	CHECK(D3D::isIntegerVertexFormat(Kld::PrimitiveType::U16x2));
	CHECK_FALSE(D3D::isIntegerVertexFormat(Kld::PrimitiveType::UN16x2));

	CHECK(D3D::mipLevelCount(1024, 1024) == 11);
	CHECK(D3D::mipLevelCount(1, 1) == 1);
	CHECK(D3D::alignUp(257, D3D::textureAlignment) == 512);
	CHECK(D3D::alignUp(256, D3D::textureAlignment) == 256);
}

#endif
