#include "KaldiD3D12Device.hpp"
#include <stdexcept>
#include <string>
#ifdef _WIN32
#include "D3D12Capabilities.hpp"
#include "D3D12Context.hpp"
#endif

namespace {
/** @brief Last `kld_create_rendering_device` failure on this thread. */
thread_local std::string d3d12LastError;
}

namespace D3D12 {

Device::Device() = default;

Device::Device(const SDL_SysWMinfo& window)
{
#ifndef _WIN32
	(void)window;
	throw std::runtime_error(
		"KaldiD3D12: the Direct3D 12 backend is only available on Windows");
#else
	if (window.info.win.window == nullptr) {
		throw std::invalid_argument("KaldiD3D12: the SDL window has no Win32 handle");
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
	// The device now owns a real `D3D12::Context`, but no command executor exists
	// yet. D3D12 is an explicit API, so an executor is not a thin translation
	// layer: it needs a command-list recorder, a descriptor-heap allocator, root
	// signature/pipeline-state objects per pipeline variant, and fence-driven
	// retirement. Returning `UnsupportedBaseline` is the honest answer; the
	// capabilities above are real.
	//
	// The intended structure, now that the context is available:
	//
	//   1. keep `std::unordered_map<Kld::HandleId, ...>` registries per resource
	//      kind and translate `Kld::OpCreate*` into committed resources;
	//   2. record each `processCommandQueue()` batch into one command list, close,
	//      execute and signal a fence; `Present` waits on that fence and flips;
	//   3. destroy commands are *retirement requests*: they enter a per-fence-value
	//      queue and are released once the GPU passes that value
	//      (`Kld::kDefaultMaxFramesInFlight` frames of slack);
	//   4. `OpCreatePipeline` goes through `D3D::spirvToHlsl()` + D3DCompile/dxc,
	//      producing root signatures from `Kld::PipelineBindingLayout`;
	//   5. use `KLD_DISPATCH_*` from `Kaldi/KldDispatch.hpp` for the opcode table.
	//
	// None of that can be compiled or validated on this Linux host: the D3D12
	// translation units are behind `win32` in KaldiD3D12.pro.
	static constexpr char message[] =
		"the Direct3D 12 backend has no command executor yet (provisional stub)";
	return Kld::HalStatus { Kld::HalStatusCode::UnsupportedBaseline, message };
}

const char* Device::apiName() const noexcept
{
	return context_ != nullptr ? "Direct3D 12" : "none";
}

}

MhRenderingDeviceHandle kld_create_rendering_device(const SDL_SysWMinfo* window_info,
						    const MhRenderingDeviceCreationInfo* creationInfo)
{
	(void)creationInfo; // The drawable is already sized by the windowing layer.
	if (window_info == nullptr) {
		d3d12LastError = "kld_create_rendering_device: window_info is null";
		return nullptr;
	}
	try {
		return reinterpret_cast<MhRenderingDeviceHandle>(new D3D12::Device(*window_info));
	} catch (const std::exception& error) {
		d3d12LastError = error.what();
		return nullptr;
	}
}
void kld_destroy_rendering_device(MhRenderingDeviceHandle device)
{
	delete reinterpret_cast<D3D12::Device*>(device);
}
const char* kld_get_renderer_name(void)
{
	return "Direct3D12 Renderer";
}
const char* kld_get_version(void)
{
	return "0.0.1";
}
const char* kld_get_last_error_message(void)
{
	return d3d12LastError.empty() ? nullptr : d3d12LastError.c_str();
}
