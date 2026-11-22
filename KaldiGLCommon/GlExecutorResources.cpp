/**
 * @file GlExecutorResources.cpp
 * @brief Buffer, texture, sampler, image and framebuffer handlers for the GL executor.
 */
#include "GlCommandExecutor.hpp"
#include "GlExecutorMapping.hpp"
#include <algorithm>
#include <limits>

namespace KALDIGL_NS {

namespace {

/** @brief Size of mip level @p level on an axis of @p size texels. */
std::uint32_t mipExtent(std::uint32_t size, unsigned level) noexcept
{
	const std::uint32_t shifted = level >= 32 ? 0u : size >> level;
	return shifted == 0 ? 1u : shifted;
}

/** @brief True when @p map already owns @p id. */
template <typename Map>
bool contains(const Map& map, Kld::HandleId id) noexcept
{
	return map.find(id) != map.end();
}

} // namespace

Kld::HalStatus CommandExecutor::createBuffer(const Kld::OpCreateBufferObject& op)
{
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "buffer handle is zero or already in use");
	}
	GlBufferObject object;
	object.size = op.size;
	object.policy = op.policy;
	object.target = detail::bufferTarget(op.policy);
	glGenBuffers(1, &object.name);
	if (object.name == 0) {
		return fail(Kld::HalStatusCode::AllocationFailed, "glGenBuffers returned no name");
	}
	glBindBuffer(object.target, object.name);
	glBufferData(object.target, op.size, op.data, GL_DYNAMIC_DRAW);
	glBindBuffer(object.target, 0);
	buffers_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::destroyBuffer(const Kld::OpDestroy& op)
{
	const auto found = buffers_.find(op.id);
	if (found == buffers_.end()) {
		return failInvalidHandle("buffer", op.id);
	}
	glDeleteBuffers(1, &found->second.name);
	buffers_.erase(found);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::resizeBuffer(const Kld::OpResizeBufferObject& op)
{
	GlBufferObject* object = buffer(op.id);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.id);
	}
	glBindBuffer(object->target, object->name);
	// GL has no "resize with preservation"; the HAL allows backend-defined contents
	// after a resize, so a fresh allocation is sufficient.
	glBufferData(object->target, op.size, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(object->target, 0);
	object->size = op.size;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::updateBuffer(const Kld::OpUpdateBufferObject& op)
{
	GlBufferObject* object = buffer(op.id);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.id);
	}
	if (op.data == nullptr || op.offset > object->size || op.size > object->size - op.offset) {
		return fail(Kld::HalStatusCode::InvalidCommand, "buffer update range is outside the buffer");
	}
	glBindBuffer(object->target, object->name);
	glBufferSubData(object->target, op.offset, op.size, op.data);
	glBindBuffer(object->target, 0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::mapBuffer(const Kld::OpMapBufferObject& op)
{
	GlBufferObject* object = buffer(op.id);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.id);
	}
	const std::uint32_t size = op.size != 0 ? op.size : object->size - std::min(op.offset, object->size);
	if (op.offset > object->size || size > object->size - op.offset || op.callback == nullptr) {
		return fail(Kld::HalStatusCode::InvalidCommand, "buffer map range or callback is invalid");
	}
	glBindBuffer(object->target, object->name);
	const GLbitfield bits = toGlMapBits(op.access, op.offset == 0 && size == object->size);
	void* mapped = glMapBufferRange(object->target, 0, object->size, bits);
	if (mapped == nullptr) {
		glBindBuffer(object->target, 0);
		return fail(Kld::HalStatusCode::BackendError, "glMapBufferRange failed");
	}
	auto* bytes = static_cast<std::uint8_t*>(mapped) + op.offset;
	op.callback(bytes, size, op.userData);
	if (op.access != Kld::BufferMapAccess::ReadOnly) {
		glFlushMappedBufferRange(object->target, op.offset, size);
	}
	glUnmapBuffer(object->target);
	glBindBuffer(object->target, 0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createVertexArray(const Kld::OpCreateVertexArrayObject& op)
{
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "vertex array handle is zero or already in use");
	}
	if (op.descriptor == nullptr || op.elementCount > op.descriptor->descriptors.size()) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the vertex array descriptor or element count is invalid");
	}
	GlVertexArrayObject object;
	glGenVertexArrays(1, &object.name);
	if (object.name == 0) {
		return fail(Kld::HalStatusCode::AllocationFailed, "glGenVertexArrays returned no name");
	}
	glBindVertexArray(object.name);
	const Kld::VertexDescriptor descriptor { op.descriptor->stride,
						 op.descriptor->descriptors.first(op.elementCount) };
	(void)applyVertexDescriptor(descriptor, 0);
	glBindVertexArray(currentVertexArray_ != 0 ? currentVertexArray_ : defaultVertexArray_);
	// Kaldi 1.0 has no BindVertexArrayObject opcode: pipelines own the VAO they
	// draw with. The object is kept (and released) so a future opcode can use it.
	vertexArrays_.emplace(op.id, object);
	return Kld::HalOk;
}

