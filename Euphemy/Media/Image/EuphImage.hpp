#ifndef EUPHIMAGE_HPP
#define EUPHIMAGE_HPP
#include <glm/glm.hpp>
#include <span>
#include <functional>
#include <Euphemy/Media/Image/EuphImageType.hpp>
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Elvavena/Util/ElvAllocatorBasic.hpp>
#include <cstring>
#include <Euphemy/Config/EuphLib.hpp>
namespace Euph {
namespace Media {
namespace Image {

/**
 * @brief Enumeration for texture filtering modes.
 */
enum TextureFiltering {
	NEAREST_NEIGHBOUR, /**< Nearest neighbor filtering. */
	DITHERED,          /**< Dithered filtering. */
	THREE_POINT,       /**< Three-point filtering. */
	BILINEAR           /**< Bilinear filtering. */
};

/**
 * @brief Enumeration for alpha blending modes.
 */
enum AlphaBlending {
	ALPHA_TESTING,  /**< Alpha testing. */
	ALPHA_DITHERING,/**< Alpha dithering / Screen-door transparency / Stipled alpha. */
	ALPHA_BLENDING  /**< Alpha blending. */
};

/**
 * @brief Enumeration for texture wrapping modes.
 */
enum Wrap {
	REPEAT,          /**< Repeat the texture. */
	MIRRORED_REPEAT, /**< Mirror and repeat the texture. */
	CLAMP_TO_EDGE,   /**< Clamp to the edge of the texture. */
	CLAMP_TO_BORDER  /**< Clamp to a border color. */
};

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(ReadOnlyImage2D)
DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(Image2D)
DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(ResizeableImage2D)

/**
 * @brief Abstract class representing a read-only 2D image.
 */
class MH_EUPH_API ReadOnlyImage2D {
public:
	/**
	 * @brief Function type for iterating over image pixels.
	 * @param pos The position of the pixel.
	 * @param color The color of the pixel.
	 */
	typedef std::function<void(const glm::uvec2& pos, const glm::fvec4& color)> ColourIterator;

	/**
	 * @brief Function type for iterating over image pixels with normalized coordinates.
	 * @param pos The normalized position of the pixel.
	 * @param color The color of the pixel.
	 */
	typedef std::function<void(const glm::fvec2& pos, const glm::fvec4& color)> ColourIterator2;

protected:
	unsigned width;   /**< Width of the image. */
	unsigned height;  /**< Height of the image. */
	unsigned stride;  /**< Stride of the image. */
	float widthF;     /**< Floating-point width of the image. */
	float heightF;    /**< Floating-point height of the image. */
	float widthR;     /**< Reciprocal of the image width. */
	float heightR;    /**< Reciprocal of the image height. */

	/**
	 * @brief Recalculate derived dimensions (floating-point and reciprocal values).
	 */
	void recalculateDimensions();

public:
	/**
	 * @brief Virtual destructor.
	 */
	virtual ~ReadOnlyImage2D() = default;

	/**
	 * @brief Get the width of the image.
	 * @return Width of the image.
	 */
	unsigned getWidth() const;

	/**
	 * @brief Get the height of the image.
	 * @return Height of the image.
	 */
	unsigned getHeight() const;

	/**
	 * @brief Get the stride of the image.
	 * @return Stride of the image.
	 */
	unsigned getStride() const;

	/**
	 * @brief Get the floating-point width of the image.
	 * @return Floating-point width of the image.
	 */
	float getWidthF() const;

	/**
	 * @brief Get the floating-point height of the image.
	 * @return Floating-point height of the image.
	 */
	float getHeightF() const;

	/**
	 * @brief Get the reciprocal of the image width.
	 * @return Reciprocal of the image width.
	 */
	float getWidthR() const;

	/**
	 * @brief Get the reciprocal of the image height.
	 * @return Reciprocal of the image height.
	 */
	float getHeightR() const;

	/**
	 * @brief Get the format of the image.
	 * @return Image format.
	 */
	virtual Format getFormat() const = 0;

