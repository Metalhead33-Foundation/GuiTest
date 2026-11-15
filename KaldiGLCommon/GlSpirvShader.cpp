/**
 * @file GlSpirvShader.cpp
 * @brief Implements the SPIR-V -> GLSL translation declared in GlSpirvShader.hpp.
 *
 * The implementation is a cleaned-up version of the legacy `GlPipeline` helper:
 * the descriptor-set/binding "decoration stripping" hack is gone (SPIRV-Cross
 * drops Vulkan-only decorations on its own once `vulkan_semantics` is off), the
 * combined image sampler names are kept, and the output is a plain
 * `std::string` so callers can hand it straight to `glShaderSource()`.
 */
#include "GlSpirvShader.hpp"
#include <spirv_cross/spirv_glsl.hpp>
#include <stdexcept>

namespace KALDIGL_NS {

namespace {

/** @brief SPIR-V magic number from the Khronos specification. */
constexpr uint32_t spirvMagic = 0x07230203u;

/** @brief Number of words in the SPIR-V module header. */
constexpr std::size_t spirvHeaderWords = 5;

/** @brief Validates the module header before SPIRV-Cross sees the words. */
void validateSpirv(std::span<const uint32_t> spirv)
{
	if (!isSpirvBinary(spirv)) {
		throw std::invalid_argument("GlSpirvShader: shader binary is not a SPIR-V module");
	}
}

/** @brief Applies the portable subset of `GlslOptions` to a SPIRV-Cross compiler. */
void applyOptions(spirv_cross::CompilerGLSL& glsl, const GlslOptions& options)
{
	spirv_cross::CompilerGLSL::Options glslOptions;
	glslOptions.version = options.version;
	glslOptions.es = options.es;
	glslOptions.vertex.fixup_clipspace = options.fixupClipspace;
	glslOptions.vulkan_semantics = options.vulkanSemantics;
	glslOptions.fragment.default_float_precision = options.fragmentFloatPrecision;
	glslOptions.fragment.default_int_precision = options.fragmentIntPrecision;
	glsl.set_common_options(glslOptions);
}

/** @brief Compiles validated SPIR-V words into GLSL text. */
std::string compileGlsl(std::span<const uint32_t> spirv, const GlslOptions& options)
{
	spirv_cross::CompilerGLSL glsl(spirv.data(), spirv.size());
	applyOptions(glsl, options);

	// Desktop GL and GLSL ES both sample through a single combined sampler object,
	// so every (image, sampler) pair Vulkan keeps apart has to be merged here.
	glsl.build_combined_image_samplers();
	for (const spirv_cross::CombinedImageSampler& remap : glsl.get_combined_image_samplers()) {
		glsl.set_name(remap.combined_id, glsl.get_name(remap.image_id));
	}

	return glsl.compile();
}

} // namespace

GlslOptions defaultGlslOptions() noexcept
{
	GlslOptions options;
#if KALDIGL_IS_GLES
	options.version = 320;
	options.es = true;
	// GLSL ES has no default precision for fragment-stage floats or ints.
	options.fragmentFloatPrecision = spirv_cross::CompilerGLSL::Options::Mediump;
	options.fragmentIntPrecision = spirv_cross::CompilerGLSL::Options::Mediump;
#else
	options.version = 430;
	options.es = false;
	options.fragmentFloatPrecision = spirv_cross::CompilerGLSL::Options::Highp;
	options.fragmentIntPrecision = spirv_cross::CompilerGLSL::Options::Highp;
#endif
	return options;
}

bool isSpirvBinary(std::span<const uint32_t> spirv) noexcept
{
	return spirv.size() >= spirvHeaderWords && spirv[0] == spirvMagic;
}

std::string spirvToGlsl(std::span<const uint32_t> spirv, const GlslOptions& options)
{
	return spirvToGlsl(spirv, options, GlslCacheCallbacks{ nullptr, nullptr, nullptr });
}

std::string spirvToGlsl(std::span<const uint32_t> spirv, const GlslOptions& options, const GlslCacheCallbacks& cache)
{
	validateSpirv(spirv);
	if (cache.load != nullptr) {
		std::string cached;
		if (cache.load(cache.userData, spirv, cached)) {
			return cached;
		}
	}
	std::string source = compileGlsl(spirv, options);
	if (cache.store != nullptr) {
		cache.store(cache.userData, spirv, source);
	}
	return source;
}

std::string spirvToGlsl(const Kld::ShaderBinaryDescriptor& binary, const GlslOptions& options)
{
	return spirvToGlsl(binary, options, GlslCacheCallbacks{ nullptr, nullptr, nullptr });
}

std::string spirvToGlsl(const Kld::ShaderBinaryDescriptor& binary, const GlslOptions& options, const GlslCacheCallbacks& cache)
{
	if (binary.spirv == nullptr) {
		throw std::invalid_argument("GlSpirvShader: shader binary descriptor has no SPIR-V words");
	}
	return spirvToGlsl(std::span<const uint32_t>(binary.spirv, binary.wordCount), options, cache);
}

} // namespace KALDIGL_NS
