#ifndef EUPHIMAGETYPE_H
#define EUPHIMAGETYPE_H
#include <cstdint>
#include <concepts>
#include <glm/glm.hpp>
namespace Euph {
namespace Media {
namespace Image {

/**
 * @brief Enum class representing various pixel formats.
 */
enum class Format : uint8_t {
	INVALID = 0x0000,						 ///< Invalid format.
	STANDARD_TYPE_MASK = 0x007F,				///< Mask for standard types.
	SPECIAL_TYPE_START = 0x0080,				///< Start value for special types.

	/// Standard type bases
	COLOR_COMPONENT_MASK = 0x0007,			  ///< Mask for color components.
	COLOR_RESOLUTION_MASK = 0x0078,			 ///< Mask for color resolution.
	ST_GREYSCALE = 0x0001,					///< Standard type for greyscale.
	ST_RG = 0x0002,						   ///< Standard type for two-color.
	ST_RGB = 0x0003,						  ///< Standard type for RGB.
	ST_BGR = 0x0004,						  ///< Standard type for BGR.
	ST_RGBA = 0x0005,						 ///< Standard type for RGBA.
	ST_BGRA = 0x0006,						 ///< Standard type for BGRA.
	ST_ARGB = 0x0007,						 ///< Standard type for ARGB.
	SC_UINT8 = 0x0008,						///< Standard component for 8-bit unsigned integer.
	SC_INT8 = 0x0010,						 ///< Standard component for 8-bit signed integer.
	SC_UINT16 = 0x0018,					   ///< Standard component for 16-bit unsigned integer.
	SC_INT16 = 0x0020,						///< Standard component for 16-bit signed integer.
	SC_UINT32 = 0x0028,					   ///< Standard component for 32-bit unsigned integer.
	SC_INT32 = 0x0030,						///< Standard component for 32-bit signed integer.
	SC_F16 = 0x0038,						  ///< Standard component for 16-bit floating-point.
	SC_F32 = 0x0040,						  ///< Standard component for 32-bit floating-point.
	SC_F64 = 0x0048,						  ///< Standard component for 64-bit floating-point.
	B_DXT1 = 0x50,							///< DXT1 block compression.
	B_DXT3 = 0x58,							///< DXT3 block compression.
	B_DXT5 = 0x60,							///< DXT5 block compression.