	/**
	 * @brief Sample a color from the image.
	 * @param pos Normalized position of the pixel.
	 * @param screenpos Screen-space position of the pixel, used exclusively in dithering.
	 * @param colourKernel Output color.
	 * @param filteringType Texture filtering mode.
	 * @param wrap Texture wrapping mode.
	 */
	void sample(const glm::fvec2& pos, const glm::uvec2& screenpos, glm::fvec4& colourKernel,
				TextureFiltering filteringType = NEAREST_NEIGHBOUR, Wrap wrap = REPEAT) const;

	/**
	 * @brief Sample a color from the image (convenience method).
	 * @param pos Normalized position of the pixel.
	 * @param screenpos Screen-space position of the pixel, used exclusively in dithering.
	 * @param filteringType Texture filtering mode.
	 * @param wrap Texture wrapping mode.
	 * @return Sampled color.
	 */
	inline glm::fvec4 sample(const glm::fvec2& pos, const glm::uvec2& screenpos,
							 TextureFiltering filteringType = NEAREST_NEIGHBOUR, Wrap wrap = REPEAT) const {
		glm::fvec4 tmp;
		sample(pos, screenpos, tmp, filteringType, wrap);
		return tmp;
	}

	/**
	 * @brief Get the pixel color at a specific position.
	 * @param pos Position of the pixel.
	 * @param colourKernel Output color.
	 * @param wrap Texture wrapping mode.
	 */
	virtual void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap = REPEAT) const = 0;

	/**
	 * @brief Get the pixel color at a specific position (convenience method).
	 * @param pos Position of the pixel.
	 * @param wrap Texture wrapping mode.
	 * @return Pixel color.
	 */
	inline glm::fvec4 getPixel(const glm::uvec2& pos, Wrap wrap = REPEAT) const {
		glm::fvec4 tmp;
		getPixel(pos, tmp, wrap);
		return tmp;
	}

	/**
	 * @brief Get a pointer to the raw pixel data (read-only).
	 * @return Pointer to raw pixel data (read-only).
	 */
	virtual const void* getRawPixels() const = 0;

	/**
	 * @brief Iterate over all pixels in the image.
	 * @param program Function to apply to each pixel.
	 */
	virtual void iterateOverPixels(const ColourIterator& program) const = 0;

	/**
	 * @brief Iterate over all pixels in the image using normalized coordinates.
	 * @param program Function to apply to each pixel.
	 */
	virtual void iterateOverPixels(const ColourIterator2& program) const = 0;

	/**
	 * @brief Iterate over a subset of the image pixels.
	 * @param program Function to apply to each pixel.
	 * @param offset Offset of the subset.
	 * @param dimensions Dimensions of the subset.
	 */
	virtual void iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset,
								   const glm::uvec2& dimensions) const = 0;

	/**
	 * @brief Iterate over a subset of the image pixels using normalized coordinates.
	 * @param program Function to apply to each pixel.
	 * @param offset Offset of the subset.
	 * @param dimensions Dimensions of the subset.
	 */
	virtual void iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset,
								   const glm::uvec2& dimensions) const = 0;

	/**
	 * @brief Save the image into a file.
	 * @param destination Decode target for saving.
	 */
	void saveInto(DecodeTarget& destination) const;

	/**
	 * @brief Produce a 1-bit alpha map from the image.
	 * @tparam Allocator Allocator type for the vector.
	 * @param alphas Output vector of alpha values.
	 * @param threshhold Threshold for determining alpha.
	 */
	template <class Allocator>
	inline void produceAlphaMap(std::vector<bool, Allocator>& alphas, float threshhold) const {
		const int width = getWidth();
		alphas.resize(getWidth() * getHeight());
		iterateOverPixels((ColourIterator)[&alphas, threshhold, width](const glm::uvec2& pos, const glm::fvec4& clr) {
			int index = (pos.y * width) + pos.x;
			alphas[index] = clr.a >= threshhold;
		});
	}
};

/**
 * @brief Abstract class representing a modifiable 2D image.
 */