void CommandExecutor::applyDefaultTextureParameters(GlTextureObject& object) const
{
	glBindTexture(object.target, object.name);
	glTexParameteri(object.target, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(object.target, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(object.target, GL_TEXTURE_WRAP_R, GL_REPEAT);
	GLenum minification = object.levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
	if (config_.forceNearestFiltering) {
		minification = object.levels > 1
			? (config_.forceTrilinear ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST)
			: GL_NEAREST;
	} else if (config_.forceTrilinear && object.levels > 1) {
		minification = GL_LINEAR_MIPMAP_LINEAR;
	}
	glTexParameteri(object.target, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(minification));
	glTexParameteri(object.target, GL_TEXTURE_MAG_FILTER,
			config_.forceNearestFiltering ? GL_NEAREST : GL_LINEAR);
	glTexParameteri(object.target, GL_TEXTURE_MAX_LEVEL,
			object.levels > 0 ? static_cast<GLint>(object.levels - 1) : 0);
#if !KALDIGL_IS_GLES
	if (config_.textureLodBias != 0.0f) {
		glTexParameterf(object.target, GL_TEXTURE_LOD_BIAS, config_.textureLodBias);
	}
#endif
	if (anisotropyEnabled_ && config_.maxAnisotropy > 1.0f) {
		glTexParameterf(object.target, GL_TEXTURE_MAX_ANISOTROPY_EXT, config_.maxAnisotropy);
	}
}

Kld::HalStatus CommandExecutor::createTexture1D(const Kld::OpCreateTexture1D& op)
{
#if KALDIGL_IS_GLES
	(void)op;
	return failUnsupported("OpenGL ES has no 1D texture targets");
#else
	const std::optional<TextureFormat> translated = translateTextureFormat(op.format);
	if (!translated.has_value() || translated->compressed) {
		return failUnsupported("the requested 1D texture format cannot be expressed in OpenGL");
	}
	if (op.width == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a 1D texture needs a non-zero width");
	}
	if (config_.maxTextureSize != 0 && op.width > config_.maxTextureSize) {
		return failUnsupported("the texture exceeds the configured maximum texture size");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "texture handle is zero or already in use");
	}
	GlTextureObject object;
	object.target = GL_TEXTURE_1D;
	object.internalFormat = translated->internalFormat;
	object.format = op.format;
	object.width = op.width;
	object.height = 1;
	object.levels = 1;
	glGenTextures(1, &object.name);
	if (object.name == 0) {
		return fail(Kld::HalStatusCode::AllocationFailed, "glGenTextures returned no name");
	}
	applyDefaultTextureParameters(object);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage1D(GL_TEXTURE_1D, 0, static_cast<GLint>(object.internalFormat),
		     static_cast<GLsizei>(op.width), 0, translated->format, translated->type, op.pixels);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	textures_.emplace(op.id, object);
	return Kld::HalOk;
#endif
}

Kld::HalStatus CommandExecutor::createTexture2D(const Kld::OpCreateTexture2D& op)
{
	const std::optional<TextureFormat> translated = translateTextureFormat(op.format);
	if (!translated.has_value()) {
		return failUnsupported("the requested texture format cannot be expressed in this GL dialect");
	}
	if (translated->compressed) {
		return failUnsupported("use CreateCompressedTexture2D for block-compressed formats");
	}
	if (op.width == 0 || op.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a 2D texture needs non-zero extents");
	}
	if (config_.maxTextureSize != 0
	    && (op.width > config_.maxTextureSize || op.height > config_.maxTextureSize)) {
		return failUnsupported("the texture exceeds the configured maximum texture size");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "texture handle is zero or already in use");
	}
	GlTextureObject object;
	object.target = GL_TEXTURE_2D;
	object.internalFormat = translated->internalFormat;
	object.format = op.format;
	object.width = op.width;
	object.height = op.height;
	object.levels = op.mipLevels > 0 ? op.mipLevels : 1;
	glGenTextures(1, &object.name);
	if (object.name == 0) {
		return fail(Kld::HalStatusCode::AllocationFailed, "glGenTextures returned no name");
	}
	applyDefaultTextureParameters(object);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (std::uint8_t level = 0; level < object.levels; ++level) {
		glTexImage2D(GL_TEXTURE_2D, level, static_cast<GLint>(object.internalFormat),
			     static_cast<GLsizei>(mipExtent(op.width, level)),
			     static_cast<GLsizei>(mipExtent(op.height, level)), 0,
			     translated->format, translated->type, level == 0 ? op.pixels : nullptr);
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	if (op.generateMipmaps && object.levels > 1) {
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	textures_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createCompressedTexture2D(const Kld::OpCreateCompressedTexture2D& op)
{
	const std::optional<TextureFormat> translated = translateTextureFormat(op.format);
	if (!translated.has_value() || !translated->compressed) {
		return failUnsupported("the requested format is not a supported block-compressed format");
	}
	if (op.width == 0 || op.height == 0 || op.mipCount == 0 || op.mips == nullptr) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "a compressed texture needs extents, a mip count and a mip array");
	}
	if (op.layout.blockWidth == 0 || op.layout.blockHeight == 0 || op.layout.bytesPerBlock == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the compressed block layout is incomplete");
	}
	for (std::uint8_t level = 0; level < op.mipCount; ++level) {
		if (op.mips[level].data == nullptr || op.mips[level].byteSize == 0) {
			return fail(Kld::HalStatusCode::InvalidCommand, "a compressed mip has no payload");
		}
	}
	if (config_.maxTextureSize != 0
	    && (op.width > config_.maxTextureSize || op.height > config_.maxTextureSize)) {
		return failUnsupported("the texture exceeds the configured maximum texture size");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "texture handle is zero or already in use");
	}
	GlTextureObject object;
	object.target = GL_TEXTURE_2D;
	object.internalFormat = translated->internalFormat;
	object.format = op.format;
	object.width = op.width;
	object.height = op.height;
	object.levels = op.mipCount;
	object.compressed = true;
	glGenTextures(1, &object.name);
	if (object.name == 0) {
		return fail(Kld::HalStatusCode::AllocationFailed, "glGenTextures returned no name");
	}
	applyDefaultTextureParameters(object);
	for (std::uint8_t level = 0; level < op.mipCount; ++level) {
		const Kld::TextureMipData& mip = op.mips[level];
		glCompressedTexImage2D(GL_TEXTURE_2D, level, object.internalFormat, mip.width, mip.height, 0,
				       static_cast<GLsizei>(mip.byteSize), mip.data);
	}
	textures_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createTextureFromDecodeTarget(
	const Kld::OpCreateTexture2DFromDecodeTarget& op)
{
	if (op.source == nullptr || op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the decode target or texture handle is invalid");
	}
	GlTextureObject object;
	object.target = GL_TEXTURE_2D;
	object.format = op.source->getFormat();
	glGenTextures(1, &object.name);
	if (object.name == 0) {
		return fail(Kld::HalStatusCode::AllocationFailed, "glGenTextures returned no name");
	}
	try {
		const TextureUploadInfo info = uploadTexture2D(object.name, *op.source, op.mipLevels);
		object.width = static_cast<std::uint16_t>(info.width);
		object.height = static_cast<std::uint16_t>(info.height);
		object.levels = info.levels > 0 ? info.levels : 1;
		object.compressed = isCompressedFormat(object.format);
	} catch (const std::exception& error) {
		glDeleteTextures(1, &object.name);
		return fail(Kld::HalStatusCode::BackendError, error.what());
	}
	const std::optional<TextureFormat> translated = translateTextureFormat(object.format);
	if (translated.has_value()) {
		object.internalFormat = translated->internalFormat;
	}
	applyDefaultTextureParameters(object);
	textures_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createTexture3D(const Kld::OpCreateTexture3D& op)
{
	const std::optional<TextureFormat> translated = translateTextureFormat(op.format);
	if (!translated.has_value() || translated->compressed) {
		return failUnsupported("the requested 3D texture format cannot be expressed in this GL dialect");
	}
	if (op.width == 0 || op.height == 0 || op.depth == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a 3D texture needs non-zero extents");
	}
	if (config_.maxTextureSize != 0
	    && (op.width > config_.maxTextureSize || op.height > config_.maxTextureSize
		|| op.depth > config_.maxTextureSize)) {
		return failUnsupported("the texture exceeds the configured maximum texture size");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "texture handle is zero or already in use");
	}
	GlTextureObject object;
	object.target = GL_TEXTURE_3D;
	object.internalFormat = translated->internalFormat;
	object.format = op.format;
	object.width = op.width;
	object.height = op.height;
	object.depth = op.depth;
	if (op.generateMipmaps) {
		std::uint32_t extent = std::max({ op.width, op.height, op.depth });
		object.levels = 1;
		while (extent > 1) {
			extent >>= 1;
			++object.levels;
		}
	}
	glGenTextures(1, &object.name);
	if (object.name == 0) {
		return fail(Kld::HalStatusCode::AllocationFailed, "glGenTextures returned no name");
	}
	applyDefaultTextureParameters(object);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (std::uint8_t level = 0; level < object.levels; ++level) {
		glTexImage3D(GL_TEXTURE_3D, level, static_cast<GLint>(object.internalFormat),
			     static_cast<GLsizei>(mipExtent(op.width, level)),
			     static_cast<GLsizei>(mipExtent(op.height, level)),
			     static_cast<GLsizei>(mipExtent(op.depth, level)), 0,
			     translated->format, translated->type, level == 0 ? op.pixels : nullptr);
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	if (op.generateMipmaps && object.levels > 1) {
		glGenerateMipmap(GL_TEXTURE_3D);
	}
	textures_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createTextureCube(const Kld::OpCreateTextureCube& op)
{
	const std::optional<TextureFormat> translated = translateTextureFormat(op.format);
	if (!translated.has_value() || translated->compressed) {
		return failUnsupported("the requested cube-map format cannot be expressed in this GL dialect");
	}
	if (op.size == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a cube map needs a non-zero face size");
	}
	if (config_.maxTextureSize != 0 && op.size > config_.maxTextureSize) {
		return failUnsupported("the texture exceeds the configured maximum texture size");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "texture handle is zero or already in use");
	}
	GlTextureObject object;
	object.target = GL_TEXTURE_CUBE_MAP;
	object.internalFormat = translated->internalFormat;
	object.format = op.format;
	object.width = op.size;
	object.height = op.size;
	object.layers = 6;
	object.levels = op.mipLevels > 0 ? op.mipLevels : 1;
	glGenTextures(1, &object.name);
	if (object.name == 0) {
		return fail(Kld::HalStatusCode::AllocationFailed, "glGenTextures returned no name");
	}
	applyDefaultTextureParameters(object);
	for (std::uint8_t level = 0; level < object.levels; ++level) {
		const GLsizei extent = static_cast<GLsizei>(mipExtent(op.size, level));
		for (std::uint32_t face = 0; face < 6; ++face) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level,
				     static_cast<GLint>(object.internalFormat), extent, extent, 0,
				     translated->format, translated->type, nullptr);
		}
	}
	if (op.generateMipmaps && object.levels > 1) {
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	}
	textures_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createTexture2DArray(const Kld::OpCreateTexture2DArray& op)
{
	const std::optional<TextureFormat> translated = translateTextureFormat(op.format);
	if (!translated.has_value() || translated->compressed) {
		return failUnsupported("the requested array format cannot be expressed in this GL dialect");
	}
	if (op.width == 0 || op.height == 0 || op.layers == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a texture array needs extents and layers");
	}
	if (config_.maxTextureSize != 0
	    && (op.width > config_.maxTextureSize || op.height > config_.maxTextureSize)) {
		return failUnsupported("the texture exceeds the configured maximum texture size");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "texture handle is zero or already in use");
	}
	GlTextureObject object;
	object.target = GL_TEXTURE_2D_ARRAY;
	object.internalFormat = translated->internalFormat;
	object.format = op.format;
	object.width = op.width;
	object.height = op.height;
	object.depth = 1;
	object.layers = op.layers;
	object.levels = op.mipLevels > 0 ? op.mipLevels : 1;
	glGenTextures(1, &object.name);
	if (object.name == 0) {
		return fail(Kld::HalStatusCode::AllocationFailed, "glGenTextures returned no name");
	}
	applyDefaultTextureParameters(object);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (std::uint8_t level = 0; level < object.levels; ++level) {
		glTexImage3D(GL_TEXTURE_2D_ARRAY, level, static_cast<GLint>(object.internalFormat),
			     static_cast<GLsizei>(mipExtent(op.width, level)),
			     static_cast<GLsizei>(mipExtent(op.height, level)),
			     static_cast<GLsizei>(op.layers), 0, translated->format, translated->type,
			     level == 0 ? op.pixels : nullptr);
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	if (op.generateMipmaps && object.levels > 1) {
		glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
	}
	textures_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::destroyTexture(const Kld::OpDestroy& op)
{
	const auto found = textures_.find(op.id);
	if (found == textures_.end()) {
		return failInvalidHandle("texture", op.id);
	}
	glDeleteTextures(1, &found->second.name);
	textures_.erase(found);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::resizeTexture2D(const Kld::OpResizeTexture2D& op)
{
	GlTextureObject* object = texture(op.id);
	if (object == nullptr) {
		return failInvalidHandle("texture", op.id);
	}
	const std::optional<TextureFormat> translated = translateTextureFormat(op.format);
	if (!translated.has_value() || translated->compressed) {
		return failUnsupported("the requested resize format cannot be expressed in this GL dialect");
	}
	if (op.width == 0 || op.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a 2D texture needs non-zero extents");
	}
	if (object->target != GL_TEXTURE_2D) {
		return fail(Kld::HalStatusCode::InvalidCommand, "ResizeTexture2D requires a 2D texture");
	}
	object->internalFormat = translated->internalFormat;
	object->format = op.format;
	object->width = op.width;
	object->height = op.height;
	object->levels = op.mipLevels > 0 ? op.mipLevels : 1;
	glBindTexture(GL_TEXTURE_2D, object->name);
	for (std::uint8_t level = 0; level < object->levels; ++level) {
		glTexImage2D(GL_TEXTURE_2D, level, static_cast<GLint>(object->internalFormat),
			     static_cast<GLsizei>(mipExtent(op.width, level)),
			     static_cast<GLsizei>(mipExtent(op.height, level)), 0,
			     translated->format, translated->type, nullptr);
	}
	applyDefaultTextureParameters(*object);
	if (op.generateMipmaps && object->levels > 1) {
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::updateTexture2D(const Kld::OpUpdateTexture2D& op)
{
	GlTextureObject* object = texture(op.id);
	if (object == nullptr) {
		return failInvalidHandle("texture", op.id);
	}
	if (object->target != GL_TEXTURE_2D) {
		return fail(Kld::HalStatusCode::InvalidCommand, "UpdateTexture2D requires a 2D texture");
	}
	const std::optional<TextureFormat> translated = translateTextureFormat(op.format);
	if (!translated.has_value() || translated->compressed) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the update format is unsupported; use the compressed update for block data");
	}
	if (op.pixels == nullptr) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the texture update has no pixel data");
	}
	glBindTexture(GL_TEXTURE_2D, object->name);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, op.mipLevel, op.x, op.y, op.width, op.height,
			translated->format, translated->type, op.pixels);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::updateCompressedTexture2D(
	const Kld::OpUpdateCompressedTexture2D& op)
{
	GlTextureObject* object = texture(op.id);
	if (object == nullptr) {
		return failInvalidHandle("texture", op.id);
	}
	if (object->target != GL_TEXTURE_2D || !object->compressed) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "UpdateCompressedTexture2D requires a block-compressed 2D texture");
	}
	if (op.data == nullptr || op.byteSize == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the compressed update has no payload");
	}
	glBindTexture(GL_TEXTURE_2D, object->name);
	glCompressedTexSubImage2D(GL_TEXTURE_2D, op.mipLevel, op.x, op.y, op.width, op.height,
				  object->internalFormat, static_cast<GLsizei>(op.byteSize), op.data);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::updateTextureCubeFace(const Kld::OpUpdateTextureCubeFace& op)
{
	GlTextureObject* object = texture(op.id);
	if (object == nullptr) {
		return failInvalidHandle("texture", op.id);
	}
	if (object->target != GL_TEXTURE_CUBE_MAP) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "UpdateTextureCubeFace requires a cube texture");
	}
	const std::optional<TextureFormat> translated = translateTextureFormat(op.format);
	if (!translated.has_value() || translated->compressed || op.pixels == nullptr) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the cube face update needs an uncompressed format and pixel data");
	}
	glBindTexture(GL_TEXTURE_CUBE_MAP, object->name);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(detail::cubeFaceTarget(op.face), op.mipLevel, op.x, op.y, op.width, op.height,
			translated->format, translated->type, op.pixels);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::updateTexture2DArray(const Kld::OpUpdateTexture2DArray& op)
{
	GlTextureObject* object = texture(op.id);
	if (object == nullptr) {
		return failInvalidHandle("texture", op.id);
	}
	if (object->target != GL_TEXTURE_2D_ARRAY) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "UpdateTexture2DArray requires a 2D array texture");
	}
	const std::optional<TextureFormat> translated = translateTextureFormat(op.format);
	if (!translated.has_value() || translated->compressed || op.pixels == nullptr) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the array update needs an uncompressed format and pixel data");
	}
	if (op.layer >= object->layers) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the array layer is out of range");
	}
	glBindTexture(GL_TEXTURE_2D_ARRAY, object->name);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, op.mipLevel, op.x, op.y, op.layer, op.width, op.height, 1,
			translated->format, translated->type, op.pixels);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::generateTextureMipmaps(const Kld::OpGenerateTextureMipmaps& op)
{
	GlTextureObject* object = texture(op.id);
	if (object == nullptr) {
		return failInvalidHandle("texture", op.id);
	}
	glBindTexture(object->target, object->name);
	glGenerateMipmap(object->target);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createSampler(const Kld::OpCreateSampler& op)
{
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "sampler handle is zero or already in use");
	}
	Kld::SamplerFiltering filtering = op.filtering;
	if (config_.forceNearestFiltering) {
		filtering = Kld::SamplerFiltering::NEAREST;
	} else if (config_.forceTrilinear && detail::isMipmapFiltering(filtering)) {
		filtering = Kld::SamplerFiltering::LINEAR_MIPMAP_LINEAR;
	}
	float anisotropy = op.maxAnisotropy;
	if (config_.maxAnisotropy > 0.0f && anisotropy > config_.maxAnisotropy) {
		anisotropy = config_.maxAnisotropy;
	}
	if (anisotropy > 1.0f && !anisotropyEnabled_) {
		return failUnsupported("anisotropic filtering was requested but GL_EXT_texture_filter_anisotropic is missing");
	}
