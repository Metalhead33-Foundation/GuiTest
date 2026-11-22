#ifndef KALDIGLEXECUTORMAPPING_HPP
#define KALDIGLEXECUTORMAPPING_HPP
/**
 * @file GlExecutorMapping.hpp
 * @brief Pure Kaldi -> OpenGL / OpenGL ES enumerator translation for the executor.
 *
 * These are the table lookups `GlCommandExecutor` needs in several translation
 * units, kept out of the class and out of the public headers. Every function is
 * `inline`, total (an out-of-range enum yields a neutral GL value) and free of GL
 * calls, so the tables stay testable and the handlers stay readable.
 */
#include "GlBackend.hpp"
#if KALDIGL_IS_GLES
#include <glad/gles2.h>
#else
#include <glad/gl.h>
#endif
#include <Kaldi/KaldiOperation.hpp>
#include <cstdint>

namespace KALDIGL_NS {
namespace detail {

/** @brief GL enum for a Kaldi blend factor. */
inline GLenum blendFactor(Kld::BlendFactor factor) noexcept
{
	switch (factor) {
		case Kld::BlendFactor::Zero: return GL_ZERO;
		case Kld::BlendFactor::One: return GL_ONE;
		case Kld::BlendFactor::SrcColor: return GL_SRC_COLOR;
		case Kld::BlendFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
		case Kld::BlendFactor::DstColor: return GL_DST_COLOR;
		case Kld::BlendFactor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
		case Kld::BlendFactor::SrcAlpha: return GL_SRC_ALPHA;
		case Kld::BlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
		case Kld::BlendFactor::DstAlpha: return GL_DST_ALPHA;
		case Kld::BlendFactor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
	}
	return GL_ONE;
}

/** @brief GL enum for a Kaldi blend operation. */
inline GLenum blendOp(Kld::BlendOp operation) noexcept
{
	switch (operation) {
		case Kld::BlendOp::Add: return GL_FUNC_ADD;
		case Kld::BlendOp::Subtract: return GL_FUNC_SUBTRACT;
		case Kld::BlendOp::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
		case Kld::BlendOp::Min: return GL_MIN;
		case Kld::BlendOp::Max: return GL_MAX;
	}
	return GL_FUNC_ADD;
}

/** @brief GL enum for a Kaldi depth/stencil comparison. */
inline GLenum compareOp(Kld::CompareOp operation) noexcept
{
	switch (operation) {
		case Kld::CompareOp::Never: return GL_NEVER;
		case Kld::CompareOp::Less: return GL_LESS;
		case Kld::CompareOp::Equal: return GL_EQUAL;
		case Kld::CompareOp::LessOrEqual: return GL_LEQUAL;
		case Kld::CompareOp::Greater: return GL_GREATER;
		case Kld::CompareOp::NotEqual: return GL_NOTEQUAL;
		case Kld::CompareOp::GreaterOrEqual: return GL_GEQUAL;
		case Kld::CompareOp::AlwaysPass: return GL_ALWAYS;
	}
	return GL_ALWAYS;
}

/** @brief GL enum for a Kaldi stencil operation. */
inline GLenum stencilOp(Kld::StencilOp operation) noexcept
{
	switch (operation) {
		case Kld::StencilOp::Keep: return GL_KEEP;
		case Kld::StencilOp::Zero: return GL_ZERO;
		case Kld::StencilOp::Replace: return GL_REPLACE;
		case Kld::StencilOp::IncrementClamp: return GL_INCR;
		case Kld::StencilOp::DecrementClamp: return GL_DECR;
		case Kld::StencilOp::Invert: return GL_INVERT;
		case Kld::StencilOp::IncrementWrap: return GL_INCR_WRAP;
		case Kld::StencilOp::DecrementWrap: return GL_DECR_WRAP;
	}
	return GL_KEEP;
}

/** @brief GL face selector for a Kaldi culling mode; @c 0 means "disable culling". */
inline GLenum cullFace(Kld::CullMode mode) noexcept
{
	switch (mode) {
		case Kld::CullMode::NoCulling: return 0;
		case Kld::CullMode::Front: return GL_FRONT;
		case Kld::CullMode::Back: return GL_BACK;
		case Kld::CullMode::FrontAndBack: return GL_FRONT_AND_BACK;
	}
	return 0;
}

/** @brief GL winding for a Kaldi front-face rule. */
inline GLenum frontFace(Kld::FrontFace face) noexcept
{
	return face == Kld::FrontFace::Clockwise ? GL_CW : GL_CCW;
}

/** @brief GL wrap mode for a Kaldi sampler wrapping mode. */
inline GLenum samplerWrap(Kld::SamplerWrapping wrapping) noexcept
{
	switch (wrapping) {
		case Kld::SamplerWrapping::CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
		case Kld::SamplerWrapping::CLAMP_TO_BORDER: return GL_CLAMP_TO_BORDER;
		case Kld::SamplerWrapping::MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
		case Kld::SamplerWrapping::REPEAT: return GL_REPEAT;
	}
	return GL_REPEAT;
}

/** @brief GL minification filter for a Kaldi filtering mode. */
inline GLenum minFilter(Kld::SamplerFiltering filtering) noexcept
{
	switch (filtering) {
		case Kld::SamplerFiltering::NEAREST: return GL_NEAREST;
		case Kld::SamplerFiltering::LINEAR: return GL_LINEAR;
		case Kld::SamplerFiltering::NEAREST_MIPMAP_NEAREST: return GL_NEAREST_MIPMAP_NEAREST;
		case Kld::SamplerFiltering::LINEAR_MIPMAP_NEAREST: return GL_LINEAR_MIPMAP_NEAREST;
		case Kld::SamplerFiltering::NEAREST_MIPMAP_LINEAR: return GL_NEAREST_MIPMAP_LINEAR;
		case Kld::SamplerFiltering::LINEAR_MIPMAP_LINEAR: return GL_LINEAR_MIPMAP_LINEAR;
	}
	return GL_LINEAR;
}

/** @brief True when @p filtering samples a mip chain. */
inline bool isMipmapFiltering(Kld::SamplerFiltering filtering) noexcept
{
	return filtering == Kld::SamplerFiltering::NEAREST_MIPMAP_NEAREST
		|| filtering == Kld::SamplerFiltering::LINEAR_MIPMAP_NEAREST
		|| filtering == Kld::SamplerFiltering::NEAREST_MIPMAP_LINEAR
		|| filtering == Kld::SamplerFiltering::LINEAR_MIPMAP_LINEAR;
}

/** @brief GL magnification filter for a Kaldi filtering mode. */
inline GLenum magFilter(Kld::SamplerFiltering filtering) noexcept
{
	return filtering == Kld::SamplerFiltering::NEAREST
			|| filtering == Kld::SamplerFiltering::NEAREST_MIPMAP_NEAREST
			|| filtering == Kld::SamplerFiltering::NEAREST_MIPMAP_LINEAR
		? GL_NEAREST
		: GL_LINEAR;
}

/** @brief GL index element type for a Kaldi index type. */
inline GLenum indexType(Kld::IndexType type) noexcept
{
	return type == Kld::IndexType::Uint32 ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
}

/** @brief GL texture target for a Kaldi sampled-texture binding kind. */
inline GLenum textureTarget(Kld::TextureBindingType type) noexcept
{
	switch (type) {
#if !KALDIGL_IS_GLES
		case Kld::TextureBindingType::Texture1D: return GL_TEXTURE_1D;
#else
		case Kld::TextureBindingType::Texture1D: return 0;
#endif
		case Kld::TextureBindingType::Texture2D: return GL_TEXTURE_2D;
		case Kld::TextureBindingType::Texture3D: return GL_TEXTURE_3D;
		case Kld::TextureBindingType::ArrayTexture2D: return GL_TEXTURE_2D_ARRAY;
		case Kld::TextureBindingType::TextureCube: return GL_TEXTURE_CUBE_MAP;
	}
	return GL_TEXTURE_2D;
}

/** @brief GL access mode for a Kaldi storage binding. */
inline GLenum storageAccess(Kld::StorageAccess access) noexcept
{
	switch (access) {
		case Kld::StorageAccess::ReadOnly: return GL_READ_ONLY;
		case Kld::StorageAccess::WriteOnly: return GL_WRITE_ONLY;
		case Kld::StorageAccess::ReadWrite: return GL_READ_WRITE;
	}
	return GL_READ_ONLY;
}

/** @brief OpenGL face index (`0..5`) for a Kaldi cube face. */
inline std::uint32_t cubeFaceIndex(Kld::TextureCubeFace face) noexcept
{
	switch (face) {
		case Kld::TextureCubeFace::PositiveX: return 0;
		case Kld::TextureCubeFace::NegativeX: return 1;
		case Kld::TextureCubeFace::PositiveY: return 2;
		case Kld::TextureCubeFace::NegativeY: return 3;
		case Kld::TextureCubeFace::PositiveZ: return 4;
		case Kld::TextureCubeFace::NegativeZ: return 5;
	}
	return 0;
}

/** @brief GL cube-map face target for a Kaldi cube face. */
inline GLenum cubeFaceTarget(Kld::TextureCubeFace face) noexcept
{
	return GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubeFaceIndex(face);
}

/** @brief GL attachment point for a framebuffer role and color index. */
inline GLenum framebufferAttachment(Kld::FramebufferAttachmentRole role, std::uint32_t index) noexcept
{
	switch (role) {
		case Kld::FramebufferAttachmentRole::Color: return GL_COLOR_ATTACHMENT0 + index;
		case Kld::FramebufferAttachmentRole::Depth: return GL_DEPTH_ATTACHMENT;
		case Kld::FramebufferAttachmentRole::Stencil: return GL_STENCIL_ATTACHMENT;
		case Kld::FramebufferAttachmentRole::DepthStencil: return GL_DEPTH_STENCIL_ATTACHMENT;
	}
	return GL_COLOR_ATTACHMENT0;
}

/** @brief GL buffer target that best matches a `Kld::BufferUsage` bitmask. */
inline GLenum bufferTarget(std::uint32_t policy) noexcept
{
	if ((policy & Kld::BUFFER_USAGE_VERTEX) != 0) return GL_ARRAY_BUFFER;
	if ((policy & Kld::BUFFER_USAGE_INDEX) != 0) return GL_ELEMENT_ARRAY_BUFFER;
	if ((policy & Kld::BUFFER_USAGE_UNIFORM) != 0) return GL_UNIFORM_BUFFER;
	if ((policy & Kld::BUFFER_USAGE_STORAGE) != 0) return GL_SHADER_STORAGE_BUFFER;
	if ((policy & Kld::BUFFER_USAGE_INDIRECT) != 0) return GL_DRAW_INDIRECT_BUFFER;
	if ((policy & Kld::BUFFER_USAGE_READBACK) != 0) return GL_COPY_READ_BUFFER;
	if ((policy & Kld::BUFFER_USAGE_TRANSFER_SRC) != 0) return GL_COPY_READ_BUFFER;
	if ((policy & Kld::BUFFER_USAGE_TRANSFER_DST) != 0) return GL_COPY_WRITE_BUFFER;
	return GL_ARRAY_BUFFER;
}

/** @brief GL shader stage for a Kaldi shader stage. */
inline GLenum shaderStage(Kld::ShaderStage stage) noexcept
{
	switch (stage) {
		case Kld::ShaderStage::Vertex: return GL_VERTEX_SHADER;
		case Kld::ShaderStage::Fragment: return GL_FRAGMENT_SHADER;
		case Kld::ShaderStage::Geometry: return GL_GEOMETRY_SHADER;
		case Kld::ShaderStage::TessControl: return GL_TESS_CONTROL_SHADER;
		case Kld::ShaderStage::TessEvaluation: return GL_TESS_EVALUATION_SHADER;
		case Kld::ShaderStage::Compute: return GL_COMPUTE_SHADER;
	}
	return GL_VERTEX_SHADER;
}

/** @brief True when @p format is a depth or stencil Euphemy format. */
inline bool isDepthStencilFormat(Kld::TextureFormat format) noexcept
{
	switch (format) {
		case Kld::TextureFormat::DEPTH_COMPONENT16:
		case Kld::TextureFormat::DEPTH_COMPONENT24:
		case Kld::TextureFormat::DEPTH_COMPONENT32:
		case Kld::TextureFormat::DEPTH_COMPONENT32F:
		case Kld::TextureFormat::DEPTH24_STENCIL8:
		case Kld::TextureFormat::DEPTH32F_STENCIL8:
		case Kld::TextureFormat::STENCIL_INDEX8:
			return true;
		default:
			return false;
	}
}

/** @brief True when @p format is a stencil-only Euphemy format. */
inline bool isStencilOnlyFormat(Kld::TextureFormat format) noexcept
{
	return format == Kld::TextureFormat::STENCIL_INDEX8;
}

/** @brief Byte size of one client pixel described by @p format and @p type. */
inline std::uint32_t clientPixelSize(GLenum format, GLenum type) noexcept
{
	std::uint32_t components = 1;
	switch (format) {
		case GL_RG: components = 2; break;
		case GL_RGB: components = 3; break;
		case GL_RGBA: components = 4; break;
		default: components = 1; break;
	}
	switch (type) {
		case GL_UNSIGNED_INT_24_8: return 4u;
		case GL_UNSIGNED_SHORT_5_6_5:
		case GL_UNSIGNED_SHORT_5_5_5_1:
		case GL_UNSIGNED_SHORT_4_4_4_4: return 2u;
		case GL_UNSIGNED_BYTE:
		case GL_BYTE: return components;
		case GL_UNSIGNED_SHORT:
		case GL_SHORT:
		case GL_HALF_FLOAT: return components * 2u;
		default: return components * 4u;
	}
}

/**
 * @brief Conservative `glMemoryBarrier` bits for a coarse stage/access pair.
 *
 * OpenGL has no explicit resource states, so a Kaldi transition becomes a memory
 * barrier covering every way the destination stage might read the resource. An
 * empty or unrecognised request still synchronises fully (`GL_ALL_BARRIER_BITS`):
 * over-synchronising is always correct, under-synchronising is not.
 */
inline GLbitfield memoryBarrierBits(std::uint32_t stages, std::uint32_t access) noexcept
{
	GLbitfield bits = 0;
	if ((stages & (Kld::PIPELINE_STAGE_TRANSFER | Kld::PIPELINE_STAGE_BOTTOM_OF_PIPE)) != 0) {
		bits |= GL_BUFFER_UPDATE_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT;
	}
	if ((stages & Kld::PIPELINE_STAGE_VERTEX_INPUT) != 0) {
		bits |= GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
	}
	if ((stages & (Kld::PIPELINE_STAGE_VERTEX_SHADER | Kld::PIPELINE_STAGE_FRAGMENT_SHADER |
		       Kld::PIPELINE_STAGE_COMPUTE_SHADER)) != 0) {
		bits |= GL_UNIFORM_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT |
			GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
	}
	if ((stages & (Kld::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT |
		       Kld::PIPELINE_STAGE_EARLY_FRAGMENT_TESTS |
		       Kld::PIPELINE_STAGE_LATE_FRAGMENT_TESTS)) != 0) {
		bits |= GL_FRAMEBUFFER_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT;
	}
	if ((access & (Kld::ACCESS_HOST_READ | Kld::ACCESS_HOST_WRITE)) != 0) {
		bits |= GL_BUFFER_UPDATE_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT;
	}
	return bits != 0 ? bits : GL_ALL_BARRIER_BITS;
}

}
}

#endif // KALDIGLEXECUTORMAPPING_HPP
