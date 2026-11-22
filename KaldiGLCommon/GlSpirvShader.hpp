#ifndef KALDIGLSPIRVSHADER_HPP
#define KALDIGLSPIRVSHADER_HPP
/**
 * @file GlSpirvShader.hpp
 * @brief SPIR-V to GLSL / GLSL ES cross-compilation shared by KaldiGL and KaldiGLES.
 *
 * Kaldi's HAL contract is SPIR-V-only (`Kld::ShaderBinaryDescriptor`), so every
 * GL-family backend has to turn those words back into GLSL text that
 * `glShaderSource()` can consume. That translation is identical for desktop
 * OpenGL and OpenGL ES apart from the version, the `es` flag and the default
 * precisions - which is exactly what `GlslOptions` parameterises.
 *
 * This header used to live in `KaldiGL/` and carried the legacy
 * `ShaderModuleCreateInfo(Ref)` structs. Both are gone: the entry points either
 * take a `std::span<const uint32_t>` of SPIR-V words or a
 * `Kld::ShaderBinaryDescriptor`.
 *
 * @section kaldigl_spirv_cache On-disk shader caches
 *
 * The legacy TODO asked for a shader cache "without connecting this library
 * directly to our I/O modules". That is what `GlslCacheCallbacks` is for: the
 * backend (or Lotte, or the host application) registers two plain C function
 * pointers and decides where the cache lives. Pass a default-constructed
 * `GlslCacheCallbacks` to disable caching entirely.
 */
#include "GlBackend.hpp"
#include <Kaldi/KaldiOperation.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace KALDIGL_NS {

/**
 * @brief Options for the SPIR-V -> GLSL translation.
 *
 * The defaults come from defaultGlslOptions(), which is dialect-aware: 4.30
 * core on desktop OpenGL, 3.20 ES on OpenGL ES.
 */
struct GlslOptions {
	/** @brief GLSL version written into the `#version` directive (430 or 320). */
	uint32_t version = 430;
	/** @brief Emit `#version 320 es` style GLSL ES output. */
	bool es = false;
	/** @brief Rewrite clip-space Z to the OpenGL [-1, 1] convention. */
	bool fixupClipspace = true;
	/** @brief Keep Vulkan semantics (set/binding decorations, no default uniforms). */
	bool vulkanSemantics = false;
	/** @brief Default precision for floating point fragment variables. */
	spirv_cross::CompilerGLSL::Options::Precision fragmentFloatPrecision =
		spirv_cross::CompilerGLSL::Options::DontCare;
	/** @brief Default precision for integer fragment variables. */
	spirv_cross::CompilerGLSL::Options::Precision fragmentIntPrecision =
		spirv_cross::CompilerGLSL::Options::DontCare;
};

/** @brief Returns the translation defaults for the dialect this unit was built for. */
GlslOptions defaultGlslOptions() noexcept;

/**
 * @brief Optional cache hooks for an external shader cache.
 *
 * `load` returns @c true when it filled @p outGlsl from the cache. `store`
 * receives the freshly generated source and may ignore it.
 */
struct GlslCacheCallbacks {
	/** @brief Cache lookup; may be @c nullptr. */
	bool (*load)(void* userData, std::span<const uint32_t> spirv, std::string& outGlsl);
	/** @brief Cache fill; may be @c nullptr. */
	void (*store)(void* userData, std::span<const uint32_t> spirv, std::string_view glsl);
	/** @brief Opaque pointer forwarded to both callbacks. */
	void* userData;
};

/**
 * @brief Heuristically validates a SPIR-V word blob.
 *
 * Checks the magic number and that at least the five-word module header is present.
 */
bool isSpirvBinary(std::span<const uint32_t> spirv) noexcept;

/** @brief Cross-compiles SPIR-V words to GLSL source text. */
std::string spirvToGlsl(std::span<const uint32_t> spirv, const GlslOptions& options = defaultGlslOptions());

/** @brief Same as above, but consults the supplied shader cache first. */
std::string spirvToGlsl(std::span<const uint32_t> spirv, const GlslOptions& options, const GlslCacheCallbacks& cache);

/** @brief Cross-compiles a Kaldi shader binary handed over by the HAL. */
std::string spirvToGlsl(const Kld::ShaderBinaryDescriptor& binary, const GlslOptions& options = defaultGlslOptions());

/** @brief Same as above, but consults the supplied shader cache first. */
std::string spirvToGlsl(const Kld::ShaderBinaryDescriptor& binary, const GlslOptions& options, const GlslCacheCallbacks& cache);

}
#endif // KALDIGLSPIRVSHADER_HPP