#if KALDIGL_IS_GLES
	if (op.lodBias + config_.textureLodBias != 0.0f) {
		return failUnsupported("OpenGL ES has no per-sampler LOD bias");
	}
#endif
	GlSamplerObject object;
	glGenSamplers(1, &object.name);
	if (object.name == 0) {
		return fail(Kld::HalStatusCode::AllocationFailed, "glGenSamplers returned no name");
	}
	glSamplerParameteri(object.name, GL_TEXTURE_WRAP_S, static_cast<GLint>(detail::samplerWrap(op.wrapping[0])));
	glSamplerParameteri(object.name, GL_TEXTURE_WRAP_T, static_cast<GLint>(detail::samplerWrap(op.wrapping[1])));
	glSamplerParameteri(object.name, GL_TEXTURE_WRAP_R, static_cast<GLint>(detail::samplerWrap(op.wrapping[2])));
	glSamplerParameteri(object.name, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(detail::minFilter(filtering)));
	glSamplerParameteri(object.name, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(detail::magFilter(filtering)));
	glSamplerParameterf(object.name, GL_TEXTURE_MIN_LOD, op.minLod);
	glSamplerParameterf(object.name, GL_TEXTURE_MAX_LOD, op.maxLod);
#if !KALDIGL_IS_GLES
	glSamplerParameterf(object.name, GL_TEXTURE_LOD_BIAS, op.lodBias + config_.textureLodBias);
