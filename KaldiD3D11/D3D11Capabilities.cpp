/**
 * @file D3D11Capabilities.cpp
 * @brief Implements D3D11Capabilities.hpp.
 *
 * Everything here is a read-only device query; no object is created and no
 * command is recorded, so the probe is safe to call right after device creation.
 */
#include "D3D11Capabilities.hpp"
#include <cstdint>

namespace D3D11 {

namespace {

/** @brief Highest MSAA sample count an RGBA8 render target accepts; 1 when none. */
std::uint32_t highestSampleCount(ID3D11Device* device) noexcept
{
	static constexpr UINT candidates[] = { 64, 32, 16, 8, 4, 2 };
	for (const UINT count : candidates) {
		UINT qualityLevels = 0;
		if (SUCCEEDED(device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, count, &qualityLevels)) &&
		    qualityLevels > 0) {
			return count;
		}
	}
	return 1;
}

/** @brief True when @p format advertises at least the @p required support bits. */
bool formatSupports(ID3D11Device* device, DXGI_FORMAT format, UINT required) noexcept
{
	UINT support = 0;
	return SUCCEEDED(device->CheckFormatSupport(format, &support)) && (support & required) == required;
}

} // namespace

Kld::HalCapabilities probeCapabilities(ID3D11Device* device) noexcept
{
	Kld::HalCapabilities capabilities = Kld::halCapabilitiesNone();
	if (device == nullptr) {
		return capabilities;
	}

	// D3D11 has no runtime query for texture extents: the D3D11_REQ_* values are
	// the guaranteed minima every compliant device reaches.
	capabilities.maxTexture1DSize = D3D11_REQ_TEXTURE1D_U_DIMENSION;
	capabilities.maxTexture2DSize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
	capabilities.maxTexture3DSize = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
	capabilities.maxTextureArrayLayers = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
	capabilities.maxCubeMapSize = D3D11_REQ_TEXTURECUBE_DIMENSION;
	capabilities.maxSamples = highestSampleCount(device);
	capabilities.maxColorAttachments = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
	capabilities.maxVertexAttributes = D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT;
	capabilities.maxVertexBuffers = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
	capabilities.maxSampledTextureBindings = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
	capabilities.maxSamplerBindings = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
	capabilities.maxUniformBufferBindings = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
	capabilities.maxFramesInFlight = Kld::kDefaultMaxFramesInFlight;

	// Compute shaders, indirect draws and UAVs (storage resources) all arrive with
	// shader model 5 / feature level 11_0.
	const D3D_FEATURE_LEVEL level = device->GetFeatureLevel();
	const bool modern = level >= D3D_FEATURE_LEVEL_11_0;
	capabilities.supportsCompute = modern;
	capabilities.supportsIndirectDraw = modern;
	capabilities.supportsStorageResources = modern;
	capabilities.maxStorageBufferBindings = modern ? D3D11_PS_CS_UAV_REGISTER_COUNT : 0;
	capabilities.maxStorageImageBindings = modern ? D3D11_PS_CS_UAV_REGISTER_COUNT : 0;

	capabilities.supportsQueries = true;
	capabilities.supportsReadback = true;
	capabilities.supportsMultisampleResolve = true;
	// Debug names through SetPrivateData are not label regions, and the D3D11
	// annotation layer/PIX markers are not wired up yet, so markers are not
	// consumed by this backend today.
	capabilities.supportsDebugLabels = false;
	capabilities.supportsTextureCompression =
		formatSupports(device, DXGI_FORMAT_BC1_UNORM, D3D11_FORMAT_SUPPORT_SHADER_SAMPLE) &&
		formatSupports(device, DXGI_FORMAT_BC3_UNORM, D3D11_FORMAT_SUPPORT_SHADER_SAMPLE);

	return capabilities;
}

}
