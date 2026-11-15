#ifndef KALDIGLUTIL_HPP
#define KALDIGLUTIL_HPP
/**
 * @file GlUtil.hpp
 * @brief OpenGL / OpenGL ES helper surface shared by KaldiGL and KaldiGLES.
 *
 * Historically this file lived in `KaldiGL/` and was written for the legacy
 * object-oriented branch, before Kaldi existed. It has been moved here and
 * rewritten against the current contracts:
 *
 *  - Euphemy remains the source of truth for CPU-side image data
 *    (`Euph::Media::Image::DecodeTarget` / `ImageView`).
 *  - Kaldi remains the source of truth for portable graphics metadata
 *    (`Kld::VertexDescriptor`, `Kld::BufferMapAccess`, ...).
 *  - Nothing in here talks to Kaldi's command queue; these are pure "translate
 *    and talk to the driver" helpers that a backend implementation calls while
 *    it executes a command.
 *
 * @note Every function assumes a current context on the calling thread. The
 * texture upload helpers bind the target texture to @c GL_TEXTURE_2D,
 * @c GL_TEXTURE_3D, @c GL_TEXTURE_2D_ARRAY or @c GL_TEXTURE_CUBE_MAP on the
 * <em>active texture unit</em> and leave it bound. `applyVertexDescriptor()`
 * requires a bound vertex array object, exactly like `glVertexAttribPointer()`
 * does in a core profile.
 */
#include "GlBackend.hpp"
#if KALDIGL_IS_GLES
#include <glad/gles2.h>
#else
#include <glad/gl.h>
#endif
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Kaldi/KaldiOperation.hpp>
#include <Kaldi/KldVertexFormat.hpp>
#include <cstdint>
#include <optional>

