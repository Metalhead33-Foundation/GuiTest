#include "KaldiD3D12Device.hpp"
namespace D3D12 {
Device::Device() {}

const Kld::HalCapabilities& Device::capabilities() const noexcept
{
	// PROVISIONAL. This class owns no ID3D12Device yet, so there is nothing to
	// probe. D3D12::probeCapabilities() already exists in D3D12Capabilities.cpp
	// and is meant to fill this member once the device class owns the API device.
	return capabilities_;
}

Kld::HalStatus Device::processCommandQueue(std::span<const Kld::GfxOp>)
{
	// PROVISIONAL, NON-FUNCTIONAL STUB - read this before implementing anything.
	//
	// `Device` owns no ID3D12Device/command queue, so no command can be executed.
	// `UnsupportedBaseline` is the honest answer and matches `capabilities()`.
	//
	// The intended structure, once the class owns `D3D12::Context`:
	//
	//   1. `capabilities_ = D3D12::probeCapabilities(context->device());`
	//   2. per-kind handle registries (buffers, textures, images, samplers,
	//      framebuffers, pipelines, queries, fences), like `GlCommandExecutor.hpp`;
	//   3. the command-buffer model differs from D3D11: record into an allocator/
	//      command list, translate `Kld::ResourceState` into `D3D12_RESOURCE_STATES`
	//      barriers (`D3D12_RESOURCE_BARRIER_TYPE_TRANSITION`), and build root
	//      signatures/pipeline state objects from `Kld::PipelineBindingLayout`;
	//   4. `OpCreatePipeline` goes through `D3D::spirvToHlsl()` + dxc/D3DCompile;
	//   5. destroy commands must wait for the fences of every frame that could
	//      reference the resource (deferred retirement queue keyed by fence value);
	//   6. dispatch through `KLD_DISPATCH_*` for the shared opcode table.
	//
	// This is unverifiable on the current host and intentionally incomplete.
	static constexpr char message[] =
		"the Direct3D 12 backend has no device/command executor yet (provisional stub)";
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
	return "Direct3D12 Renderer";
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