class MH_EUPH_API Image2D : public ReadOnlyImage2D
{
public:
	/**
	 * @brief Function type for modifying pixel colors.
	 * @param pos Position of the pixel.
	 * @return New color for the pixel.
	 */
	typedef std::function<glm::fvec4(const glm::uvec2& pos)> ColourProgrammer;
	/**
	 * @brief Function type for modifying pixel colors with existing color data.
	 * @param pos Position of the pixel.
	 * @param color Current color of the pixel.
	 * @return New color for the pixel.
	 */
	typedef std::function<glm::fvec4(const glm::uvec2& pos, const glm::fvec4& color)> ColourProgrammer2;
	/**
	 * @brief Function type for modifying pixel colors.
	 * @param pos The normalized position of the pixel.
	 * @return New color for the pixel.
	 */
	typedef std::function<glm::fvec4(const glm::fvec2& pos)> ColourProgrammer3;
	/**
	 * @brief Function type for modifying pixel colors with existing color data.
	 * @param pos The normalized position of the pixel.
	 * @param color Current color of the pixel.
	 * @return New color for the pixel.
	 */
	typedef std::function<glm::fvec4(const glm::fvec2& pos, const glm::fvec4& color)> ColourProgrammer4;
	/**
	 * @brief Virtual destructor.
	 */
	virtual ~Image2D() = default;

	/**
	 * @brief Set the color of a specific pixel.
	 * @param pos Position of the pixel.
	 * @param colourKernel Color to set.
	 */
	virtual void setPixel(const glm::uvec2& pos, const glm::fvec4& colourKernel) = 0;

	/**
	 * @brief Set the color of a pixel using dithering.
	 * @param pos Position of the pixel.
	 * @param colourKernel Color to set.
	 */
	virtual void setPixelDithered(const glm::uvec2& pos, const glm::fvec4& colourKernel) = 0;

	/**
	 * @brief Set the color of a pixel with alpha blending.
	 * @param pos Position of the pixel.
	 * @param colourKernel Color to set.
	 * @param blendingType Alpha blending mode.
	 * @return True if the operation was successful, false otherwise.
	 */
	bool setPixelWithBlending(const glm::uvec2& pos, const glm::fvec4& colourKernel, AlphaBlending blendingType);

