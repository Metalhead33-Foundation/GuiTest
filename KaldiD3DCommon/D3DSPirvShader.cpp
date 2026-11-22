/**
 * @file D3DSPirvShader.cpp
 * @brief Implements the SPIR-V -> HLSL translation declared in D3DSPirvShader.hpp.
 *
 * This file does not touch any Direct3D API, only SPIRV-Cross, so it compiles
 * on every host - which is why the `win32` gate in the backend `.pro` files only
 * covers the DXGI/D3D dependent sources.
 */
#include "D3DSPirvShader.hpp"
#include <stdexcept>

namespace D3D {

namespace {

/** @brief SPIR-V magic number from the Khronos specification. */
constexpr uint32_t spirvMagic = 0x07230203u;

/** @brief Number of words in the SPIR-V module header. */
constexpr std::size_t spirvHeaderWords = 5;

} // namespace

bool isSpirvBinary(std::span<const uint32_t> spirv) noexcept
{
	return spirv.size() >= spirvHeaderWords && spirv[0] == spirvMagic;
}

std::string spirvToHlsl(std::span<const uint32_t> spirv, const HlslOptions& options)
{
	if (!isSpirvBinary(spirv)) {
		throw std::invalid_argument("D3DSPirvShader: shader binary is not a SPIR-V module");
	}

	spirv_cross::CompilerHLSL hlsl(spirv.data(), spirv.size());

	// The GLSL `version` field is irrelevant for HLSL output; only the HLSL
	// `shader_model` matters. Vulkan semantics stay off so the emitted source
	// uses plain HLSL semantics rather than SPIR-V flavoured GLSL.
	spirv_cross::CompilerGLSL::Options glslOptions;
	glslOptions.vulkan_semantics = false;

	spirv_cross::CompilerHLSL::Options hlslOptions;
	hlslOptions.shader_model = options.shaderModel;
	hlslOptions.point_size_compat = options.pointSizeCompat;
	hlslOptions.point_coord_compat = options.pointCoordCompat;
	hlslOptions.enable_16bit_types = options.enable16BitTypes;

	hlsl.set_common_options(glslOptions);
	hlsl.set_hlsl_options(hlslOptions);

	// Unlike GLSL, HLSL keeps textures and samplers apart; no combined-sampler
	// rebuilding is required (the legacy commented-out code was right about that).
	return hlsl.compile();
}

std::string spirvToHlsl(const Kld::ShaderBinaryDescriptor& binary, const HlslOptions& options)
{
	if (binary.spirv == nullptr) {
		throw std::invalid_argument("D3DSPirvShader: shader binary descriptor has no SPIR-V words");
	}
	return spirvToHlsl(std::span<const uint32_t>(binary.spirv, binary.wordCount), options);
}

} // namespace D3D
