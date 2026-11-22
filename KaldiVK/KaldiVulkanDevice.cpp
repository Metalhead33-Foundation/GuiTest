#include "KaldiVulkanDevice.hpp"
#include "VkCapabilities.hpp"
#include <stdexcept>
#include <string>

namespace {
/**
 * @brief Last failure recorded on this thread.
 *
 * Creation failures and command failures both land here, so `kld_get_last_error_message()`
 * reports a real message for either; a successful call leaves it untouched.
 */
thread_local std::string vkLastError;

/** @brief Records @p status in the thread-local error slot when it is a failure. */
void recordFailure(const Kld::HalStatus& status)
{
	if (status.ok()) {
		return;
	}
	vkLastError = status.message != nullptr ? status.message : "command queue failed";
}
}

namespace Vk {

Device::Device() = default;

Device::Device(const SDL_SysWMinfo& window, const ContextConfig& config)
	: context_(std::make_unique<Context>(Kld::nativeWindowHandlesFrom(window), config))
{
	capabilities_ = probeCapabilities(*context_);
	executor_ = std::make_unique<CommandExecutor>(*context_, capabilities_);
}

Device::~Device() = default;

Kld::HalStatus Device::processCommandQueue(std::span<const Kld::GfxOp> commands)
{
	if (executor_ == nullptr) {
		vkLastError = "the device has no Vulkan context to execute commands on";
		return Kld::HalStatus { Kld::HalStatusCode::InitializationFailed,
					vkLastError.c_str() };
	}
	const Kld::HalStatus status = executor_->process(commands);
	recordFailure(status);
	return status;
}

const char* Device::apiName() const noexcept
{
	return context_ != nullptr ? "Vulkan" : "none";
}

}

MhRenderingDeviceHandle kld_create_rendering_device(const SDL_SysWMinfo* window_info, const MhRenderingDeviceCreationInfo* creationInfo)
{
	(void)creationInfo; // The swapchain derives its extent from the surface.
	if (window_info == nullptr) {
		vkLastError = "kld_create_rendering_device: window_info is null";
		return nullptr;
	}
	try {
		return reinterpret_cast<MhRenderingDeviceHandle>(new Vk::Device(*window_info));
	} catch (const std::exception& error) {
		vkLastError = error.what();
		return nullptr;
	}
}
void kld_destroy_rendering_device(MhRenderingDeviceHandle device)
{
	delete reinterpret_cast<Vk::Device*>(device);
}
const char* kld_get_renderer_name(void)
{
	return "Vulkan Renderer";
}
const char* kld_get_version(void)
{
	return "0.1.0";
}
const char* kld_get_last_error_message(void)
{
	return vkLastError.empty() ? nullptr : vkLastError.c_str();
}
