#ifndef KALDID3DSPIRVSHADER_HPP
#define KALDID3DSPIRVSHADER_HPP
/**
 * @file D3DSPirvShader.hpp
 * @brief SPIR-V to HLSL cross-compilation shared by KaldiD3D11 and KaldiD3D12.
 *
 * Kaldi's HAL contract is SPIR-V-only, so both Direct3D backends have to turn
 * `Kld::ShaderBinaryDescriptor` words into HLSL source (and from there into
 * DXBC/DXIL with `d3dcompiler`, `dxc` or the driver's shader compiler). The
 * translation itself is identical for both backends; only the Shader Model the
 * generated source targets differs, which is what `HlslOptions::shaderModel`
 * expresses (50 for D3D11, 51+/60 for D3D12).
 *
 * This header used to live in `KaldiD3D11/` and carried the legacy
 * `ShaderModuleCreateInfo(Ref)` structs. Both are gone.
 */
#include <Kaldi/KaldiOperation.hpp>
#include <spirv_cross/spirv_hlsl.hpp>
#include <cstdint>
#include <span>
#include <string>

namespace D3D {

/** @brief Options for the SPIR-V -> HLSL translation. */
struct HlslOptions {
	/** @brief HLSL shader model written into the generated source (50 = SM 5.0). */
	uint32_t shaderModel = 50;
	/** @brief Accept and ignore the SPIR-V `PointSize` builtin (unsupported in SM 4+). */
	bool pointSizeCompat = false;
	/** @brief Emulate `gl_PointCoord` as a constant (unsupported in HLSL). */
	bool pointCoordCompat = false;
	/** @brief Use 16-bit native types; requires Shader Model 6.2. */
	bool enable16BitTypes = false;
};

/** @brief Returns the HLSL translation defaults for Direct3D 11 (Shader Model 5.0). */
constexpr HlslOptions direct3D11Options() noexcept
{
	HlslOptions options;
	options.shaderModel = 50;
	return options;
}

/** @brief Returns the HLSL translation defaults for Direct3D 12 (Shader Model 5.1). */
constexpr HlslOptions direct3D12Options() noexcept
{
	HlslOptions options;
	options.shaderModel = 51;
	return options;
}

/**
 * @brief Heuristically validates a SPIR-V word blob.
 *
 * Checks the magic number and that at least the five-word module header is present.
 */
bool isSpirvBinary(std::span<const uint32_t> spirv) noexcept;

/**
 * @brief Cross-compiles SPIR-V words to HLSL source text.
 * @throws std::invalid_argument When the blob does not look like SPIR-V
 * @throws std::runtime_error (via `spirv_cross::CompilerError`) on invalid SPIR-V
 */
std::string spirvToHlsl(std::span<const uint32_t> spirv, const HlslOptions& options = direct3D11Options());

/** @brief Cross-compiles a Kaldi shader binary handed over by the HAL. */
std::string spirvToHlsl(const Kld::ShaderBinaryDescriptor& binary, const HlslOptions& options = direct3D11Options());

}

#endif // KALDID3DSPIRVSHADER_HPP
