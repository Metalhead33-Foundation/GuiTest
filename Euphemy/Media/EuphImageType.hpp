#ifndef EUPHIMAGETYPE_HPP
#define EUPHIMAGETYPE_HPP
#include <cstdint>
namespace Euph {
namespace Media {

enum class ImageFormat : uint8_t {
		INVALID = 0x0000,
		STANDARD_TYPE_MASK = 0x007F,
		SPECIAL_TYPE_START = 0x0080,
		/// Standard type bases
		COLOR_COMPONENT_MASK = 0x0007,
		COLOR_RESOLUTION_MASK = 0x0078,
		ST_GREYSCALE = 0x0001,
		ST_RG = 0x0002,
		ST_RGB = 0x0003,
		ST_BGR = 0x0004,
		ST_RGBA = 0x0005,
		ST_BGRA = 0x0006,
		ST_ARGB = 0x0007,
		SC_UINT8 = 0x0008,
		SC_INT8 = 0x0010,
		SC_UINT16 = 0x0018,
		SC_INT16 = 0x0020,
		SC_UINT32 = 0x0028,
		SC_INT32 = 0x0030,
		SC_F16 = 0x0038,
		SC_F32 = 0x0040,
		SC_F64 = 0x0048,
		B_DXT1 = 0x50,
		B_DXT3 = 0x58,
		B_DXT5 = 0x60,
		/// Standard types bases
		// Greyscale
		R8U = ST_GREYSCALE | SC_UINT8,
		R8S = ST_GREYSCALE | SC_INT8,
		R16U = ST_GREYSCALE | SC_UINT16,
		R16S = ST_GREYSCALE | SC_INT16,
		R16F = ST_GREYSCALE | SC_F16,
		R32U = ST_GREYSCALE | SC_UINT32,
		R32S = ST_GREYSCALE | SC_INT32,
		R32F = ST_GREYSCALE | SC_F32,
		R64F = ST_GREYSCALE | SC_F64,
		// Two-color
		RG8U = ST_RG | SC_UINT8,
		RG8S = ST_RG | SC_INT8,
		RG16U = ST_RG | SC_UINT16,
		RG16S = ST_RG | SC_INT16,
		RG16F = ST_RG | SC_F16,
		RG32U = ST_RG | SC_UINT32,
		RG32S = ST_RG | SC_INT32,
		RG32F = ST_RG | SC_F32,
		RG64F = ST_RG | SC_F64,
		// RGB
		RGB8U = ST_RGB | SC_UINT8,
		RGB8S = ST_RGB | SC_INT8,
		RGB16U = ST_RGB | SC_UINT16,
		RGB16S = ST_RGB | SC_INT16,
		RGB16F = ST_RGB | SC_F16,
		RGB32U = ST_RGB | SC_UINT32,
		RGB32S = ST_RGB | SC_INT32,
		RGB32F = ST_RGB | SC_F32,
		RGB64F = ST_RGB | SC_F64,
		// BGR
		BGR8U = ST_BGR | SC_UINT8,
		BGR8S = ST_BGR | SC_INT8,
		BGR16U = ST_BGR | SC_UINT16,
		BGR16S = ST_BGR | SC_INT16,
		BGR16F = ST_BGR | SC_F16,
		BGR32U = ST_BGR | SC_UINT32,
		BGR32S = ST_BGR | SC_INT32,
		BGR32F = ST_BGR | SC_F32,
		BGR64F = ST_BGR | SC_F64,
		// RGBA
		RGBA8U = ST_RGBA | SC_UINT8,
		RGBA8S = ST_RGBA | SC_INT8,
		RGBA16U = ST_RGBA | SC_UINT16,
		RGBA16S = ST_RGBA | SC_INT16,
		RGBA16F = ST_RGBA | SC_F16,
		RGBA32U = ST_RGBA | SC_UINT32,
		RGBA32S = ST_RGBA | SC_INT32,
		RGBA32F = ST_RGBA | SC_F32,
		RGBA64F = ST_RGBA | SC_F64,
		// BGRA
		BGRA8U = ST_BGRA | SC_UINT8,
		BGRA8S = ST_BGRA | SC_INT8,
		BGRA16U = ST_BGRA | SC_UINT16,
		BGRA16S = ST_BGRA | SC_INT16,
		BGRA16F = ST_BGRA | SC_F16,
		BGRA32U = ST_BGRA | SC_UINT32,
		BGRA32S = ST_BGRA | SC_INT32,
		BGRA32F = ST_BGRA | SC_F32,
		BGRA64F = ST_BGRA | SC_F64,
		// ARGB
		ARGB8U = ST_ARGB | SC_UINT8,
		ARGB8S = ST_ARGB | SC_INT8,
		ARGB16U = ST_ARGB | SC_UINT16,
		ARGB16S = ST_ARGB | SC_INT16,
		ARGB16F = ST_ARGB | SC_F16,
		ARGB32U = ST_ARGB | SC_UINT32,
		ARGB32S = ST_ARGB | SC_INT32,
		ARGB32F = ST_ARGB | SC_F32,
		ARGB64F = ST_ARGB | SC_F64,
		/// Special texture types
		INDEXED = SPECIAL_TYPE_START + 0,
		RGB565 = INDEXED + 1,
		RGB555 = RGB565 + 1,
		RGB444 = RGB555 + 1,
		RGBA444 = RGB444 + 1,
		DEPTH_COMPONENT16 = RGBA444 + 1,
		DEPTH_COMPONENT24 = DEPTH_COMPONENT16 + 1,
		DEPTH_COMPONENT32 = DEPTH_COMPONENT24 + 1,
		DEPTH_COMPONENT32F = DEPTH_COMPONENT32 + 1,
		DEPTH24_STENCIL8 = DEPTH_COMPONENT32F + 1,
		DEPTH32F_STENCIL8 = DEPTH24_STENCIL8 + 1,
		STENCIL_INDEX8 = DEPTH32F_STENCIL8 + 1
};

inline constexpr std::size_t pixelByteSize(ImageFormat format) {
	switch (format) {
		case ImageFormat::DEPTH_COMPONENT16: return sizeof(uint16_t);
		case ImageFormat::DEPTH_COMPONENT24: return 3;
		case ImageFormat::DEPTH_COMPONENT32: return sizeof(uint32_t);
		case ImageFormat::DEPTH_COMPONENT32F: return sizeof(uint32_t);
		case ImageFormat::DEPTH24_STENCIL8: return sizeof(uint32_t);
		case ImageFormat::DEPTH32F_STENCIL8: return sizeof(uint32_t)+sizeof(uint8_t);
		case ImageFormat::STENCIL_INDEX8: return sizeof(uint8_t);
		case ImageFormat::R8U: return sizeof(uint8_t);
		case ImageFormat::R8S: return sizeof(int8_t);
		case ImageFormat::R16U: return sizeof(uint16_t);
		case ImageFormat::R16S: return sizeof(int16_t);
		case ImageFormat::R16F: return sizeof(uint16_t);
		case ImageFormat::R32U: return sizeof(uint32_t);
		case ImageFormat::R32S: return sizeof(int32_t);
		case ImageFormat::R32F: return sizeof(float);
		case ImageFormat::R64F: return sizeof(double);
		case ImageFormat::RG8U: return 2*sizeof(uint8_t);
		case ImageFormat::RG8S: return 2*sizeof(int8_t);
		case ImageFormat::RG16U: return 2*sizeof(uint16_t);
		case ImageFormat::RG16S: return 2*sizeof(int16_t);
		case ImageFormat::RG16F: return 2*sizeof(uint16_t);
		case ImageFormat::RG32U: return 2*sizeof(uint32_t);
		case ImageFormat::RG32S: return 2*sizeof(int32_t);
		case ImageFormat::RG32F: return 2*sizeof(float);
		case ImageFormat::RG64F: return 2*sizeof(double);
		case ImageFormat::RGB8U: return 3*sizeof(uint8_t);
		case ImageFormat::RGB8S: return 3*sizeof(int8_t);
		case ImageFormat::RGB16U: return 3*sizeof(uint16_t);
		case ImageFormat::RGB16S: return 3*sizeof(int16_t);
		case ImageFormat::RGB16F: return 3*sizeof(uint16_t);
		case ImageFormat::RGB32U: return 3*sizeof(uint32_t);
		case ImageFormat::RGB32S: return 3*sizeof(int32_t);
		case ImageFormat::RGB32F: return 3*sizeof(float);
		case ImageFormat::RGB64F: return 3*sizeof(double);
		case ImageFormat::BGR8U: return 3*sizeof(uint8_t);
		case ImageFormat::BGR8S: return 3*sizeof(int8_t);
		case ImageFormat::BGR16U: return 3*sizeof(uint16_t);
		case ImageFormat::BGR16S: return 3*sizeof(int16_t);
		case ImageFormat::BGR16F: return 3*sizeof(uint16_t);
		case ImageFormat::BGR32U: return 3*sizeof(uint32_t);
		case ImageFormat::BGR32S: return 3*sizeof(int32_t);
		case ImageFormat::BGR32F: return 3*sizeof(float);
		case ImageFormat::BGR64F: return 3*sizeof(double);
		case ImageFormat::RGBA8U: return 4*sizeof(uint8_t);
		case ImageFormat::RGBA8S: return 4*sizeof(int8_t);
		case ImageFormat::RGBA16U: return 4*sizeof(uint16_t);
		case ImageFormat::RGBA16S: return 4*sizeof(int16_t);
		case ImageFormat::RGBA16F: return 4*sizeof(uint16_t);
		case ImageFormat::RGBA32U: return 4*sizeof(uint32_t);
		case ImageFormat::RGBA32S: return 4*sizeof(int32_t);
		case ImageFormat::RGBA32F: return 4*sizeof(float);
		case ImageFormat::RGBA64F: return 4*sizeof(double);
		case ImageFormat::BGRA8U: return 4*sizeof(uint8_t);
		case ImageFormat::BGRA8S: return 4*sizeof(int8_t);
		case ImageFormat::BGRA16U: return 4*sizeof(uint16_t);
		case ImageFormat::BGRA16S: return 4*sizeof(int16_t);
		case ImageFormat::BGRA16F: return 4*sizeof(uint16_t);
		case ImageFormat::BGRA32U: return 4*sizeof(uint32_t);
		case ImageFormat::BGRA32S: return 4*sizeof(int32_t);
		case ImageFormat::BGRA32F: return 4*sizeof(float);
		case ImageFormat::BGRA64F: return 4*sizeof(double);
		case ImageFormat::ARGB8U: return 4*sizeof(uint8_t);
		case ImageFormat::ARGB8S: return 4*sizeof(int8_t);
		case ImageFormat::ARGB16U: return 4*sizeof(uint16_t);
		case ImageFormat::ARGB16S: return 4*sizeof(int16_t);
		case ImageFormat::ARGB16F: return 4*sizeof(uint16_t);
		case ImageFormat::ARGB32U: return 4*sizeof(uint32_t);
		case ImageFormat::ARGB32S: return 4*sizeof(int32_t);
		case ImageFormat::ARGB32F: return 4*sizeof(float);
		case ImageFormat::ARGB64F: return 4*sizeof(double);
		case ImageFormat::INDEXED: return sizeof(uint8_t);
		case ImageFormat::RGB565: return sizeof(uint16_t);
		case ImageFormat::RGB555: return sizeof(uint16_t);
		case ImageFormat::RGBA444: return sizeof(uint16_t);
		default: return 0;
	}
}

}
}
#endif // EUPHIMAGETYPE_HPP
