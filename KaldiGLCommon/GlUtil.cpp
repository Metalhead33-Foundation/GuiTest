/**
 * @file GlUtil.cpp
 * @brief Implements the OpenGL / OpenGL ES helpers declared in GlUtil.hpp.
 *
 * The format tables below are dialect-aware: `KALDIGL_IS_GLES` selects the
 * OpenGL ES 3.2 answer wherever desktop OpenGL and GLSL ES disagree. Formats
 * that the current dialect cannot express deliberately translate to @c 0 so
 * the caller gets a diagnosable "unsupported" instead of a silently wrong
 * upload.
 */
#include "GlUtil.hpp"
#include <limits>
#include <stdexcept>
#include <string>

namespace KALDIGL_NS {

namespace {

using namespace Euph::Media::Image;

/** @brief Reads the color component encoded in a standard Euphemy format. */
constexpr unsigned colourComponent(Format format) noexcept
{
	return static_cast<unsigned>(format) & static_cast<unsigned>(Format::COLOR_COMPONENT_MASK);
}

/** @brief Reads the component resolution encoded in a standard Euphemy format. */
constexpr unsigned componentResolution(Format format) noexcept
{
	return static_cast<unsigned>(format) & static_cast<unsigned>(Format::COLOR_RESOLUTION_MASK);
}

/** @brief True for formats whose value encodes a standard color+resolution pair. */
constexpr bool isStandardFormat(Format format) noexcept
{
	return (static_cast<unsigned>(format) & static_cast<unsigned>(Format::SPECIAL_TYPE_START)) == 0;
}

/** @brief Describes an unsupported translation in exception messages. */
std::string unsupportedFormatMessage(Format format, const char* what)
{
	return std::string("GlUtil: unsupported ") + what + " for Euphemy image format 0x" +
	       std::to_string(static_cast<unsigned>(format));
}

} // namespace

bool isCompressedFormat(Format format) noexcept
{
	switch (format) {
		case Format::B_DXT1:
		case Format::B_DXT3:
		case Format::B_DXT5: return true;
		default: return false;
	}
}

GLenum translateToInternalFormat(Format format) noexcept
{
	switch (format) {
		// ── Standard single channel ──────────────────────────────────────────
		case Format::R8U: return GL_R8;
		case Format::R8S: return GL_R8_SNORM;
		case Format::R16U: return KALDIGL_GL_DESKTOP_ONLY(GL_R16);
		case Format::R16S: return KALDIGL_GL_DESKTOP_ONLY(GL_R16_SNORM);
		case Format::R16F: return GL_R16F;
		case Format::R32U: return 0;
		case Format::R32S: return 0;
		case Format::R32F: return GL_R32F;
		case Format::R64F: return 0;
		// ── Standard two channel ─────────────────────────────────────────────
		case Format::RG8U: return GL_RG8;
		case Format::RG8S: return GL_RG8_SNORM;
		case Format::RG16U: return KALDIGL_GL_DESKTOP_ONLY(GL_RG16);
		case Format::RG16S: return KALDIGL_GL_DESKTOP_ONLY(GL_RG16_SNORM);
		case Format::RG16F: return GL_RG16F;
		case Format::RG32U: return 0;
		case Format::RG32S: return 0;
		case Format::RG32F: return GL_RG32F;
		case Format::RG64F: return 0;
		// ── Standard three channel ───────────────────────────────────────────
		case Format::RGB8U: return GL_RGB8;
		case Format::RGB8S: return GL_RGB8_SNORM;
		case Format::RGB16U: return KALDIGL_GL_DESKTOP_ONLY(GL_RGB16);
		case Format::RGB16S: return KALDIGL_GL_DESKTOP_ONLY(GL_RGB16_SNORM);
		case Format::RGB16F: return GL_RGB16F;
		case Format::RGB32U: return 0;
		case Format::RGB32S: return 0;
		case Format::RGB32F: return GL_RGB32F;
		case Format::RGB64F: return 0;
		// ── Standard three channel, BGR memory order ─────────────────────────
		// GPU-side storage is identical to RGB; only the client format differs.
		case Format::BGR8U: return GL_RGB8;
		case Format::BGR8S: return GL_RGB8_SNORM;
		case Format::BGR16U: return KALDIGL_GL_DESKTOP_ONLY(GL_RGB16);
		case Format::BGR16S: return KALDIGL_GL_DESKTOP_ONLY(GL_RGB16_SNORM);
		case Format::BGR16F: return GL_RGB16F;
		case Format::BGR32U: return 0;
		case Format::BGR32S: return 0;
		case Format::BGR32F: return GL_RGB32F;
		case Format::BGR64F: return 0;
		// ── Standard four channel ────────────────────────────────────────────
		case Format::RGBA8U: return GL_RGBA8;
		case Format::RGBA8S: return GL_RGBA8_SNORM;
		case Format::RGBA16U: return KALDIGL_GL_DESKTOP_ONLY(GL_RGBA16);
		case Format::RGBA16S: return KALDIGL_GL_DESKTOP_ONLY(GL_RGBA16_SNORM);
		case Format::RGBA16F: return GL_RGBA16F;
		case Format::RGBA32U: return 0;
		case Format::RGBA32S: return 0;
		case Format::RGBA32F: return GL_RGBA32F;
		case Format::RGBA64F: return 0;
		// ── Standard four channel, BGRA memory order ─────────────────────────
		case Format::BGRA8U: return GL_RGBA8;
		case Format::BGRA8S: return GL_RGBA8_SNORM;
		case Format::BGRA16U: return KALDIGL_GL_DESKTOP_ONLY(GL_RGBA16);
		case Format::BGRA16S: return KALDIGL_GL_DESKTOP_ONLY(GL_RGBA16_SNORM);
		case Format::BGRA16F: return GL_RGBA16F;
		case Format::BGRA32U: return 0;
		case Format::BGRA32S: return 0;
		case Format::BGRA32F: return GL_RGBA32F;
		case Format::BGRA64F: return 0;
		// ── Standard four channel, ARGB memory order ─────────────────────────
		// GL has no ARGB client format, so these can never be uploaded directly.
		case Format::ARGB8U: return 0;
		case Format::ARGB8S: return 0;
		case Format::ARGB16U: return 0;
		case Format::ARGB16S: return 0;
		case Format::ARGB16F: return 0;
		case Format::ARGB32U: return 0;
		case Format::ARGB32S: return 0;
		case Format::ARGB32F: return 0;
		case Format::ARGB64F: return 0;
		// ── Special types ────────────────────────────────────────────────────
		case Format::INDEXED: return 0;
		case Format::RGB565: return GL_RGB565;
		case Format::RGB555: return GL_RGB5_A1;
		case Format::RGB444: return GL_RGBA4;
		case Format::RGBA444: return GL_RGBA4;
		case Format::DEPTH_COMPONENT16: return GL_DEPTH_COMPONENT16;
		case Format::DEPTH_COMPONENT24: return GL_DEPTH_COMPONENT24;
		case Format::DEPTH_COMPONENT32: return KALDIGL_GL_DESKTOP_ONLY(GL_DEPTH_COMPONENT32);
		case Format::DEPTH_COMPONENT32F: return GL_DEPTH_COMPONENT32F;
		case Format::DEPTH24_STENCIL8: return GL_DEPTH24_STENCIL8;
		case Format::DEPTH32F_STENCIL8: return GL_DEPTH32F_STENCIL8;
		case Format::STENCIL_INDEX8: return GL_STENCIL_INDEX8;
		// ── Block compression ────────────────────────────────────────────────
		case Format::B_DXT1: return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		case Format::B_DXT3: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		case Format::B_DXT5: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		default: return 0;
	}
}

GLenum translateToFormat(Format format) noexcept
{
	switch (format) {
		// Packed special types take the plain colour layout as client format.
		case Format::RGB565:
		case Format::RGB555:
		case Format::RGB444: return GL_RGB;
		case Format::RGBA444: return GL_RGBA;
		case Format::DEPTH_COMPONENT16:
		case Format::DEPTH_COMPONENT24:
		case Format::DEPTH_COMPONENT32:
		case Format::DEPTH_COMPONENT32F: return GL_DEPTH_COMPONENT;
		case Format::DEPTH24_STENCIL8:
		case Format::DEPTH32F_STENCIL8: return GL_DEPTH_STENCIL;
		case Format::STENCIL_INDEX8: return GL_STENCIL_INDEX;
		// Palette indices and block-compressed images are never client formats.
		case Format::INDEXED:
		case Format::B_DXT1:
		case Format::B_DXT3:
		case Format::B_DXT5: return 0;
		default: break;
	}
	if (!isStandardFormat(format)) {
		return 0;
	}
	switch (colourComponent(format)) {
		case static_cast<unsigned>(Format::ST_GREYSCALE): return GL_RED;
		case static_cast<unsigned>(Format::ST_RG): return GL_RG;
		case static_cast<unsigned>(Format::ST_RGB): return GL_RGB;
		case static_cast<unsigned>(Format::ST_BGR): return KALDIGL_GL_DESKTOP_ONLY(GL_BGR);
		case static_cast<unsigned>(Format::ST_RGBA): return GL_RGBA;
		case static_cast<unsigned>(Format::ST_BGRA): return KALDIGL_GL_DESKTOP_ONLY(GL_BGRA);
		case static_cast<unsigned>(Format::ST_ARGB): return 0;
		default: return 0;
	}
}

GLenum translateToType(Format format) noexcept
{
	switch (format) {
		case Format::RGB565: return GL_UNSIGNED_SHORT_5_6_5;
		case Format::RGB555: return GL_UNSIGNED_SHORT_5_5_5_1;
		case Format::RGB444: return GL_UNSIGNED_SHORT_4_4_4_4;
		case Format::RGBA444: return GL_UNSIGNED_SHORT_4_4_4_4;
		case Format::DEPTH_COMPONENT16: return GL_UNSIGNED_SHORT;
		case Format::DEPTH_COMPONENT24:
		case Format::DEPTH_COMPONENT32: return GL_UNSIGNED_INT;
		case Format::DEPTH_COMPONENT32F: return GL_FLOAT;
		case Format::DEPTH24_STENCIL8: return GL_UNSIGNED_INT_24_8;
		case Format::DEPTH32F_STENCIL8: return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
		case Format::STENCIL_INDEX8: return GL_UNSIGNED_BYTE;
		case Format::INDEXED:
		case Format::B_DXT1:
		case Format::B_DXT3:
		case Format::B_DXT5: return 0;
		default: break;
	}
	if (!isStandardFormat(format)) {
		return 0;
	}
	switch (componentResolution(format)) {
		case static_cast<unsigned>(Format::SC_UINT8): return GL_UNSIGNED_BYTE;
		case static_cast<unsigned>(Format::SC_INT8): return GL_BYTE;
		case static_cast<unsigned>(Format::SC_UINT16): return GL_UNSIGNED_SHORT;
		case static_cast<unsigned>(Format::SC_INT16): return GL_SHORT;
		case static_cast<unsigned>(Format::SC_UINT32): return GL_UNSIGNED_INT;
		case static_cast<unsigned>(Format::SC_INT32): return GL_INT;
		case static_cast<unsigned>(Format::SC_F16): return GL_HALF_FLOAT;
		case static_cast<unsigned>(Format::SC_F32): return GL_FLOAT;
		case static_cast<unsigned>(Format::SC_F64): return KALDIGL_GL_DESKTOP_ONLY(GL_DOUBLE);
		default: return 0;
	}
}

std::optional<TextureFormat> translateTextureFormat(Format format) noexcept
{
	TextureFormat translated;
	translated.internalFormat = translateToInternalFormat(format);
	if (translated.internalFormat == 0) {
		return std::nullopt;
	}
	translated.compressed = isCompressedFormat(format);
	if (translated.compressed) {
		// `glCompressedTexImage*` only needs the internal format and a byte size.
		return translated;
	}
	translated.format = translateToFormat(format);
	translated.type = translateToType(format);
	if (translated.format == 0 || translated.type == 0) {
		return std::nullopt;
	}
	return translated;
}

TextureFormat requireTextureFormat(Format format)
{
	const std::optional<TextureFormat> translated = translateTextureFormat(format);
	if (!translated.has_value()) {
		throw std::runtime_error(unsupportedFormatMessage(format, "texture format"));
	}
	return *translated;
}

namespace {

/** @brief Uploads one mip level of a 2D target, honouring the frame row stride. */
void uploadLevel2D(GLenum target, const TextureFormat& format, Format sourceFormat, const Frame& frame, unsigned level)
{
	if (format.compressed) {
		glCompressedTexImage2D(target, static_cast<GLint>(level), format.internalFormat,
							   static_cast<GLsizei>(frame.width), static_cast<GLsizei>(frame.height), 0,
							   static_cast<GLsizei>(frame.data.size()), frame.data.data());
		return;
	}
	const std::size_t pixelSize = pixelByteSize(sourceFormat);
	if (pixelSize != 0 && frame.width != 0) {
		const unsigned rowLength = frame.stride / static_cast<unsigned>(pixelSize);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength == frame.width ? 0 : static_cast<GLint>(rowLength));
	}
	glTexImage2D(target, static_cast<GLint>(level), static_cast<GLint>(format.internalFormat),
				 static_cast<GLsizei>(frame.width), static_cast<GLsizei>(frame.height), 0,
				 format.format, format.type, frame.data.data());
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

/** @brief Rounds a size down to the maximum number of uploadable mip levels. */
uint8_t clampMipCount(std::size_t frameCount)
{
	constexpr std::size_t maximum = std::numeric_limits<uint8_t>::max();
	return static_cast<uint8_t>(frameCount < maximum ? frameCount : maximum);
}

/** @brief Rejects decode targets that cannot be uploaded at all. */
const std::pmr::vector<Frame>& requireFrames(const DecodeTarget& source)
{
	const std::pmr::vector<Frame>& frames = source.getFrames();
	if (frames.empty()) {
		throw std::runtime_error("GlUtil: decode target has no frames to upload");
	}
	return frames;
}

} // namespace

TextureUploadInfo uploadTexture2D(GLuint texture, const DecodeTarget& source, uint8_t wantedMipmaps)
{
	const TextureFormat format = requireTextureFormat(source.getFormat());
	const std::pmr::vector<Frame>& frames = requireFrames(source);

	glBindTexture(GL_TEXTURE_2D, texture);
	const Frame& first = frames.front();
	TextureUploadInfo info;
	info.width = first.width;
	info.height = first.height;
	info.depth = 1;
	info.stride = format.compressed ? 0u : first.stride;

	const uint8_t uploadable = clampMipCount(frames.size());
	for (uint8_t level = 0; level < uploadable; ++level) {
		uploadLevel2D(GL_TEXTURE_2D, format, source.getFormat(), frames[level], level);
	}
	info.levels = uploadable;
	if (info.levels < wantedMipmaps) {
		glGenerateMipmap(GL_TEXTURE_2D);
		info.levels = wantedMipmaps;
	}
	return info;
}

TextureUploadInfo uploadTexture2D(GLuint texture, const ImageView& source, uint8_t wantedMipmaps)
{
	const TextureFormat format = requireTextureFormat(source.format);

	glBindTexture(GL_TEXTURE_2D, texture);
	if (format.compressed) {
		// ImageView does not expose the byte size of a compressed image; the caller
		// must use the DecodeTarget overload for block-compressed uploads.
		throw std::runtime_error(unsupportedFormatMessage(source.format, "ImageView upload of a compressed format"));
	}
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format.internalFormat),
				 static_cast<GLsizei>(source.width), static_cast<GLsizei>(source.height), 0,
				 format.format, format.type, source.data);

	TextureUploadInfo info;
	info.width = source.width;
	info.height = source.height;
	info.depth = 1;
	info.stride = source.stride;
	info.levels = 1;
	if (info.levels < wantedMipmaps) {
		glGenerateMipmap(GL_TEXTURE_2D);
		info.levels = wantedMipmaps;
	}
	return info;
}