	/**
	 * @brief Get a modifiable pointer to the raw pixel data.
	 * @return Pointer to raw pixel data.
	 */
	virtual void* getRawPixels() = 0;
	/**
	 * @brief Get a pointer to the raw pixel data (read-only).
	 * @return Pointer to raw pixel data (read-only).
	 */
	virtual const void* getRawPixels() const = 0;
	/**
	 * @brief Clear the entire image to a specific color.
	 * @param colourKernel Color to set.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const glm::fvec4& colourKernel, bool dither = false) = 0;
	/**
	 * @brief Clear the entire image to a specific color.
	 * @param program The function used for modifying the pixel color.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const ColourProgrammer& program, bool dither = false) = 0;
	/**
	 * @brief Clear the entire image to a specific color.
	 * @param program The function used for modifying the pixel color.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const ColourProgrammer2& program, bool dither = false) = 0;
	/**
	 * @brief Clear the entire image to a specific color.
	 * @param program The function used for modifying the pixel color.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const ColourProgrammer3& program, bool dither = false) = 0;
	/**
	 * @brief Clear the entire image to a specific color.
	 * @param program The function used for modifying the pixel color.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const ColourProgrammer4& program, bool dither = false) = 0;
	/**
	 * @brief Clear the a part of the image to a specific color.
	 * @param colourKernel Color to set.
	 * @param offset The top-left pixel affected by this function.
	 * @param dimensions The size of the subregion of the image affected by this function.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const glm::fvec4& colourKernel, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither = false) = 0;
	/**
	 * @brief Clear the a part of the image to a specific color.
	 * @param program The function used for modifying the pixel color.
	 * @param offset The top-left pixel affected by this function.
	 * @param dimensions The size of the subregion of the image affected by this function.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const ColourProgrammer& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither = false) = 0;
	/**
	 * @brief Clear the a part of the image to a specific color.
	 * @param program The function used for modifying the pixel color.
	 * @param offset The top-left pixel affected by this function.
	 * @param dimensions The size of the subregion of the image affected by this function.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const ColourProgrammer2& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither = false) = 0;
	/**
	 * @brief Clear the a part of the image to a specific color.
	 * @param program The function used for modifying the pixel color.
	 * @param offset The top-left pixel affected by this function.
	 * @param dimensions The size of the subregion of the image affected by this function.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const ColourProgrammer3& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither = false) = 0;
	/**
	 * @brief Clear the a part of the image to a specific color.
	 * @param program The function used for modifying the pixel color.
	 * @param offset The top-left pixel affected by this function.
	 * @param dimensions The size of the subregion of the image affected by this function.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const ColourProgrammer4& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither = false) = 0;
	/**
	 * @brief Blit (copy) a region from another image into this image.
	 * @param cpy The source image.
	 * @param destinationOffset The offset in this image where the source image will be copied.
	 * @param sourceOffset The offset in the source image from where copying will begin.
	 * @param sourceDimensionsFromOffset The dimensions of the region to copy from the source image.
	 * @param dither Whether to use dithering during the blit operation.
	 */
	void blit(const ReadOnlyImage2D& cpy, const glm::uvec2& destinationOffset, const glm::uvec2& sourceOffset,
			  const glm::uvec2& sourceDimensionsFromOffset, bool dither = false);
	/**
	 * @brief Blit (copy) a region from a span of pixels into this image.
	 * @tparam Pixel The pixel format.
	 * @param pixels The span of pixels to copy.
	 * @param sourceDimensions The dimensions of the source pixel data.
	 * @param destinationOffset The offset in this image where the source pixels will be copied.
	 * @param sourceOffset The offset in the source pixel data from where copying will begin.
	 * @param sourceDimensionsFromOffset The dimensions of the region to copy from the source pixel data.
	 * @param dither Whether to use dithering during the blit operation.
	 */
	template <PixelConcept Pixel> void blit(const std::span<const Pixel>& pixels, const glm::uvec2& sourceDimensions,
																	 const glm::uvec2& destinationOffset, const glm::uvec2& sourceOffset,
																	 const glm::uvec2& sourceDimensionsFromOffset, bool dither = false) {
		if((destinationOffset.x > width) || (destinationOffset.y > height) ||
			(sourceOffset.x > sourceDimensions.x) || (sourceOffset.y > sourceDimensions.y)) {
			return;
		}
		const glm::uvec2 maxDim = calculateMaximumOffsetForBlit(glm::uvec2(width,height),sourceDimensions,destinationOffset,sourceOffset,sourceDimensionsFromOffset);
		const size_t stride = pixelByteSize(Pixel::FMT_ID)*maxDim.x;
		if constexpr(getFormat() == Pixel::FMT_ID) {
			std::byte * const startPtrForDst = static_cast<std::byte*>(getRawPixels());
			for(unsigned y = 0; y < maxDim.y; ++y) {
				unsigned dstOffsetIndex = (width*(y+destinationOffset.y))+destinationOffset.x;
				unsigned srcOffsetIndex = (sourceDimensions.x*(y+sourceOffset.y))+sourceOffset.x;
				std::memcpy(&startPtrForDst[pixelByteSize(Pixel::FMT_ID)*dstOffsetIndex],&pixels[pixelByteSize(Pixel::FMT_ID)*srcOffsetIndex],stride);
			}
		} else {
			clearToColour([&pixels,&sourceDimensions,&destinationOffset,&sourceOffset](const glm::uvec2& curPos) {
				const glm::uvec2 offsetIndexInSource = (curPos - destinationOffset)+sourceOffset;
				const unsigned index = (offsetIndexInSource.y*sourceDimensions.x)+offsetIndexInSource.x;
				glm::fvec4 kernel;
				pixels[index].toKernel(kernel);
				return kernel;
			}, destinationOffset, maxDim-destinationOffset, dither);
		}
	};
};

/**
 * @brief Abstract class representing a resizable 2D image.
 */
class MH_EUPH_API ResizeableImage2D : public Image2D {
public:
	/**
	 * @brief Virtual destructor.
	 */
	virtual ~ResizeableImage2D() = default;

	/**
	 * @brief Resize the image.
	 * @param newWidth New width for the image.
	 * @param newHeight New height for the image.
	 * @return True if the resize operation was successful, false otherwise.
	 */
	virtual bool resize(unsigned newWidth, unsigned newHeight) = 0;
};

}
}
}
#endif // EUPHIMAGE_HPP
