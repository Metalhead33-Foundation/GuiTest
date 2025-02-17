#ifndef EUPHSTANDARDIMAGE_HPP
#define EUPHSTANDARDIMAGE_HPP
#include <Euphemy/Media/Image/EuphImage.hpp>
#include <Euphemy/Media/Image/EuphPixelFormat.hpp>
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Elvavena/Util/ElvSpanHelpers.hpp>
#include <vector>
#include <algorithm>

namespace Euph {
namespace Media {
namespace Image {

// Resizable image
uIResizeableImage2D MH_EUPH_API createUptrResizeableImage2D(const std::span<const std::byte>& bytes, Format format, unsigned width, unsigned height,
														   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
uIResizeableImage2D MH_EUPH_API createUptrResizeableImage2D(const Frame& frame, Format format,
														   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
uIResizeableImage2D MH_EUPH_API createUptrResizeableImage2D(Format format, unsigned width, unsigned height,
														   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sIResizeableImage2D MH_EUPH_API createSptrResizeableImage2D(const std::span<const std::byte>& bytes, Format format, unsigned width, unsigned height,
														   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sIResizeableImage2D MH_EUPH_API createSptrResizeableImage2D(const Frame& frame, Format format,
														   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sIResizeableImage2D MH_EUPH_API createSptrResizeableImage2D(Format format, unsigned width, unsigned height,
														   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
// Reference image
uIMutableImage2D MH_EUPH_API createUptrReferenceImage2D(std::span<std::byte> bytes, Format format, unsigned width, unsigned height,
									   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
uIMutableImage2D MH_EUPH_API createUptrReferenceImage2D(Frame& frame, Format format,
									   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sIMutableImage2D MH_EUPH_API createSptrReferenceImage2D(std::span<std::byte> bytes, Format format, unsigned width, unsigned height,
									   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sIMutableImage2D MH_EUPH_API createSptrReferenceImage2D(Frame& frame, Format format,
									   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
// Read-OnlyReference image
uIReadOnlyImage2D MH_EUPH_API createUptrReadOnlyReferenceImage2D(const std::span<const std::byte>& bytes, Format format, unsigned width, unsigned height,
												std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
uIReadOnlyImage2D MH_EUPH_API createUptrRReadOnlyeferenceImage2D(const Frame& frame, Format format,
												std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sIReadOnlyImage2D MH_EUPH_API createSptrReadOnlyReferenceImage2D(const std::span<const std::byte>& bytes, Format format, unsigned width, unsigned height,
												std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sIReadOnlyImage2D MH_EUPH_API createSptrReadOnlyReferenceImage2D(const Frame& frame, Format format,
												std::pmr::memory_resource* memRes = std::pmr::get_default_resource());

// Full palette
uIMutablePalette MH_EUPH_API createUptrFullPalette(Format format, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sIMutablePalette MH_EUPH_API createSptrFullPalette(Format format, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
uIMutablePalette MH_EUPH_API createUptrFullPalette(const std::span<const std::byte>& bytes, Format format, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sIMutablePalette MH_EUPH_API createSptrFullPalette(const std::span<const std::byte>& bytes, Format format, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
uIMutablePalette MH_EUPH_API createUptrFullPalette(const Palette& paletteSource, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sIMutablePalette MH_EUPH_API createSptrFullPalette(const Palette& paletteSource, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
// Mutable reference palette
uIMutablePalette MH_EUPH_API createUptrMutableReferencePalette(std::span<std::byte> bytes, Format format, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sIMutablePalette MH_EUPH_API createSptrMutableReferencePalette(std::span<std::byte> bytes, Format format, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
uIMutablePalette MH_EUPH_API createUptrMutableReferencePalette(Palette& paletteSource, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sIMutablePalette MH_EUPH_API createSptrMutableReferencePalette(Palette& paletteSource, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
// Read-only reference palette
uIReadOnlyPalette MH_EUPH_API createUptrReadOnlyReferencePalette(const std::span<const std::byte>& bytes, Format format, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sIReadOnlyPalette MH_EUPH_API createSptrReadOnlyReferencePalette(const std::span<const std::byte>& bytes, Format format, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
uIReadOnlyPalette MH_EUPH_API createUptrReadOnlyReferencePalette(const Palette& paletteSource, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sIReadOnlyPalette MH_EUPH_API createSptrReadOnlyReferencePalette(const Palette& paletteSource, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());

/**
 * @brief Helper structure for image operations using a specified pixel type.
 *
 * This structure provides a collection of static helper methods to operate on image data
 * with generic pixel types. These methods include pixel access, manipulation, iteration,
 * and clearing functionality.
 *
 * @tparam Pixel The pixel type conforming to the PixelConcept.
 */
template <PixelConcept Pixel> struct ImageImplementationHelpers {
	/// Type alias for a read-only span of pixels.
	typedef std::span<const Pixel> ReadOnlyPixelStorage;
	/// Type alias for a mutable span of pixels.
	typedef std::span<Pixel> PixelStorage;	/**
	 * @brief Retrieves a pixel at a given position with wrapping behavior and converts it to a color kernel.
	 *
	 * @param pixels The read-only storage of pixels.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param pos The position of the pixel.
	 * @param colourKernel The output color kernel.
	 * @param wrap The wrapping mode to handle out-of-bound coordinates.
	 */
	static inline void getPixel(const ReadOnlyPixelStorage& pixels, unsigned width, unsigned height, const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap)
	{
		unsigned x,y;
		switch (wrap) {
		case Wrap::REPEAT:
		{
			x = pos.x % width;
			y = pos.y % height;
			break;
		}
		case Wrap::MIRRORED_REPEAT:
		{
			const unsigned doubleWidth = 2 * width;
			const unsigned doubleHeight = 2 * height;
			const unsigned modulo_double_x = pos.x % doubleWidth;
			const unsigned modulo_double_y = pos.y % doubleHeight;
			x = (modulo_double_x < width)
					? pos.x % width
					: (doubleWidth - 1 - (pos.x % width));
			y = (modulo_double_y < height)
					? pos.y % height
					: (doubleHeight - 1 - (pos.y % height));

			break;
		}
		case Wrap::CLAMP_TO_BORDER: // Can't tell the difference between the two
		case Wrap::CLAMP_TO_EDGE:
		{
			x = std::min(pos.x,width-1);
			y = std::min(pos.y,height-1);
			break;
		}
		}
		const Pixel& pxl = pixels[toLinearIndex(width,x,y)];
		pxl.toKernel (colourKernel);
	}
	/**
	 * @brief Sets a pixel's color at a given position using a color kernel.
	 *
	 * @param pixels The mutable storage of pixels.
	 * @param width The width of the image.
	 * @param pos The position of the pixel.
	 * @param colourKernel The input color kernel.
	 */
	static inline void setPixel(const PixelStorage& pixels, unsigned width, const glm::uvec2& pos, const glm::fvec4& colourKernel)
	{
		Pixel& pxl = pixels[toLinearIndex(width,pos.x,pos.y)];
		pxl.fromKernel(colourKernel);
	}
	/**
	 * @brief Sets a pixel's color at a given position using a color kernel with dithering.
	 *
	 * @param pixels The mutable storage of pixels.
	 * @param width The width of the image.
	 * @param pos The position of the pixel.
	 * @param colourKernel The input color kernel.
	 */
	static inline void setPixelDithered(const PixelStorage& pixels, unsigned width, const glm::uvec2& pos, const glm::fvec4& colourKernel)
	{
		Pixel& pxl = pixels[toLinearIndex(width,pos.x,pos.y)];
		pxl.fromKernelDithered(colourKernel,pos);
	}
	/**
	 * @brief Iterates over all pixels in an image and applies a function to each pixel.
	 *
	 * @param pixels The read-only storage of pixels.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param program The function to apply to each pixel.
	 */
	static inline void iterateOverPixels(const ReadOnlyPixelStorage& pixels, unsigned width, unsigned height, const IReadOnlyImage2D::ColourIterator& program)
	{
		Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,[&program](const Pixel& pxl, const glm::uvec2& pos) {
			glm::fvec4 kernel;
			pxl.toKernel(kernel);
			program(pos,kernel);
		},glm::uvec2(width,height));
	}
	/**
	 * @brief Iterates over all pixels in an image and applies a function with normalized coordinates.
	 *
	 * @param pixels The read-only storage of pixels.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param widthR The reciprocal of the width for normalization.
	 * @param heightR The reciprocal of the height for normalization.
	 * @param program The function to apply to each pixel.
	 */
	static inline void iterateOverPixels(const ReadOnlyPixelStorage& pixels, unsigned width, unsigned height, float widthR, float heightR, const IReadOnlyImage2D::ColourIterator2& program)
	{
		Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,[&program,width,height,widthR,heightR](const Pixel& pxl, const glm::uvec2& pos) {
			glm::fvec4 kernel;
			pxl.toKernel(kernel);
			program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR),kernel);
		}, glm::uvec2(width,height));
	}
	/**
	 * @brief Iterates over all pixels in an image and applies a function to each pixel.
	 *
	 * @param pixels The read-only storage of pixels.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param program The function to apply to each pixel.
	 * @param offset The top-left pixel affected by the program.
	 * @param dimensions The size of the subregion affected by the program.
	 */
	static inline void iterateOverPixels(const ReadOnlyPixelStorage& pixels, unsigned width, unsigned height, const IReadOnlyImage2D::ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions)
	{
		Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,[&program](const Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 kernel;
				pxl.toKernel(kernel);
				program(pos,kernel);
			},glm::uvec2(width,height),offset,dimensions);
	}
	/**
	 * @brief Iterates over all pixels in an image and applies a function with normalized coordinates.
	 *
	 * @param pixels The read-only storage of pixels.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param widthR The reciprocal of the width for normalization.
	 * @param heightR The reciprocal of the height for normalization.
	 * @param program The function to apply to each pixel.
	 * @param offset The top-left pixel affected by the program.
	 * @param dimensions The size of the subregion affected by the program.
	 */
	static inline void iterateOverPixels(const ReadOnlyPixelStorage& pixels, unsigned width, unsigned height, float widthR, float heightR, const IReadOnlyImage2D::ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions)
	{
		Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,[&program,width,height,widthR,heightR](const Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 kernel;
				pxl.toKernel(kernel);
				program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR),kernel);
			},glm::uvec2(width,height),offset,dimensions);
	}
	/**
	 * @brief Clears the entire image to a specific color kernel.
	 *
	 * @param pixels The mutable storage of pixels.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param colourKernel The color kernel to clear to.
	 * @param dither Whether to apply dithering during the operation.
	 */
	static inline void clearToColour(PixelStorage pixels, unsigned width, unsigned height, const glm::fvec4& colourKernel, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&colourKernel](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernelDithered(colourKernel, pos);
			},glm::uvec2(width,height));
		} else {
			Pixel pxl;
			pxl.fromKernel(colourKernel);
			std::fill(std::begin(pixels),std::end(pixels),pxl);
		}
	}
	/**
	 * @brief Clears the entire image using a programmable function to define colors.
	 *
	 * @param pixels The mutable storage of pixels.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param program The color programming function.
	 * @param dither Whether to apply dithering during the operation.
	 */
	static inline void clearToColour(PixelStorage pixels, unsigned width, unsigned height, const IMutableImage2D::ColourProgrammer& program, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&program](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernelDithered(program(pos), pos);
			},glm::uvec2(width,height));
		} else {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&program](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernel(program(pos));
			},glm::uvec2(width,height));
		}
	}
	/**
	 * @brief Clears the entire image using a programmable function to define colors.
	 *
	 * @param pixels The mutable storage of pixels.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param program The color programming function.
	 * @param dither Whether to apply dithering during the operation.
	 */
	static inline void clearToColour(PixelStorage pixels, unsigned width, unsigned height, const IMutableImage2D::ColourProgrammer2& program, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&program](Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 preExisting;
				pxl.toKernel(preExisting);
				pxl.fromKernelDithered(program(pos,preExisting), pos);
			},glm::uvec2(width,height));
		} else {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&program](Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 preExisting;
				pxl.toKernel(preExisting);
				pxl.fromKernel(program(pos,preExisting));
			},glm::uvec2(width,height));
		}
	}
	/**
	 * @brief Clears the entire image using a programmable function to define colors.
	 *
	 * @param pixels The mutable storage of pixels.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param widthR The reciprocal of the width for normalization.
	 * @param heightR The reciprocal of the height for normalization.
	 * @param program The color programming function.
	 * @param dither Whether to apply dithering during the operation.
	 */
	static inline void clearToColour(PixelStorage pixels, unsigned width, unsigned height, float widthR, float heightR, const IMutableImage2D::ColourProgrammer3& program, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&program,width,height,widthR,heightR](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernelDithered(program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR)), pos);
			},glm::uvec2(width,height));
		} else {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&program,width,height,widthR,heightR](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernel(program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR)));
			},glm::uvec2(width,height));
		}
	}
	/**
	 * @brief Clears the entire image using a programmable function to define colors.
	 *
	 * @param pixels The mutable storage of pixels.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param widthR The reciprocal of the width for normalization.
	 * @param heightR The reciprocal of the height for normalization.
	 * @param program The color programming function.
	 * @param dither Whether to apply dithering during the operation.
	 */
	static inline void clearToColour(PixelStorage pixels,unsigned width, unsigned height, float widthR, float heightR, const IMutableImage2D::ColourProgrammer4& program, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&program,width,height,widthR,heightR](Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 preExisting;
				pxl.toKernel(preExisting);
				pxl.fromKernelDithered(program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR),preExisting), pos);
			},glm::uvec2(width,height));
		} else {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&program,width,height,widthR,heightR](Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 preExisting;
				pxl.toKernel(preExisting);
				pxl.fromKernel(program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR),preExisting));
			},glm::uvec2(width,height));
		}
	}
	/**
	 * @brief Clears a specific rectangular area in the image to a given color kernel.
	 *
	 * @param pixels The mutable storage of pixels.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param colourKernel The color kernel to clear to.
	 * @param offset The top-left corner of the rectangle.
	 * @param dimensions The dimensions of the rectangle.
	 * @param dither Whether to apply dithering during the operation.
	 */
	static inline void clearToColour(PixelStorage pixels,unsigned width, unsigned height, const glm::fvec4& colourKernel, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&colourKernel](Pixel& pxl, const glm::uvec2& pos) {
					pxl.fromKernelDithered(colourKernel, pos);
				},glm::uvec2(width,height),offset,dimensions);
		} else {
			Pixel srcPxl;
			srcPxl.fromKernel(colourKernel);
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[srcPxl](Pixel& dstPxl, const glm::uvec2& pos) {
					(void)pos;
					dstPxl = srcPxl;
				},glm::uvec2(width,height),offset,dimensions);
		}
	}
	/**
	 * @brief Clears a specific rectangular area in the image using a programmable function to define colors.
	 *
	 * @param pixels The mutable storage of pixels.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param program The color programming function.
	 * @param offset The top-left corner of the rectangle.
	 * @param dimensions The dimensions of the rectangle.
	 * @param dither Whether to apply dithering during the operation.
	 */
	static inline void clearToColour(PixelStorage pixels, unsigned width, unsigned height, const IMutableImage2D::ColourProgrammer& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&program](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernelDithered(program(pos), pos);
			}, glm::uvec2(width,height), offset, dimensions);
		} else {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&program](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernel(program(pos));
			}, glm::uvec2(width,height), offset, dimensions);
		}
	}
	/**
	 * @brief Clears a specific rectangular area in the image using a programmable function to define colors.
	 *
	 * @param pixels The mutable storage of pixels.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param program The color programming function.
	 * @param offset The top-left corner of the rectangle.
	 * @param dimensions The dimensions of the rectangle.
	 * @param dither Whether to apply dithering during the operation.
	 */
	static inline void clearToColour(PixelStorage pixels, unsigned width, unsigned height, const IMutableImage2D::ColourProgrammer2& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&program](Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 preExisting;
				pxl.toKernel(preExisting);
				pxl.fromKernelDithered(program(pos,preExisting), pos);
			},glm::uvec2(width,height), offset, dimensions);
		} else {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&program](Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 preExisting;
				pxl.toKernel(preExisting);
				pxl.fromKernel(program(pos,preExisting));
			}, glm::uvec2(width,height),offset, dimensions);
		}
	}
	/**
	 * @brief Clears a specific rectangular area in the image using a programmable function to define colors.
	 *
	 * @param pixels The mutable storage of pixels.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param widthR The reciprocal of the width for normalization.
	 * @param heightR The reciprocal of the height for normalization.
	 * @param program The color programming function.
	 * @param offset The top-left corner of the rectangle.
	 * @param dimensions The dimensions of the rectangle.
	 * @param dither Whether to apply dithering during the operation.
	 */
	static inline void clearToColour(PixelStorage pixels,unsigned width, unsigned height, float widthR, float heightR, const IMutableImage2D::ColourProgrammer3& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&program,width,height,widthR,heightR](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernelDithered(program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR)), pos);
			},glm::uvec2(width,height), offset, dimensions);
		} else {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&program,width,height,widthR,heightR](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernel(program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR)));
			},glm::uvec2(width,height), offset, dimensions);
		}
	}
	/**
	 * @brief Clears a specific rectangular area in the image using a programmable function to define colors.
	 *
	 * @param pixels The mutable storage of pixels.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param widthR The reciprocal of the width for normalization.
	 * @param heightR The reciprocal of the height for normalization.
	 * @param program The color programming function.
	 * @param offset The top-left corner of the rectangle.
	 * @param dimensions The dimensions of the rectangle.
	 * @param dither Whether to apply dithering during the operation.
	 */
	static inline void clearToColour(PixelStorage pixels,unsigned width, unsigned height, float widthR, float heightR, const IMutableImage2D::ColourProgrammer4& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&program,width,height,widthR,heightR](Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 preExisting;
				pxl.toKernel(preExisting);
				pxl.fromKernelDithered(program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR),preExisting), pos);
			},glm::uvec2(width,height), offset, dimensions);
		} else {
			Elv::Util::span_wrappers<Pixel>::over_2d_span_mut(pixels,[&program,width,height,widthR,heightR](Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 preExisting;
				pxl.toKernel(preExisting);
				pxl.fromKernel(program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR),preExisting));
			}, glm::uvec2(width,height),offset, dimensions);
		}
	}
};

