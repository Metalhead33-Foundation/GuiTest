#ifndef KALDID3D11CAPABILITIES_HPP
#define KALDID3D11CAPABILITIES_HPP
/**
 * @file D3D11Capabilities.hpp
 * @brief Probes `Kld::HalCapabilities` from an `ID3D11Device`.
 *
 * Direct3D 11 exposes very few *queryable* hard limits: texture extents and
 * slice counts are guaranteed minima fixed by the API (`D3D11_REQ_*`), so the
 * probe reports those and only queries what actually varies between devices:
 *
 * | `Kld::HalCapabilities` field | D3D11 source |
 * |---|---|
 * | texture extents / array layers / cube size | `D3D11_REQ_*` constants |
 * | `maxSamples` | `CheckMultisampleQualityLevels(R8G8B8A8_UNORM, ...)` |
 * | `maxColorAttachments` | `D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT` |
 * | `maxVertexAttributes` / `maxVertexBuffers` | `D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT` / `..._RESOURCE_SLOT_COUNT` |
 * | `maxSampledTextureBindings` / `maxSamplerBindings` / `maxUniformBufferBindings` | `D3D11_COMMONSHADER_*_SLOT_COUNT` |
 * | `maxStorageBufferBindings` / `maxStorageImageBindings` | `D3D11_PS_CS_UAV_REGISTER_COUNT` on FL 11_0+ |
 * | `supportsCompute`, `supportsIndirectDraw`, `supportsStorageResources` | feature level 11_0+ |
 * | `supportsTextureCompression` | BC1/BC3 `CheckFormatSupport` with shader sampling |
 * | queries, readback, multisample resolve | always true (core D3D11) |
 * | `supportsDebugLabels` | false until an annotation/PIX marker path is wired up |
 *
 * @note Windows-only. The consuming `.pro` compiles this translation unit inside
 * a `win32` scope, so Linux/macOS builds of the repository never see it.
 */
#if __has_include(<d3d11.h>)
#include <d3d11.h>
#else
#error "KaldiD3D11 needs <d3d11.h> from the Windows SDK (or the DXSDK fallback include path)."
#endif
#include <Kaldi/KldHal.hpp>

namespace D3D11 {

/**
 * @brief Probes the HAL capability snapshot of @p device.
 * @param device The D3D11 device whose immediate context will execute commands.
 * @return The probed snapshot, or `Kld::halCapabilitiesNone()` when @p device is null.
 */
Kld::HalCapabilities probeCapabilities(ID3D11Device* device) noexcept;

}

#endif // KALDID3D11CAPABILITIES_HPP