#endif
	if (anisotropy > 1.0f) {
		glSamplerParameterf(object.name, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}
	samplers_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::destroySampler(const Kld::OpDestroy& op)
{
	const auto found = samplers_.find(op.id);
	if (found == samplers_.end()) {
		return failInvalidHandle("sampler", op.id);
	}
	glDeleteSamplers(1, &found->second.name);
	samplers_.erase(found);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createImage2D(const Kld::OpCreateImage2D& op)
{
	const std::optional<TextureFormat> translated = translateTextureFormat(op.format);
	if (!translated.has_value() || translated->compressed) {
		return failUnsupported("the requested image format cannot be used as an attachment");
	}
	if (op.width == 0 || op.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a 2D image needs non-zero extents");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "image handle is zero or already in use");
	}
	GlTextureObject object;
	object.target = GL_TEXTURE_2D;
	object.internalFormat = translated->internalFormat;
	object.format = op.format;
	object.width = op.width;
	object.height = op.height;
	object.levels = op.mipLevels > 0 ? op.mipLevels : 1;
	glGenTextures(1, &object.name);
	if (object.name == 0) {
		return fail(Kld::HalStatusCode::AllocationFailed, "glGenTextures returned no name");
	}
	applyDefaultTextureParameters(object);
	for (std::uint8_t level = 0; level < object.levels; ++level) {
		glTexImage2D(GL_TEXTURE_2D, level, static_cast<GLint>(object.internalFormat),
			     static_cast<GLsizei>(mipExtent(op.width, level)),
			     static_cast<GLsizei>(mipExtent(op.height, level)), 0,
			     translated->format, translated->type, nullptr);
	}
	images_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::resizeImage2D(const Kld::OpResizeImage2D& op)
{
	GlTextureObject* object = image(op.id);
	if (object == nullptr) {
		return failInvalidHandle("image", op.id);
	}
	if (object->multisampled) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "a multisampled image cannot be resized in place; destroy and recreate it");
	}
	const std::optional<TextureFormat> translated = translateTextureFormat(op.format);
	if (!translated.has_value() || translated->compressed) {
		return failUnsupported("the requested resize format cannot be used as an attachment");
	}
	if (op.width == 0 || op.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a 2D image needs non-zero extents");
	}
	object->internalFormat = translated->internalFormat;
	object->format = op.format;
	object->width = op.width;
	object->height = op.height;
	object->levels = op.mipLevels > 0 ? op.mipLevels : 1;
	glBindTexture(GL_TEXTURE_2D, object->name);
	for (std::uint8_t level = 0; level < object->levels; ++level) {
		glTexImage2D(GL_TEXTURE_2D, level, static_cast<GLint>(object->internalFormat),
			     static_cast<GLsizei>(mipExtent(op.width, level)),
			     static_cast<GLsizei>(mipExtent(op.height, level)), 0,
			     translated->format, translated->type, nullptr);
	}
	applyDefaultTextureParameters(*object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createMultisampledImage2D(
	const Kld::OpCreateMultisampledImage2D& op)
{
	const std::optional<TextureFormat> translated = translateTextureFormat(op.format);
	if (!translated.has_value() || translated->compressed) {
		return failUnsupported("the requested format cannot be used as a multisample attachment");
	}
	if (op.width == 0 || op.height == 0 || op.samples < 2) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "a multisampled image needs extents and at least two samples");
	}
	if (capabilities_.maxSamples != 0 && op.samples > capabilities_.maxSamples) {
		return failUnsupported("the requested sample count exceeds the probed limit");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "image handle is zero or already in use");
	}
	GlTextureObject object;
	object.target = GL_TEXTURE_2D_MULTISAMPLE;
	object.internalFormat = translated->internalFormat;
	object.format = op.format;
	object.width = op.width;
	object.height = op.height;
	object.levels = 1;
	object.multisampled = true;
	glGenTextures(1, &object.name);
	if (object.name == 0) {
		return fail(Kld::HalStatusCode::AllocationFailed, "glGenTextures returned no name");
	}
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, object.name);
	glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, op.samples, object.internalFormat, op.width,
				  op.height, GL_TRUE);
	images_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::updateImage2D(const Kld::OpUpdateImage2D& op)
{
	GlTextureObject* object = image(op.id);
	if (object == nullptr) {
		return failInvalidHandle("image", op.id);
	}
	if (object->multisampled) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "a multisampled image cannot be updated with texel data");
	}
	const std::optional<TextureFormat> translated = translateTextureFormat(op.format);
	if (!translated.has_value() || translated->compressed || op.pixels == nullptr) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the image update needs an uncompressed format and pixel data");
	}
	glBindTexture(GL_TEXTURE_2D, object->name);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, op.mipLevel, op.x, op.y, op.width, op.height, translated->format,
			translated->type, op.pixels);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::destroyImage(const Kld::OpDestroyImage& op)
{
	const auto found = images_.find(op.id);
	if (found == images_.end()) {
		return failInvalidHandle("image", op.id);
	}
	glDeleteTextures(1, &found->second.name);
	images_.erase(found);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::attachImage(const GlTextureObject& object, GLenum framebufferTarget,
					    GLenum attachment, std::uint8_t mipLevel,
					    std::uint16_t layer)
{
	if (object.target == GL_TEXTURE_CUBE_MAP) {
		const GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + std::min<std::uint16_t>(layer, 5);
		glFramebufferTexture2D(framebufferTarget, attachment, face, object.name, mipLevel);
		return Kld::HalOk;
	}
	if (object.target == GL_TEXTURE_2D || object.target == GL_TEXTURE_2D_MULTISAMPLE) {
		glFramebufferTexture2D(framebufferTarget, attachment, object.target, object.name, mipLevel);
		return Kld::HalOk;
	}
	glFramebufferTextureLayer(framebufferTarget, attachment, object.name, mipLevel, layer);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createFramebuffer(const Kld::OpCreateFramebuffer& op)
{
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "framebuffer handle is zero or already in use");
	}
	if (op.attachmentCount != 0 && op.attachments == nullptr) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the attachment array is missing");
	}
	GlFramebufferObject object;
	object.width = op.width;
	object.height = op.height;
	glGenFramebuffers(1, &object.name);
	if (object.name == 0) {
		return fail(Kld::HalStatusCode::AllocationFailed, "glGenFramebuffers returned no name");
	}
	const Kld::HandleId previousFramebuffer = boundFramebuffer_;
	glBindFramebuffer(GL_FRAMEBUFFER, object.name);
	std::vector<GLenum> drawBuffers;
	for (std::uint32_t index = 0; index < op.attachmentCount; ++index) {
		const Kld::FramebufferAttachment& attachment = op.attachments[index];
		GlTextureObject* target = image(attachment.image);
		if (target == nullptr) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteFramebuffers(1, &object.name);
			boundFramebuffer_ = Kld::DefaultFramebuffer;
			return failInvalidHandle("image", attachment.image);
		}
		(void)attachImage(*target, GL_FRAMEBUFFER,
				  detail::framebufferAttachment(attachment.role,
								attachment.colorAttachmentIndex),
				  0, 0);
		object.attachments.push_back(GlFramebufferAttachment {
			attachment.image, attachment.role, attachment.colorAttachmentIndex });
		if (attachment.role == Kld::FramebufferAttachmentRole::Color) {
			drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + attachment.colorAttachmentIndex);
		}
	}
	if (drawBuffers.empty()) {
		const GLenum none = GL_NONE;
		glDrawBuffers(1, &none);
	} else {
		glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
	}
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &object.name);
		boundFramebuffer_ = Kld::DefaultFramebuffer;
		return fail(Kld::HalStatusCode::BackendError,
			    "glCheckFramebufferStatus reported 0x" + std::to_string(status));
	}
	framebuffers_.emplace(op.id, object);
	return bindFramebuffer(Kld::OpBindFramebuffer { previousFramebuffer });
}

