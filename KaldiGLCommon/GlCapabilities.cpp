/**
 * @file GlCapabilities.cpp
 * @brief Implements GlCapabilities.hpp.
 *
 * This unit is compiled twice - once for desktop OpenGL, once for OpenGL ES -
 * with `KALDIGL_IS_GLES` selecting the few places where the two dialects disagree
 * on which query is valid (1D textures, vertex attribute bindings, compute and
 * indirect-draw version gates, debug-label extensions).
 */
#include "GlCapabilities.hpp"
#if KALDIGL_IS_GLES
#include <glad/gles2.h>
#else
#include <glad/gl.h>
#endif
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace KALDIGL_NS {

namespace {

/** @brief Reads one integer limit; a failed query yields @c 0. */
GLint queryInteger(GLenum name) noexcept
{
	GLint value = 0;
	glGetIntegerv(name, &value);
	return value;
}

/** @brief Scans the indexed extension list of the current context. */
bool extensionInList(std::string_view extension) noexcept
{
	GLint count = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &count);
	if (count <= 0) {
		return false;
	}
	for (GLint index = 0; index < count; ++index) {
		const char* name = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(index)));
		if (name != nullptr && extension == name) {
			return true;
		}
	}
	return false;
}

/** @brief Splits the legacy space-separated `GL_EXTENSIONS` string. */
bool extensionInString(std::string_view extension) noexcept
{
	const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
	if (extensions == nullptr) {
		return false;
	}
	std::string_view remaining(extensions);
	while (!remaining.empty()) {
		const std::size_t separator = remaining.find(' ');
		if (remaining.substr(0, separator) == extension) {
			return true;
		}
		if (separator == std::string_view::npos) {
			break;
		}
		remaining.remove_prefix(separator + 1);
	}
	return false;
}

/**
 * @brief True when the driver exposes the block-compressed formats `GlUtil` uploads.
 *
 * `GlUtil` only produces the S3TC/DXT1-DXT5 internal formats, so "texture
 * compression" in the Kaldi sense is exactly the presence of one of those
 * extension families. Desktop ETC2/BPTC/ASTC support alone does not qualify,
 * because Kaldi has no command vocabulary for those encodings yet.
 */
bool supportsS3tc() noexcept
{
	return hasGlExtension("GL_EXT_texture_compression_s3tc")
		|| hasGlExtension("GL_EXT_texture_compression_dxt1")
		|| hasGlExtension("GL_ANGLE_texture_compression_dxt3")
		|| hasGlExtension("GL_ANGLE_texture_compression_dxt5");
}

} // namespace

GlVersion queryGlVersion() noexcept
{
	GlVersion version;
	if (glGetString == nullptr) {
		return version;
	}
	const char* text = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	if (text == nullptr) {
		return version;
	}
	constexpr const char esPrefix[] = "OpenGL ES ";
	if (std::strncmp(text, esPrefix, sizeof(esPrefix) - 1) == 0) {
		text += sizeof(esPrefix) - 1;
	}
	if (std::sscanf(text, "%d.%d", &version.major, &version.minor) != 2) {
		version.major = 0;
		version.minor = 0;
	}
	return version;
}

bool hasGlExtension(std::string_view extension) noexcept
{
	if (extension.empty() || glGetIntegerv == nullptr || glGetString == nullptr) {
		return false;
	}
	if (glGetStringi != nullptr) {
		GLint count = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &count);
		if (count > 0) {
			return extensionInList(extension);
		}
	}
	return extensionInString(extension);
}

