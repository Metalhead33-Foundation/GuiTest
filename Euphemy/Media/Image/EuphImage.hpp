#ifndef EUPHIMAGE_HPP
#define EUPHIMAGE_HPP
#include <glm/glm.hpp>
#include <span>
#include <functional>
#include <Euphemy/Media/Image/EuphImageType.hpp>
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <cstring>
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
	ALPHA_DITHERING,/**< Alpha dithering. */
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

class ReadOnlyImage2D
{
public:
	// Colour iterators
	typedef std::function<void(const glm::uvec2&, const glm::fvec4&)> ColourIterator;
	typedef std::function<void(const glm::fvec2&, const glm::fvec4&)> ColourIterator2;
protected:
	unsigned width, height, stride;
	float widthF, heightF;
	float widthR, heightR;
	void recalculateDimensions();
public:
	virtual ~ReadOnlyImage2D() = default;
	// Getting stats
	unsigned getWidth() const;
	unsigned getHeight() const;
	unsigned getStride() const;
	float getWidthF() const;
	float getHeightF() const;
	float getWidthR() const;
	float getHeightR() const;
	virtual Format getFormat() const = 0;
	// Sampling
	void sample(const glm::fvec2& pos, const glm::uvec2& screenpos, glm::fvec4& colourKernel, TextureFiltering filteringType = NEAREST_NEIGHBOUR, Wrap wrap = REPEAT) const;
	inline glm::fvec4 sample(const glm::fvec2& pos, const glm::uvec2& screenpos, TextureFiltering filteringType = NEAREST_NEIGHBOUR, Wrap wrap = REPEAT) const {
		glm::fvec4 tmp;
		sample(pos,screenpos,tmp,filteringType,wrap);
		return tmp;
	}
	virtual void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap = REPEAT) const = 0;
	inline glm::fvec4 getPixel(const glm::uvec2& pos, Wrap wrap = REPEAT) const {
		glm::fvec4 tmp;
		getPixel(pos,tmp,wrap);
		return tmp;
	}
	// Editing
	virtual const void* getRawPixels() const = 0;
	virtual void iterateOverPixels(const ColourIterator& program) const = 0;
	virtual void iterateOverPixels(const ColourIterator2& program) const = 0;
	virtual void iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const = 0;
	virtual void iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const = 0;
	// For saving into files
	void saveInto(DecodeTarget& destination) const;
	// Produce 1-bit alpha map!
	template <class Allocator> inline void produceAlphaMap(std::vector<bool,Allocator>& alphas, float threshhold) const {
		const int width = getWidth();
		alphas.resize(getWidth() * getHeight());
		iterateOverPixels((ColourIterator)[&alphas,threshhold,width](const glm::uvec2& pos, const glm::fvec4& clr) {
			int index = (pos.y*width)+pos.x;
			alphas[index] = clr.a >= threshhold;
		});
	}
};
class Image2D : public ReadOnlyImage2D
{
public:
	// Colour programmes
	typedef std::function<glm::fvec4(const glm::uvec2&)> ColourProgrammer;
	typedef std::function<glm::fvec4(const glm::uvec2&, const glm::fvec4&)> ColourProgrammer2;
	typedef std::function<glm::fvec4(const glm::fvec2&)> ColourProgrammer3;
	typedef std::function<glm::fvec4(const glm::fvec2&, const glm::fvec4&)> ColourProgrammer4;
public:
	virtual ~Image2D() = default;
	// Editing
	virtual void setPixel(const glm::uvec2& pos, const glm::fvec4& colourKernel) = 0;
	virtual void setPixelDithered(const glm::uvec2& pos, const glm::fvec4& colourKernel) = 0;
	bool setPixelWithBlending(const glm::uvec2& pos,const glm::fvec4& colourKernel, AlphaBlending blendingType);
	virtual void* getRawPixels() = 0;
	virtual const void* getRawPixels() const = 0;
	virtual void clearToColour(const glm::fvec4& colourKernel, bool dither = false) = 0;
	virtual void clearToColour(const ColourProgrammer& program, bool dither = false) = 0;
	virtual void clearToColour(const ColourProgrammer2& program, bool dither = false) = 0;
	virtual void clearToColour(const ColourProgrammer3& program, bool dither = false) = 0;
	virtual void clearToColour(const ColourProgrammer4& program, bool dither = false) = 0;
	virtual void clearToColour(const glm::fvec4& colourKernel, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither = false) = 0;
	virtual void clearToColour(const ColourProgrammer& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither = false) = 0;
	virtual void clearToColour(const ColourProgrammer2& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither = false) = 0;
	virtual void clearToColour(const ColourProgrammer3& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither = false) = 0;
	virtual void clearToColour(const ColourProgrammer4& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither = false) = 0;
	void blit(const ReadOnlyImage2D& cpy, const glm::uvec2& destinationOffset, const glm::uvec2& sourceOffset,
			  const glm::uvec2& sourceDimensionsFromOffset, bool dither = false);
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
class ResizeableImage2D : public Image2D {
public:
	virtual ~ResizeableImage2D() = default;
	virtual bool resize(unsigned newWidth, unsigned newHeight) = 0;
};

}
}
}
#endif // EUPHIMAGE_HPP
