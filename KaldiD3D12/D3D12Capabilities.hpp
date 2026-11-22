#ifndef KALDID3D12CAPABILITIES_HPP
#define KALDID3D12CAPABILITIES_HPP
/**
 * @file D3D12Capabilities.hpp
 * @brief Probes `Kld::HalCapabilities` from an `ID3D12Device`.
 *
 * Direct3D 12's resource limits are fixed by the API (`D3D12_REQ_*`), and every
 * D3D12 device must reach feature level 11_0, so the probe mostly fills in
 * constants and only queries what varies between adapters:
 *
 * | `Kld::HalCapabilities` field | D3D12 source |
 * |---|---|
 * | texture extents / array layers / cube size | `D3D12_REQ_*` constants |
 * | `maxSamples` | `CheckFeatureSupport(MULTISAMPLE_QUALITY_LEVELS, R8G8B8A8_UNORM)` |
 * | `maxColorAttachments` | `D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT` |
 * | `maxVertexAttributes` / `maxVertexBuffers` | `D3D12_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT` / `..._RESOURCE_SLOT_COUNT` |
 * | `maxSampledTextureBindings` / `maxSamplerBindings` / `maxUniformBufferBindings` | `D3D12_COMMONSHADER_*_SLOT_COUNT` |
 * | `maxStorageBufferBindings` / `maxStorageImageBindings` | `D3D12_PS_CS_UAV_REGISTER_COUNT` |
 * | `supportsCompute`, `supportsIndirectDraw`, `supportsStorageResources` | always true (FL 11_0 is mandatory) |
 * | `supportsTextureCompression` | BC1/BC3 `CheckFeatureSupport(FORMAT_SUPPORT)` |
 * | `supportsDebugLabels` | true (`BeginEvent`/`EndEvent` are core) |
 * | queries, readback, multisample resolve | always true (core D3D12) |
 *
 * @note Windows-only and **unverifiable in this repository** (`win32` scope of
 * `KaldiD3D12.pro`); written against the documented D3D12 API.
 */
#if __has_include(<directx/d3d12.h>)
#include <directx/d3d12.h>
#elif __has_include(<d3d12.h>)
#include <d3d12.h>
#else
#error "KaldiD3D12 needs <directx/d3d12.h> from microsoft/DirectX-Headers (or <d3d12.h> from the Windows SDK)."
#endif
#include <Kaldi/KldHal.hpp>

namespace D3D12 {

/**
 * @brief Probes the HAL capability snapshot of @p device.
 * @param device The D3D12 device that will execute the command lists.
 * @return The probed snapshot, or `Kld::halCapabilitiesNone()` when @p device is null.
 */
Kld::HalCapabilities probeCapabilities(ID3D12Device* device) noexcept;

}

#endif // KALDID3D12CAPABILITIES_HPP
