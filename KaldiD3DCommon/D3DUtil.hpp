#ifndef KALDID3DUTIL_HPP
#define KALDID3DUTIL_HPP
/**
 * @file D3DUtil.hpp
 * @brief DXGI format translation shared by the Direct3D 11 and Direct3D 12 backends.
 *
 * Both Direct3D backends describe GPU resources with DXGI formats and describe
 * vertex input with the same `Kld::VertexDescriptor`. The translation tables are
 * therefore identical and live here, in `KaldiD3DCommon`, mirroring what
 * `KaldiGLCommon/GlUtil.hpp` and `KaldiVK/VkUtil.hpp` do for the other APIs.
 *
 * `DXGI_FORMAT_UNKNOWN` is the "cannot express this" answer throughout. Unlike
 * OpenGL, DXGI has very few three-component formats and no SNORM/UNORM
 * single-channel 16-bit variants, so a lot of Euphemy formats are legitimately
 * untranslatable and need a CPU-side conversion first:
 *
 * | Euphemy | Reason |
 * |---|---|
 * | `RGB8U`, `RGB16U`, `RGB16F`, `RGB444`, `RGB555`, `RGB565` | DXGI has no 3x8 or R-high packed layouts |
 * | `ARGB*` | DXGI has no ARGB memory order |
 * | `R16S`, `RGB8S`, `BGR*`, `BGRA16*` | no matching UNORM/SNORM layout |
 * | `STENCIL_INDEX8` | no 8-bit stencil-only format |
 *
 * These headers are only compiled by the Windows-side Direct3D builds
 * (`win32` scope in the `.pro` files); they are never part of a Linux build.
 */
#if __has_include(<directx/dxgiformat.h>)
#include <directx/dxgiformat.h>
#elif __has_include(<dxgiformat.h>)
#include <dxgiformat.h>
#else
#error "KaldiD3DCommon needs the Windows SDK or microsoft/DirectX-Headers on the include path."
#endif
#include <Kaldi/KldVertexFormat.hpp>
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <cstdint>

namespace D3D {

/** @brief Translates a Euphemy image format to its DXGI equivalent. */
DXGI_FORMAT translateToDxgiFormat(Euph::Media::Image::Format format) noexcept;

/** @brief Translates a portable Kaldi vertex attribute format to DXGI. */
DXGI_FORMAT translateToDxgiFormat(Kld::PrimitiveType type) noexcept;

/** @brief Byte size of one vertex attribute element, or @c 0 when unknown. */
uint32_t vertexFormatByteSize(Kld::PrimitiveType type) noexcept;

/**
 * @brief True when the attribute is consumed as an integer by the shader.
 *
 * Direct3D validates the input layout against the shader signature: integer
 * attributes must be declared with `DXGI_FORMAT_*_UINT`/`_SINT` and sampled with
 * an `int`/`uint` input, while unnormalized-integer-as-float attributes use the
 * `_UNORM`/`_SNORM` variants instead.
 */
bool isIntegerVertexFormat(Kld::PrimitiveType type) noexcept;

/** @brief True when the Euphemy format describes a depth or depth/stencil resource. */
bool isDepthFormat(Euph::Media::Image::Format format) noexcept;

/** @brief True when the Euphemy format is a block-compressed (BC1-BC3) format. */
bool isCompressedFormat(Euph::Media::Image::Format format) noexcept;

/** @brief Rounds @p value up to the next multiple of @p alignment. */
constexpr uint64_t alignUp(uint64_t value, uint64_t alignment) noexcept
{
	return alignment == 0 ? value : ((value + alignment - 1) / alignment) * alignment;
}

/** @brief 256-byte alignment required by D3D12 placed/committed texture footprints. */
constexpr uint64_t textureAlignment = 256;

/**
 * @brief Number of mip levels in a full chain that ends at 1x1.
 *
 * Both Direct3D backends have to size resource descriptions themselves because
 * `Kld::OpResizeTexture2D` only carries a mip count, not a chain description.
 */
constexpr uint32_t mipLevelCount(uint32_t width, uint32_t height) noexcept
{
	uint32_t extent = width > height ? width : height;
	uint32_t levels = 1;
	while (extent > 1) {
		extent >>= 1;
		++levels;
	}
	return levels;
}

}

#endif // KALDID3DUTIL_HPP