TextureUploadInfo uploadTexture3D(GLuint texture, const DecodeTarget& source, bool asArray, uint8_t wantedMipmaps)
{
	const TextureFormat format = requireTextureFormat(source.getFormat());
	const std::pmr::vector<Frame>& frames = requireFrames(source);
	const GLenum target = asArray ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_3D;

	glBindTexture(target, texture);
	const Frame& first = frames.front();
	const GLsizei depth = static_cast<GLsizei>(frames.size());
	if (format.compressed) {
		glCompressedTexImage3D(target, 0, format.internalFormat,
							   static_cast<GLsizei>(first.width), static_cast<GLsizei>(first.height), depth,
							   0, 0, nullptr);
	} else {
		glTexImage3D(target, 0, static_cast<GLint>(format.internalFormat),
					 static_cast<GLsizei>(first.width), static_cast<GLsizei>(first.height), depth,
					 0, format.format, format.type, nullptr);
	}

	const std::size_t pixelSize = pixelByteSize(source.getFormat());
	for (std::size_t slice = 0; slice < frames.size(); ++slice) {
		const Frame& frame = frames[slice];
		if (format.compressed) {
			glCompressedTexSubImage3D(target, 0, 0, 0, static_cast<GLint>(slice),
									  static_cast<GLsizei>(frame.width), static_cast<GLsizei>(frame.height), 1,
									  format.internalFormat, static_cast<GLsizei>(frame.data.size()), frame.data.data());
			continue;
		}
		if (pixelSize != 0 && frame.width != 0) {
			const unsigned rowLength = frame.stride / static_cast<unsigned>(pixelSize);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength == frame.width ? 0 : static_cast<GLint>(rowLength));
		}
		glTexSubImage3D(target, 0, 0, 0, static_cast<GLint>(slice),
						static_cast<GLsizei>(frame.width), static_cast<GLsizei>(frame.height), 1,
						format.format, format.type, frame.data.data());
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}

	TextureUploadInfo info;
	info.width = first.width;
	info.height = first.height;
	info.depth = static_cast<unsigned>(frames.size());
	info.stride = format.compressed ? 0u : first.stride;
	info.levels = 1;
	if (info.levels < wantedMipmaps) {
		glGenerateMipmap(target);
		info.levels = wantedMipmaps;
	}
	return info;
}

