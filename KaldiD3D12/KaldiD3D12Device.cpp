#include "KaldiD3D12Device.hpp"
namespace D3D12 {
Device::Device() {}
}
MhRenderingDeviceHandle kld_create_rendering_device(const SDL_SysWMinfo* window_info, const MhRenderingDeviceCreationInfo& creationInfo)
{
	// TODO: Actually implement
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