/**
 * @brief Helper structure for implementing functionality related to paletted images.
 *
 * This structure provides a collection of static helper methods to operate on paletted image data
 * with generic pixel types. These methods include pixel access, manipulation, iteration,
 * and clearing functionality.
 *
 * @tparam Pixel The pixel type conforming to the PixelConcept.
 */
template <PixelConcept Pixel> struct PalettedImageImplementationHelpers {
	/**
	 * @brief A type alias for the palette, which is a span of constant Pixels.
	 */
	typedef std::span<const Pixel> Palette;
	/**
	 * @brief A type alias for pixel storage, which is a span of constant uint8_t values.
	 */
	typedef std::span<const uint8_t> PixelStorage;
	/**
	 * @brief Retrieves a pixel's color at a specified position in the image, applying the specified wrapping behavior.
	 *
	 * @param palette A reference to the palette containing the pixel data.
	 * @param pixels A reference to the pixel storage.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param pos The position of the pixel to retrieve.
	 * @param colourKernel A reference to a vector to store the retrieved pixel's color.
	 * @param wrap The wrapping mode to use when accessing the pixel (e.g., REPEAT, MIRRORED_REPEAT, CLAMP_TO_BORDER, CLAMP_TO_EDGE).
	 */
	static inline void getPixel(const Palette& palette, const PixelStorage& pixels, unsigned width, unsigned height,
								const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap)
	{
		unsigned x,y;
		switch (wrap) {
		case Wrap::REPEAT:
		{
			x = pos.x % width;
			y = pos.y % height;
			break;
		}
		case Wrap::MIRRORED_REPEAT:
		{
			const unsigned doubleWidth = 2 * width;
			const unsigned doubleHeight = 2 * height;
			const unsigned modulo_double_x = pos.x % doubleWidth;
			const unsigned modulo_double_y = pos.y % doubleHeight;
			x = (modulo_double_x < width)
					? pos.x % width
					: (doubleWidth - 1 - (pos.x % width));
			y = (modulo_double_y < height)
					? pos.y % height
					: (doubleHeight - 1 - (pos.y % height));

			break;
		}
		case Wrap::CLAMP_TO_BORDER: // Can't tell the difference between the two
		case Wrap::CLAMP_TO_EDGE:
		{
			x = std::min(pos.x,width-1);
			y = std::min(pos.y,height-1);
			break;
		}
		}
		const Pixel& pxl = palette[pixels[toLinearIndex(width,x,y)]];
		pxl.toKernel(colourKernel);
	}
	/**
	 * @brief Iterates over all pixels in the image and applies a user-defined program.
	 *
	 * @param palette A reference to the palette containing the pixel data.
	 * @param pixels A reference to the pixel storage.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param program A user-defined function to apply to each pixel. The function should take a pixel position and its color.
	 */
	static inline void iterateOverPixels(const Palette& palette, const PixelStorage& pixels, unsigned width, unsigned height, const IReadOnlyImage2D::ColourIterator& program)
	{
		Elv::Util::span_wrappers<uint8_t>::over_2d_span(pixels,glm::uvec2(width,height),[&program,&palette](const uint8_t& pxl, const glm::uvec2& pos) {
			glm::fvec4 kernel;
			palette[pxl].toKernel(kernel);
			program(pos,kernel);
		});
	}
	/**
	 * @brief Iterates over all pixels in the image and applies a user-defined program, scaling positions by given ratios.
	 *
	 * @param palette A reference to the palette containing the pixel data.
	 * @param pixels A reference to the pixel storage.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param widthR The reciprocal of the width for normalization.
	 * @param heightR The reciprocal of the height for normalization.
	 * @param program A user-defined function to apply to each pixel. The function should take a scaled position and its color.
	 */
	static inline void iterateOverPixels(const Palette& palette, const PixelStorage& pixels, unsigned width, unsigned height, float widthR, float heightR, const IReadOnlyImage2D::ColourIterator2& program)
	{
		Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program,&palette,widthR,heightR](const Pixel& pxl, const glm::uvec2& pos) {
			glm::fvec4 kernel;
			palette[pxl].toKernel(kernel);
			program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR),kernel);
		});
	}
	/**
	 * @brief Iterates over a subregion of the image and applies a user-defined program.
	 *
	 * @param palette A reference to the palette containing the pixel data.
	 * @param pixels A reference to the pixel storage.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param program A user-defined function to apply to each pixel. The function should take a pixel position and its color.
	 * @param offset The offset of the subregion within the image.
	 * @param dimensions The dimensions of the subregion to iterate over.
	 */
	static inline void iterateOverPixels(const Palette& palette, const PixelStorage& pixels, unsigned width, unsigned height,
										 const IReadOnlyImage2D::ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions)
	{
		Elv::Util::span_wrappers<uint8_t>::over_2d_span(pixels,glm::uvec2(width,height),[&program,&palette](const uint8_t& pxl, const glm::uvec2& pos) {
				glm::fvec4 kernel;
				palette[pxl].toKernel(kernel);
				program(pos,kernel);
			}, offset,dimensions);
	}

	/**
	 * @brief Iterates over a subregion of the image and applies a user-defined program, scaling positions by given ratios.
	 *
	 * @param palette A reference to the palette containing the pixel data.
	 * @param pixels A reference to the pixel storage.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param widthR The reciprocal of the width for normalization.
	 * @param heightR The reciprocal of the height for normalization.
	 * @param program A user-defined function to apply to each pixel. The function should take a scaled position and its color.
	 * @param offset The offset of the subregion within the image.
	 * @param dimensions The dimensions of the subregion to iterate over.
	 */
	static inline void iterateOverPixels(const Palette& palette, const PixelStorage& pixels, unsigned width, unsigned height, float widthR, float heightR, const IReadOnlyImage2D::ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions)
	{
		Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program,&palette,widthR,heightR](const Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 kernel;
				palette[pxl].toKernel(kernel);
				program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR),kernel);
			}, offset,dimensions);
	}
};


