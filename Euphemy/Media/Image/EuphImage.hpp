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
enum class TextureFiltering : uint8_t {
	NEAREST_NEIGHBOUR, /**< Nearest neighbor filtering. */
	DITHERED,		  /**< Dithered filtering. */
	THREE_POINT,	   /**< Three-point filtering. */
	BILINEAR		   /**< Bilinear filtering. */
};

/**
 * @brief Enumeration for alpha blending modes.
 */
enum class AlphaBlending : uint8_t {
	ALPHA_TESTING,  /**< Alpha testing. */
	ALPHA_DITHERING,/**< Alpha dithering / Screen-door transparency / Stipled alpha. */
	ALPHA_BLENDING  /**< Alpha blending. */
};

/**
 * @brief Enumeration for texture wrapping modes.
 */
enum class Wrap : uint8_t {
	REPEAT,		  /**< Repeat the texture. */
	MIRRORED_REPEAT, /**< Mirror and repeat the texture. */
	CLAMP_TO_EDGE,   /**< Clamp to the edge of the texture. */
	CLAMP_TO_BORDER  /**< Clamp to a border colour. */
};

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(IReadOnlyPalette)
DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(IMutablePalette)
DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(IReadOnlyImage2D)
DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(IMutableImage2D)
DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(IResizeableImage2D)

/**
 * @brief Abstract class representing a read-only 2D image.
 */
class MH_EUPH_API IReadOnlyImage2D {
public:
	/**
	 * @brief Function type for iterating over image pixels.
	 * @param pos The position of the pixel.
	 * @param colour The colour of the pixel.
	 */
	typedef std::function<void(const glm::uvec2& pos, const glm::fvec4& colour)> ColourIterator;

	/**
	 * @brief Function type for iterating over image pixels with normalized coordinates.
	 * @param pos The normalized position of the pixel.
	 * @param colour The colour of the pixel.
	 */
	typedef std::function<void(const glm::fvec2& pos, const glm::fvec4& colour)> ColourIterator2;

protected:
	unsigned width;   /**< Width of the image. */
	unsigned height;  /**< Height of the image. */
	unsigned stride;  /**< Stride of the image. */
	float widthF;	 /**< Floating-point width of the image. */
	float heightF;	/**< Floating-point height of the image. */
	float widthR;	 /**< Reciprocal of the image width. */
	float heightR;	/**< Reciprocal of the image height. */

	/**
	 * @brief Recalculate derived dimensions (floating-point and reciprocal values).
	 */
	void recalculateDimensions();
	/**
	 * @brief Copy constructor.
	 * @param cpy Source image to copy from.
	 */
	IReadOnlyImage2D(const IReadOnlyImage2D& cpy);
	/**
	 * @brief Copy assignment operator.
	 * @param cpy Source image to copy from.
	 * @return Reference to this image.
	 */
	IReadOnlyImage2D& operator=(const IReadOnlyImage2D& cpy);
public:
	/**
	 * @brief Virtual destructor.
	 */
	virtual ~IReadOnlyImage2D() = default;

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
	 * @brief Sample a colour from the image.
	 * @param pos Normalized position of the pixel.
	 * @param screenpos Screen-space position of the pixel, used exclusively in dithering.
	 * @param colourKernel Output colour.
	 * @param filteringType Texture filtering mode.
	 * @param wrap Texture wrapping mode.
	 */
	void sample(const glm::fvec2& pos, const glm::uvec2& screenpos, glm::fvec4& colourKernel,
				TextureFiltering filteringType = TextureFiltering::NEAREST_NEIGHBOUR, Wrap wrap = Wrap::REPEAT) const;

	/**
	 * @brief Sample a colour from the image (convenience method).
	 * @param pos Normalized position of the pixel.
	 * @param screenpos Screen-space position of the pixel, used exclusively in dithering.
	 * @param filteringType Texture filtering mode.
	 * @param wrap Texture wrapping mode.
	 * @return Sampled colour.
	 */
	inline glm::fvec4 sample(const glm::fvec2& pos, const glm::uvec2& screenpos,
							 TextureFiltering filteringType = TextureFiltering::NEAREST_NEIGHBOUR, Wrap wrap = Wrap::REPEAT) const {
		glm::fvec4 tmp;
		sample(pos, screenpos, tmp, filteringType, wrap);
		return tmp;
	}

