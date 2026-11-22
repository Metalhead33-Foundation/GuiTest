#ifndef KALDIVKCONTEXT_HPP
#define KALDIVKCONTEXT_HPP
/**
 * @file VkContext.hpp
 * @brief The Vulkan equivalent of the GL context layer.
 *
 * Where the GL backends need a `Context` (EGL/GLX/WGL) that owns a drawable,
 * Vulkan needs a whole chain of objects before a single command can be recorded:
 * `VkInstance` -> `VkSurfaceKHR` -> `VkPhysicalDevice` -> `VkDevice` -> queues.
 * `Vk::Context` owns exactly that chain and nothing more; swapchain images and
 * command recording belong to the backend implementation.
 *
 * The platform half (`VkPlatform.hpp`) turns `Kld::NativeWindowHandles` into a
 * surface, so Vulkan windows come from the same SDL extraction the GL backends
 * use - no second copy of the `SDL_SysWMinfo` switch.
 *
 * @section kaldivk_context_device_selection Device selection
 *
 * Standard practice is "pick the best device", which is what happens when
 * @ref ContextConfig::preferredDeviceName and @ref ContextConfig::preferredDeviceIndex
 * are left at their defaults: discrete GPUs win over integrated ones, and a device
 * without a graphics queue or without `VK_KHR_swapchain` is skipped entirely.
 * A configuration file can override either field.
 */
#include "VkPlatform.hpp"
#include <string>
#include <string_view>

namespace Vk {

/** @brief Everything the caller may tune when the instance and device are created. */
struct ContextConfig {
	/** @brief `VkApplicationInfo::pApplicationName`. */
	std::string_view applicationName = "Kaldi";
	/** @brief `VkApplicationInfo::pEngineName`. */
	std::string_view engineName = "Kaldi";
	/** @brief Application version reported to the driver. */
	uint32_t applicationVersion = 0;
	/** @brief Engine version reported to the driver. */
	uint32_t engineVersion = 0;
	/** @brief Instance API version requested from the loader. */
	uint32_t apiVersion = VK_API_VERSION_1_0;
	/** @brief Enable `VK_LAYER_KHRONOS_validation` when it is installed. */
	bool enableValidation = false;
	/** @brief Only accept a device that offers `VK_KHR_swapchain`. */
	bool requireSwapchain = true;
	/** @brief Substring of the preferred device name; empty means "automatic". */
	std::string_view preferredDeviceName;
	/** @brief Index into `vkEnumeratePhysicalDevices`; -1 means "automatic". */
	int32_t preferredDeviceIndex = -1;
	/** @brief Give discrete GPUs a bonus during automatic selection. */
	bool preferDiscreteGpu = true;
};

/**
 * @brief Owns the instance, surface, physical/logical device and its queues.
 *
 * @note Not copyable and not movable: the destructor has to run in a defined
 * order (`vkDeviceWaitIdle` before destroying handles) and a moved-from Vulkan
 * context would be indistinguishable from a valid one.
 */
class Context final {
public:
	/**
	 * @brief Creates the whole instance/device chain for @p window.
	 * @param window Native window handles, normally from `Kld::nativeWindowHandlesFrom`.
	 * @param config Instance/device tuning and device-selection hints.
	 * @throws std::invalid_argument When @p window has no possible surface.
	 * @throws std::runtime_error On any Vulkan failure; the message names the call
	 *         and carries `resultName()`.
	 */
	Context(const Kld::NativeWindowHandles& window, const ContextConfig& config = ContextConfig());

	/** @brief Waits for idle and destroys the device, surface and instance. */
	~Context();

	Context(const Context&) = delete;
	Context& operator=(const Context&) = delete;
	Context(Context&&) = delete;
	Context& operator=(Context&&) = delete;

	/** @brief The instance. */
	VkInstance instance() const noexcept { return instance_; }
	/** @brief The window surface. */
	VkSurfaceKHR surface() const noexcept { return surface_; }
	/** @brief The chosen physical device. */
	VkPhysicalDevice physicalDevice() const noexcept { return physicalDevice_; }
	/** @brief The logical device. */
	VkDevice device() const noexcept { return device_; }
	/** @brief Queue used for rendering and transfers. */
	VkQueue graphicsQueue() const noexcept { return graphicsQueue_; }
	/** @brief Queue used for presentation (may equal graphicsQueue()). */
	VkQueue presentQueue() const noexcept { return presentQueue_; }
	/** @brief Queue family index of graphicsQueue(). */
	uint32_t graphicsQueueFamily() const noexcept { return graphicsQueueFamily_; }
	/** @brief Queue family index of presentQueue(). */
	uint32_t presentQueueFamily() const noexcept { return presentQueueFamily_; }
	/** @brief `VkPhysicalDeviceProperties` of the chosen device. */
	const VkPhysicalDeviceProperties& properties() const noexcept { return properties_; }
	/**
	 * @brief The features the logical device was created with.
	 *
	 * The context enables every optional feature the physical device supports
	 * (robust buffer access, the sparse-residency family, shader resource
	 * residency and inherited queries excepted), because the executor translates
	 * portable state - anisotropic filtering, wide lines, depth clamp, independent
	 * blend, BC compression - into Vulkan structures that are only valid when the
	 * matching feature is enabled. Backends must still check this snapshot before
	 * relying on one of those options.
	 */
	const VkPhysicalDeviceFeatures& enabledFeatures() const noexcept { return features_; }
	/** @brief The configuration the context was created with. */
	const ContextConfig& config() const noexcept { return config_; }
	/** @brief `VkPhysicalDeviceProperties::deviceName`. */
	std::string_view deviceName() const noexcept { return std::string_view(properties_.deviceName); }
	/** @brief True when the instance, device and surface are all valid. */
	bool isValid() const noexcept
	{
		return instance_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE && surface_ != VK_NULL_HANDLE;
	}

private:
	/** @brief Destroys the chain in reverse creation order; safe on partial construction. */
	void destroy() noexcept;

	ContextConfig config_;
	VkInstance instance_ = VK_NULL_HANDLE;
	VkSurfaceKHR surface_ = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
	VkDevice device_ = VK_NULL_HANDLE;
	VkQueue graphicsQueue_ = VK_NULL_HANDLE;
	VkQueue presentQueue_ = VK_NULL_HANDLE;
	uint32_t graphicsQueueFamily_ = 0;
	uint32_t presentQueueFamily_ = 0;
	VkPhysicalDeviceProperties properties_{};
	VkPhysicalDeviceFeatures features_{};
};

}

#endif // KALDIVKCONTEXT_HPP