/**
 * @brief A read-only 2D image wrapper for pixel data.
 *
 * This class provides a read-only interface to a 2D image, referencing external pixel data
 * through a `std::span`. The image supports iteration and pixel access with custom operations.
 *
 * @tparam Pixel The pixel type, which must satisfy the PixelConcept requirements.
 */
template <PixelConcept Pixel>
class ReadOnlyReferenceImage : public IReadOnlyImage2D {
private:
	/** @brief Span containing the pixel data. */
	std::span<const Pixel> pixels;

public:
	/**
	 * @brief Retrieves the format of the pixel data.
	 *
	 * @return The format ID of the pixel type, defined as `Pixel::FMT_ID`.
	 */
	Format getFormat() const override {
		return Pixel::FMT_ID;
	}

	/**
	 * @brief Fetches the color of a specific pixel, applying a wrap mode if necessary.
	 *
	 * @param pos The position of the pixel in the image.
	 * @param colourKernel The output color kernel to be populated.
	 * @param wrap The wrap mode to handle out-of-bounds coordinates.
	 */
	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override {
		ImageImplementationHelpers<Pixel>::getPixel(pixels, width, height, pos, colourKernel, wrap);
	}

	/**
	 * @brief Provides a pointer to the raw pixel data.
	 *
	 * @return A constant void pointer to the pixel data.
	 */
	const void* getRawPixels() const override {
		return pixels.data();
	}

