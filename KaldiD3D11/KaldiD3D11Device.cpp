#include "KaldiD3D11Device.hpp"
#include <stdexcept>
#include <string>
#ifdef _WIN32
#include "D3D11Capabilities.hpp"
#include "D3D11Context.hpp"
#endif

namespace {
/** @brief Last `kld_create_rendering_device` failure on this thread. */
thread_local std::string d3d11LastError;
}

namespace D3D11 {

Device::Device() = default;

Device::Device(const SDL_SysWMinfo& window)
{
#ifndef _WIN32
	(void)window;
	throw std::runtime_error(
		"KaldiD3D11: the Direct3D 11 backend is only available on Windows");
#else
	if (window.info.win.window == nullptr) {
		throw std::invalid_argument("KaldiD3D11: the SDL window has no Win32 handle");
	}
	context_ = new Context(window.info.win.window);
	try {
		capabilities_ = probeCapabilities(context_->device());
	} catch (...) {
		delete context_;
		context_ = nullptr;
		throw;
	}
#endif
}

Device::~Device()
{
#ifdef _WIN32
	// `Context` is only defined in a Windows translation unit, so the delete has
	// to live in the platform-specific branch.
	delete context_;
#endif
	context_ = nullptr;
}

const Kld::HalCapabilities& Device::capabilities() const noexcept
{
	return capabilities_;
}

Kld::HalStatus Device::processCommandQueue(std::span<const Kld::GfxOp>)
{
	// PROVISIONAL, NON-FUNCTIONAL STUB - read this before implementing anything.
	//
	// The device now owns a real `D3D11::Context`, but no command executor exists
	// yet, so not a single command could be executed correctly. Returning
	// `UnsupportedBaseline` is the honest answer; the capabilities above are real.
	//
	// The intended structure, now that the context is available:
	//
	//   1. keep `std::unordered_map<Kld::HandleId, ...>` registries per resource
	//      kind (buffers, textures, images, samplers, framebuffers, pipelines,
	//      queries, fences), exactly like `GlCommandExecutor.hpp`;
	//   2. translate `Kld::OpCreate*` into CreateBuffer/CreateTexture2D/...,
	//      `OpCreatePipeline` through `D3D::spirvToHlsl()` + D3DCompile/dxc, and
	//      the bind/draw ops through `context_->deviceContext()`;
	//   3. destroy commands are logical retirement requests: D3D11 has no GPU
	//      timeline, so queue them and release after the next Present/Flush
	//      (compare `Kld::kDefaultMaxFramesInFlight`);
	//   4. use `KLD_DISPATCH_*` from `Kaldi/KldDispatch.hpp` so this backend shares
	//      the opcode table and the computed-goto/switch structure with GL/GLES/VK.
	//
	// None of that can be compiled or validated on this Linux host: the D3D11
	// translation units are behind `win32` in KaldiD3D11.pro.
	static constexpr char message[] =
		"the Direct3D 11 backend has no command executor yet (provisional stub)";
	return Kld::HalStatus { Kld::HalStatusCode::UnsupportedBaseline, message };
}

const char* Device::apiName() const noexcept
{
	return context_ != nullptr ? "Direct3D 11" : "none";
}

}

MhRenderingDeviceHandle kld_create_rendering_device(const SDL_SysWMinfo* window_info,
						    const MhRenderingDeviceCreationInfo* creationInfo)
{
	(void)creationInfo; // The drawable is already sized by the windowing layer.
	if (window_info == nullptr) {
		d3d11LastError = "kld_create_rendering_device: window_info is null";
		return nullptr;
	}
	try {
		return reinterpret_cast<MhRenderingDeviceHandle>(new D3D11::Device(*window_info));
	} catch (const std::exception& error) {
		d3d11LastError = error.what();
		return nullptr;
	}
}
void kld_destroy_rendering_device(MhRenderingDeviceHandle device)
{
	delete reinterpret_cast<D3D11::Device*>(device);
}
const char* kld_get_renderer_name(void)
{
	return "Direct3D11 Renderer";
}
const char* kld_get_version(void)
{
	return "0.0.1";
}
const char* kld_get_last_error_message(void)
{
	return d3d11LastError.empty() ? nullptr : d3d11LastError.c_str();
}
