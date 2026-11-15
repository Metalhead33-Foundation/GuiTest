#ifndef KALDIVULKANDEVICE_HPP
#define KALDIVULKANDEVICE_HPP
/**
 * @file KaldiVulkanDevice.hpp
 * @brief Vulkan implementation of the Kaldi rendering device contract.
 *
 * The device owns the instance/device/surface chain (`Vk::Context`) and therefore
 * the window-system half of the backend. Command translation
 * (`Kld::Device::capabilities()` and `Kld::Device::processCommandQueue()`) is
 * still unimplemented, so `Device` remains abstract: the constructors below are
 * the ownership skeleton that the first command-queue implementation inherits.
 */
#include <Kaldi/KldDevice.hpp>
#include "VkContext.hpp"
#include "KldImplLib.h"
#include <memory>

namespace Vk {
/**
 * @brief Vulkan implementation of the Kaldi rendering device contract.
 *
 * @note Abstract: the command-queue half of `Kld::Device` is not implemented yet.
 */
class MH_KALDI_IMPL_API Device : public Kld::Device
{
public:
	/** @brief Creates a detached device without a context. */
	Device();

	/**
	 * @brief Creates a device and the whole Vulkan instance chain for @p window.
	 * @param window SDL window-manager info of the window to present to.
	 * @param config Instance/device tuning and device-selection hints.
	 * @throws std::invalid_argument When @p window has no usable native drawable.
	 * @throws std::runtime_error When any Vulkan object cannot be created.
	 */
	explicit Device(const SDL_SysWMinfo& window, const ContextConfig& config = ContextConfig());

	/** @brief Waits for idle and destroys the device, surface and instance. */
	~Device() override;

	Device(const Device&) = delete;
	Device& operator=(const Device&) = delete;
	Device(Device&&) = delete;
	Device& operator=(Device&&) = delete;

	/** @brief The owned context, or @c nullptr for a detached device. */
	Context* context() noexcept { return context_.get(); }
	/** @brief The owned context, or @c nullptr for a detached device. */
	const Context* context() const noexcept { return context_.get(); }
	/** @brief True when the device owns a context. */
	bool hasContext() const noexcept { return context_ != nullptr; }

	/** @brief "Vulkan", or "none" when the device is detached. */
	const char* apiName() const noexcept;

private:
	std::unique_ptr<Context> context_;
};
}
#endif // KALDIVULKANDEVICE_HPP

