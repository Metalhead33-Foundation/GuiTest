#include "KaldiGLESDevice.hpp"
#include <stdexcept>

namespace GLES {

Device::Device() = default;

Device::Device(const SDL_SysWMinfo& window, const ContextConfig& config)
	: context_(createContext(window, config))
{
	context_->makeCurrent();
	if (loadGlFunctions() == 0) {
		context_->releaseCurrent();
		throw std::runtime_error("KaldiGLES: the OpenGL ES entry points could not be loaded");
	}
	applyDefaultState();
}

Device::~Device()
{
	if (context_ == nullptr) {
		return;
	}
	// The entry points were loaded by the constructor that created the context.
	context_->releaseCurrent();
	unloadGlFunctions();
}

const char* Device::apiName() const noexcept
{
	return context_ != nullptr ? context_->apiName().data() : "none";
}

void Device::swapBuffers()
{
	if (context_ == nullptr) {
		throw std::logic_error("KaldiGLES: the device has no context to present");
	}
	context_->swapBuffers();
}

}

MhRenderingDeviceHandle kld_create_rendering_device(const SDL_SysWMinfo* window_info, const MhRenderingDeviceCreationInfo& creationInfo)
{
	// A device can only be handed out once GLES::Device implements
	// capabilities()/processCommandQueue(); until then the context layer is
	// exercised directly through GLES::createContext().
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
	return "OpenGL ES Renderer";
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