TextureUploadInfo uploadTextureCube(GLuint texture, const DecodeTarget& source, uint8_t wantedMipmaps)
{
	constexpr std::size_t faceCount = 6;
	const TextureFormat format = requireTextureFormat(source.getFormat());
	const std::pmr::vector<Frame>& frames = requireFrames(source);
	if (frames.size() != faceCount) {
		throw std::runtime_error("GlUtil: cube map upload requires exactly six frames, got " +
								 std::to_string(frames.size()));
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
	for (std::size_t face = 0; face < faceCount; ++face) {
		const GLenum faceTarget = static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(face));
		uploadLevel2D(faceTarget, format, source.getFormat(), frames[face], 0);
	}

	const Frame& first = frames.front();
	TextureUploadInfo info;
	info.width = first.width;
	info.height = first.height;
	info.depth = static_cast<unsigned>(faceCount);
	info.stride = format.compressed ? 0u : first.stride;
	info.levels = 1;
	if (info.levels < wantedMipmaps) {
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		info.levels = wantedMipmaps;
	}
	return info;
}

GLbitfield toGlMapBits(Kld::BufferMapAccess access, bool entireBuffer)
{
	switch (access) {
		case Kld::BufferMapAccess::ReadOnly:
			return GL_MAP_READ_BIT;
		case Kld::BufferMapAccess::ReadWrite:
			return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT;
		case Kld::BufferMapAccess::WriteOnly:
			return GL_MAP_WRITE_BIT |
				   (entireBuffer ? GL_MAP_INVALIDATE_BUFFER_BIT : GL_MAP_INVALIDATE_RANGE_BIT) |
				   GL_MAP_FLUSH_EXPLICIT_BIT;
	}
	return GL_MAP_READ_BIT;
}

