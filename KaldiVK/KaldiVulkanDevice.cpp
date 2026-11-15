#include "KaldiVulkanDevice.hpp"
#include <stdexcept>

namespace Vk {

Device::Device() = default;

Device::Device(const SDL_SysWMinfo& window, const ContextConfig& config)
	: context_(std::make_unique<Context>(Kld::nativeWindowHandlesFrom(window), config))
{
}

Device::~Device() = default;

const char* Device::apiName() const noexcept
{
	return context_ != nullptr ? "Vulkan" : "none";
}

}

MhRenderingDeviceHandle kld_create_rendering_device(const SDL_SysWMinfo* window_info, const MhRenderingDeviceCreationInfo& creationInfo)
{
	// A device can only be handed out once Vk::Device implements
	// capabilities()/processCommandQueue(); until then the context layer is
	// exercised directly through Vk::Context.
	(void)window_info;
	(void)creationInfo;
	return nullptr;
}
void kld_destroy_rendering_device(MhRenderingDeviceHandle device)
{
	// TODO: Actually implement
	(void)device;
}
const char* kld_get_renderer_name(void)
{
	// TODO: Actually implement
	return "Vulkan Renderer";
}
const char* kld_get_version(void)
{
	// TODO: Actually implement
	return "0.0.1";
}
const char* kld_get_last_error_message(void)
{
	// TODO: Actually implement
	return nullptr;
}
