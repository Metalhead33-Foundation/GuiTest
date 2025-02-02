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
uResizeableImage2D MH_EUPH_API createUptrResizeableImage2D(const std::span<const std::byte>& bytes, Format format, unsigned width, unsigned height,
														   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
uResizeableImage2D MH_EUPH_API createUptrResizeableImage2D(const Frame& frame, Format format,
														   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
uResizeableImage2D MH_EUPH_API createUptrResizeableImage2D(Format format, unsigned width, unsigned height,
														   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sResizeableImage2D MH_EUPH_API createSptrResizeableImage2D(const std::span<const std::byte>& bytes, Format format, unsigned width, unsigned height,
														   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sResizeableImage2D MH_EUPH_API createSptrResizeableImage2D(const Frame& frame, Format format,
														   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sResizeableImage2D MH_EUPH_API createSptrResizeableImage2D(Format format, unsigned width, unsigned height,
														   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
// Reference image
uImage2D MH_EUPH_API createUptrReferenceImage2D(std::span<std::byte> bytes, Format format, unsigned width, unsigned height,
									   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
uImage2D MH_EUPH_API createUptrReferenceImage2D(Frame& frame, Format format,
									   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sImage2D MH_EUPH_API createSptrReferenceImage2D(std::span<std::byte> bytes, Format format, unsigned width, unsigned height,
									   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sImage2D MH_EUPH_API createSptrReferenceImage2D(Frame& frame, Format format,
									   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
// Read-OnlyReference image
uReadOnlyImage2D MH_EUPH_API createUptrReadOnlyReferenceImage2D(const std::span<const std::byte>& bytes, Format format, unsigned width, unsigned height,
												std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
uReadOnlyImage2D MH_EUPH_API createUptrRReadOnlyeferenceImage2D(const Frame& frame, Format format,
												std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sReadOnlyImage2D MH_EUPH_API createSptrReadOnlyReferenceImage2D(const std::span<const std::byte>& bytes, Format format, unsigned width, unsigned height,
												std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
sReadOnlyImage2D MH_EUPH_API createSptrReadOnlyReferenceImage2D(const Frame& frame, Format format,
												std::pmr::memory_resource* memRes = std::pmr::get_default_resource());

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
	typedef std::span<Pixel> PixelStorage;    /**
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
		case Euph::Media::Image::REPEAT:
		{
			x = pos.x % width;
			y = pos.y % height;
			break;
		}
		case Euph::Media::Image::MIRRORED_REPEAT:
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
		case Euph::Media::Image::CLAMP_TO_BORDER: // Can't tell the difference between the two
		case Euph::Media::Image::CLAMP_TO_EDGE:
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
	static inline void iterateOverPixels(const ReadOnlyPixelStorage& pixels, unsigned width, unsigned height, const ReadOnlyImage2D::ColourIterator& program)
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
	static inline void iterateOverPixels(const ReadOnlyPixelStorage& pixels, unsigned width, unsigned height, float widthR, float heightR, const ReadOnlyImage2D::ColourIterator2& program)
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
	static inline void iterateOverPixels(const ReadOnlyPixelStorage& pixels, unsigned width, unsigned height, const ReadOnlyImage2D::ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions)
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
	static inline void iterateOverPixels(const ReadOnlyPixelStorage& pixels, unsigned width, unsigned height, float widthR, float heightR, const ReadOnlyImage2D::ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions)
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
	static inline void clearToColour(PixelStorage pixels, unsigned width, unsigned height, const Image2D::ColourProgrammer& program, bool dither)
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
	static inline void clearToColour(PixelStorage pixels, unsigned width, unsigned height, const Image2D::ColourProgrammer2& program, bool dither)
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
	static inline void clearToColour(PixelStorage pixels, unsigned width, unsigned height, float widthR, float heightR, const Image2D::ColourProgrammer3& program, bool dither)
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
	static inline void clearToColour(PixelStorage pixels,unsigned width, unsigned height, float widthR, float heightR, const Image2D::ColourProgrammer4& program, bool dither)
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
	static inline void clearToColour(PixelStorage pixels, unsigned width, unsigned height, const Image2D::ColourProgrammer& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither)
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
	static inline void clearToColour(PixelStorage pixels, unsigned width, unsigned height, const Image2D::ColourProgrammer2& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither)
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
	static inline void clearToColour(PixelStorage pixels,unsigned width, unsigned height, float widthR, float heightR, const Image2D::ColourProgrammer3& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither)
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
	static inline void clearToColour(PixelStorage pixels,unsigned width, unsigned height, float widthR, float heightR, const Image2D::ColourProgrammer4& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither)
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
		case Euph::Media::Image::REPEAT:
		{
			x = pos.x % width;
			y = pos.y % height;
			break;
		}
		case Euph::Media::Image::MIRRORED_REPEAT:
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
		case Euph::Media::Image::CLAMP_TO_BORDER: // Can't tell the difference between the two
		case Euph::Media::Image::CLAMP_TO_EDGE:
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
	static inline void iterateOverPixels(const Palette& palette, const PixelStorage& pixels, unsigned width, unsigned height, const ReadOnlyImage2D::ColourIterator& program)
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
	static inline void iterateOverPixels(const Palette& palette, const PixelStorage& pixels, unsigned width, unsigned height, float widthR, float heightR, const ReadOnlyImage2D::ColourIterator2& program)
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
										 const ReadOnlyImage2D::ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions)
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
	static inline void iterateOverPixels(const Palette& palette, const PixelStorage& pixels, unsigned width, unsigned height, float widthR, float heightR, const ReadOnlyImage2D::ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions)
	{
		Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program,&palette,widthR,heightR](const Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 kernel;
				palette[pxl].toKernel(kernel);
				program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR),kernel);
			}, offset,dimensions);
	}
};

/**
 * @brief A read-only 2D image class that uses a palette for pixel color representation.
 *
 * This class represents a 2D image where each pixel is an index into a palette of colors.
 * The image data is stored as a span of pixel indices, and the corresponding colors are stored in a palette.
 * The class provides methods to retrieve the image format, pixel color, raw pixel data, and to iterate over pixels.
 *
 * @tparam Pixel The type of pixel in the palette. Must satisfy the PixelConcept.
 */
template <PixelConcept Pixel>
class ReadOnlyPalettedImage : public ReadOnlyImage2D {
public:
	/**
	 * @brief A type alias for the palette, which is a span of constant Pixel elements.
	 */
	typedef std::span<const Pixel> Palette;

private:
	/**
	 * @brief The palette of colors used by the image.
	 */
	Palette palette;

	/**
	 * @brief The image data, stored as a span of constant uint8_t elements representing pixel indices.
	 */
	std::span<const uint8_t> pixels;

public:
	/**
	 * @brief Returns the format of the image.
	 *
	 * @return Format::INDEXED indicating that the image uses a palette for color representation.
	 */
	Format getFormat() const override {
		return Format::INDEXED;
	}

	/**
	 * @brief Retrieves the color of a pixel at a given position.
	 *
	 * @param pos The position of the pixel in the image.
	 * @param colourKernel The output color kernel to be filled with the pixel's color.
	 * @param wrap The wrapping mode to use if the position is out of bounds.
	 */
	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override {
		PalettedImageImplementationHelpers<Pixel>::getPixel(palette, pixels, width, height, pos, colourKernel, wrap);
	}

	/**
	 * @brief Returns a pointer to the raw pixel data.
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
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette, pixels, width, height, program);
	}

	/**
	 * @brief Iterates over all pixels in the image and applies a color program.
	 *
	 * @param program The color program to apply to each pixel.
	 */
	void iterateOverPixels(const ColourIterator2& program) const override {
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette, pixels, width, height, widthR, heightR, program);
	}

	/**
	 * @brief Iterates over a subset of pixels in the image and applies a color program.
	 *
	 * @param program The color program to apply to each pixel.
	 * @param offset The offset from the top-left corner of the image to start iteration.
	 * @param dimensions The dimensions of the sub-region to iterate over.
	 */
	void iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override {
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette, pixels, width, height, program, offset, dimensions);
	}

	/**
	 * @brief Iterates over a subset of pixels in the image and applies a color program.
	 *
	 * @param program The color program to apply to each pixel.
	 * @param offset The offset from the top-left corner of the image to start iteration.
	 * @param dimensions The dimensions of the sub-region to iterate over.
	 */
	void iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override {
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette, pixels, width, height, widthR, heightR, program, offset, dimensions);
	}

	/**
	 * @brief Returns a constant reference to the palette.
	 *
	 * @return A constant reference to the palette.
	 */
	const Palette& getPalette() const {
		return palette;
	}

	/**
	 * @brief Returns a modifiable reference to the palette.
	 *
	 * @return A modifiable reference to the palette.
	 */
	Palette& getPalette() {
		return palette;
	}

	/**
	 * @brief Sets the palette to a new value.
	 *
	 * @param nPalette The new palette to set.
	 */
	void setPalette(const Palette& nPalette) {
		palette = nPalette;
	}

	/**
	 * @brief Returns a constant span of the pixel data.
	 *
	 * @return A constant span of the pixel data.
	 */
	std::span<const uint8_t> getPixels() const {
		return pixels;
	}

	/**
	 * @brief Constructor that initializes the image with a palette, pixel data, width, and height.
	 *
	 * @param palette The palette of colors for the image.
	 * @param pixels The pixel data, represented as a span of uint8_t indices into the palette.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 */
	ReadOnlyPalettedImage(const Palette& palette, const std::span<const uint8_t>& pixels, unsigned width, unsigned height)
		: palette(palette), pixels(pixels) {
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}

	/**
	 * @brief Copy constructor.
	 *
	 * @param cpy The ReadOnlyPalettedImage instance to copy from.
	 */
	ReadOnlyPalettedImage(const ReadOnlyPalettedImage& cpy)
		: palette(cpy.palette), pixels(cpy.pixels) {
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
	}

	/**
	 * @brief Copy assignment operator.
	 *
	 * @param cpy The ReadOnlyPalettedImage instance to copy from.
	 * @return A reference to the modified instance.
	 */
	ReadOnlyPalettedImage& operator=(const ReadOnlyPalettedImage& cpy) {
		this->palette = cpy.palette;
		this->pixels = cpy.pixels;
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
		return *this;
	}
};

/**
 * @brief A class representing a read-only 2D paletted image.
 *
 * This class inherits from `ReadOnlyImage2D` and provides functionality
 * to handle paletted images where each pixel is an index into a palette
 * of colors.
 *
 * @tparam Pixel The type of the pixel in the palette.
 */
template <PixelConcept Pixel>
class ReferencePalettedImage : public ReadOnlyImage2D {
public:
	/**
	 * @brief Type alias for the palette, which is a span of constant pixels.
	 */
	typedef std::span<const Pixel> Palette;

private:
	/**
	 * @brief The palette containing the colors.
	 */
	Palette palette;

	/**
	 * @brief The span of pixel indices.
	 */
	std::span<uint8_t> pixels;

public:
	/**
	 * @brief Returns the format of the image.
	 *
	 * @return The format of the image, which is `Format::INDEXED`.
	 */
	Format getFormat() const override {
		return Format::INDEXED;
	}

	/**
	 * @brief Retrieves the color of a pixel at a given position.
	 *
	 * @param pos The position of the pixel.
	 * @param colourKernel The output color in RGBA format.
	 * @param wrap The wrapping mode for out-of-bounds access.
	 */
	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override {
		PalettedImageImplementationHelpers<Pixel>::getPixel(palette, pixels, width, height, pos, colourKernel, wrap);
	}

	/**
	 * @brief Returns a pointer to the raw pixel data.
	 *
	 * @return A pointer to the raw pixel data.
	 */
	const void* getRawPixels() const override {
		return pixels.data();
	}

	/**
	 * @brief Iterates over all pixels in the image using a provided color iterator.
	 *
	 * @param program The color iterator to apply to each pixel.
	 */
	void iterateOverPixels(const ColourIterator& program) const override {
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette, pixels, width, height, program);
	}

	/**
	 * @brief Iterates over all pixels in the image using a provided color iterator.
	 *
	 * @param program The color iterator to apply to each pixel.
	 */
	void iterateOverPixels(const ColourIterator2& program) const override {
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette, pixels, width, height, widthR, heightR, program);
	}

	/**
	 * @brief Iterates over a subset of pixels in the image using a provided color iterator.
	 *
	 * @param program The color iterator to apply to each pixel.
	 * @param offset The offset from which to start iterating.
	 * @param dimensions The dimensions of the area to iterate over.
	 */
	void iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override {
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette, pixels, width, height, program, offset, dimensions);
	}

	/**
	 * @brief Iterates over a subset of pixels in the image using a provided color iterator.
	 *
	 * @param program The color iterator to apply to each pixel.
	 * @param offset The offset from which to start iterating.
	 * @param dimensions The dimensions of the area to iterate over.
	 */
	void iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override {
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette, pixels, width, height, widthR, heightR, program, offset, dimensions);
	}

	/**
	 * @brief Returns a constant reference to the palette.
	 *
	 * @return A constant reference to the palette.
	 */
	const Palette& getPalette() const {
		return palette;
	}

	/**
	 * @brief Returns a reference to the palette.
	 *
	 * @return A reference to the palette.
	 */
	Palette& getPalette() {
		return palette;
	}

	/**
	 * @brief Sets a new palette for the image.
	 *
	 * @param nPalette The new palette to set.
	 */
	void setPalette(const Palette& nPalette) {
		palette = nPalette;
	}

	/**
	 * @brief Returns a span of the pixel indices.
	 *
	 * @return A span of the pixel indices.
	 */
	std::span<uint8_t> getPixels() {
		return pixels;
	}

	/**
	 * @brief Returns a constant span of the pixel indices.
	 *
	 * @return A constant span of the pixel indices.
	 */
	std::span<const uint8_t> getPixels() const {
		return pixels;
	}

	/**
	 * @brief Constructor for the ReferencePalettedImage.
	 *
	 * @param palette The palette of colors.
	 * @param pixels The span of pixel indices.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 */
	ReferencePalettedImage(const Palette& palette, const std::span<uint8_t>& pixels, unsigned width, unsigned height)
		: palette(palette), pixels(pixels) {
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}

	/**
	 * @brief Copy constructor for the ReferencePalettedImage.
	 *
	 * @param cpy The image to copy from.
	 */
	ReferencePalettedImage(const ReferencePalettedImage& cpy)
		: palette(cpy.palette), pixels(cpy.pixels) {
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
	}

	/**
	 * @brief Assignment operator for the ReferencePalettedImage.
	 *
	 * @param cpy The image to assign from.
	 * @return A reference to the assigned image.
	 */
	ReferencePalettedImage& operator=(const ReferencePalettedImage& cpy) {
		this->palette = cpy.palette;
		this->pixels = cpy.pixels;
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
		return *this;
	}
};

/**
 * @class PalettedImage
 * @brief A class representing a 2D image stored in a paletted format.
 *
 * @tparam Pixel The type of pixel in the palette.
 */
template <PixelConcept Pixel> class PalettedImage : public ReadOnlyImage2D {
public:
	/**
	 * @brief Alias for the palette type, which is a span of constant pixels.
	 */
	typedef std::span<const Pixel> Palette;
private:
	Palette palette;                  ///< The palette containing the colors.
	std::pmr::vector<uint8_t> pixels; ///< The indexed pixels of the image.
	std::pmr::memory_resource* memRes; ///< The memory resource used for allocation.
public:
	/**
	 * @brief Retrieves the format of the image.
	 * @return The format, which is always `Format::INDEXED` for this class.
	 */
	Format getFormat() const override
	{
		return Format::INDEXED;
	}
	/**
	 * @brief Retrieves the color at a specified position using the palette.
	 * @param pos The position of the pixel.
	 * @param colourKernel The output color (in RGBA format).
	 * @param wrap The wrapping mode to use for out-of-bounds positions.
	 */
	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override
	{
		PalettedImageImplementationHelpers<Pixel>::getPixel(palette,pixels,width,height,pos,colourKernel,wrap);
	}
	/**
	 * @brief Retrieves the raw pixel data.
	 * @return A pointer to the raw pixel data.
	 */
	const void* getRawPixels() const override
	{
		return pixels.data();
	}
	/**
	 * @brief Iterates over all pixels and applies a given program.
	 * @param program The function to execute for each pixel.
	 */
	void iterateOverPixels(const ColourIterator& program) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,program);
	}
	/**
	 * @brief Iterates over all pixels with additional dimensions and applies a given program.
	 * @param program The function to execute for each pixel.
	 */
	void iterateOverPixels(const ColourIterator2& program) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,widthR,heightR,program);
	}
	/**
	 * @brief Iterates over a subsection of the image and applies a given program.
	 * @param program The function to execute for each pixel.
	 * @param offset The starting offset of the subsection.
	 * @param dimensions The dimensions of the subsection.
	 */
	void iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,program,offset,dimensions);
	}
	/**
	 * @brief Iterates over a subsection of the image with additional dimensions and applies a given program.
	 * @param program The function to execute for each pixel.
	 * @param offset The starting offset of the subsection.
	 * @param dimensions The dimensions of the subsection.
	 */
	void iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,widthR,heightR,program,offset,dimensions);
	}
	/**
	 * @brief Gets the current palette.
	 * @return A constant reference to the palette.
	 */
	const Palette& getPalette() const { return palette; }

	/**
	 * @brief Gets the current palette.
	 * @return A reference to the palette.
	 */
	Palette& getPalette() { return palette; }
	/**
	 * @brief Sets a new palette.
	 * @param nPalette The new palette to set.
	 */
	void setPalette(const Palette& nPalette) { palette = nPalette; }
	/**
	 * @brief Gets the pixel data.
	 * @return A reference to the pixel data vector.
	 */
	std::pmr::vector<uint8_t>& getPixels() { return pixels; }
	/**
	 * @brief Gets the pixel data.
	 * @return A constant reference to the pixel data vector.
	 */
	const std::pmr::vector<uint8_t>& getPixels() const { return pixels; }
	/**
	 * @brief Gets the memory resource used by the image.
	 * @return A pointer to the memory resource.
	 */
	std::pmr::memory_resource* getMemRes() const { return memRes; }
	/**
	 * @brief Constructs an empty paletted image with specified dimensions.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource for allocation (default: system default).
	 */
	PalettedImage(unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: pixels(memRes), memRes(memRes) {
		this->pixels.resize(width * height);
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	/**
	 * @brief Constructs a paletted image with the specified palette and dimensions.
	 * @param palette The palette for the image.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource for allocation (default: system default).
	 */
	PalettedImage(const Palette& palette, unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: palette(palette), pixels(memRes), memRes(memRes) {
		this->pixels.resize(width * height);
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	/**
	 * @brief Constructs a paletted image with the specified palette and raw pixel data.
	 * @param palette The palette for the image.
	 * @param pixels A span of raw pixel data.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource for allocation (default: system default).
	 */
	PalettedImage(const Palette& palette, const std::span<const uint8_t>& pixels, unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: palette(palette), pixels(memRes), memRes(memRes) {
		this->pixels.resize(pixels.size());
		std::memcpy(this->pixels.data(),pixels.data(),pixels.size_bytes());
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	/**
	 * @brief Copy constructor.
	 * @param cpy The image to copy from.
	 */
	PalettedImage(const PalettedImage& cpy)
		: palette(cpy.palette), pixels(cpy.pixels), memRes(cpy.memRes) {
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
	}
	/**
	 * @brief Copy assignment operator.
	 * @param cpy The image to copy from.
	 * @return A reference to this instance.
	 */
	PalettedImage& operator=(const PalettedImage& cpy)
	{
		this->memRes = cpy.memRes;
		this->palette = cpy.palette;
		this->pixels = cpy.pixels;
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
		return *this;
	}
	/**
	 * @brief Move constructor.
	 * @param mov The image to move from.
	 */
	PalettedImage(PalettedImage&& mov)
		: palette(mov.palette), pixels(std::move(mov.pixels)), memRes(mov.memRes) {
		this->width = mov.width;
		this->height = mov.height;
		recalculateDimensions();
	}
	/**
	 * @brief Move assignment operator.
	 * @param mov The image to move from.
	 * @return A reference to this instance.
	 */
	PalettedImage& operator=(PalettedImage&& mov)
	{
		this->memRes = mov.memRes;
		this->palette = mov.palette;
		this->pixels = std::move(mov.pixels);
		this->width = mov.width;
		this->height = mov.height;
		recalculateDimensions();
		return *this;
	}
};
/**
 * @class FullPalettedImage
 * @brief Represents a full paletted image with indexed pixel data.
 *
 * This class extends ReadOnlyImage2D and uses a palette to store color information.
 * Each pixel in the image is represented by an index into this palette.
 *
 * @tparam Pixel The type of the pixel data stored in the palette.
 */
template <PixelConcept Pixel> class FullPalettedImage : public ReadOnlyImage2D {
public:
	/**
	 * @typedef Palette
	 * @brief Type alias for the palette vector.
	 */
	typedef std::pmr::vector<Pixel> Palette;
private:
	Palette palette; ///< The palette containing color information.
	std::pmr::vector<uint8_t> pixels; ///< The pixel data, each element is an index into the palette.
	std::pmr::memory_resource* memRes; ///< The memory resource used for allocations.
public:
	/**
	 * @brief Get the format of the image.
	 * @return Format::INDEXED as this is a paletted image.
	 */
	Format getFormat() const override
	{
		return Format::INDEXED;
	}
	/**
	 * @brief Get the color of a pixel at a given position.
	 * @param pos The position of the pixel.
	 * @param colourKernel The output color in RGBA format.
	 * @param wrap The wrap mode for handling out-of-bounds positions.
	 */
	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override
	{
		PalettedImageImplementationHelpers<Pixel>::getPixel(palette,pixels,width,height,pos,colourKernel,wrap);
	}
	/**
	 * @brief Get a pointer to the raw pixel data.
	 * @return A constant pointer to the raw pixel data.
	 */
	const void* getRawPixels() const override
	{
		return pixels.data();
	}
	/**
	 * @brief Iterate over all pixels in the image using a given program.
	 * @param program The program to apply to each pixel.
	 */
	void iterateOverPixels(const ColourIterator& program) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,program);
	}
	/**
	 * @brief Iterate over all pixels in the image using a given program.
	 * @param program The program to apply to each pixel.
	 */
	void iterateOverPixels(const ColourIterator2& program) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,widthR,heightR,program);
	}
	/**
	 * @brief Iterate over a subset of pixels in the image using a given program.
	 * @param program The program to apply to each pixel.
	 * @param offset The starting offset for the iteration.
	 * @param dimensions The dimensions of the area to iterate over.
	 */
	void iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,program,offset,dimensions);
	}
	/**
	 * @brief Iterate over a subset of pixels in the image using a given program.
	 * @param program The program to apply to each pixel.
	 * @param offset The starting offset for the iteration.
	 * @param dimensions The dimensions of the area to iterate over.
	 */
	void iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,widthR,heightR,program,offset,dimensions);
	}
	/**
	 * @brief Get the palette of the image.
	 * @return A constant reference to the palette.
	 */
	const Palette& getPalette() const { return palette; }
	/**
	 * @brief Get the palette of the image.
	 * @return A reference to the palette.
	 */
	Palette& getPalette() { return palette; }
	/**
	 * @brief Set the palette of the image.
	 * @param nPalette The new palette to set.
	 */
	void setPalette(const Palette& nPalette) { palette = nPalette; }
	/**
	 * @brief Get the pixel data of the image.
	 * @return A reference to the pixel data.
	 */
	std::pmr::vector<uint8_t>& getPixels() { return pixels; }
	/**
	 * @brief Get the pixel data of the image.
	 * @return A constant reference to the pixel data.
	 */
	const std::pmr::vector<uint8_t>& getPixels() const { return pixels; }
	/**
	 * @brief Get the memory resource used by the image.
	 * @return A pointer to the memory resource.
	 */
	std::pmr::memory_resource* getMemRes() const { return memRes; }
	/**
	 * @brief Constructor to create an image with a specified width and height.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource to use for allocations (default is the default resource).
	 */
	FullPalettedImage(unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: pixels(memRes), memRes(memRes) {
		this->pixels.resize(width * height);
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	/**
	 * @brief Constructor to create an image with a specified width and height.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource to use for allocations (default is the default resource).
	 */
	FullPalettedImage(const std::span<const Pixel>& palette, unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: palette(memRes), pixels(memRes), memRes(memRes) {
		this->palette.resize(palette.size());
		std::memcpy(this->palette.data(),palette.data(),palette.size_bytes());
		this->pixels.resize(width * height);
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	/**
	 * @brief Constructor to create an image with a specified palette, pixel data, width, and height.
	 * @param palette The initial palette.
	 * @param pixels The initial pixel data.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource to use for allocations (default is the default resource).
	 */
	FullPalettedImage(const std::span<const Pixel>& palette, const std::span<const uint8_t>& pixels, unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: palette(memRes), pixels(memRes), memRes(memRes) {
		this->palette.resize(palette.size());
		std::memcpy(this->palette.data(),palette.data(),palette.size_bytes());
		this->pixels.resize(pixels.size());
		std::memcpy(this->pixels.data(),pixels.data(),pixels.size_bytes());
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	/**
	 * @brief Copy constructor.
	 * @param cpy The image to copy.
	 */
	FullPalettedImage(const FullPalettedImage& cpy)
		: palette(cpy.palette), pixels(cpy.pixels), memRes(cpy.memRes) {
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
	}
	/**
	 * @brief Assignment operator.
	 * @param cpy The image to copy.
	 * @return A reference to the current image.
	 */
	FullPalettedImage& operator=(const FullPalettedImage& cpy)
	{
		this->memRes = cpy.memRes;
		this->palette = cpy.palette;
		this->pixels = cpy.pixels;
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
		return *this;
	}
	/**
	 * @brief Move constructor.
	 * @param mov The image to move.
	 */
	FullPalettedImage(FullPalettedImage&& mov)
		: palette(std::move(mov.palette)), pixels(std::move(mov.pixels)), memRes(mov.memRes) {
		this->width = mov.width;
		this->height = mov.height;
		recalculateDimensions();
	}
	/**
	 * @brief Move assignment operator.
	 * @param mov The image to move.
	 * @return A reference to the current image.
	 */
	FullPalettedImage& operator=(FullPalettedImage&& mov)
	{
		this->memRes = mov.memRes;
		this->palette = std::move(mov.palette);
		this->pixels = std::move(mov.pixels);
		this->width = mov.width;
		this->height = mov.height;
		recalculateDimensions();
		return *this;
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
class ReadOnlyReferenceImage : public ReadOnlyImage2D {
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
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
	}

	/**
	 * @brief Copy assignment operator.
	 *
	 * @param cpy The image to copy from.
	 * @return A reference to the current object.
	 */
	ReadOnlyReferenceImage& operator=(const ReadOnlyReferenceImage& cpy) {
		this->pixels = cpy.pixels;
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
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
template <PixelConcept Pixel> class ReferenceImage : public Image2D {
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
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
	}

	/**
	 * @brief Copy assignment operator.
	 * @param cpy The image to copy.
	 * @return A reference to the copied image.
	 */
	ReferenceImage& operator=(const ReferenceImage& cpy)
	{
		this->pixels = cpy.pixels;
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
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
 *               define the following:
 *               - `FMT_ID`: A static member indicating the format of the pixel.
 *               - `fromKernel(const glm::fvec4&)`: A method to set the pixel
 *                 from a color kernel.
 *               - `fromKernelDithered(const glm::fvec4&, const glm::uvec2&)`: A
 *                 method to set the pixel from a color kernel with dithering.
 *
 * @note This class inherits from `ResizeableImage2D` and implements its interface.
 */
template <PixelConcept Pixel>
class StandardImage : public ResizeableImage2D {
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
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
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
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
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

}
}
}
#endif // EUPHSTANDARDIMAGE_HPP