namespace KALDIGL_NS {

// ─────────────────────────────────────────────────────────────────────────────
// Format translation
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief The descriptor triplet OpenGL needs to interpret a CPU-side pixel buffer.
 *
 * `internalFormat` describes the GPU-side storage, `format`/`type` describe the
 * layout of the pixels the caller hands over in `DecodeTarget` frames.
 */
struct TextureFormat {
	/** @brief Sized internal format passed to `glTexImage*`. */
	GLenum internalFormat = 0;
	/** @brief Client pixel format of the upload data. */
	GLenum format = 0;
	/** @brief Client pixel component type of the upload data. */
	GLenum type = 0;
	/** @brief True for block-compressed formats that must go through `glCompressedTex*`. */
	bool compressed = false;
};

/** @brief True for the S3TC/DXT block-compressed formats Euphemy can produce. */
bool isCompressedFormat(Euph::Media::Image::Format format) noexcept;

/**
 * @brief Translates the GPU-side storage format of a Euphemy pixel format.
 * @return A sized internal format, or @c 0 when the format is not uploadable here.
 *
 * Formats that have no OpenGL / OpenGL ES equivalent return @c 0. The most
 * prominent examples are the 32-bit unsigned/signed integer color formats,
 * 64-bit double formats, and - on GLES - every BGR/ARGB layout plus the 16-bit
 * SNORM layouts, because `GL_EXT_texture_format_BGRA8888` and the 16-bit SNORM
 * formats are not part of the ES 3.2 core header set we ship.
 */
GLenum translateToInternalFormat(Euph::Media::Image::Format format) noexcept;

/** @brief Translates the client pixel format of a Euphemy pixel format, or @c 0. */
GLenum translateToFormat(Euph::Media::Image::Format format) noexcept;

/** @brief Translates the client component type of a Euphemy pixel format, or @c 0. */
GLenum translateToType(Euph::Media::Image::Format format) noexcept;

/**
 * @brief Builds the full format descriptor for a Euphemy pixel format.
 * @return The descriptor, or `std::nullopt` when any of its parts is unsupported.
 */
std::optional<TextureFormat> translateTextureFormat(Euph::Media::Image::Format format) noexcept;

/**
 * @brief Same as translateTextureFormat(), but throws instead of returning `std::nullopt`.
 * @throws std::runtime_error When the format cannot be expressed in the current dialect.
 */
TextureFormat requireTextureFormat(Euph::Media::Image::Format format);

// ─────────────────────────────────────────────────────────────────────────────
// Texture upload
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Geometry and mip-level bookkeeping produced by a texture upload. */
struct TextureUploadInfo {
	/** @brief Width of mip level 0 in texels. */
	unsigned width = 0;
	/** @brief Height of mip level 0 in texels. */
	unsigned height = 0;
	/** @brief Depth, array layer count or face count of mip level 0. */
	unsigned depth = 0;
	/** @brief Row stride of mip level 0 in bytes; @c 0 for block-compressed data. */
	unsigned stride = 0;
	/** @brief Number of mip levels that were actually uploaded. */
	uint8_t levels = 0;
};

/**
 * @brief Uploads every frame of a decode target as a mip chain of a 2D texture.
 * @param texture Name of an existing texture object (the caller need not bind it).
 * @param source Decoded image whose frames become mip levels 0..N-1.
 * @param wantedMipmaps Mip levels the caller wants; missing ones are generated.
 * @throws std::runtime_error On unsupported formats or inconsistent frame data.
 */
TextureUploadInfo uploadTexture2D(GLuint texture, const Euph::Media::Image::DecodeTarget& source, uint8_t wantedMipmaps = 0);

/** @brief Uploads a single-level 2D texture from a raw image view. */
TextureUploadInfo uploadTexture2D(GLuint texture, const Euph::Media::Image::ImageView& source, uint8_t wantedMipmaps = 0);

/**
 * @brief Uploads every frame of a decode target as a slice of a 3D/2D-array texture.
 * @param asArray @c true targets `GL_TEXTURE_2D_ARRAY`, @c false targets `GL_TEXTURE_3D`.
 */
TextureUploadInfo uploadTexture3D(GLuint texture, const Euph::Media::Image::DecodeTarget& source, bool asArray, uint8_t wantedMipmaps = 0);

/**
 * @brief Uploads exactly six frames as the faces of a cube map.
 * @throws std::runtime_error When the decode target does not hold exactly six frames.
 */
TextureUploadInfo uploadTextureCube(GLuint texture, const Euph::Media::Image::DecodeTarget& source, uint8_t wantedMipmaps = 0);

// ─────────────────────────────────────────────────────────────────────────────
// Buffers and vertex input
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Translates a Kaldi mapping access mode into `GL_MAP_*` bits.
 * @param access Portable mapping access requested by `OpMapBufferObject`.
 * @param entireBuffer Hint that the whole buffer is about to be replaced.
 *
 * `GL_MAP_FLUSH_EXPLICIT_BIT` is always requested for writable mappings, so
 * callers must call `glFlushMappedBufferRange()` for the ranges they touched
 * before unmapping. A read-only mapping requests `GL_MAP_READ_BIT` alone.
 */
GLbitfield toGlMapBits(Kld::BufferMapAccess access, bool entireBuffer);

/**
 * @brief Feeds a Kaldi vertex layout into the currently bound vertex array object.
 *
 * The attribute location is the index inside `Kld::VertexDescriptor::descriptors`:
 * the GL and Vulkan backends address attributes by location, while
 * `Kld::AttributeDescriptor::SemanticName`/`SemanticIndex` exist for the
 * Direct3D input-layout model. Use one `VertexDescriptor` per bound vertex
 * array object; multi-stream descriptors are not represented by Kaldi yet.
 *
 * Integer attributes use `glVertexAttribIPointer()` unless they are marked
 * normalized; double attributes use `glVertexAttribLPointer()` on desktop GL.
 *
 * @return @c false when an attribute used a primitive type the current dialect
 *         cannot express (64-bit components on GLES). Successfully translated
 *         attributes are still applied; the caller decides whether that is fatal.
 */
bool applyVertexDescriptor(const Kld::VertexDescriptor& vertexDescriptor);

}
#endif // KALDIGLUTIL_HPP
