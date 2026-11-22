#ifndef KALDIVKCAPABILITIES_HPP
#define KALDIVKCAPABILITIES_HPP
/**
 * @file VkCapabilities.hpp
 * @brief Probes `Kld::HalCapabilities` from a live Vulkan context.
 *
 * The Vulkan backend gets its limits from `VkPhysicalDeviceProperties::limits`,
 * which is why the probe takes the whole `Vk::Context` (it already holds the
 * physical device, its properties and the queue family it renders on). Only the
 * few fields Vulkan cannot answer from `limits` alone need extra queries:
 *
 * | `Kld::HalCapabilities` field | Vulkan source |
 * |---|---|
 * | `maxTexture1DSize` / `2DSize` / `3DSize` | `maxImageDimension1D/2D/3D` |
 * | `maxTextureArrayLayers` | `maxImageArrayLayers` |
 * | `maxCubeMapSize` | `maxImageDimensionCube` |
 * | `maxSamples` | highest bit of `framebufferColorSampleCounts` |
 * | `maxColorAttachments` | `maxColorAttachments` |
 * | `maxVertexAttributes` / `maxVertexBuffers` | `maxVertexInputAttributes` / `maxVertexInputBindings` |
 * | `maxSampledTextureBindings` / `maxSamplerBindings` | `maxPerStageDescriptorSampledImages` / `maxPerStageDescriptorSamplers` |
 * | `maxUniformBufferBindings` | `maxPerStageDescriptorUniformBuffers` |
 * | `maxStorageBufferBindings` / `maxStorageImageBindings` | `maxPerStageDescriptorStorageBuffers` / `maxPerStageDescriptorStorageImages` |
 * | `supportsCompute` | `VK_QUEUE_COMPUTE_BIT` on the context's graphics queue family |
 * | `supportsTextureCompression` | `VK_FORMAT_BC1_RGB_UNORM_BLOCK` / `BC3` with sampled + transfer-dst features |
 * | `supportsStorageResources` | non-zero per-stage storage descriptor limits |
 * | `supportsMultisampleResolve` | `maxSamples > 1` (`vkCmdResolveImage` is core) |
 * | `supportsIndirectDraw`, `supportsQueries`, `supportsReadback` | always true (Vulkan 1.0 core) |
 * | `supportsDebugLabels` | false until `VK_EXT_debug_marker`/`VK_EXT_debug_utils` is enabled |
 *
 * Per-stage descriptor limits are used for the binding counts: a Kaldi binding
 * layout is one pipeline layout, so the per-stage number is the portable bound a
 * renderer can plan against. `maxFramesInFlight` reports
 * `Kld::kDefaultMaxFramesInFlight`, because the context does not own a swapchain.
 */
#include "VkContext.hpp"
#include <Kaldi/KldHal.hpp>
#include <cstdint>

namespace Vk {

/**
 * @brief Highest sample count present in @p samples.
 * @return 64, 32, 16, 8, 4 or 2 for the matching bit, and 1 for an empty mask
 *         (Vulkan always supports one sample).
 */
uint32_t highestSampleCount(VkSampleCountFlags samples) noexcept;

/**
 * @brief Probes the HAL capability snapshot of @p context.
 * @return The probed snapshot, or `Kld::halCapabilitiesNone()` when the context
 *         is not valid.
 *
 * The probe reads `VkPhysicalDeviceProperties` from the context and issues two
 * extra physical-device queries (queue-family flags and BC format support). It
 * does not allocate GPU resources and does not touch the command queues.
 */
Kld::HalCapabilities probeCapabilities(const Context& context);

}

#endif // KALDIVKCAPABILITIES_HPP
