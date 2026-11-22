/**
 * @file VkContext.cpp
 * @brief Implements VkContext.hpp.
 *
 * glad is bootstrapped in three steps, because each step only knows about the
 * entry points of the level below it: global (`vkCreateInstance`), instance
 * (`vkEnumeratePhysicalDevices`, `vkCreateDevice`), then device level.
 */
#include "VkContext.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace Vk {

namespace {

/** @brief Builds the exception message for a failed Vulkan call. */
[[noreturn]] void throwVkFailure(const char* step, VkResult result)
{
	std::string message = "VkContext: ";
	message += step;
	message += " failed (";
	message += resultName(result);
	message += ")";
	throw std::runtime_error(message);
}

/** @brief Loads the global Vulkan entry points through glad. */
void loadGlobalFunctions()
{
	if (gladLoaderLoadVulkan(VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE) == 0) {
		throw std::runtime_error("VkContext: the Vulkan loader could not be loaded");
	}
}

/** @brief True when the instance extension @p name is offered by the loader. */
bool instanceExtensionAvailable(const char* name)
{
	uint32_t count = 0;
	if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) != VK_SUCCESS || count == 0) {
		return false;
	}
	std::vector<VkExtensionProperties> available(count);
	if (vkEnumerateInstanceExtensionProperties(nullptr, &count, available.data()) != VK_SUCCESS) {
		return false;
	}
	return std::any_of(available.begin(), available.end(), [name](const VkExtensionProperties& extension) {
		return std::strcmp(extension.extensionName, name) == 0;
	});
}

/** @brief True when the instance layer @p name is installed. */
bool instanceLayerAvailable(const char* name)
{
	uint32_t count = 0;
	if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS || count == 0) {
		return false;
	}
	std::vector<VkLayerProperties> available(count);
	if (vkEnumerateInstanceLayerProperties(&count, available.data()) != VK_SUCCESS) {
		return false;
	}
	return std::any_of(available.begin(), available.end(), [name](const VkLayerProperties& layer) {
		return std::strcmp(layer.layerName, name) == 0;
	});
}

/** @brief True when the physical device offers the extension @p name. */
bool deviceExtensionAvailable(VkPhysicalDevice device, const char* name)
{
	uint32_t count = 0;
	if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS || count == 0) {
		return false;
	}
	std::vector<VkExtensionProperties> available(count);
	if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available.data()) != VK_SUCCESS) {
		return false;
	}
	return std::any_of(available.begin(), available.end(), [name](const VkExtensionProperties& extension) {
		return std::strcmp(extension.extensionName, name) == 0;
	});
}

/** @brief Queue family indices that the context needs. */
struct QueueSelection {
	uint32_t graphics = 0;
	uint32_t present = 0;
	bool valid = false;
};

/** @brief Finds graphics and present queue families on @p device. */
QueueSelection selectQueues(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
	if (familyCount == 0) {
		return {};
	}
	std::vector<VkQueueFamilyProperties> families(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());

	QueueSelection selection;
	bool haveGraphics = false;
	for (uint32_t index = 0; index < familyCount; ++index) {
		if ((families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) {
			continue;
		}
		VkBool32 supportsPresent = VK_FALSE;
		if (vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &supportsPresent) != VK_SUCCESS) {
			continue;
		}
		if (!haveGraphics) {
			selection.graphics = index;
			haveGraphics = true;
		}
		if (supportsPresent == VK_TRUE) {
			selection.present = index;
			selection.valid = true;
			// A family that can do both is the common case and keeps the device
			// creation down to a single queue.
			if (selection.graphics == index) {
				return selection;
			}
		}
	}
	if (!selection.valid && haveGraphics) {
		// Graphics works, presentation does not: the surface is not usable.
		return {};
	}
	return selection;
}

