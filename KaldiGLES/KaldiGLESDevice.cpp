#include "KaldiGLESDevice.hpp"
#include "GlCapabilities.hpp"
#include <stdexcept>
#include <string>

namespace {
/**
 * @brief Last failure recorded on this thread.
 *
 * `kld_get_last_error_message()` reads from here. Creation failures and command
 * failures both land in it, so a caller that only speaks the C ABI still learns
 * *why* something failed instead of getting `nullptr`. A successful call leaves it
 * alone, which is what "last error" means: the message survives until the next
 * failure rather than being cleared by the next call.
 */
thread_local std::string glesLastError;

/** @brief Records @p status in the thread-local error slot when it is a failure. */
void recordFailure(const Kld::HalStatus& status)
{
	if (status.ok()) {
		return;
	}
	glesLastError = status.message != nullptr ? status.message : "command queue failed";
}
}

namespace GLES {

Device::Device() = default;

Device::Device(const SDL_SysWMinfo& window, const DeviceConfig& config)
	: context_(createContext(window, config.context))
{
	context_->makeCurrent();
	if (loadGlFunctions() == 0) {
		context_->releaseCurrent();
		throw std::runtime_error("KaldiGLES: the OpenGL ES entry points could not be loaded");
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
		glesLastError = "the device has no GL context to execute commands on";
		return Kld::HalStatus { Kld::HalStatusCode::InitializationFailed,
					glesLastError.c_str() };
	}
	const Kld::HalStatus status = executor_->process(commands);
	recordFailure(status);
	return status;
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

MhRenderingDeviceHandle kld_create_rendering_device(const SDL_SysWMinfo* window_info, const MhRenderingDeviceCreationInfo* creationInfo)
{
	(void)creationInfo; // The drawable is already sized by the windowing layer.
	if (window_info == nullptr) {
		glesLastError = "kld_create_rendering_device: window_info is null";
		return nullptr;
	}
	try {
		return reinterpret_cast<MhRenderingDeviceHandle>(new GLES::Device(*window_info));
	} catch (const std::exception& error) {
		glesLastError = error.what();
		return nullptr;
	}
}
void kld_destroy_rendering_device(MhRenderingDeviceHandle device)
{
	delete reinterpret_cast<GLES::Device*>(device);
}
const char* kld_get_renderer_name(void)
{
	return "OpenGL ES Renderer";
}
const char* kld_get_version(void)
{
	return "0.1.0";
}
const char* kld_get_last_error_message(void)
{
	return glesLastError.empty() ? nullptr : glesLastError.c_str();
}
