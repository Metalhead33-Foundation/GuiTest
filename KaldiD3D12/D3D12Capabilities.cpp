/**
 * @file D3D12Capabilities.cpp
 * @brief Implements D3D12Capabilities.hpp.
 *
 * Everything here is a read-only device query; no resource or command list is
 * created, so the probe is safe to call right after device creation.
 */
#include "D3D12Capabilities.hpp"
#include <cstdint>

namespace D3D12 {

namespace {

/** @brief Highest MSAA sample count an RGBA8 render target accepts; 1 when none. */
std::uint32_t highestSampleCount(ID3D12Device* device) noexcept
{
	static constexpr UINT candidates[] = { 64, 32, 16, 8, 4, 2 };
	for (const UINT count : candidates) {
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS data {};
		data.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		data.SampleCount = count;
		data.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &data, sizeof(data))) &&
		    data.NumQualityLevels > 0) {
			return count;
		}
	}
	return 1;
}

/** @brief True when @p format advertises at least the @p required support bits. */
bool formatSupports(ID3D12Device* device, DXGI_FORMAT format, D3D12_FORMAT_SUPPORT1 required) noexcept
{
	D3D12_FEATURE_DATA_FORMAT_SUPPORT data {};
	data.Format = format;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &data, sizeof(data)))) {
		return false;
	}
	const auto support = static_cast<D3D12_FORMAT_SUPPORT1>(data.Support1 & required);
	return support == required;
}

} // namespace

Kld::HalCapabilities probeCapabilities(ID3D12Device* device) noexcept
{
	Kld::HalCapabilities capabilities = Kld::halCapabilitiesNone();
	if (device == nullptr) {
		return capabilities;
	}

	capabilities.maxTexture1DSize = D3D12_REQ_TEXTURE1D_U_DIMENSION;
	capabilities.maxTexture2DSize = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
	capabilities.maxTexture3DSize = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
	capabilities.maxTextureArrayLayers = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
	capabilities.maxCubeMapSize = D3D12_REQ_TEXTURECUBE_DIMENSION;
	capabilities.maxSamples = highestSampleCount(device);
	capabilities.maxColorAttachments = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
	capabilities.maxVertexAttributes = D3D12_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT;
	capabilities.maxVertexBuffers = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
	capabilities.maxSampledTextureBindings = D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
	capabilities.maxSamplerBindings = D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT;
	capabilities.maxUniformBufferBindings = D3D12_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
	capabilities.maxStorageBufferBindings = D3D12_PS_CS_UAV_REGISTER_COUNT;
	capabilities.maxStorageImageBindings = D3D12_PS_CS_UAV_REGISTER_COUNT;
	capabilities.maxFramesInFlight = Kld::kDefaultMaxFramesInFlight;

	// D3D12 devices are feature level 11_0 by definition, so compute, indirect
	// work, UAVs (storage resources), queries, readback and resolves are all
	// available without further device features.
	capabilities.supportsCompute = true;
	capabilities.supportsIndirectDraw = true;
	capabilities.supportsStorageResources = true;
	capabilities.supportsQueries = true;
	capabilities.supportsReadback = true;
	capabilities.supportsMultisampleResolve = true;
	// BeginEvent/EndEvent are part of the command-list interface (PIX markers).
	capabilities.supportsDebugLabels = true;
	capabilities.supportsTextureCompression =
		formatSupports(device, DXGI_FORMAT_BC1_UNORM,
			       static_cast<D3D12_FORMAT_SUPPORT1>(D3D12_FORMAT_SUPPORT1_TEXTURE2D |
								  D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE)) &&
		formatSupports(device, DXGI_FORMAT_BC3_UNORM,
			       static_cast<D3D12_FORMAT_SUPPORT1>(D3D12_FORMAT_SUPPORT1_TEXTURE2D |
								  D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE));

	return capabilities;
}

}
