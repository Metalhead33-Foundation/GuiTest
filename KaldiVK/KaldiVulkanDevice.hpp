#ifndef KALDIVULKANDEVICE_HPP
#define KALDIVULKANDEVICE_HPP
/**
 * @file KaldiVulkanDevice.hpp
 * @brief Vulkan implementation of the Kaldi rendering device contract.
 *
 * The device owns the instance/device/surface chain (`Vk::Context`) and therefore
 * the window-system half of the backend. `capabilities()` is probed from the
 * selected physical device; `processCommandQueue()` is still unimplemented, so
 * `Device` remains abstract until the command-queue implementation lands.
 */
#include <Kaldi/KldDevice.hpp>
#include "VkCommandExecutor.hpp"
#include "VkContext.hpp"
#include "KldImplLib.h"
#include <memory>
#include <span>

namespace Vk {
/**
 * @brief Vulkan implementation of the Kaldi rendering device contract.
 *
 * The device owns the context and a `CommandExecutor`. `capabilities()` is
 * probed from the selected physical device; `processCommandQueue()` delegates to
 * the executor, whose implemented command scope is documented in
 * `VkCommandExecutor.hpp` (buffers and the frame lifecycle today, with named
 * `UnsupportedBaseline` answers for the still-pending handlers).
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

	/** @brief Waits for idle, destroys the executor, device, surface and instance. */
	~Device() override;

	Device(const Device&) = delete;
	Device& operator=(const Device&) = delete;
	Device(Device&&) = delete;
	Device& operator=(Device&&) = delete;

	/** @brief The capabilities probed from the owned context, or none when detached. */
	const Kld::HalCapabilities& capabilities() const override { return capabilities_; }

	/** @brief Executes the command queue through the owned Vulkan executor. */
	Kld::HalStatus processCommandQueue(std::span<const Kld::GfxOp> commands) override;

	/** @brief The owned context, or @c nullptr for a detached device. */
	Context* context() noexcept { return context_.get(); }
	/** @brief The owned context, or @c nullptr for a detached device. */
	const Context* context() const noexcept { return context_.get(); }
	/** @brief The owned command executor, or @c nullptr for a detached device. */
	CommandExecutor* executor() noexcept { return executor_.get(); }
	/** @brief The owned command executor, or @c nullptr for a detached device. */
	const CommandExecutor* executor() const noexcept { return executor_.get(); }
	/** @brief True when the device owns a context. */
	bool hasContext() const noexcept { return context_ != nullptr; }

	/** @brief "Vulkan", or "none" when the device is detached. */
	const char* apiName() const noexcept;

private:
	std::unique_ptr<Context> context_;
	std::unique_ptr<CommandExecutor> executor_;
	Kld::HalCapabilities capabilities_ = Kld::halCapabilitiesNone();
};
}
#endif // KALDIVULKANDEVICE_HPP

