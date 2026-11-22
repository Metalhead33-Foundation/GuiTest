#include "KaldiGLDevice.hpp"
#include "GlCapabilities.hpp"
#include <stdexcept>
#include <string>

namespace {
/** @brief Last `kld_create_rendering_device` failure on this thread. */
thread_local std::string glLastError;
}

namespace GL {

Device::Device() = default;

Device::Device(const SDL_SysWMinfo& window, const DeviceConfig& config)
	: context_(createContext(window, config.context))
{
	context_->makeCurrent();
	if (loadGlFunctions() == 0) {
		context_->releaseCurrent();
		throw std::runtime_error("KaldiGL: the OpenGL entry points could not be loaded");
	}
	applyDefaultState();
	capabilities_ = probeCapabilities();
	executor_ = std::make_unique<CommandExecutor>(*context_, capabilities_, config.executor);
}

Device::Device(const SDL_SysWMinfo& window, const ContextConfig& config)
	: Device(window, DeviceConfig { config, ExecutorConfig {} })
{
}

Device::~Device()
{
	if (context_ == nullptr) {
		return;
	}
	// GL objects must be deleted while their context is current. If making it
	// current fails, leaking the executor is safer than deleting objects in
	// whatever context happens to be current on this thread.
	bool current = false;
	try {
		context_->makeCurrent();
		current = true;
	} catch (...) {
		current = false;
	}
	if (current) {
		executor_.reset();
	} else {
		(void)executor_.release();
	}
	context_->releaseCurrent();
	unloadGlFunctions();
}

Kld::HalStatus Device::processCommandQueue(std::span<const Kld::GfxOp> commands)
{
	if (executor_ == nullptr) {
		return Kld::HalStatus { Kld::HalStatusCode::InitializationFailed,
					"the device has no GL context to execute commands on" };
	}
	return executor_->process(commands);
}

const char* Device::apiName() const noexcept
{
	return context_ != nullptr ? context_->apiName().data() : "none";
}

void Device::swapBuffers()
{
	if (context_ == nullptr) {
		throw std::logic_error("KaldiGL: the device has no context to present");
	}
	context_->swapBuffers();
}

}

MhRenderingDeviceHandle kld_create_rendering_device(const SDL_SysWMinfo* window_info, const MhRenderingDeviceCreationInfo& creationInfo)
{
	(void)creationInfo; // The drawable is already sized by the windowing layer.
	if (window_info == nullptr) {
		glLastError = "kld_create_rendering_device: window_info is null";
		return nullptr;
	}
	try {
		return reinterpret_cast<MhRenderingDeviceHandle>(new GL::Device(*window_info));
	} catch (const std::exception& error) {
		glLastError = error.what();
		return nullptr;
	}
}
void kld_destroy_rendering_device(MhRenderingDeviceHandle device)
{
	delete reinterpret_cast<GL::Device*>(device);
}
const char* kld_get_renderer_name(void)
{
	return "OpenGL Renderer";
}
const char* kld_get_version(void)
{
	return "0.1.0";
}
const char* kld_get_last_error_message(void)
{
	return glLastError.empty() ? nullptr : glLastError.c_str();
}
