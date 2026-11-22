#ifndef KALDIGLCAPABILITIES_HPP
#define KALDIGLCAPABILITIES_HPP
/**
 * @file GlCapabilities.hpp
 * @brief Probes `Kld::HalCapabilities` from the current OpenGL / OpenGL ES context.
 *
 * The Kaldi HAL contract separates "what the backend can do" from "which API the
 * backend uses". This header is the OpenGL side of that separation: it turns the
 * `glGet*` state of a current context into the portable capability snapshot the
 * client sees, so higher layers never have to know about `GL_MAX_*` tokens,
 * OpenGL ES version gates or extension strings.
 *
 * @section kaldigl_capabilities_mapping What is queried
 *
 * | `Kld::HalCapabilities` field | OpenGL source |
 * |---|---|
 * | `maxTexture1DSize` | `GL_MAX_TEXTURE_SIZE` (desktop), `0` on GLES (no 1D targets) |
 * | `maxTexture2DSize` | `GL_MAX_TEXTURE_SIZE` |
 * | `maxTexture3DSize` | `GL_MAX_3D_TEXTURE_SIZE` |
 * | `maxTextureArrayLayers` | `GL_MAX_ARRAY_TEXTURE_LAYERS` |
 * | `maxCubeMapSize` | `GL_MAX_CUBE_MAP_TEXTURE_SIZE` |
 * | `maxSamples` | `GL_MAX_SAMPLES` |
 * | `maxColorAttachments` | `GL_MAX_COLOR_ATTACHMENTS` |
 * | `maxVertexAttributes` | `GL_MAX_VERTEX_ATTRIBS` |
 * | `maxVertexBuffers` | `GL_MAX_VERTEX_ATTRIB_BINDINGS` (1 on pre-3.2 GLES) |
 * | `maxSampledTextureBindings`, `maxSamplerBindings` | `GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS` |
 * | `maxUniformBufferBindings` | `GL_MAX_UNIFORM_BUFFER_BINDINGS` |
 * | `maxStorageBufferBindings` | `GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS` when computable |
 * | `maxStorageImageBindings` | `GL_MAX_IMAGE_UNITS` when computable |
 * | `supportsCompute`, `supportsStorageResources` | desktop 4.3+, GLES 3.1+ |
 * | `supportsIndirectDraw` | desktop 4.0+, GLES 3.1+ |
 * | `supportsQueries`, `supportsMultisampleResolve` | GL 3.3 / GLES 3.0+ |
 * | `supportsDebugLabels` | GL 4.3+ or `GL_KHR_debug` |
 * | `supportsTextureCompression` | the S3TC/DXT extension `GlUtil` uploads through |
 * | `supportsReadback` | always true (`glReadPixels`, `glMapBufferRange`) |
 *
 * Version gates use the *actual* context version read from `GL_VERSION`, not the
 * version the context was requested with, so a driver that returns a newer (or,
 * in theory, older) context is reported truthfully.
 *
 * @section kaldigl_capabilities_prereq Preconditions
 *
 * Every function here assumes the calling thread has the backend's context
 * current and that loadGlFunctions() has succeeded. Without them the probe
 * returns `Kld::halCapabilitiesNone()` instead of touching the GL entry points.
 */
#include "GlBackend.hpp"
#include <Kaldi/KldHal.hpp>
#include <string_view>

namespace KALDIGL_NS {

/** @brief Major/minor version of the current context. */
struct GlVersion {
	/** @brief Major version (4 on desktop, 3 on OpenGL ES); 0 when unknown. */
	int major = 0;
	/** @brief Minor version (3 on desktop, 2 on OpenGL ES); 0 when unknown. */
	int minor = 0;

	/** @brief True when the version could be read from a live context. */
	constexpr bool valid() const noexcept { return major > 0; }
};

/** @brief True when @p version is at least `major.minor`. */
constexpr bool glVersionAtLeast(const GlVersion& version, int major, int minor) noexcept
{
	return version.major > major || (version.major == major && version.minor >= minor);
}

/**
 * @brief Reads the version of the current context from `GL_VERSION`.
 * @return The parsed version, or `{0, 0}` when no context is current or the
 *         version string is unparsable.
 *
 * `GL_VERSION` reports spelling differences between desktop OpenGL (`"4.5 ..."`)
 * and OpenGL ES (`"OpenGL ES 3.2 ..."`); both forms are handled.
 */
GlVersion queryGlVersion() noexcept;

/**
 * @brief True when the current context advertises the extension @p extension.
 * @param extension Full extension name, for example `"GL_KHR_debug"`.
 *
 * Uses the `GL_NUM_EXTENSIONS` / `glGetStringi` path on everything this backend
 * creates, and falls back to splitting the legacy `GL_EXTENSIONS` string for old
 * or ES 2 style contexts. Nothing is cached: extension sets belong to contexts,
 * and a process may create more than one.
 */
bool hasGlExtension(std::string_view extension) noexcept;

/**
 * @brief Probes a `Kld::HalCapabilities` snapshot from the current GL context.
 * @return The probed snapshot, or `Kld::halCapabilitiesNone()` when there is no
 *         current context or the GL entry points are not loaded.
 *
 * The returned struct always carries `Kld::KaldiHalVersion1_0`; a non-zero
 * `maxTexture2DSize` is the practical "this snapshot is real" indicator.
 */
Kld::HalCapabilities probeCapabilities() noexcept;

}

#endif // KALDIGLCAPABILITIES_HPP