	/**
	 * @brief Iterates over all pixels and applies a given program.
	 *
	 * @param program The function to apply to each pixel.
	 */
	void iterateOverPixels(const ColourIterator& program) const override {
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels, width, height, program);
	}

	/**
	 * @brief Iterates over all pixels using a program that considers additional dimensions.
	 *
	 * @param program The function to apply to each pixel.
	 */
	void iterateOverPixels(const ColourIterator2& program) const override {
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels, width, height, widthR, heightR, program);
	}

	/**
	 * @brief Iterates over a subregion of the image and applies a given program.
	 *
	 * @param program The function to apply to each pixel.
	 * @param offset The top-left corner of the subregion.
	 * @param dimensions The dimensions of the subregion.
	 */
	void iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override {
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels, width, height, program, offset, dimensions);
	}

	/**
	 * @brief Iterates over a subregion of the image using a program that considers additional dimensions.
	 *
	 * @param program The function to apply to each pixel.
	 * @param offset The top-left corner of the subregion.
	 * @param dimensions The dimensions of the subregion.
	 */
	void iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override {
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels, width, height, widthR, heightR, program, offset, dimensions);
	}

	/**
	 * @brief Constructs a ReadOnlyReferenceImage with the given pixel data and dimensions.
	 *
	 * @param pixels A span referencing the pixel data.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 */
	ReadOnlyReferenceImage(std::span<const Pixel> pixels, unsigned width, unsigned height)
		: pixels(pixels) {
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}

	/**
	 * @brief Copy constructor.
	 *
	 * @param cpy The image to copy from.
	 */
	ReadOnlyReferenceImage(const ReadOnlyReferenceImage& cpy)
		: pixels(cpy.pixels) {
		IReadOnlyImage2D::operator=(cpy);
	}

	/**
	 * @brief Copy assignment operator.
	 *
	 * @param cpy The image to copy from.
	 * @return A reference to the current object.
	 */
	ReadOnlyReferenceImage& operator=(const ReadOnlyReferenceImage& cpy) {
		this->pixels = cpy.pixels;
		IReadOnlyImage2D::operator=(cpy);
		return *this;
	}
};

/**
 * @brief A modifiable 2D image wrapper for pixel data.
 *
 * This class provides a 2D image that references an external span of pixels.
 * It supports various pixel operations such as getting, setting, iterating, and clearing pixels.
 *
 * @tparam Pixel A type that conforms to the PixelConcept, providing required pixel format and operations.
 */
