/**
 * @file D3DUtil.cpp
 * @brief Implements the DXGI translation tables declared in D3DUtil.hpp.
 */
#include "D3DUtil.hpp"

namespace D3D {

namespace {

using Euph::Media::Image::Format;

/** @brief Returns the attribute element count encoded in a Kaldi primitive type. */
constexpr unsigned elementCount(Kld::PrimitiveType type) noexcept
{
	return static_cast<unsigned>(type) & static_cast<unsigned>(Kld::PrimitiveType::SIZE_MASK);
}

/** @brief Returns the scalar type encoded in a Kaldi primitive type. */
constexpr unsigned scalarType(Kld::PrimitiveType type) noexcept
{
	return static_cast<unsigned>(type) & static_cast<unsigned>(Kld::PrimitiveType::TYPE_MASK);
}

/** @brief Returns the byte size of one scalar of a Kaldi primitive type. */
constexpr unsigned scalarSize(Kld::PrimitiveType type) noexcept
{
	switch (scalarType(type)) {
		case Kld::PrimitiveType::TYPE_HALF_FLOAT: return 2;
		case Kld::PrimitiveType::TYPE_FLOAT: return 4;
		case Kld::PrimitiveType::TYPE_DOUBLE: return 8;
		case Kld::PrimitiveType::TYPE_UBYTE:
		case Kld::PrimitiveType::TYPE_SBYTE: return 1;
		case Kld::PrimitiveType::TYPE_USHORT:
		case Kld::PrimitiveType::TYPE_SSHORT: return 2;
		case Kld::PrimitiveType::TYPE_UINT:
		case Kld::PrimitiveType::TYPE_SINT: return 4;
		default: return 0;
	}
}

/** @brief Selects one of the 1/2/3/4-component formats of a family. */
constexpr DXGI_FORMAT pickFormat(unsigned count, DXGI_FORMAT one, DXGI_FORMAT two, DXGI_FORMAT three, DXGI_FORMAT four) noexcept
{
	switch (count) {
		case 1: return one;
		case 2: return two;
		case 3: return three;
		case 4: return four;
		default: return DXGI_FORMAT_UNKNOWN;
	}
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

bool isDepthFormat(Format format) noexcept
{
	switch (format) {
		case Format::DEPTH_COMPONENT16:
		case Format::DEPTH_COMPONENT24:
		case Format::DEPTH_COMPONENT32:
		case Format::DEPTH_COMPONENT32F:
		case Format::DEPTH24_STENCIL8:
		case Format::DEPTH32F_STENCIL8: return true;
		default: return false;
	}
}

DXGI_FORMAT translateToDxgiFormat(Format format) noexcept
{
	switch (format) {
		// ── Single channel ───────────────────────────────────────────────────
		case Format::R8U: return DXGI_FORMAT_R8_UNORM;
		case Format::R8S: return DXGI_FORMAT_R8_SNORM;
		case Format::R16U: return DXGI_FORMAT_R16_UNORM;
		case Format::R16S: return DXGI_FORMAT_UNKNOWN; // DXGI has no R16_SNORM
		case Format::R16F: return DXGI_FORMAT_R16_FLOAT;
		case Format::R32U: return DXGI_FORMAT_R32_UINT;
		case Format::R32S: return DXGI_FORMAT_R32_SINT;
		case Format::R32F: return DXGI_FORMAT_R32_FLOAT;
		case Format::R64F: return DXGI_FORMAT_UNKNOWN; // no 64-bit DXGI formats
		// ── Two channel ──────────────────────────────────────────────────────
		case Format::RG8U: return DXGI_FORMAT_R8G8_UNORM;
		case Format::RG8S: return DXGI_FORMAT_R8G8_SNORM;
		case Format::RG16U: return DXGI_FORMAT_R16G16_UNORM;
		case Format::RG16S: return DXGI_FORMAT_R16G16_SNORM;
		case Format::RG16F: return DXGI_FORMAT_R16G16_FLOAT;
		case Format::RG32U: return DXGI_FORMAT_R32G32_UINT;
		case Format::RG32S: return DXGI_FORMAT_R32G32_SINT;
		case Format::RG32F: return DXGI_FORMAT_R32G32_FLOAT;
		case Format::RG64F: return DXGI_FORMAT_UNKNOWN;
		// ── Three channel (DXGI only has 32-bit-per-channel variants) ────────
		case Format::RGB8U:
		case Format::RGB8S:
		case Format::RGB16U:
		case Format::RGB16S:
		case Format::RGB16F:
		case Format::RGB64F:
		case Format::BGR8U:
		case Format::BGR8S:
		case Format::BGR16U:
		case Format::BGR16S:
		case Format::BGR16F:
		case Format::BGR32U:
		case Format::BGR32S:
		case Format::BGR32F:
		case Format::BGR64F: return DXGI_FORMAT_UNKNOWN;
		case Format::RGB32U: return DXGI_FORMAT_R32G32B32_UINT;
		case Format::RGB32S: return DXGI_FORMAT_R32G32B32_SINT;
		case Format::RGB32F: return DXGI_FORMAT_R32G32B32_FLOAT;
		// ── Four channel, RGBA order ─────────────────────────────────────────
		case Format::RGBA8U: return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Format::RGBA8S: return DXGI_FORMAT_R8G8B8A8_SNORM;
		case Format::RGBA16U: return DXGI_FORMAT_R16G16B16A16_UNORM;
		case Format::RGBA16S: return DXGI_FORMAT_R16G16B16A16_SNORM;
		case Format::RGBA16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case Format::RGBA32U: return DXGI_FORMAT_R32G32B32A32_UINT;
		case Format::RGBA32S: return DXGI_FORMAT_R32G32B32A32_SINT;
		case Format::RGBA32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case Format::RGBA64F: return DXGI_FORMAT_UNKNOWN;
		// ── Four channel, BGRA order ─────────────────────────────────────────
		case Format::BGRA8U: return DXGI_FORMAT_B8G8R8A8_UNORM;
		case Format::BGRA8S:
		case Format::BGRA16U:
		case Format::BGRA16S:
		case Format::BGRA16F:
		case Format::BGRA32U:
		case Format::BGRA32S:
		case Format::BGRA32F:
		case Format::BGRA64F: return DXGI_FORMAT_UNKNOWN;
		// ── Four channel, ARGB order ─────────────────────────────────────────
		case Format::ARGB8U:
		case Format::ARGB8S:
		case Format::ARGB16U:
		case Format::ARGB16S:
		case Format::ARGB16F:
		case Format::ARGB32U:
		case Format::ARGB32S:
		case Format::ARGB32F:
		case Format::ARGB64F: return DXGI_FORMAT_UNKNOWN;
		// ── Special types ────────────────────────────────────────────────────
		case Format::INDEXED: return DXGI_FORMAT_UNKNOWN;
		case Format::RGB565:
		case Format::RGB555:
		case Format::RGB444:
		case Format::RGBA444: return DXGI_FORMAT_UNKNOWN; // DXGI packed layouts are B-high
		case Format::DEPTH_COMPONENT16: return DXGI_FORMAT_D16_UNORM;
		case Format::DEPTH_COMPONENT24: return DXGI_FORMAT_UNKNOWN; // only D24S8 exists
		case Format::DEPTH_COMPONENT32: return DXGI_FORMAT_D32_FLOAT; // no 32-bit UNORM depth
		case Format::DEPTH_COMPONENT32F: return DXGI_FORMAT_D32_FLOAT;
		case Format::DEPTH24_STENCIL8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case Format::DEPTH32F_STENCIL8: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		case Format::STENCIL_INDEX8: return DXGI_FORMAT_UNKNOWN;
		// ── Block compression ────────────────────────────────────────────────
		case Format::B_DXT1: return DXGI_FORMAT_BC1_UNORM;
		case Format::B_DXT3: return DXGI_FORMAT_BC2_UNORM;
		case Format::B_DXT5: return DXGI_FORMAT_BC3_UNORM;
		default: return DXGI_FORMAT_UNKNOWN;
	}
}

DXGI_FORMAT translateToDxgiFormat(Kld::PrimitiveType type) noexcept
{
	if (scalarSize(type) == 0 || elementCount(type) == 0) {
		return DXGI_FORMAT_UNKNOWN;
	}
	const unsigned count = elementCount(type);
	const bool normalized = (type & Kld::PrimitiveType::IS_NORMALIZED) != 0;
	switch (scalarType(type)) {
		case Kld::PrimitiveType::TYPE_FLOAT:
			return pickFormat(count, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32G32_FLOAT,
							  DXGI_FORMAT_R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT);
		case Kld::PrimitiveType::TYPE_HALF_FLOAT:
			return pickFormat(count, DXGI_FORMAT_R16_FLOAT, DXGI_FORMAT_R16G16_FLOAT,
							  DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R16G16B16A16_FLOAT);
		case Kld::PrimitiveType::TYPE_DOUBLE:
			return DXGI_FORMAT_UNKNOWN; // Direct3D has no 64-bit input layout formats
		case Kld::PrimitiveType::TYPE_UBYTE:
			if (normalized) {
				return pickFormat(count, DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_R8G8_UNORM,
								  DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R8G8B8A8_UNORM);
			}
			return pickFormat(count, DXGI_FORMAT_R8_UINT, DXGI_FORMAT_R8G8_UINT,
							  DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R8G8B8A8_UINT);
		case Kld::PrimitiveType::TYPE_SBYTE:
			if (normalized) {
				return pickFormat(count, DXGI_FORMAT_R8_SNORM, DXGI_FORMAT_R8G8_SNORM,
								  DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R8G8B8A8_SNORM);
			}
			return pickFormat(count, DXGI_FORMAT_R8_SINT, DXGI_FORMAT_R8G8_SINT,
							  DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R8G8B8A8_SINT);
		case Kld::PrimitiveType::TYPE_USHORT:
			if (normalized) {
				return pickFormat(count, DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_R16G16_UNORM,
								  DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R16G16B16A16_UNORM);
			}
			return pickFormat(count, DXGI_FORMAT_R16_UINT, DXGI_FORMAT_R16G16_UINT,
							  DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R16G16B16A16_UINT);
		case Kld::PrimitiveType::TYPE_SSHORT:
			if (normalized) {
				return pickFormat(count, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R16G16_SNORM,
								  DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R16G16B16A16_SNORM);
			}
			return pickFormat(count, DXGI_FORMAT_R16_SINT, DXGI_FORMAT_R16G16_SINT,
							  DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R16G16B16A16_SINT);
		case Kld::PrimitiveType::TYPE_UINT:
			if (normalized) {
				return DXGI_FORMAT_UNKNOWN; // no normalized 32-bit input formats
			}
			return pickFormat(count, DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32G32_UINT,
							  DXGI_FORMAT_R32G32B32_UINT, DXGI_FORMAT_R32G32B32A32_UINT);
		case Kld::PrimitiveType::TYPE_SINT:
			if (normalized) {
				return DXGI_FORMAT_UNKNOWN;
			}
			return pickFormat(count, DXGI_FORMAT_R32_SINT, DXGI_FORMAT_R32G32_SINT,
							  DXGI_FORMAT_R32G32B32_SINT, DXGI_FORMAT_R32G32B32A32_SINT);
		default:
			return DXGI_FORMAT_UNKNOWN;
	}
}

uint32_t vertexFormatByteSize(Kld::PrimitiveType type) noexcept
{
	const unsigned count = elementCount(type);
	const unsigned size = scalarSize(type);
	if (count == 0 || size == 0) {
		return 0;
	}
	return static_cast<uint32_t>(count * size);
}

bool isIntegerVertexFormat(Kld::PrimitiveType type) noexcept
{
	if ((type & Kld::PrimitiveType::IS_NORMALIZED) != 0) {
		return false;
	}
	switch (scalarType(type)) {
		case Kld::PrimitiveType::TYPE_UBYTE:
		case Kld::PrimitiveType::TYPE_SBYTE:
		case Kld::PrimitiveType::TYPE_USHORT:
		case Kld::PrimitiveType::TYPE_SSHORT:
		case Kld::PrimitiveType::TYPE_UINT:
		case Kld::PrimitiveType::TYPE_SINT: return true;
		default: return false;
	}
}

} // namespace D3D