Kld::HalStatus CommandExecutor::destroyFramebuffer(const Kld::OpDestroyFramebuffer& op)
{
	const auto found = framebuffers_.find(op.id);
	if (found == framebuffers_.end()) {
		return failInvalidHandle("framebuffer", op.id);
	}
	if (boundFramebuffer_ == op.id) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		boundFramebuffer_ = Kld::DefaultFramebuffer;
	}
	glDeleteFramebuffers(1, &found->second.name);
	framebuffers_.erase(found);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::blit2D(const GlTextureObject& source, std::uint16_t sourceX,
				       std::uint16_t sourceY, const GlTextureObject& destination,
				       std::uint16_t destinationX, std::uint16_t destinationY,
				       std::uint16_t width, std::uint16_t height)
{
	GLbitfield mask = GL_COLOR_BUFFER_BIT;
	GLenum readAttachment = GL_COLOR_ATTACHMENT0;
	if (detail::isDepthStencilFormat(source.format)) {
		if (detail::isStencilOnlyFormat(source.format)) {
			mask = GL_STENCIL_BUFFER_BIT;
			readAttachment = GL_STENCIL_ATTACHMENT;
		} else if (source.format == Kld::TextureFormat::DEPTH24_STENCIL8
			   || source.format == Kld::TextureFormat::DEPTH32F_STENCIL8) {
			mask = GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
			readAttachment = GL_DEPTH_STENCIL_ATTACHMENT;
		} else {
			mask = GL_DEPTH_BUFFER_BIT;
			readAttachment = GL_DEPTH_ATTACHMENT;
		}
	}
	// Fast path: identical single-sample 2D layouts support a direct copy.
	if (!source.multisampled && !destination.multisampled && source.target == GL_TEXTURE_2D
	    && destination.target == GL_TEXTURE_2D && source.format == destination.format) {
		glCopyImageSubData(source.name, GL_TEXTURE_2D, 0, sourceX, sourceY, 0, destination.name,
				   GL_TEXTURE_2D, 0, destinationX, destinationY, 0, width, height, 1);
		return Kld::HalOk;
	}
	// General path (including multisample resolves): attach both sides to the
	// scratch framebuffers and blit.
	glBindFramebuffer(GL_READ_FRAMEBUFFER, scratchReadFramebuffer_);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, readAttachment, source.target, source.name, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, scratchDrawFramebuffer_);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, readAttachment, GL_TEXTURE_2D, destination.name, 0);
	const bool complete = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE
		&& glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	if (complete) {
		glBlitFramebuffer(sourceX, sourceY, sourceX + width, sourceY + height, destinationX,
				  destinationY, destinationX + width, destinationY + height, mask,
				  GL_NEAREST);
	}
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, readAttachment, source.target, 0, 0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, readAttachment, GL_TEXTURE_2D, 0, 0);
	if (boundFramebuffer_ == Kld::DefaultFramebuffer) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	} else {
		GlFramebufferObject* fb = framebuffer(boundFramebuffer_);
		glBindFramebuffer(GL_FRAMEBUFFER, fb != nullptr ? fb->name : 0);
	}
	if (!complete) {
		return failUnsupported("blit requires color-, depth- or stencil-renderable formats");
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::blitTexture2D(const Kld::OpBlitTexture2D& op)
{
	GlTextureObject* source = texture(op.source);
	GlTextureObject* destination = texture(op.destination);
	if (source == nullptr) {
		return failInvalidHandle("texture", op.source);
	}
	if (destination == nullptr) {
		return failInvalidHandle("texture", op.destination);
	}
	if (op.width == 0 || op.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a blit needs a non-zero extent");
	}
	return blit2D(*source, op.sourceX, op.sourceY, *destination, op.destinationX, op.destinationY,
		      op.width, op.height);
}

Kld::HalStatus CommandExecutor::blitImage2D(const Kld::OpBlitImage2D& op)
{
	GlTextureObject* source = image(op.source);
	GlTextureObject* destination = image(op.destination);
	if (source == nullptr) {
		return failInvalidHandle("image", op.source);
	}
	if (destination == nullptr) {
		return failInvalidHandle("image", op.destination);
	}
	if (op.width == 0 || op.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a blit needs a non-zero extent");
	}
	return blit2D(*source, op.sourceX, op.sourceY, *destination, op.destinationX, op.destinationY,
		      op.width, op.height);
}

Kld::HalStatus CommandExecutor::resolveImage2D(const Kld::OpResolveImage2D& op)
{
	GlTextureObject* source = image(op.source);
	GlTextureObject* destination = image(op.destination);
	if (source == nullptr) {
		return failInvalidHandle("image", op.source);
	}
	if (destination == nullptr) {
		return failInvalidHandle("image", op.destination);
	}
	if (op.width == 0 || op.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a resolve needs a non-zero extent");
	}
	return blit2D(*source, op.sourceX, op.sourceY, *destination, op.destinationX, op.destinationY,
		      op.width, op.height);
}

}