Kld::HalCapabilities probeCapabilities() noexcept
{
	if (glGetIntegerv == nullptr || glGetString == nullptr) {
		return Kld::halCapabilitiesNone();
	}
	const GlVersion version = queryGlVersion();
	if (!version.valid()) {
		// No current context: glGetString(GL_VERSION) returned null.
		return Kld::halCapabilitiesNone();
	}

	Kld::HalCapabilities capabilities = Kld::halCapabilitiesNone();

	const GLint maxTextureSize = queryInteger(GL_MAX_TEXTURE_SIZE);
	capabilities.maxTexture2DSize = static_cast<std::uint32_t>(maxTextureSize);
	capabilities.maxTexture3DSize = static_cast<std::uint32_t>(queryInteger(GL_MAX_3D_TEXTURE_SIZE));
	capabilities.maxTextureArrayLayers = static_cast<std::uint32_t>(queryInteger(GL_MAX_ARRAY_TEXTURE_LAYERS));
	capabilities.maxCubeMapSize = static_cast<std::uint32_t>(queryInteger(GL_MAX_CUBE_MAP_TEXTURE_SIZE));
	capabilities.maxSamples = static_cast<std::uint32_t>(queryInteger(GL_MAX_SAMPLES));
	capabilities.maxColorAttachments = static_cast<std::uint32_t>(queryInteger(GL_MAX_COLOR_ATTACHMENTS));
	capabilities.maxVertexAttributes = static_cast<std::uint32_t>(queryInteger(GL_MAX_VERTEX_ATTRIBS));
	// GL binds sampler objects to texture units, so the texture-unit count is
	// also the maximum number of distinct sampler bindings.
	const GLint textureUnits = queryInteger(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
	capabilities.maxSampledTextureBindings = static_cast<std::uint32_t>(textureUnits);
	capabilities.maxSamplerBindings = static_cast<std::uint32_t>(textureUnits);
	capabilities.maxUniformBufferBindings = static_cast<std::uint32_t>(queryInteger(GL_MAX_UNIFORM_BUFFER_BINDINGS));
	capabilities.maxFramesInFlight = Kld::kDefaultMaxFramesInFlight;
	capabilities.supportsReadback = true;

#if KALDIGL_IS_GLES
	const bool computeSupported = glVersionAtLeast(version, 3, 1);
	// OpenGL ES has no 1D texture targets at any version.
	capabilities.maxTexture1DSize = 0;
	// Vertex attribute bindings are an ES 3.2 feature; earlier contexts can only
	// bind one vertex buffer at a time.
	capabilities.maxVertexBuffers = glVersionAtLeast(version, 3, 2)
		? static_cast<std::uint32_t>(queryInteger(GL_MAX_VERTEX_ATTRIB_BINDINGS))
		: 1u;
	capabilities.supportsQueries = glVersionAtLeast(version, 3, 0);
	capabilities.supportsMultisampleResolve = glVersionAtLeast(version, 3, 0);
	capabilities.supportsIndirectDraw = glVersionAtLeast(version, 3, 1);
	capabilities.supportsDebugLabels = hasGlExtension("GL_KHR_debug");
#else
	const bool computeSupported = glVersionAtLeast(version, 4, 3);
	// The desktop 1D texture limit is shared with GL_MAX_TEXTURE_SIZE.
	capabilities.maxTexture1DSize = static_cast<std::uint32_t>(maxTextureSize);
	capabilities.maxVertexBuffers = static_cast<std::uint32_t>(queryInteger(GL_MAX_VERTEX_ATTRIB_BINDINGS));
	capabilities.supportsQueries = glVersionAtLeast(version, 3, 3);
	capabilities.supportsMultisampleResolve = glVersionAtLeast(version, 3, 0);
	capabilities.supportsIndirectDraw = glVersionAtLeast(version, 4, 0);
	// glPushDebugGroup/glPopDebugGroup are core since 4.3; KHR_debug covers
	// drivers that expose the extension on an older context.
	capabilities.supportsDebugLabels = computeSupported || hasGlExtension("GL_KHR_debug");
#endif

	capabilities.supportsCompute = computeSupported;
	capabilities.supportsStorageResources = computeSupported;
	capabilities.supportsTextureCompression = supportsS3tc();
	if (computeSupported) {
		capabilities.maxStorageBufferBindings =
			static_cast<std::uint32_t>(queryInteger(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS));
		capabilities.maxStorageImageBindings = static_cast<std::uint32_t>(queryInteger(GL_MAX_IMAGE_UNITS));
	}
	// Explicit swapchain objects are a validated view of the context drawable.
	capabilities.supportsExplicitSwapchains = true;
	capabilities.version = Kld::KaldiHalVersion1_1;

	return capabilities;
}

}