/** @brief Scores a candidate device; higher is better, 0 means unusable. */
int scoreDevice(const VkPhysicalDeviceProperties& properties, int32_t index, const ContextConfig& config)
{
	if (config.preferredDeviceIndex >= 0) {
		return config.preferredDeviceIndex == index ? 100000 : 0;
	}
	if (!config.preferredDeviceName.empty()) {
		const bool matches = std::string_view(properties.deviceName).find(config.preferredDeviceName) != std::string_view::npos;
		return matches ? 100000 : 0;
	}
	int score = 1;
	switch (properties.deviceType) {
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: score += config.preferDiscreteGpu ? 100 : 10; break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score += 50; break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: score += 40; break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU: score += 1; break;
		default: score += 5; break;
	}
	return score;
}

} // namespace

Context::Context(const Kld::NativeWindowHandles& window, const ContextConfig& config)
	: config_(config)
{
	try {
		loadGlobalFunctions();

		// ── Instance ──────────────────────────────────────────────────────────
		std::vector<const char*> extensions = requiredInstanceExtensions(window.system);
		for (const char* required : extensions) {
			if (!instanceExtensionAvailable(required)) {
				throw std::runtime_error(std::string("VkContext: the Vulkan loader does not offer ") + required);
			}
		}
		// Optional: command-buffer debug labels. Enabling it is what makes
		// HalCapabilities::supportsDebugLabels truthful, so try before falling
		// back to silently dropped labels.
		if (instanceExtensionAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			debugUtils_ = true;
		}

		std::vector<const char*> layers;
		if (config_.enableValidation && instanceLayerAvailable("VK_LAYER_KHRONOS_validation")) {
			layers.push_back("VK_LAYER_KHRONOS_validation");
		}

		const std::string applicationName(config_.applicationName);
		const std::string engineName(config_.engineName);
		VkApplicationInfo applicationInfo{};
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pApplicationName = applicationName.c_str();
		applicationInfo.applicationVersion = config_.applicationVersion;
		applicationInfo.pEngineName = engineName.c_str();
		applicationInfo.engineVersion = config_.engineVersion;
		applicationInfo.apiVersion = config_.apiVersion;

		VkInstanceCreateInfo instanceInfo{};
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.pApplicationInfo = &applicationInfo;
		instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		instanceInfo.ppEnabledExtensionNames = extensions.data();
		instanceInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
		instanceInfo.ppEnabledLayerNames = layers.data();

		VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance_);
		if (result != VK_SUCCESS) {
			throwVkFailure("vkCreateInstance", result);
		}
		if (gladLoaderLoadVulkan(instance_, VK_NULL_HANDLE, VK_NULL_HANDLE) == 0) {
			throw std::runtime_error("VkContext: the instance entry points could not be loaded");
		}

		surface_ = createSurface(instance_, window);

		// ── Physical device ───────────────────────────────────────────────────
		uint32_t deviceCount = 0;
		result = vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
		if (result != VK_SUCCESS) {
			throwVkFailure("vkEnumeratePhysicalDevices", result);
		}
		if (deviceCount == 0) {
			throw std::runtime_error("VkContext: no Vulkan-capable physical device was found");
		}
		std::vector<VkPhysicalDevice> devices(deviceCount);
		result = vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());
		if (result != VK_SUCCESS) {
			throwVkFailure("vkEnumeratePhysicalDevices", result);
		}

		int bestScore = 0;
		QueueSelection bestQueues;
		for (int32_t index = 0; index < static_cast<int32_t>(deviceCount); ++index) {
			const VkPhysicalDevice candidate = devices[static_cast<std::size_t>(index)];
			if (config_.requireSwapchain && !deviceExtensionAvailable(candidate, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
				continue;
			}
			const QueueSelection queues = selectQueues(candidate, surface_);
			if (!queues.valid) {
				continue;
			}
			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(candidate, &properties);
			const int score = scoreDevice(properties, index, config_);
			if (score > bestScore) {
				bestScore = score;
				physicalDevice_ = candidate;
				bestQueues = queues;
				properties_ = properties;
			}
		}
		if (physicalDevice_ == VK_NULL_HANDLE) {
			throw std::runtime_error("VkContext: no physical device can present to this window");
		}
		graphicsQueueFamily_ = bestQueues.graphics;
		presentQueueFamily_ = bestQueues.present;

		// ── Logical device ────────────────────────────────────────────────────
		const float priority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> queueInfos;
		VkDeviceQueueCreateInfo graphicsQueueInfo{};
		graphicsQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		graphicsQueueInfo.queueFamilyIndex = graphicsQueueFamily_;
		graphicsQueueInfo.queueCount = 1;
		graphicsQueueInfo.pQueuePriorities = &priority;
		queueInfos.push_back(graphicsQueueInfo);
		if (presentQueueFamily_ != graphicsQueueFamily_) {
			VkDeviceQueueCreateInfo presentQueueInfo = graphicsQueueInfo;
			presentQueueInfo.queueFamilyIndex = presentQueueFamily_;
			queueInfos.push_back(presentQueueInfo);
		}

		std::vector<const char*> deviceExtensions;
		if (deviceExtensionAvailable(physicalDevice_, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
			deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		// Enable every optional feature the device offers. Vulkan validates
		// structures (depth clamp, wide lines, anisotropic samplers, BC formats,
		// independent blend, ...) against the enabled set, and the portable HAL has
		// no per-feature negotiation step, so requesting the whole supported set is
		// both correct and the only way to keep those translations valid.
		VkPhysicalDeviceFeatures supported{};
		vkGetPhysicalDeviceFeatures(physicalDevice_, &supported);
		supported.robustBufferAccess = VK_FALSE;
		supported.sparseBinding = VK_FALSE;
		supported.sparseResidencyBuffer = VK_FALSE;
		supported.sparseResidencyImage2D = VK_FALSE;
		supported.sparseResidencyImage3D = VK_FALSE;
		supported.sparseResidency2Samples = VK_FALSE;
		supported.sparseResidency4Samples = VK_FALSE;
		supported.sparseResidency8Samples = VK_FALSE;
		supported.sparseResidency16Samples = VK_FALSE;
		supported.sparseResidencyAliased = VK_FALSE;
		supported.shaderResourceResidency = VK_FALSE;
		supported.variableMultisampleRate = VK_FALSE;
		supported.inheritedQueries = VK_FALSE;
		features_ = supported;

		VkDeviceCreateInfo deviceInfo{};
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
		deviceInfo.pQueueCreateInfos = queueInfos.data();
		deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
		deviceInfo.pEnabledFeatures = &features_;

		result = vkCreateDevice(physicalDevice_, &deviceInfo, nullptr, &device_);
		if (result != VK_SUCCESS) {
			throwVkFailure("vkCreateDevice", result);
		}
		if (gladLoaderLoadVulkan(instance_, physicalDevice_, device_) == 0) {
			throw std::runtime_error("VkContext: the device entry points could not be loaded");
		}

		vkGetDeviceQueue(device_, graphicsQueueFamily_, 0, &graphicsQueue_);
		vkGetDeviceQueue(device_, presentQueueFamily_, 0, &presentQueue_);
	} catch (...) {
		destroy();
		throw;
	}
}

void Context::destroy() noexcept
{
	if (device_ != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(device_);
		vkDestroyDevice(device_, nullptr);
		device_ = VK_NULL_HANDLE;
	}
	if (surface_ != VK_NULL_HANDLE && instance_ != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(instance_, surface_, nullptr);
		surface_ = VK_NULL_HANDLE;
	}
	if (instance_ != VK_NULL_HANDLE) {
		vkDestroyInstance(instance_, nullptr);
		instance_ = VK_NULL_HANDLE;
	}
	gladLoaderUnloadVulkan();
}

Context::~Context()
{
	destroy();
}

}