bool applyVertexDescriptor(const Kld::VertexDescriptor& vertexDescriptor)
{
	bool supported = true;
	for (std::size_t location = 0; location < vertexDescriptor.descriptors.size(); ++location) {
		const Kld::AttributeDescriptor& attribute = vertexDescriptor.descriptors[location];
		const GLuint index = static_cast<GLuint>(location);
		const GLint size = static_cast<GLint>(attribute.type & Kld::PrimitiveType::SIZE_MASK);
		if (size < 1 || size > 4) {
			supported = false;
			continue;
		}
		const bool normalized = (attribute.type & Kld::PrimitiveType::IS_NORMALIZED) != 0;
		const void* offset = reinterpret_cast<const void*>(attribute.offset);
		const GLsizei stride = static_cast<GLsizei>(vertexDescriptor.stride);
		const unsigned scalar = attribute.type & Kld::PrimitiveType::TYPE_MASK;

		GLenum type = GL_INVALID_ENUM;
		bool integer = false;
		bool wide = false;
		switch (scalar) {
			case Kld::PrimitiveType::TYPE_HALF_FLOAT: type = GL_HALF_FLOAT; break;
			case Kld::PrimitiveType::TYPE_FLOAT: type = GL_FLOAT; break;
			case Kld::PrimitiveType::TYPE_DOUBLE: wide = true; break;
			case Kld::PrimitiveType::TYPE_UBYTE: type = GL_UNSIGNED_BYTE; integer = true; break;
			case Kld::PrimitiveType::TYPE_SBYTE: type = GL_BYTE; integer = true; break;
			case Kld::PrimitiveType::TYPE_USHORT: type = GL_UNSIGNED_SHORT; integer = true; break;
			case Kld::PrimitiveType::TYPE_SSHORT: type = GL_SHORT; integer = true; break;
			case Kld::PrimitiveType::TYPE_UINT: type = GL_UNSIGNED_INT; integer = true; break;
			case Kld::PrimitiveType::TYPE_SINT: type = GL_INT; integer = true; break;
			default: break;
		}
		if (wide) {
#if KALDIGL_IS_GLES
			// GLSL ES has no 64-bit vertex attribute types.
			supported = false;
			continue;
#else
			glVertexAttribLPointer(index, size, GL_DOUBLE, stride, offset);
			glEnableVertexAttribArray(index);
			continue;
#endif
		}
		if (type == GL_INVALID_ENUM) {
			supported = false;
			continue;
		}
		if (integer && !normalized) {
			glVertexAttribIPointer(index, size, type, stride, offset);
		} else {
			glVertexAttribPointer(index, size, type, normalized ? GL_TRUE : GL_FALSE, stride, offset);
		}
		glEnableVertexAttribArray(index);
	}
	return supported;
}

} // namespace KALDIGL_NS
