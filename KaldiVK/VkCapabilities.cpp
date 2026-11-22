/**
 * @file VkCapabilities.cpp
 * @brief Implements VkCapabilities.hpp.
 *
 * Everything here runs against a `VkPhysicalDevice` that the context already
 * selected, so no instance or device creation happens in this unit.
 */
#include "VkCapabilities.hpp"
#include <vector>

namespace Vk {

namespace {

/** @brief True when the graphics queue family can run compute commands. */
bool supportsComputeOnQueue(VkPhysicalDevice device, uint32_t family) noexcept
{
	if (vkGetPhysicalDeviceQueueFamilyProperties == nullptr) {
		return false;
	}
	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
	if (count == 0 || family >= count) {
		return false;
	}
	std::vector<VkQueueFamilyProperties> families(count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());
	return (families[family].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0;
}

/**
 * @brief True when BC1/BC3 sampled images can be uploaded and sampled.
 *
 * `VkUtil` maps the Euphemy DXT formats onto the BC1/BC3 block-compressed
 * formats, so those two decide the Kaldi texture-compression capability. Block
 * compression is optional in Vulkan, unlike in OpenGL where S3TC is ubiquitous.
 */
bool supportsBcFormats(VkPhysicalDevice device) noexcept
{
	if (vkGetPhysicalDeviceFormatProperties == nullptr) {
		return false;
	}
	constexpr VkFormatFeatureFlags required =
		VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
	const auto supports = [device](VkFormat format) {
		VkFormatProperties properties{};
		vkGetPhysicalDeviceFormatProperties(device, format, &properties);
		return (properties.optimalTilingFeatures & required) == required;
	};
	return supports(VK_FORMAT_BC1_RGB_UNORM_BLOCK) && supports(VK_FORMAT_BC3_UNORM_BLOCK);
}

} // namespace

uint32_t highestSampleCount(VkSampleCountFlags samples) noexcept
{
	static constexpr VkSampleCountFlagBits candidates[] = {
		VK_SAMPLE_COUNT_64_BIT,
		VK_SAMPLE_COUNT_32_BIT,
		VK_SAMPLE_COUNT_16_BIT,
		VK_SAMPLE_COUNT_8_BIT,
		VK_SAMPLE_COUNT_4_BIT,
		VK_SAMPLE_COUNT_2_BIT
	};
	for (const VkSampleCountFlagBits candidate : candidates) {
		if ((samples & static_cast<VkSampleCountFlags>(candidate)) != 0) {
			return static_cast<uint32_t>(candidate);
		}
	}
	// VK_SAMPLE_COUNT_1_BIT is mandatory for every format.
	return 1;
}

Kld::HalCapabilities probeCapabilities(const Context& context)
{
	Kld::HalCapabilities capabilities = Kld::halCapabilitiesNone();
	if (!context.isValid()) {
		return capabilities;
	}

	const VkPhysicalDeviceProperties& properties = context.properties();
	const VkPhysicalDeviceLimits& limits = properties.limits;

	capabilities.maxTexture1DSize = limits.maxImageDimension1D;
	capabilities.maxTexture2DSize = limits.maxImageDimension2D;
	capabilities.maxTexture3DSize = limits.maxImageDimension3D;
	capabilities.maxTextureArrayLayers = limits.maxImageArrayLayers;
	capabilities.maxCubeMapSize = limits.maxImageDimensionCube;
	capabilities.maxSamples = highestSampleCount(limits.framebufferColorSampleCounts);
	capabilities.maxColorAttachments = limits.maxColorAttachments;
	capabilities.maxVertexAttributes = limits.maxVertexInputAttributes;
	capabilities.maxVertexBuffers = limits.maxVertexInputBindings;
	capabilities.maxSampledTextureBindings = limits.maxPerStageDescriptorSampledImages;
	capabilities.maxSamplerBindings = limits.maxPerStageDescriptorSamplers;
	capabilities.maxUniformBufferBindings = limits.maxPerStageDescriptorUniformBuffers;
	capabilities.maxStorageBufferBindings = limits.maxPerStageDescriptorStorageBuffers;
	capabilities.maxStorageImageBindings = limits.maxPerStageDescriptorStorageImages;
	capabilities.maxFramesInFlight = Kld::kDefaultMaxFramesInFlight;

	capabilities.supportsCompute =
		supportsComputeOnQueue(context.physicalDevice(), context.graphicsQueueFamily());
	// vkCmdDrawIndirect/vkCmdDispatchIndirect are core Vulkan 1.0. A drawCount
	// above one additionally needs VkPhysicalDeviceFeatures::multiDrawIndirect,
	// which is a command-level validation later, not a different command class.
	capabilities.supportsIndirectDraw = true;
	// Occlusion and timestamp queries are core Vulkan 1.0.
	capabilities.supportsQueries = true;
	// VK_EXT_debug_utils/VK_EXT_debug_marker are not enabled by Vk::Context yet;
	// until they are, debug labels would be dropped instead of being consumed.
	capabilities.supportsDebugLabels = false;
	capabilities.supportsReadback = true;
	capabilities.supportsStorageResources =
		capabilities.maxStorageBufferBindings > 0 && capabilities.maxStorageImageBindings > 0;
	capabilities.supportsMultisampleResolve = capabilities.maxSamples > 1;
	capabilities.supportsTextureCompression = supportsBcFormats(context.physicalDevice());

	return capabilities;
}

}