template <PixelConcept Pixel> class ReferenceImage : public IMutableImage2D {
private:
	/**
	 * @brief A span of pixels that this image references.
	 */
	std::span<Pixel> pixels;

public:
	/**
	 * @brief Get the format of the image.
	 * @return The format of the image, as defined by the `FMT_ID` of the pixel type.
	 */
	Format getFormat() const override
	{
		return Pixel::FMT_ID;
	}

	/**
	 * @brief Get the color of a pixel at a given position.
	 * @param pos The position of the pixel in the image.
	 * @param colourKernel The output color kernel to store the pixel's color.
	 * @param wrap The wrap mode to use for out-of-bounds access.
	 */
	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override
	{
		ImageImplementationHelpers<Pixel>::getPixel(pixels, width, height, pos, colourKernel, wrap);
	}

	/**
	 * @brief Set the color of a pixel at a given position.
	 * @param pos The position of the pixel in the image.
	 * @param colourKernel The color kernel containing the color to set.
	 */
	void setPixel(const glm::uvec2& pos, const glm::fvec4& colourKernel) override
	{
		Pixel& pxl = pixels[toLinearIndex(width, pos.x, pos.y)];
		pxl.fromKernel(colourKernel);
	}

	/**
	 * @brief Set the color of a pixel at a given position with dithering.
	 * @param pos The position of the pixel in the image.
	 * @param colourKernel The color kernel containing the color to set.
	 */
	void setPixelDithered(const glm::uvec2& pos, const glm::fvec4& colourKernel) override
	{
		Pixel& pxl = pixels[toLinearIndex(width, pos.x, pos.y)];
		pxl.fromKernelDithered(colourKernel, pos);
	}

	/**
	 * @brief Get a pointer to the raw pixel data.
	 * @return A pointer to the raw pixel data.
	 */
	void* getRawPixels() override
	{
		return pixels.data();
	}

	/**
	 * @brief Get a constant pointer to the raw pixel data.
	 * @return A constant pointer to the raw pixel data.
	 */
	const void* getRawPixels() const override
	{
		return pixels.data();
	}

	/**
	 * @brief Iterate over all pixels in the image.
	 * @param program The program to apply to each pixel.
	 */
	void iterateOverPixels(const ColourIterator& program) const override
	{
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels, width, height, program);
	}

	/**
	 * @brief Iterate over all pixels in the image with a second program.
	 * @param program The program to apply to each pixel.
	 */
	void iterateOverPixels(const ColourIterator2& program) const override
	{
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels, width, height, widthR, heightR, program);
	}

	/**
	 * @brief Iterate over a subset of pixels in the image.
	 * @param program The program to apply to each pixel.
	 * @param offset The offset of the subset of pixels.
	 * @param dimensions The dimensions of the subset of pixels.
	 */
	void iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels, width, height, program, offset, dimensions);
	}

	/**
	 * @brief Iterate over a subset of pixels in the image with a second program.
	 * @param program The program to apply to each pixel.
	 * @param offset The offset of the subset of pixels.
	 * @param dimensions The dimensions of the subset of pixels.
	 */
	void iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels, width, height, widthR, heightR, program, offset, dimensions);
	}

	/**
	 * @brief Clear the image to a solid color.
	 * @param colourKernel The color to set all pixels to.
	 * @param dither Whether to use dithering.
	 */
	void clearToColour(const glm::fvec4& colourKernel, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, colourKernel, dither);
	}

	/**
	 * @brief Clear the image using a color programming function.
	 * @param program The color programming function to use.
	 * @param dither Whether to use dithering.
	 */
	void clearToColour(const ColourProgrammer& program, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, program, dither);
	}

	/**
	 * @brief Clear the image using a second color programming function.
	 * @param program The color programming function to use.
	 * @param dither Whether to use dithering.
	 */
	void clearToColour(const ColourProgrammer2& program, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, program, dither);
	}

	/**
	 * @brief Clear the image using a third color programming function.
	 * @param program The color programming function to use.
	 * @param dither Whether to use dithering.
	 */
	void clearToColour(const ColourProgrammer3& program, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, widthR, heightR, program, dither);
	}

	/**
	 * @brief Clear the image using a fourth color programming function.
	 * @param program The color programming function to use.
	 * @param dither Whether to use dithering.
	 */
	void clearToColour(const ColourProgrammer4& program, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, widthR, heightR, program, dither);
	}

	/**
	 * @brief Clear a subset of the image to a solid color.
	 * @param colourKernel The color to set the subset of pixels to.
	 * @param offset The offset of the subset of pixels.
	 * @param dimensions The dimensions of the subset of pixels.
	 * @param dither Whether to use dithering.
	 */
	void clearToColour(const glm::fvec4& colourKernel, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, colourKernel, offset, dimensions, dither);
	}

	/**
	 * @brief Clear a subset of the image using a color programming function.
	 * @param program The color programming function to use.
	 * @param offset The offset of the subset of pixels.
	 * @param dimensions The dimensions of the subset of pixels.
	 * @param dither Whether to use dithering.
	 */
	void clearToColour(const ColourProgrammer& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, program, offset, dimensions, dither);
	}

	/**
	 * @brief Clear a subset of the image using a second color programming function.
	 * @param program The color programming function to use.
	 * @param offset The offset of the subset of pixels.
	 * @param dimensions The dimensions of the subset of pixels.
	 * @param dither Whether to use dithering.
	 */
	void clearToColour(const ColourProgrammer2& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, program, offset, dimensions, dither);
	}

	/**
	 * @brief Clear a subset of the image using a third color programming function.
	 * @param program The color programming function to use.
	 * @param offset The offset of the subset of pixels.
	 * @param dimensions The dimensions of the subset of pixels.
	 * @param dither Whether to use dithering.
	 */
	void clearToColour(const ColourProgrammer3& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, widthR, heightR, program, offset, dimensions, dither);
	}

	/**
	 * @brief Clear a subset of the image using a fourth color programming function.
	 * @param program The color programming function to use.
	 * @param offset The offset of the subset of pixels.
	 * @param dimensions The dimensions of the subset of pixels.
	 * @param dither Whether to use dithering.
	 */
	void clearToColour(const ColourProgrammer4& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, widthR, heightR, program, offset, dimensions, dither);
	}

	/**
	 * @brief Constructor that initializes the image with a span of pixels and dimensions.
	 * @param pixels A span of pixels that the image will reference.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 */
	ReferenceImage(std::span<Pixel> pixels, unsigned width, unsigned height)
		: pixels(pixels) {
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}

	/**
	 * @brief Copy constructor.
	 * @param cpy The image to copy.
	 */
	ReferenceImage(const ReferenceImage& cpy)
		: pixels(cpy.pixels) {
		IReadOnlyImage2D::operator=(cpy);
	}

	/**
	 * @brief Copy assignment operator.
	 * @param cpy The image to copy.
	 * @return A reference to the copied image.
	 */
	ReferenceImage& operator=(const ReferenceImage& cpy)
	{
		this->pixels = cpy.pixels;
		IReadOnlyImage2D::operator=(cpy);
		return *this;
	}
};

/**
 * @class StandardImage
 * @brief A template class representing a standard 2D image that can be resized.
 *
 * The class uses a pixel concept to define the type of pixel data it holds.
 * It provides methods to manipulate and access pixel data, resize the image,
 * and clear the image to a specific color.
 *
 * @tparam Pixel The type of pixel data stored in the image. This type must
 *			   define the following:
 *			   - `FMT_ID`: A static member indicating the format of the pixel.
 *			   - `fromKernel(const glm::fvec4&)`: A method to set the pixel
 *				 from a color kernel.
 *			   - `fromKernelDithered(const glm::fvec4&, const glm::uvec2&)`: A
 *				 method to set the pixel from a color kernel with dithering.
 *
 * @note This class inherits from `ResizeableImage2D` and implements its interface.
 */
template <PixelConcept Pixel>
class StandardImage : public IResizeableImage2D {
private:
	/**
	 * @brief A vector of pixels managed by a polymorphic memory resource.
	 */
	std::pmr::vector<Pixel> pixels;

	/**
	 * @brief A pointer to the memory resource used by the pixel vector.
	 */
	std::pmr::memory_resource* memRes;

public:
	/**
	 * @brief Returns the format of the pixel data.
	 *
	 * @return The format of the pixel data, as defined by `Pixel::FMT_ID`.
	 */
	Format getFormat() const override {
		return Pixel::FMT_ID;
	}

	/**
	 * @brief Retrieves the color of a pixel at a given position.
	 *
	 * @param pos The position of the pixel.
	 * @param colourKernel The color kernel to store the pixel color.
	 * @param wrap The wrap mode to use when accessing the pixel.
	 */
	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override {
		ImageImplementationHelpers<Pixel>::getPixel(pixels, width, height, pos, colourKernel, wrap);
	}

	/**
	 * @brief Sets the color of a pixel at a given position.
	 *
	 * @param pos The position of the pixel.
	 * @param colourKernel The color kernel representing the new color.
	 */
	void setPixel(const glm::uvec2& pos, const glm::fvec4& colourKernel) override {
		Pixel& pxl = pixels[toLinearIndex(width, pos.x, pos.y)];
		pxl.fromKernel(colourKernel);
	}

	/**
	 * @brief Sets the color of a pixel at a given position with dithering.
	 *
	 * @param pos The position of the pixel.
	 * @param colourKernel The color kernel representing the new color.
	 */
	void setPixelDithered(const glm::uvec2& pos, const glm::fvec4& colourKernel) override {
		Pixel& pxl = pixels[toLinearIndex(width, pos.x, pos.y)];
		pxl.fromKernelDithered(colourKernel, pos);
	}

	/**
	 * @brief Returns a pointer to the raw pixel data.
	 *
	 * @return A pointer to the raw pixel data.
	 */
	void* getRawPixels() override {
		return pixels.data();
	}

	/**
	 * @brief Returns a constant pointer to the raw pixel data.
	 *
	 * @return A constant pointer to the raw pixel data.
	 */
	const void* getRawPixels() const override {
		return pixels.data();
	}