	/// Standard types bases
	// Greyscale
	R8U = ST_GREYSCALE | SC_UINT8,			///< 8-bit unsigned greyscale.
	R8S = ST_GREYSCALE | SC_INT8,			 ///< 8-bit signed greyscale.
	R16U = ST_GREYSCALE | SC_UINT16,		  ///< 16-bit unsigned greyscale.
	R16S = ST_GREYSCALE | SC_INT16,		   ///< 16-bit signed greyscale.
	R16F = ST_GREYSCALE | SC_F16,			 ///< 16-bit floating-point greyscale.
	R32U = ST_GREYSCALE | SC_UINT32,		  ///< 32-bit unsigned greyscale.
	R32S = ST_GREYSCALE | SC_INT32,		   ///< 32-bit signed greyscale.
	R32F = ST_GREYSCALE | SC_F32,			 ///< 32-bit floating-point greyscale.
	R64F = ST_GREYSCALE | SC_F64,			 ///< 64-bit floating-point greyscale.
	// Two-color
	RG8U = ST_RG | SC_UINT8,				  ///< 8-bit unsigned two-color.
	RG8S = ST_RG | SC_INT8,				   ///< 8-bit signed two-color.
	RG16U = ST_RG | SC_UINT16,				///< 16-bit unsigned two-color.
	RG16S = ST_RG | SC_INT16,				 ///< 16-bit signed two-color.
	RG16F = ST_RG | SC_F16,				   ///< 16-bit floating-point two-color.
	RG32U = ST_RG | SC_UINT32,				///< 32-bit unsigned two-color.
	RG32S = ST_RG | SC_INT32,				 ///< 32-bit signed two-color.
	RG32F = ST_RG | SC_F32,				   ///< 32-bit floating-point two-color.
	RG64F = ST_RG | SC_F64,				   ///< 64-bit floating-point two-color.
	// RGB
	RGB8U = ST_RGB | SC_UINT8,				///< 8-bit unsigned RGB.
	RGB8S = ST_RGB | SC_INT8,				 ///< 8-bit signed RGB.
	RGB16U = ST_RGB | SC_UINT16,			  ///< 16-bit unsigned RGB.
	RGB16S = ST_RGB | SC_INT16,			   ///< 16-bit signed RGB.
	RGB16F = ST_RGB | SC_F16,				 ///< 16-bit floating-point RGB.
	RGB32U = ST_RGB | SC_UINT32,			  ///< 32-bit unsigned RGB.
	RGB32S = ST_RGB | SC_INT32,			   ///< 32-bit signed RGB.
	RGB32F = ST_RGB | SC_F32,				 ///< 32-bit floating-point RGB.
	RGB64F = ST_RGB | SC_F64,				 ///< 64-bit floating-point RGB.
	// BGR
	BGR8U = ST_BGR | SC_UINT8,				///< 8-bit unsigned BGR.
	BGR8S = ST_BGR | SC_INT8,				 ///< 8-bit signed BGR.
	BGR16U = ST_BGR | SC_UINT16,			  ///< 16-bit unsigned BGR.
	BGR16S = ST_BGR | SC_INT16,			   ///< 16-bit signed BGR.
	BGR16F = ST_BGR | SC_F16,				 ///< 16-bit floating-point BGR.
	BGR32U = ST_BGR | SC_UINT32,			  ///< 32-bit unsigned BGR.
	BGR32S = ST_BGR | SC_INT32,			   ///< 32-bit signed BGR.
	BGR32F = ST_BGR | SC_F32,				 ///< 32-bit floating-point BGR.
	BGR64F = ST_BGR | SC_F64,				 ///< 64-bit floating-point BGR.
	// RGBA
	RGBA8U = ST_RGBA | SC_UINT8,			  ///< 8-bit unsigned RGBA.
	RGBA8S = ST_RGBA | SC_INT8,			   ///< 8-bit signed RGBA.
	RGBA16U = ST_RGBA | SC_UINT16,			///< 16-bit unsigned RGBA.
	RGBA16S = ST_RGBA | SC_INT16,			 ///< 16-bit signed RGBA.
	RGBA16F = ST_RGBA | SC_F16,			   ///< 16-bit floating-point RGBA.
	RGBA32U = ST_RGBA | SC_UINT32,			///< 32-bit unsigned RGBA.
	RGBA32S = ST_RGBA | SC_INT32,			 ///< 32-bit signed RGBA.
	RGBA32F = ST_RGBA | SC_F32,			   ///< 32-bit floating-point RGBA.
	RGBA64F = ST_RGBA | SC_F64,			   ///< 64-bit floating-point RGBA.
	// BGRA
	BGRA8U = ST_BGRA | SC_UINT8,			  ///< 8-bit unsigned BGRA.
	BGRA8S = ST_BGRA | SC_INT8,			   ///< 8-bit signed BGRA.
	BGRA16U = ST_BGRA | SC_UINT16,			///< 16-bit unsigned BGRA.
	BGRA16S = ST_BGRA | SC_INT16,			 ///< 16-bit signed BGRA.
	BGRA16F = ST_BGRA | SC_F16,			   ///< 16-bit floating-point BGRA.
	BGRA32U = ST_BGRA | SC_UINT32,			///< 32-bit unsigned BGRA.
	BGRA32S = ST_BGRA | SC_INT32,			 ///< 32-bit signed BGRA.
	BGRA32F = ST_BGRA | SC_F32,			   ///< 32-bit floating-point BGRA.
	BGRA64F = ST_BGRA | SC_F64,			   ///< 64-bit floating-point BGRA.
	// ARGB
	ARGB8U = ST_ARGB | SC_UINT8,			  ///< 8-bit unsigned ARGB.
	ARGB8S = ST_ARGB | SC_INT8,			   ///< 8-bit signed ARGB.
	ARGB16U = ST_ARGB | SC_UINT16,			///< 16-bit unsigned ARGB.
	ARGB16S = ST_ARGB | SC_INT16,			 ///< 16-bit signed ARGB.
	ARGB16F = ST_ARGB | SC_F16,			   ///< 16-bit floating-point ARGB.
	ARGB32U = ST_ARGB | SC_UINT32,			///< 32-bit unsigned ARGB.
	ARGB32S = ST_ARGB | SC_INT32,			 ///< 32-bit signed ARGB.
	ARGB32F = ST_ARGB | SC_F32,			   ///< 32-bit floating-point ARGB.
	ARGB64F = ST_ARGB | SC_F64,			   ///< 64-bit floating-point ARGB.
	/// Special texture types
	INDEXED = SPECIAL_TYPE_START + 0,		 ///< Indexed format.
	RGB565 = INDEXED + 1,					 ///< 16-bit RGB 565.
	RGB555 = RGB565 + 1,					  ///< 16-bit RGB 555.
	RGB444 = RGB555 + 1,					  ///< 16-bit RGB 444.
	RGBA444 = RGB444 + 1,					 ///< 16-bit RGBA 444.
	DEPTH_COMPONENT16 = RGBA444 + 1,		  ///< 16-bit depth component.
	DEPTH_COMPONENT24 = DEPTH_COMPONENT16 + 1,///< 24-bit depth component.
	DEPTH_COMPONENT32 = DEPTH_COMPONENT24 + 1,///< 32-bit depth component.
	DEPTH_COMPONENT32F = DEPTH_COMPONENT32 + 1,///< 32-bit floating-point depth component.
	DEPTH24_STENCIL8 = DEPTH_COMPONENT32F + 1,///< 24-bit depth + 8-bit stencil.
	DEPTH32F_STENCIL8 = DEPTH24_STENCIL8 + 1, ///< 32-bit floating-point depth + 8-bit stencil.
	STENCIL_INDEX8 = DEPTH32F_STENCIL8 + 1	///< 8-bit stencil index.
};

/**
 * @brief Retrieve the size in bytes of a pixel for a given format.
 *
 * @param format The pixel format.
 * @return The size in bytes of a pixel.
 */
inline constexpr std::size_t pixelByteSize(Format format) {
	switch (format) {
		case Format::DEPTH_COMPONENT16: return sizeof(uint16_t);
		case Format::DEPTH_COMPONENT24: return 3;
		case Format::DEPTH_COMPONENT32: return sizeof(uint32_t);
		case Format::DEPTH_COMPONENT32F: return sizeof(uint32_t);
		case Format::DEPTH24_STENCIL8: return sizeof(uint32_t);
		case Format::DEPTH32F_STENCIL8: return sizeof(uint32_t)+sizeof(uint8_t);
		case Format::STENCIL_INDEX8: return sizeof(uint8_t);
		case Format::R8U: return sizeof(uint8_t);
		case Format::R8S: return sizeof(int8_t);
		case Format::R16U: return sizeof(uint16_t);
		case Format::R16S: return sizeof(int16_t);
		case Format::R16F: return sizeof(uint16_t);
		case Format::R32U: return sizeof(uint32_t);
		case Format::R32S: return sizeof(int32_t);
		case Format::R32F: return sizeof(float);
		case Format::R64F: return sizeof(double);
		case Format::RG8U: return 2*sizeof(uint8_t);
		case Format::RG8S: return 2*sizeof(int8_t);
		case Format::RG16U: return 2*sizeof(uint16_t);
		case Format::RG16S: return 2*sizeof(int16_t);
		case Format::RG16F: return 2*sizeof(uint16_t);
		case Format::RG32U: return 2*sizeof(uint32_t);
		case Format::RG32S: return 2*sizeof(int32_t);
		case Format::RG32F: return 2*sizeof(float);
		case Format::RG64F: return 2*sizeof(double);
		case Format::RGB8U: return 3*sizeof(uint8_t);
		case Format::RGB8S: return 3*sizeof(int8_t);
		case Format::RGB16U: return 3*sizeof(uint16_t);
		case Format::RGB16S: return 3*sizeof(int16_t);
		case Format::RGB16F: return 3*sizeof(uint16_t);
		case Format::RGB32U: return 3*sizeof(uint32_t);
		case Format::RGB32S: return 3*sizeof(int32_t);
		case Format::RGB32F: return 3*sizeof(float);
		case Format::RGB64F: return 3*sizeof(double);
		case Format::BGR8U: return 3*sizeof(uint8_t);
		case Format::BGR8S: return 3*sizeof(int8_t);
		case Format::BGR16U: return 3*sizeof(uint16_t);
		case Format::BGR16S: return 3*sizeof(int16_t);
		case Format::BGR16F: return 3*sizeof(uint16_t);
		case Format::BGR32U: return 3*sizeof(uint32_t);
		case Format::BGR32S: return 3*sizeof(int32_t);
		case Format::BGR32F: return 3*sizeof(float);
		case Format::BGR64F: return 3*sizeof(double);
		case Format::RGBA8U: return 4*sizeof(uint8_t);
		case Format::RGBA8S: return 4*sizeof(int8_t);
		case Format::RGBA16U: return 4*sizeof(uint16_t);
		case Format::RGBA16S: return 4*sizeof(int16_t);
		case Format::RGBA16F: return 4*sizeof(uint16_t);
		case Format::RGBA32U: return 4*sizeof(uint32_t);
		case Format::RGBA32S: return 4*sizeof(int32_t);
		case Format::RGBA32F: return 4*sizeof(float);
		case Format::RGBA64F: return 4*sizeof(double);
		case Format::BGRA8U: return 4*sizeof(uint8_t);
		case Format::BGRA8S: return 4*sizeof(int8_t);
		case Format::BGRA16U: return 4*sizeof(uint16_t);
		case Format::BGRA16S: return 4*sizeof(int16_t);
		case Format::BGRA16F: return 4*sizeof(uint16_t);
		case Format::BGRA32U: return 4*sizeof(uint32_t);
		case Format::BGRA32S: return 4*sizeof(int32_t);
		case Format::BGRA32F: return 4*sizeof(float);
		case Format::BGRA64F: return 4*sizeof(double);
		case Format::ARGB8U: return 4*sizeof(uint8_t);
		case Format::ARGB8S: return 4*sizeof(int8_t);
		case Format::ARGB16U: return 4*sizeof(uint16_t);
		case Format::ARGB16S: return 4*sizeof(int16_t);
		case Format::ARGB16F: return 4*sizeof(uint16_t);
		case Format::ARGB32U: return 4*sizeof(uint32_t);
		case Format::ARGB32S: return 4*sizeof(int32_t);
		case Format::ARGB32F: return 4*sizeof(float);
		case Format::ARGB64F: return 4*sizeof(double);
		case Format::INDEXED: return sizeof(uint8_t);
		case Format::RGB565: return sizeof(uint16_t);
		case Format::RGB555: return sizeof(uint16_t);
		case Format::RGBA444: return sizeof(uint16_t);
		default: return 0;
	}
}

/**
 * @brief Calculates the maximum offset for a blit operation.
 *
 * This function determines the maximum offset that can be used for a blit operation
 * considering the dimensions of the destination, source, and the offsets within them.
 *
 * @param destinationDimensions The dimensions of the destination area.
 * @param sourceDimensions	  The dimensions of the source area.
 * @param destinationOffset	 The offset within the destination area.
 * @param sourceOffset		  The offset within the source area.
 * @param dimensionsFromOffset  The dimensions starting from the specified offsets.
 * @return A glm::uvec2 representing the maximum offset for the blit operation.
 */
inline constexpr const glm::uvec2 calculateMaximumOffsetForBlit(const glm::uvec2& destinationDimensions, const glm::uvec2& sourceDimensions, const glm::uvec2& destinationOffset, const glm::uvec2& sourceOffset, const glm::uvec2& dimensionsFromOffset) {
	const unsigned dstMaxX = std::min(destinationDimensions.x,dimensionsFromOffset.x+destinationOffset.x);
	const unsigned dstMaxY = std::min(destinationDimensions.x,dimensionsFromOffset.y+destinationOffset.y);
	const unsigned srcMaxX = std::min(sourceDimensions.x,dimensionsFromOffset.x+sourceOffset.x);
	const unsigned srcMaxY = std::min(sourceDimensions.y,dimensionsFromOffset.y+sourceOffset.y);
	const unsigned maxX = std::min(srcMaxX-sourceOffset.x,dstMaxX-destinationOffset.x);
	const unsigned maxY = std::min(srcMaxY-sourceOffset.y,dstMaxY-destinationOffset.y);
	return glm::uvec2(maxX,maxY);
}

/**
 * @brief Converts 2D coordinates to a linear index.
 *
 * This function computes the linear index for a given 2D coordinate (x, y) in a
 * 2D array with a specified width.
 *
 * @param width The width of the 2D array.
 * @param x	 The x-coordinate in the 2D array.
 * @param y	 The y-coordinate in the 2D array.
 * @return The linear index corresponding to the 2D coordinate (x, y).
 */
inline constexpr unsigned toLinearIndex(unsigned width, unsigned x, unsigned y) {
	return (width*y)+x;
}

/**
 * @brief Concept defining the requirements for a Pixel type.
 *
 * This concept ensures that a type can interact with kernel space for
 * image processing, including conversions to and from kernel formats.
 *
 * @tparam T The type that satisfies the concept requirements.
 *
 * Requirements:
 * - Must define a static member `FMT_ID` of type `Format` representing
 *   the pixel format.
 * - Must provide a method `fromKernel` to convert from a kernel-space
 *   representation to the pixel type.
 * - Must provide a method `fromKernelDithered` to convert from a kernel-space
 *   representation to the pixel type with dithering support.
 * - Must provide a method `toKernel` to convert from the pixel type to a
 *   kernel-space representation.
 *
 * Example:
 * @code
 * class MyPixel {
 * public:
 *	 static constexpr Format FMT_ID = Format::RGB8U;
 *
 *	 void fromKernel(const glm::fvec4& fvec);
 *	 void fromKernelDithered(const glm::fvec4& fvec, const glm::uvec2& uvec);
 *	 void toKernel(glm::fvec4& outFvec) const;
 * };
 *
 * static_assert(PixelConcept<MyPixel>, "MyPixel must satisfy PixelConcept.");
 * @endcode
 *
 * The methods must satisfy the following signatures:
 * - `void T::fromKernel(const glm::fvec4& fvec);`
 *   Converts from a kernel representation (floating-point RGBA) to the pixel type.
 * - `void T::fromKernelDithered(const glm::fvec4& fvec, const glm::uvec2& uvec);`
 *   Converts from a kernel representation (floating-point RGBA) to the pixel type
 *   with dithering applied based on pixel position.
 * - `void T::toKernel(glm::fvec4& outFvec) const;`
 *   Converts the pixel type to a kernel representation (floating-point RGBA).
 * - `static constexpr Format T::FMT_ID;`
 *   A compile-time constant representing the pixel's format.
 */
template <typename T> concept PixelConcept = requires(T t, const glm::fvec4& fvec, const glm::uvec2& uvec, glm::fvec4& outFvec) {
	// Check for fromKernel method
	{ t.fromKernel(fvec) } -> std::same_as<void>;

	// Check for fromKernelDithered method
	{ t.fromKernelDithered(fvec, uvec) } -> std::same_as<void>;

	// Check for toKernel method
	{ t.toKernel(outFvec) } -> std::same_as<void>;

	// Check for FMT_ID static member with type Format
	requires std::is_same_v<decltype(T::FMT_ID), const Format>;
};

}
}
}
#endif // EUPHIMAGETYPE_H