	/**
	 * @brief Get the pixel colour at a specific position.
	 * @param pos Position of the pixel.
	 * @param colourKernel Output colour.
	 * @param wrap Texture wrapping mode.
	 */
	virtual void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap = Wrap::REPEAT) const = 0;

	/**
	 * @brief Get the pixel colour at a specific position (convenience method).
	 * @param pos Position of the pixel.
	 * @param wrap Texture wrapping mode.
	 * @return Pixel colour.
	 */
	inline glm::fvec4 getPixel(const glm::uvec2& pos, Wrap wrap = Wrap::REPEAT) const {
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
	virtual void saveInto(DecodeTarget& destination) const;

	/**
	 * @brief Default constructor
	 */
	IReadOnlyImage2D();

	/**
	 * @brief Constructor with width and height
	 * @param width Width
	 * @param height Height
	 */
	IReadOnlyImage2D(unsigned width, unsigned height);


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
class MH_EUPH_API IMutableImage2D : public IReadOnlyImage2D
{
public:
	/**
	 * @brief Function type for modifying pixel colours.
	 * @param pos Position of the pixel.
	 * @return New colour for the pixel.
	 */
	typedef std::function<glm::fvec4(const glm::uvec2& pos)> ColourProgrammer;
	/**
	 * @brief Function type for modifying pixel colours with existing colour data.
	 * @param pos Position of the pixel.
	 * @param colour Current colour of the pixel.
	 * @return New colour for the pixel.
	 */
	typedef std::function<glm::fvec4(const glm::uvec2& pos, const glm::fvec4& colour)> ColourProgrammer2;
	/**
	 * @brief Function type for modifying pixel colours.
	 * @param pos The normalized position of the pixel.
	 * @return New colour for the pixel.
	 */
	typedef std::function<glm::fvec4(const glm::fvec2& pos)> ColourProgrammer3;
	/**
	 * @brief Function type for modifying pixel colours with existing colour data.
	 * @param pos The normalized position of the pixel.
	 * @param colour Current colour of the pixel.
	 * @return New colour for the pixel.
	 */
	typedef std::function<glm::fvec4(const glm::fvec2& pos, const glm::fvec4& colour)> ColourProgrammer4;
	/**
	 * @brief Virtual destructor.
	 */
	virtual ~IMutableImage2D() = default;
	/**
	 * @brief Default constructor
	 */
	IMutableImage2D();
	/**
	 * @brief Constructor with width and height
	 * @param width Width
	 * @param height Height
	 */
	IMutableImage2D(unsigned width, unsigned height);

	/**
	 * @brief Set the colour of a specific pixel.
	 * @param pos Position of the pixel.
	 * @param colourKernel Colour to set.
	 */
	virtual void setPixel(const glm::uvec2& pos, const glm::fvec4& colourKernel) = 0;

	/**
	 * @brief Set the colour of a pixel using dithering.
	 * @param pos Position of the pixel.
	 * @param colourKernel Colour to set.
	 */
	virtual void setPixelDithered(const glm::uvec2& pos, const glm::fvec4& colourKernel) = 0;

	/**
	 * @brief Set the colour of a pixel with alpha blending.
	 * @param pos Position of the pixel.
	 * @param colourKernel Colour to set.
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
	 * @brief Clear the entire image to a specific colour.
	 * @param colourKernel Colour to set.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const glm::fvec4& colourKernel, bool dither = false) = 0;
	/**
	 * @brief Clear the entire image to a specific colour.
	 * @param program The function used for modifying the pixel colour.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const ColourProgrammer& program, bool dither = false) = 0;
	/**
	 * @brief Clear the entire image to a specific colour.
	 * @param program The function used for modifying the pixel colour.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const ColourProgrammer2& program, bool dither = false) = 0;
	/**
	 * @brief Clear the entire image to a specific colour.
	 * @param program The function used for modifying the pixel colour.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const ColourProgrammer3& program, bool dither = false) = 0;
	/**
	 * @brief Clear the entire image to a specific colour.
	 * @param program The function used for modifying the pixel colour.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const ColourProgrammer4& program, bool dither = false) = 0;
	/**
	 * @brief Clear the a part of the image to a specific colour.
	 * @param colourKernel Colour to set.
	 * @param offset The top-left pixel affected by this function.
	 * @param dimensions The size of the subregion of the image affected by this function.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const glm::fvec4& colourKernel, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither = false) = 0;
	/**
	 * @brief Clear the a part of the image to a specific colour.
	 * @param program The function used for modifying the pixel colour.
	 * @param offset The top-left pixel affected by this function.
	 * @param dimensions The size of the subregion of the image affected by this function.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const ColourProgrammer& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither = false) = 0;
	/**
	 * @brief Clear the a part of the image to a specific colour.
	 * @param program The function used for modifying the pixel colour.
	 * @param offset The top-left pixel affected by this function.
	 * @param dimensions The size of the subregion of the image affected by this function.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const ColourProgrammer2& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither = false) = 0;
	/**
	 * @brief Clear the a part of the image to a specific colour.
	 * @param program The function used for modifying the pixel colour.
	 * @param offset The top-left pixel affected by this function.
	 * @param dimensions The size of the subregion of the image affected by this function.
	 * @param dither Whether to apply dithering.
	 */
	virtual void clearToColour(const ColourProgrammer3& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither = false) = 0;
	/**
	 * @brief Clear the a part of the image to a specific colour.
	 * @param program The function used for modifying the pixel colour.
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
	void blit(const IReadOnlyImage2D& cpy, const glm::uvec2& destinationOffset, const glm::uvec2& sourceOffset,
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
class MH_EUPH_API IResizeableImage2D : public IMutableImage2D {
public:
	/**
	 * @brief Virtual destructor.
	 */
	virtual ~IResizeableImage2D() = default;
	/**
	 * @brief Default constructor
	 */
	IResizeableImage2D();
	/**
	 * @brief Constructor with width and height
	 * @param width Width
	 * @param height Height
	 */
	IResizeableImage2D(unsigned width, unsigned height);

	/**
	 * @brief Resize the image.
	 * @param newWidth New width for the image.
	 * @param newHeight New height for the image.
	 * @return True if the resize operation was successful, false otherwise.
	 */
	virtual bool resize(unsigned newWidth, unsigned newHeight) = 0;
};

/**
 * @brief Interface for a read-only color palette.
 */
class MH_EUPH_API IReadOnlyPalette {
protected:
	int_fast16_t transparentClrIndex; ///< Index of the transparent color in the palette.
public:
	virtual ~IReadOnlyPalette() = default;

	/**
	 * @brief Retrieve a color from the palette.
	 * @param index Index of the color.
	 * @param kernel Output parameter to store the retrieved color.
	 */
	virtual void getColour(std::uint8_t index, glm::fvec4& kernel) const = 0;

	/**
	 * @brief Retrieve a color from the palette.
	 * @param index Index of the color.
	 * @return The color as a glm::fvec4.
	 */
	inline glm::fvec4 getColour(std::uint8_t index) const {
		glm::fvec4 kernel;
		getColour(index, kernel);
		return kernel;
	}

	/**
	 * @brief Get the format of the palette.
	 * @return Palette format.
	 */
	virtual Format getFormat() const = 0;

	/**
	 * @brief Get the raw color data of the palette.
	 * @return Span of bytes representing the raw colors.
	 */
	virtual const std::span<const std::byte> getRawColours() const = 0;

	/**
	 * @brief Convert a paletted image into a full-color image.
	 * @param indices Indexed image data.
	 * @param width Width of the image.
	 * @param height Height of the image.
	 * @param memRes Memory resource needed for managing the resulting texture.
	 * @return A depalettized image.
	 */
	virtual sIResizeableImage2D depalettizeS(const std::span<const uint8_t>& indices, unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource()) const = 0;
	/**
	 * @brief Convert a paletted image into a full-color image.
	 * @param indices Indexed image data.
	 * @param width Width of the image.
	 * @param height Height of the image.
	 * @param memRes Memory resource needed for managing the resulting texture.
	 * @return A depalettized image.
	 */
	virtual uIResizeableImage2D depalettizeU(const std::span<const uint8_t>& indices, unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource()) const = 0;

	/**
	 * @brief Save the palette into another palette instance.
	 * @param target Target palette instance.
	 * @param isPreallocated Whether the target is preallocated.
	 */
	void saveInto(Palette& target, bool isPreallocated=false) const;

	/**
	 * @brief Get the index of the transparent color.
	 * @return Transparent color index.
	 */
	int_fast16_t getTransparentClrIndex() const;

	/**
	 * @brief Set the index of the transparent color.
	 * @param newTransparentClrIndex New transparent color index.
	 */
	void setTransparentClrIndex(int_fast16_t newTransparentClrIndex);
};

/**
 * @brief Interface for a mutable color palette.
 */
class MH_EUPH_API IMutablePalette : public IReadOnlyPalette {
public:
	virtual ~IMutablePalette() = default;

	/**
	 * @brief Set a color in the palette.
	 * @param index Index of the color.
	 * @param colour New color value.
	 */
	virtual void setColour(std::uint8_t index, const glm::fvec4& colour) = 0;

	/**
	 * @brief Get the raw color data of the palette.
	 * @return Span of bytes representing the raw colors.
	 */
	virtual const std::span<const std::byte> getRawColours() const = 0;
	/**
	 * @brief Get the raw color data of the palette.
	 * @return Span of bytes representing the raw colors.
	 */
	virtual std::span<const std::byte> getRawColours() = 0;
};


/**
 * @brief Abstract class representing a read-only, indexed 2D image.
 */
class MH_EUPH_API IReadOnlyPalettedImage2D : public IReadOnlyImage2D {
protected:
	sIReadOnlyPalette palette;  ///< Palette used by the image.

public:
	/**
	 * @brief Get the format of the image.
	 * @return Image format.
	 */
	Format getFormat() const override;

	/**
	 * @brief Save the image into a file.
	 * @param destination Decode target for saving.
	 */
	void saveInto(DecodeTarget& destination) const override;

	/**
	 * @brief Get a pointer to the raw pixel data (read-only).
	 * @return Pointer to raw pixel data (read-only).
	 */
	virtual const std::span<const uint8_t> getIndices() const = 0;

	/**
	 * @brief Get the color of a pixel at a specific position.
	 * @param pos Position of the pixel.
	 * @param colourKernel Output parameter to store the retrieved color.
	 * @param wrap Wrap mode for handling out-of-bounds access.
	 */
	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override;

	/**
	 * @brief Iterate over all pixels in the image.
	 * @param program Color iterator program to apply to each pixel.
	 */
	void iterateOverPixels(const ColourIterator& program) const override;

	/**
	 * @brief Iterate over all pixels in the image.
	 * @param program Color iterator program to apply to each pixel.
	 */
	void iterateOverPixels(const ColourIterator2& program) const override;

	/**
	 * @brief Iterate over a subset of pixels in the image.
	 * @param program Color iterator program to apply to each pixel.
	 * @param offset Offset of the subset.
	 * @param dimensions Dimensions of the subset.
	 */
	void iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override;

	/**
	 * @brief Iterate over a subset of pixels in the image.
	 * @param program Color iterator program to apply to each pixel.
	 * @param offset Offset of the subset.
	 * @param dimensions Dimensions of the subset.
	 */
	void iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override;

	/**
	 * @brief Get the palette used by the image.
	 * @return Read-only reference to the palette.
	 */
	const sIReadOnlyPalette& getPalette() const;

	/**
	 * @brief Set the palette used by the image.
	 * @param newPalette New palette to use.
	 */
	void setPalette(const sIReadOnlyPalette& newPalette);

	/**
	 * @brief Set the palette used by the image.
	 * @param newPalette New palette to use.
	 */
	void setPalette(sIReadOnlyPalette&& newPalette);

	/**
	 * @brief Convert the paletted image into a full-color image.
	 * @param memRes Memory resource needed for managing the resulting texture.
	 * @return A depalettized image.
	 */
	sIResizeableImage2D depalettizeS(std::pmr::memory_resource* memRes = std::pmr::get_default_resource()) const;

	/**
	 * @brief Convert the paletted image into a full-color image.
	 * @param memRes Memory resource needed for managing the resulting texture.
	 * @return A depalettized image.
	 */
	uIResizeableImage2D depalettizeU(std::pmr::memory_resource* memRes = std::pmr::get_default_resource()) const;
	/**
	 * @brief Default constructor
	 */
	IReadOnlyPalettedImage2D();
	/**
	 * @brief Constructor with width and height
	 * @param width Width
	 * @param height Height
	 */
	IReadOnlyPalettedImage2D(unsigned width, unsigned height);
};

/**
 * @brief Abstract class representing a mutable, indexed 2D image.
 */
class MH_EUPH_API IMutablePalettedImage2D : public IReadOnlyPalettedImage2D {
public:
	/**
	 * @brief Get a pointer to the raw pixel data (read-only).
	 * @return Pointer to raw pixel data (read-only).
	 */
	virtual const std::span<const uint8_t> getIndices() const = 0;

	/**
	 * @brief Get a pointer to the raw pixel data (writable).
	 * @return Pointer to raw pixel data (writable).
	 */
	virtual std::span<uint8_t> getIndices() = 0;

	/**
	 * @brief Set the index of a pixel at a specific position.
	 * @param pos Position of the pixel.
	 * @param newIndex New index for the pixel.
	 */
	void setPixel(const glm::uvec2& pos, uint8_t newIndex) const;
	/**
	 * @brief Default constructor
	 */
	IMutablePalettedImage2D();
	/**
	 * @brief Constructor with width and height
	 * @param width Width
	 * @param height Height
	 */
	IMutablePalettedImage2D(unsigned width, unsigned height);
};

/**
 * @brief Abstract class representing a resizable, mutable, indexed 2D image.
 */
class MH_EUPH_API IResizeablePalettedImage2D : public IMutablePalettedImage2D {
public:

	/**
	 * @brief Resize the image.
	 * @param newWidth New width for the image.
	 * @param newHeight New height for the image.
	 * @return True if the resize operation was successful, false otherwise.
	 */
	virtual bool resize(unsigned newWidth, unsigned newHeight) = 0;
	/**
	 * @brief Default constructor
	 */
	IResizeablePalettedImage2D();
	/**
	 * @brief Constructor with width and height
	 * @param width Width
	 * @param height Height
	 */
	IResizeablePalettedImage2D(unsigned width, unsigned height);
};

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(ReadOnlyPalettedImage2D)
/**
 * @brief A class representing a read-only paletted 2D image.
 *
 * This class implements the IReadOnlyPalettedImage2D interface and provides
 * methods to access the raw pixel data and the indices of the palette.
 */
class MH_EUPH_API ReadOnlyPalettedImage2D : public IReadOnlyPalettedImage2D {
private:
	/**
	 * @brief The span of indices representing the pixel data.
	 */
	std::span<const uint8_t> indices;

public:
	/**
	 * @brief Returns a pointer to the raw pixel data.
	 *
	 * @return A constant pointer to the raw pixel data.
	 */
	const void* getRawPixels() const override;

	/**
	 * @brief Returns the span of indices.
	 *
	 * @return A constant span of indices.
	 */
	const std::span<const uint8_t> getIndices() const override;

	/**
	 * @brief Copy constructor.
	 *
	 * @param cpy The ReadOnlyPalettedImage2D object to copy.
	 */
	ReadOnlyPalettedImage2D(const ReadOnlyPalettedImage2D& cpy);

	/**
	 * @brief Copy assignment operator.
	 *
	 * @param cpy The ReadOnlyPalettedImage2D object to copy.
	 * @return A reference to the updated object.
	 */
	ReadOnlyPalettedImage2D& operator=(const ReadOnlyPalettedImage2D& cpy);

	/**
	 * @brief Move constructor.
	 *
	 * @param mov The ReadOnlyPalettedImage2D object to move.
	 */
	ReadOnlyPalettedImage2D(ReadOnlyPalettedImage2D&& mov);

	/**
	 * @brief Move assignment operator.
	 *
	 * @param mov The ReadOnlyPalettedImage2D object to move.
	 * @return A reference to the updated object.
	 */
	ReadOnlyPalettedImage2D& operator=(ReadOnlyPalettedImage2D&& mov);

	/**
	 * @brief Constructor initializing the image with indices, width, and height.
	 *
	 * @param indices A span of indices representing the pixel data.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 */
	ReadOnlyPalettedImage2D(const std::span<const uint8_t>& indices, unsigned width, unsigned height);

	/**
	 * @brief Sets new indices for the image.
	 *
	 * @param newIndices A span of new indices to set.
	 */
	void setIndices(const std::span<const uint8_t>& newIndices);
};

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(MutablePalettedImage2D)
/**
 * @brief A mutable 2D paletted image class.
 *
 * This class represents a mutable 2D image where each pixel is represented by an index into a color palette.
 * It implements the IMutablePalettedImage2D interface.
 */
class MH_EUPH_API MutablePalettedImage2D : public IMutablePalettedImage2D {
private:
	/**
	 * @brief The span of indices representing the pixels of the image.
	 *
	 * Each index corresponds to a color in the palette.
	 */
	std::span<uint8_t> indices;

public:
	/**
	 * @brief Returns a pointer to the raw pixel data.
	 *
	 * This method returns a pointer to the raw pixel data. The pixel data is represented by indices into a color palette.
	 *
	 * @return A constant pointer to the raw pixel data.
	 */
	const void* getRawPixels() const override;

	/**
	 * @brief Returns the span of indices representing the pixels of the image.
	 *
	 * This method returns a constant span of indices representing the pixels of the image. Each index corresponds to a color in the palette.
	 *
	 * @return A constant span of indices.
	 */
	const std::span<const uint8_t> getIndices() const override;

	/**
	 * @brief Returns the span of indices representing the pixels of the image.
	 *
	 * This method returns a span of indices representing the pixels of the image. Each index corresponds to a color in the palette.
	 *
	 * @return A span of indices.
	 */
	std::span<uint8_t> getIndices() override;

	/**
	 * @brief Sets the indices of the image.
	 *
	 * This method sets the indices of the image to the provided span of indices. Each index corresponds to a color in the palette.
	 *
	 * @param newIndices The new span of indices to set.
	 */
	void setIndices(const std::span<uint8_t>& newIndices);

	/**
	 * @brief Copy constructor.
	 *
	 * This constructor creates a deep copy of the provided MutablePalettedImage2D object.
	 *
	 * @param cpy The MutablePalettedImage2D object to copy.
	 */
	MutablePalettedImage2D(const MutablePalettedImage2D& cpy);

	/**
	 * @brief Copy assignment operator.
	 *
	 * This operator assigns the contents of the provided MutablePalettedImage2D object to this object.
	 *
	 * @param cpy The MutablePalettedImage2D object to copy.
	 * @return A reference to this object.
	 */
	MutablePalettedImage2D& operator=(const MutablePalettedImage2D& cpy);

	/**
	 * @brief Move constructor.
	 *
	 * This constructor moves the contents of the provided MutablePalettedImage2D object to this object.
	 *
	 * @param mov The MutablePalettedImage2D object to move.
	 */
	MutablePalettedImage2D(MutablePalettedImage2D&& mov);

	/**
	 * @brief Move assignment operator.
	 *
	 * This operator moves the contents of the provided MutablePalettedImage2D object to this object.
	 *
	 * @param mov The MutablePalettedImage2D object to move.
	 * @return A reference to this object.
	 */
	MutablePalettedImage2D& operator=(MutablePalettedImage2D&& mov);

	/**
	 * @brief Constructor that initializes the image with indices and dimensions.
	 *
	 * This constructor initializes the image with the provided span of indices and dimensions (width and height).
	 *
	 * @param indices The span of indices representing the pixels of the image.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 */
	MutablePalettedImage2D(const std::span<uint8_t>& indices, unsigned width, unsigned height);
};

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(ResizeablePalettedImage2D)
/**
 * @class ResizeablePalettedImage2D
 * @brief A class representing a 2D image with a palette that can be resized.
 *
 * This class extends the IResizeablePalettedImage2D interface and provides
 * functionality to manage a 2D image using a palette. The image data is stored
 * in a pmr::vector of indices, which map to colors in a palette. The class
 * supports resizing the image and managing its memory resource.
 *
 * @note The class uses C++17 features such as std::pmr::vector and std::span.
 */
class MH_EUPH_API ResizeablePalettedImage2D : public IResizeablePalettedImage2D {
private:
	/**
	 * @brief The vector of indices that map to colors in the palette.
	 */
	std::pmr::vector<uint8_t> indices;

	/**
	 * @brief The memory resource used for allocation.
	 */
	std::pmr::memory_resource* memRes;

public:
	/**
	 * @brief Returns a pointer to the raw pixel data.
	 *
	 * This method returns a pointer to the raw pixel data, which is typically
	 * an array of indices that map to colors in a palette.
	 *
	 * @return A pointer to the raw pixel data.
	 */
	const void* getRawPixels() const override;

	/**
	 * @brief Returns a span of the indices that map to colors in the palette.
	 *
	 * This method returns a span of the indices that map to colors in the palette.
	 * The span is read-only and should not be modified.
	 *
	 * @return A span of the indices.
	 */
	const std::span<const uint8_t> getIndices() const override;

	/**
	 * @brief Returns a span of the indices that map to colors in the palette.
	 *
	 * This method returns a span of the indices that map to colors in the palette.
	 * The span is writable and can be modified.
	 *
	 * @return A span of the indices.
	 */
	std::span<uint8_t> getIndices() override;

	/**
	 * @brief Resizes the image to the specified dimensions.
	 *
	 * This method resizes the image to the specified width and height. If the
	 * new dimensions are larger, the new pixels will be initialized to zero.
	 * If the new dimensions are smaller, the excess pixels will be discarded.
	 *
	 * @param newWidth The new width of the image.
	 * @param newHeight The new height of the image.
	 * @return True if the resize was successful, false otherwise.
	 */
	bool resize(unsigned int newWidth, unsigned int newHeight) override;

	/**
	 * @brief Returns the memory resource used for allocation.
	 *
	 * This method returns the memory resource that is currently being used for
	 * allocating memory for the image data.
	 *
	 * @return The memory resource.
	 */
	std::pmr::memory_resource* getMemRes() const;

	/**
	 * @brief Sets the memory resource used for allocation.
	 *
	 * This method sets the memory resource that will be used for allocating
	 * memory for the image data. If the image already contains data, it will
	 * be moved to the new memory resource.
	 *
	 * @param newMemRes The new memory resource.
	 */
	void setMemRes(std::pmr::memory_resource* newMemRes);

	/**
	 * @brief Copy constructor.
	 *
	 * This constructor creates a copy of the specified ResizeablePalettedImage2D object.
	 * The new object will have the same dimensions and memory resource, and will
	 * contain a copy of the indices.
	 *
	 * @param cpy The object to copy.
	 */
	ResizeablePalettedImage2D(const ResizeablePalettedImage2D& cpy);

	/**
	 * @brief Move constructor.
	 *
	 * This constructor moves the contents of the specified ResizeablePalettedImage2D object
	 * into the new object. The source object will be left in a valid but unspecified state.
	 *
	 * @param mov The object to move.
	 */
	ResizeablePalettedImage2D(ResizeablePalettedImage2D&& mov);

	/**
	 * @brief Copy assignment operator.
	 *
	 * This operator assigns the contents of the specified ResizeablePalettedImage2D object
	 * to the current object. The current object will have the same dimensions and memory
	 * resource, and will contain a copy of the indices.
	 *
	 * @param cpy The object to copy.
	 * @return A reference to the current object.
	 */
	ResizeablePalettedImage2D& operator=(const ResizeablePalettedImage2D& cpy);

	/**
	 * @brief Move assignment operator.
	 *
	 * This operator moves the contents of the specified ResizeablePalettedImage2D object
	 * into the current object. The source object will be left in a valid but unspecified state.
	 *
	 * @param mov The object to move.
	 * @return A reference to the current object.
	 */
	ResizeablePalettedImage2D& operator=(ResizeablePalettedImage2D&& mov);

	/**
	 * @brief Constructor that creates an empty image with the specified dimensions.
	 *
	 * This constructor creates an empty image with the specified width and height.
	 * The image data is initialized to zero, and the indices vector is empty.
	 *
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource to use for allocation (default is the default resource).
	 */
	ResizeablePalettedImage2D(unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());

	/**
	 * @brief Constructor that creates an image with the specified indices and dimensions.
	 *
	 * This constructor creates an image with the specified indices and dimensions.
	 * The indices vector is initialized with the provided span of indices.
	 *
	 * @param indices The span of indices that map to colors in the palette.
	 * @param width The width of the image.
	 * @param height The height of the image.
	 * @param memRes The memory resource to use for allocation (default is the default resource).
	 */
	ResizeablePalettedImage2D(const std::span<const uint8_t>& indices, unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
};

}
}
}
#endif // EUPHIMAGE_HPP