	/**
	 * @brief Iterates over all pixels in the image and applies a color program.
	 *
	 * @param program The color program to apply to each pixel.
	 */
	void iterateOverPixels(const ColourIterator& program) const override {
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels, width, height, program);
	}

	/**
	 * @brief Iterates over all pixels in the image and applies a color program.
	 *
	 * @param program The color program to apply to each pixel.
	 */
	void iterateOverPixels(const ColourIterator2& program) const override {
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels, width, height, widthR, heightR, program);
	}

	/**
	 * @brief Iterates over a region of pixels in the image and applies a color program.
	 *
	 * @param program The color program to apply to each pixel.
	 * @param offset The offset of the region.
	 * @param dimensions The dimensions of the region.
	 */
	void iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override {
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels, width, height, program, offset, dimensions);
	}

	/**
	 * @brief Iterates over a region of pixels in the image and applies a color program.
	 *
	 * @param program The color program to apply to each pixel.
	 * @param offset The offset of the region.
	 * @param dimensions The dimensions of the region.
	 */
	void iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override {
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels, width, height, widthR, heightR, program, offset, dimensions);
	}

	/**
	 * @brief Clears the image to a specified color.
	 *
	 * @param colourKernel The color kernel representing the new color.
	 * @param dither Whether to use dithering when clearing the image.
	 */
	void clearToColour(const glm::fvec4& colourKernel, bool dither) override {
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, colourKernel, dither);
	}

	/**
	 * @brief Clears the image using a color programmer.
	 *
	 * @param program The color programmer to use.
	 * @param dither Whether to use dithering when clearing the image.
	 */
	void clearToColour(const ColourProgrammer& program, bool dither) override {
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, program, dither);
	}

	/**
	 * @brief Clears the image using a color programmer.
	 *
	 * @param program The color programmer to use.
	 * @param dither Whether to use dithering when clearing the image.
	 */
	void clearToColour(const ColourProgrammer2& program, bool dither) override {
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, program, dither);
	}

	/**
	 * @brief Clears the image using a color programmer.
	 *
	 * @param program The color programmer to use.
	 * @param dither Whether to use dithering when clearing the image.
	 */
	void clearToColour(const ColourProgrammer3& program, bool dither) override {
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, widthR, heightR, program, dither);
	}

	/**
	 * @brief Clears the image using a color programmer.
	 *
	 * @param program The color programmer to use.
	 * @param dither Whether to use dithering when clearing the image.
	 */
	void clearToColour(const ColourProgrammer4& program, bool dither) override {
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, widthR, heightR, program, dither);
	}

	/**
	 * @brief Clears a region of the image to a specified color.
	 *
	 * @param colourKernel The color kernel representing the new color.
	 * @param offset The offset of the region.
	 * @param dimensions The dimensions of the region.
	 * @param dither Whether to use dithering when clearing the region.
	 */
	void clearToColour(const glm::fvec4& colourKernel, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override {
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, colourKernel, offset, dimensions, dither);
	}

	/**
	 * @brief Clears a region of the image using a color programmer.
	 *
	 * @param program The color programmer to use.
	 * @param offset The offset of the region.
	 * @param dimensions The dimensions of the region.
	 * @param dither Whether to use dithering when clearing the region.
	 */
	void clearToColour(const ColourProgrammer& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override {
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, program, offset, dimensions, dither);
	}

	/**
	 * @brief Clears a region of the image using a color programmer.
	 *
	 * @param program The color programmer to use.
	 * @param offset The offset of the region.
	 * @param dimensions The dimensions of the region.
	 * @param dither Whether to use dithering when clearing the region.
	 */
	void clearToColour(const ColourProgrammer2& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override {
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, program, offset, dimensions, dither);
	}

	/**
	 * @brief Clears a region of the image using a color programmer.
	 *
	 * @param program The color programmer to use.
	 * @param offset The offset of the region.
	 * @param dimensions The dimensions of the region.
	 * @param dither Whether to use dithering when clearing the region.
	 */
	void clearToColour(const ColourProgrammer3& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override {
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, widthR, heightR, program, offset, dimensions, dither);
	}

	/**
	 * @brief Clears a region of the image using a color programmer.
	 *
	 * @param program The color programmer to use.
	 * @param offset The offset of the region.
	 * @param dimensions The dimensions of the region.
	 * @param dither Whether to use dithering when clearing the region.
	 */
	void clearToColour(const ColourProgrammer4& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override {
		ImageImplementationHelpers<Pixel>::clearToColour(pixels, width, height, widthR, heightR, program, offset, dimensions, dither);
	}

	/**
	 * @brief Resizes the image to new dimensions.
	 *
	 * @param newWidth The new width of the image.
	 * @param newHeight The new height of the image.
	 * @return `true` if the resizing was successful, `false` otherwise.
	 */
	bool resize(unsigned newWidth, unsigned newHeight) override {
		std::pmr::vector<Pixel> newPixels(memRes);
		newPixels.resize(newWidth * newHeight);
		unsigned pixelsToCopy_X = std::min(width, newWidth);
		unsigned pixelsToCopy_Y = std::min(height, newHeight);
		for (unsigned y = 0; y < pixelsToCopy_Y; ++y) {
			std::memcpy(&newPixels[y * newWidth], &pixels[y * width], pixelsToCopy_X * pixelByteSize(Pixel::FMT_ID));
		}
		pixels = std::move(newPixels);
		width = newWidth;
		height = newHeight;
		recalculateDimensions();
		return true;
	}

	/**
	 * @brief Returns a reference to the vector of pixels.
	 *
	 * @return A reference to the vector of pixels.
	 */
	std::pmr::vector<Pixel>& getPixels() {
		return pixels;
	}

	/**
	 * @brief Returns a constant reference to the vector of pixels.
	 *
	 * @return A constant reference to the vector of pixels.
	 */
	const std::pmr::vector<Pixel>& getPixels() const {
		return pixels;
	}

	/**
	 * @brief Returns the memory resource used by the pixel vector.
	 *
	 * @return A pointer to the memory resource.
	 */
	std::pmr::memory_resource* getMemRes() const {
		return memRes;
	}

	/**
	 * @brief Constructs a `StandardImage` with specified dimensions.
	 *
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource to use for pixel storage.
	 */
	StandardImage(unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: pixels(memRes), memRes(memRes) {
		this->pixels.resize(height * width);
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}

	/**
	 * @brief Constructs a `StandardImage` from a span of pixels.
	 *
	 * @param pixels A span of pixel data.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource to use for pixel storage.
	 */
	StandardImage(const std::span<const Pixel>& pixels, unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: pixels(memRes), memRes(memRes) {
		this->pixels.resize(pixels.size());
		std::memcpy(this->pixels.data(), pixels.data(), pixels.size_bytes());
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}

	/**
	 * @brief Copy constructor for `StandardImage`.
	 *
	 * @param cpy The image to copy.
	 * */
	StandardImage(const StandardImage& cpy)
		: pixels(cpy.pixels), memRes(cpy.memRes) {
		IReadOnlyImage2D::operator=(cpy);
	}
	/**
	 * @brief Copy assignment operator.
	 * @param cpy The image to copy.
	 * @return A reference to this image.
	 */
	StandardImage& operator=(const StandardImage& cpy)
	{
		this->memRes = cpy.memRes;
		this->pixels = cpy.pixels;
		IReadOnlyImage2D::operator=(cpy);
		return *this;
	}
	/**
	 * @brief Move constructor for `StandardImage`.
	 *
	 * @param mov The image to move.
	 * */
	StandardImage(StandardImage&& mov)
		: pixels(std::move(mov.pixels)), memRes(mov.memRes) {
		this->width = mov.width;
		this->height = mov.height;
		recalculateDimensions();
	}
	/**
	 * @brief Move assignment operator.
	 * @param mov The image to move.
	 * @return A reference to this image.
	 */
	StandardImage& operator=(StandardImage&& mov)
	{
		this->memRes = mov.memRes;
		this->pixels = std::move(mov.pixels);
		this->width = mov.width;
		this->height = mov.height;
		recalculateDimensions();
		return *this;
	}
};


/**
 * @brief Helper functions for palette-based image operations.
 *
 * This struct contains static methods to convert palettized image data into a full-color image.
 * The struct is templated on a pixel type, which must conform to the PixelConcept.
 * The methods use memory resource (`std::pmr::memory_resource`) for memory allocation.
 *
 * @tparam Pixel The type of pixel that the images use, must satisfy PixelConcept.
 */
template <PixelConcept Pixel> struct PaletteImplementationHelpers {
	/**
	 * @brief Depalettizes an image using shared ownership.
	 *
	 * This function takes a palette and indices representing a palettized image, and converts it into a full-color image.
	 * The resulting image is managed using shared ownership (`std::shared_ptr`).
	 *
	 * @param palette A span of pixels representing the color palette.
	 * @param indices A span of indices representing the palettized image data.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource to use for allocations. Defaults to the default memory resource.
	 * @return A shared pointer to the depalettized image.
	 */
	static sIResizeableImage2D depalettizeS(
		const std::span<const Pixel>& palette,
		const std::span<const uint8_t>& indices,
		unsigned int width,
		unsigned int height,
		std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
	{
		auto toReturn = Elv::Util::pmr_make_shared<StandardImage<Pixel>>(memRes, width, height, memRes);
		std::pmr::vector<Pixel>& pixelsToSet = toReturn->getPixels();
		Elv::Util::over_2d_spans<uint8_t,Pixel>(
			indices,
			std::span<Pixel>(pixelsToSet),
			[&palette](const uint8_t& index, Pixel& pixel, const glm::uvec2& pos) {
				(void)pos;
				pixel = palette[index];
			},
			glm::uvec2(width, height),
			glm::uvec2(0, 0),
			glm::uvec2(width, height)
			);
		return toReturn;
	}

	/**
	 * @brief Depalettizes an image using unique ownership.
	 *
	 * This function takes a palette and indices representing a palettized image, and converts it into a full-color image.
	 * The resulting image is managed using unique ownership (`std::unique_ptr`).
	 *
	 * @param palette A span of pixels representing the color palette.
	 * @param indices A span of indices representing the palettized image data.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource to use for allocations. Defaults to the default memory resource.
	 * @return A unique pointer to the depalettized image.
	 */
	static uIResizeableImage2D depalettizeU(
		const std::span<const Pixel>& palette,
		const std::span<const uint8_t>& indices,
		unsigned int width,
		unsigned int height,
		std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
	{
		auto toReturn = Elv::Util::pmr_make_unique<StandardImage<Pixel>>(memRes, width, height, memRes);
		std::pmr::vector<Pixel>& pixelsToSet = toReturn->getPixels();
		Elv::Util::over_2d_spans<uint8_t,Pixel>(
			indices,
			std::span<Pixel>(pixelsToSet),
			[&palette](const uint8_t& index, Pixel& pixel, const glm::uvec2& pos) {
				(void)pos;
				pixel = palette[index];
			},
			glm::uvec2(width, height),
			glm::uvec2(0, 0),
			glm::uvec2(width, height)
			);
		return toReturn;
	}
};

/**
 * @brief A read-only reference to a palette.
 *
 * This class provides a read-only interface to a palette of colors, implementing the `IReadOnlyPalette` interface.
 * It uses a `std::span` to reference the palette data, ensuring efficient and safe access.
 *
 * @tparam Pixel The type of pixel that the palette uses, must satisfy PixelConcept.
 */
template <PixelConcept Pixel> class ReadOnlyReferencePalette : public IReadOnlyPalette {
private:
	/**
	 * @brief The span of colors representing the palette.
	 */
	std::span<const Pixel> colours;

public:
	/**
	 * @brief Retrieves a color from the palette.
	 *
	 * This method retrieves the color at the specified index and converts it to a kernel format.
	 *
	 * @param index The index of the color in the palette.
	 * @param kernel The output kernel vector to store the color.
	 */
	void getColour(uint8_t index, glm::fvec4& kernel) const override
	{
		colours[index].toKernel(kernel);
	}

	/**
	 * @brief Gets the format of the palette.
	 *
	 * This method returns the format identifier of the pixel type used in the palette.
	 *
	 * @return The format identifier of the pixel type.
	 */
	Format getFormat() const override
	{
		return Pixel::FMT_ID;
	}

	/**
	 * @brief Retrieves the raw color data of the palette.
	 *
	 * This method returns a span of bytes representing the raw color data in the palette.
	 *
	 * @return A span of bytes containing the raw color data.
	 */
	const std::span<const std::byte> getRawColours() const override
	{
		return Elv::Util::as_const_byte_span(colours);
	}

	/**
	 * @brief Depalettizes an image using shared ownership.
	 *
	 * This function takes indices representing a palettized image and converts it into a full-color image.
	 * The resulting image is managed using shared ownership (`std::shared_ptr`).
	 *
	 * @param indices A span of indices representing the palettized image data.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource to use for allocations. Defaults to the default memory resource.
	 * @return A shared pointer to the depalettized image.
	 */
	sIResizeableImage2D depalettizeS(
		const std::span<const uint8_t>& indices,
		unsigned int width,
		unsigned int height,
		std::pmr::memory_resource* memRes = std::pmr::get_default_resource()) const override
	{
		return PaletteImplementationHelpers<Pixel>::depalettizeS(colours, indices, width, height, memRes);
	}

	/**
	 * @brief Depalettizes an image using unique ownership.
	 *
	 * This function takes indices representing a palettized image and converts it into a full-color image.
	 * The resulting image is managed using unique ownership (`std::unique_ptr`).
	 *
	 * @param indices A span of indices representing the palettized image data.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource to use for allocations. Defaults to the default memory resource.
	 * @return A unique pointer to the depalettized image.
	 */
	uIResizeableImage2D depalettizeU(
		const std::span<const uint8_t>& indices,
		unsigned int width,
		unsigned int height,
		std::pmr::memory_resource* memRes = std::pmr::get_default_resource()) const override
	{
		return PaletteImplementationHelpers<Pixel>::depalettizeU(colours, indices, width, height, memRes);
	}

	/**
	 * @brief Gets the span of colors in the palette.
	 *
	 * This method returns the span of colors that the palette references.
	 *
	 * @return A span of colors representing the palette.
	 */
	std::span<const Pixel> getColours() const { return colours; }

	/**
	 * @brief Copy constructor.
	 *
	 * This constructor creates a copy of the given `ReadOnlyReferencePalette` object.
	 *
	 * @param cpy The object to copy.
	 */
	ReadOnlyReferencePalette(const ReadOnlyReferencePalette& cpy) : colours(cpy.colours) {}

	/**
	 * @brief Copy assignment operator.
	 *
	 * This operator assigns the contents of the given `ReadOnlyReferencePalette` object to the current object.
	 *
	 * @param cpy The object to copy.
	 * @return A reference to the current object.
	 */
	ReadOnlyReferencePalette& operator=(const ReadOnlyReferencePalette& cpy) {
		colours = cpy.colours;
		return *this;
	}

	/**
	 * @brief Constructor from a span of colors.
	 *
	 * This constructor initializes the palette with a given span of colors.
	 *
	 * @param colours The span of colors to use for the palette.
	 */
	explicit ReadOnlyReferencePalette(const std::span<const Pixel>& colours) : colours(colours) {}

	/**
	 * @brief Assignment operator from a span of colors.
	 *
	 * This operator assigns the given span of colors to the current palette.
	 *
	 * @param colours The span of colors to use for the palette.
	 * @return A reference to the current object.
	 */
	ReadOnlyReferencePalette& operator=(const std::span<const Pixel>& colours) {
		this->colours = colours;
		return *this;
	}
};

/**
 * @brief A class representing a reference-based palette.
 *
 * This class implements the `IMutablePalette` interface and manages a palette of colors using a `std::span`.
 * It provides methods to get and set colors, as well as to depalettize images.
 *
 * @tparam Pixel The type of pixel that the palette uses, must satisfy PixelConcept.
 */
template <PixelConcept Pixel> class ReferencePalette : public IMutablePalette {
private:
	std::span<Pixel> colours;  /**< The span of colors managed by the palette. */

public:
	/**
	 * @brief Retrieves a color from the palette.
	 *
	 * This method converts the color at the specified index to a `glm::fvec4` format.
	 *
	 * @param index The index of the color to retrieve.
	 * @param kernel The `glm::fvec4` variable to store the retrieved color.
	 */
	void getColour(uint8_t index, glm::fvec4& kernel) const override {
		colours[index].toKernel(kernel);
	}

	/**
	 * @brief Retrieves the format of the pixels in the palette.
	 *
	 * @return The format of the pixels.
	 */
	Format getFormat() const override {
		return Pixel::FMT_ID;
	}

	/**
	 * @brief Retrieves the raw color data as a constant byte span.
	 *
	 * @return A constant byte span of the raw color data.
	 */
	const std::span<const std::byte> getRawColours() const override {
		return Elv::Util::as_const_byte_span(colours);
	}

	/**
	 * @brief Retrieves the raw color data as a mutable byte span.
	 *
	 * @return A mutable byte span of the raw color data.
	 */
	std::span<const std::byte> getRawColours() override {
		return Elv::Util::as_byte_span(colours);
	}

	/**
	 * @brief Depalettizes an image using shared ownership.
	 *
	 * This function takes indices representing a palettized image and converts it into a full-color image using shared ownership.
	 *
	 * @param indices A span of indices representing the palettized image data.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource to use for allocations.
	 * @return A shared pointer to the depalettized image.
	 */
	sIResizeableImage2D depalettizeS(const std::span<const uint8_t>& indices, unsigned int width, unsigned int height, std::pmr::memory_resource* memRes) const override {
		return PaletteImplementationHelpers<Pixel>::depalettizeS(colours, indices, width, height, memRes);
	}

	/**
	 * @brief Depalettizes an image using unique ownership.
	 *
	 * This function takes indices representing a palettized image and converts it into a full-color image using unique ownership.
	 *
	 * @param indices A span of indices representing the palettized image data.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource to use for allocations.
	 * @return A unique pointer to the depalettized image.
	 */
	uIResizeableImage2D depalettizeU(const std::span<const uint8_t>& indices, unsigned int width, unsigned int height, std::pmr::memory_resource* memRes) const override {
		return PaletteImplementationHelpers<Pixel>::depalettizeU(colours, indices, width, height, memRes);
	}

	/**
	 * @brief Sets a color in the palette.
	 *
	 * This method converts a `glm::fvec4` color to the pixel format and sets it at the specified index.
	 *
	 * @param index The index of the color to set.
	 * @param colour The `glm::fvec4` color to set.
	 */
	void setColour(uint8_t index, const glm::fvec4& colour) override {
		colours[index].fromKernel(colour);
	}

	/**
	 * @brief Retrieves the span of colors in the palette as a constant span.
	 *
	 * @return A constant span of the colors.
	 */
	std::span<const Pixel> getColours() const {
		return colours;
	}

	/**
	 * @brief Retrieves the span of colors in the palette as a mutable span.
	 *
	 * @return A mutable span of the colors.
	 */
	std::span<Pixel> getColours() {
		return colours;
	}

	/**
	 * @brief Copy constructor.
	 *
	 * @param cpy The `ReferencePalette` object to copy.
	 */
	ReferencePalette(const ReferencePalette& cpy) : colours(cpy.colours) {
	}

	/**
	 * @brief Copy assignment operator.
	 *
	 * @param cpy The `ReferencePalette` object to copy.
	 * @return A reference to the current object.
	 */
	ReferencePalette& operator=(const ReferencePalette& cpy) {
		colours = cpy.colours;
		return *this;
	}

	/**
	 * @brief Constructor that initializes the palette with a span of colors.
	 *
	 * @param colours The span of colors to initialize the palette with.
	 */
	explicit ReferencePalette(std::span<Pixel> colours) : colours(colours) {
	}

	/**
	 * @brief Assignment operator that initializes the palette with a span of colors.
	 *
	 * @param colours The span of colors to initialize the palette with.
	 * @return A reference to the current object.
	 */
	ReferencePalette& operator=(std::span<Pixel> colours) {
		this->colours = colours;
		return *this;
	}
};

/**
 * @brief A full palette implementation managing 256 colors.
 *
 * This class implements the `IMutablePalette` interface and manages a palette of 256 colors.
 * Each color is represented by a pixel type that must conform to the `PixelConcept`.
 * The class provides methods to get and set colors, get the format of the palette, and depalettize images.
 *
 * @tparam Pixel The type of pixel that the palette uses, must satisfy PixelConcept.
 */
template <PixelConcept Pixel> class FullPalette : public IMutablePalette {
private:
	/// @brief An array of 256 colors, each represented by a pixel.
	std::array<Pixel, 256> colours;

public:
	/**
	 * @brief Gets the color at a specific index.
	 *
	 * This function retrieves the color at the specified index and stores it in the provided `glm::fvec4` kernel.
	 *
	 * @param index The index of the color to retrieve (0-255).
	 * @param kernel The `glm::fvec4` to store the retrieved color.
	 */
	void getColour(uint8_t index, glm::fvec4& kernel) const override {
		colours[index].toKernel(kernel);
	}

	/**
	 * @brief Gets the format of the palette.
	 *
	 * This function returns the format identifier of the pixel type used in the palette.
	 *
	 * @return The format identifier of the pixel type.
	 */
	Format getFormat() const override {
		return Pixel::FMT_ID;
	}

	/**
	 * @brief Gets the raw bytes of the palette (const version).
	 *
	 * This function returns a span of bytes representing the raw data of the palette.
	 * The span is const, meaning the data cannot be modified.
	 *
	 * @return A const span of bytes.
	 */
	const std::span<const std::byte> getRawColours() const override {
		return Elv::Util::as_const_byte_span(colours);
	}

	/**
	 * @brief Gets the raw bytes of the palette (non-const version).
	 *
	 * This function returns a span of bytes representing the raw data of the palette.
	 * The span is non-const, meaning the data can be modified.
	 *
	 * @return A span of bytes.
	 */
	std::span<const std::byte> getRawColours() override {
		return Elv::Util::as_byte_span(colours);
	}

	/**
	 * @brief Depalettizes an image using shared ownership.
	 *
	 * This function takes indices representing a palettized image and converts it into a full-color image.
	 * The resulting image is managed using shared ownership (`std::shared_ptr`).
	 *
	 * @param indices A span of indices representing the palettized image data.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource to use for allocations.
	 * @return A shared pointer to the depalettized image.
	 */
	sIResizeableImage2D depalettizeS(const std::span<const uint8_t>& indices, unsigned int width, unsigned int height, std::pmr::memory_resource* memRes) const override {
		return PaletteImplementationHelpers<Pixel>::depalettizeS(colours, indices, width, height, memRes);
	}

	/**
	 * @brief Depalettizes an image using unique ownership.
	 *
	 * This function takes indices representing a palettized image and converts it into a full-color image.
	 * The resulting image is managed using unique ownership (`std::unique_ptr`).
	 *
	 * @param indices A span of indices representing the palettized image data.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource to use for allocations.
	 * @return A unique pointer to the depalettized image.
	 */
	uIResizeableImage2D depalettizeU(const std::span<const uint8_t>& indices, unsigned int width, unsigned int height, std::pmr::memory_resource* memRes) const override {
		return PaletteImplementationHelpers<Pixel>::depalettizeU(colours, indices, width, height, memRes);
	}

	/**
	 * @brief Sets the color at a specific index.
	 *
	 * This function sets the color at the specified index using the provided `glm::fvec4` color.
	 *
	 * @param index The index of the color to set (0-255).
	 * @param colour The `glm::fvec4` representing the new color.
	 */
	void setColour(uint8_t index, const glm::fvec4& colour) override {
		colours[index].fromKernel(colour);
	}

	/**
	 * @brief Gets the array of colors (const version).
	 *
	 * This function returns a const reference to the array of colors in the palette.
	 *
	 * @return A const reference to the array of colors.
	 */
	const std::array<Pixel, 256>& getColours() const {
		return colours;
	}

	/**
	 * @brief Gets the array of colors (non-const version).
	 *
	 * This function returns a non-const reference to the array of colors in the palette.
	 *
	 * @return A non-const reference to the array of colors.
	 */
	std::array<Pixel, 256>& getColours() {
		return colours;
	}

	/**
	 * @brief Default constructor.
	 *
	 * Initializes a new `FullPalette` with default-constructed colors.
	 */
	FullPalette() {
	}

	/**
	 * @brief Constructor with initial colors.
	 *
	 * Initializes a new `FullPalette` with the provided colors.
	 *
	 * @param colours A span of colors to initialize the palette with.
	 */
	explicit FullPalette(const std::span<const Pixel>& colours) {
		std::copy(std::begin(colours), std::end(colours), std::begin(this->colours));
	}
};

}
}
}
#endif // EUPHSTANDARDIMAGE_HPP
