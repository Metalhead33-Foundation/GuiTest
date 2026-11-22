#include "KaldiD3D11Device.hpp"
namespace D3D11 {

Device::Device() {}

const Kld::HalCapabilities& Device::capabilities() const noexcept
{
	// PROVISIONAL. This class owns no ID3D11Device yet, so there is nothing to
	// probe. D3D11::probeCapabilities() already exists in D3D11Capabilities.cpp
	// and is meant to fill this member once the device class owns the API device.
	return capabilities_;
}

Kld::HalStatus Device::processCommandQueue(std::span<const Kld::GfxOp>)
{
	// PROVISIONAL, NON-FUNCTIONAL STUB - read this before implementing anything.
	//
	// There is deliberately no handler body here: `Device` does not own an
	// ID3D11Device/ID3D11DeviceContext, so not a single command could be executed
	// correctly. Returning `UnsupportedBaseline` is the honest answer and matches
	// the capability snapshot above (which reports no usable limits).
	//
	// The intended structure, once the class owns `D3D11::Context`:
	//
	//   1. `capabilities_ = D3D11::probeCapabilities(context->device());`
	//   2. keep `std::unordered_map<Kld::HandleId, ...>` registries per resource
	//      kind (buffers, textures, images, samplers, framebuffers, pipelines,
	//      queries, fences), exactly like `GlCommandExecutor.hpp`;
	//   3. translate `Kld::OpCreate*` into CreateBuffer/CreateTexture2D/...,
	//      `OpCreatePipeline` through `D3D::spirvToHlsl()` + D3DCompile/dxc, and
	//      the bind/draw ops through the immediate context;
	//   4. destroy commands are logical retirement requests: D3D11 has no GPU
	//      timeline, so queue them and release after the next Present/Flush
	//      (compare `Kld::kDefaultMaxFramesInFlight`);
	//   5. use `KLD_DISPATCH_*` from `Kaldi/KldDispatch.hpp` so this backend shares
	//      the opcode table and the computed-goto/switch structure with GL/GLES/VK.
	//
	// None of that can be compiled or validated on this Linux host: the D3D11
	// translation units are behind `win32` in KaldiD3D11.pro. Do not ship a
	// "working" path based on this file without a Windows test.
	static constexpr char message[] =
		"the Direct3D 11 backend has no device/command executor yet (provisional stub)";
	return Kld::HalStatus { Kld::HalStatusCode::UnsupportedBaseline, message };
}

}
MhRenderingDeviceHandle kld_create_rendering_device(const SDL_SysWMinfo* window_info, const MhRenderingDeviceCreationInfo& creationInfo)
{
	// TODO: Actually implement (once the device implements capabilities/processCommandQueue)
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
	return "Direct3D11 Renderer";
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