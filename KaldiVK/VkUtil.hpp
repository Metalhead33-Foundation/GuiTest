#ifndef KALDIVKUTIL_HPP
#define KALDIVKUTIL_HPP
/**
 * @file VkUtil.hpp
 * @brief Vulkan equivalents of `KaldiGLCommon/GlUtil.hpp`.
 *
 * The GL backends have `GlUtil` to translate Euphemy image formats and Kaldi
 * metadata into driver state. The Vulkan backend needs exactly the same
 * translations, only to `VkFormat`, `VkVertexInputAttributeDescription`,
 * `VkImageLayout`, ... This header is that counterpart.
 *
 * Everything here is a pure translation function: no device, no allocation, no
 * command buffer. That keeps the header usable from unit tests and from the
 * backend's own recording code.
 *
 * `VK_FORMAT_UNDEFINED` is the "cannot express this" answer. Vulkan - unlike
 * OpenGL - genuinely cannot express ARGB memory order, three-channel 8/16-bit
 * SNORM, or the packed `RGB444` layouts; those need a CPU-side conversion in
 * Euphemy before they reach the backend.
 */
#include <Kaldi/KaldiOperation.hpp>
#include <Kaldi/KldVertexFormat.hpp>
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <glad/vulkan.h>
#include <cstdint>
#include <vector>

namespace Vk {

// ─────────────────────────────────────────────────────────────────────────────
// Formats
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Translates a Euphemy image format to its Vulkan equivalent. */
VkFormat translateToVkFormat(Euph::Media::Image::Format format) noexcept;

/** @brief Translates a portable Kaldi vertex attribute format to Vulkan. */
VkFormat translateToVkFormat(Kld::PrimitiveType type) noexcept;

/** @brief Byte size of one vertex attribute element, or @c 0 when unknown. */
uint32_t vertexFormatByteSize(Kld::PrimitiveType type) noexcept;

/** @brief True when the Euphemy format describes a depth or depth/stencil format. */
bool isDepthFormat(Euph::Media::Image::Format format) noexcept;

/** @brief True when the Euphemy format is a block-compressed (BC1-BC3) format. */
bool isCompressedFormat(Euph::Media::Image::Format format) noexcept;

/** @brief True when the format has a stencil aspect. */
bool hasStencilComponent(Euph::Media::Image::Format format) noexcept;

/** @brief Aspect mask matching the image format. */
VkImageAspectFlags toVkAspectMask(Euph::Media::Image::Format format) noexcept;

// ─────────────────────────────────────────────────────────────────────────────
// Vertex input
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Describes the single vertex stream Kaldi models today.
 *
 * `Kld::VertexDescriptor` has one stride and no stream index, so the backend uses
 * one binding at slot 0 with `VK_VERTEX_INPUT_RATE_VERTEX`.
 */
VkVertexInputBindingDescription toVkVertexBinding(const Kld::VertexDescriptor& descriptor) noexcept;

/**
 * @brief Builds vertex attribute descriptions from a Kaldi vertex descriptor.
 *
 * The attribute `location` is the index inside `Kld::VertexDescriptor::descriptors`,
 * so the vertex shader's `layout(location = N)` must match that order.
 */
std::vector<VkVertexInputAttributeDescription> toVkVertexAttributes(const Kld::VertexDescriptor& descriptor);

// ─────────────────────────────────────────────────────────────────────────────
// Images and samplers
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Base image type of a Kaldi texture binding kind. */
VkImageType toVkImageType(Kld::TextureBindingType type) noexcept;

/** @brief View type of a Kaldi texture binding kind. */
VkImageViewType toVkImageViewType(Kld::TextureBindingType type) noexcept;

/** @brief Translates Kaldi's `ImageUsage` bitmask into `VkImageUsageFlags`. */
VkImageUsageFlags toVkImageUsage(uint32_t imageUsage) noexcept;

/** @brief Magnification/minification filter for a Kaldi filtering mode. */
VkFilter toVkFilter(Kld::SamplerFiltering filtering) noexcept;

/** @brief Mipmap filter for a Kaldi filtering mode. */
VkSamplerMipmapMode toVkMipmapMode(Kld::SamplerFiltering filtering) noexcept;

/** @brief Address mode for a Kaldi wrapping mode. */
VkSamplerAddressMode toVkAddressMode(Kld::SamplerWrapping wrapping) noexcept;

// ─────────────────────────────────────────────────────────────────────────────
// Synchronisation
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Image layout that matches a Kaldi coarse resource state. */
VkImageLayout toVkImageLayout(Kld::ResourceState state) noexcept;

/** @brief Access mask that matches a Kaldi coarse resource state. */
VkAccessFlags toVkAccessFlags(Kld::ResourceState state) noexcept;

/** @brief Translates Kaldi's `PipelineStageFlags` bitmask into `VkPipelineStageFlags`. */
VkPipelineStageFlags toVkPipelineStageFlags(uint32_t stageMask) noexcept;

/** @brief Translates Kaldi's `AccessFlags` bitmask into `VkAccessFlags`. */
VkAccessFlags toVkAccessFlags(uint32_t accessMask) noexcept;

}

#endif // KALDIVKUTIL_HPP
