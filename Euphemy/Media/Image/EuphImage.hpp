#ifndef EUPHIMAGE_HPP
#define EUPHIMAGE_HPP
#include <glm/glm.hpp>
#include <span>
#include <functional>
#include <Euphemy/Media/Image/EuphImageType.hpp>
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
namespace Euph {
namespace Media {
namespace Image {

enum TextureFiltering {
	NEAREST_NEIGHBOUR,
	DITHERED,
	THREE_POINT,
	BILINEAR
};
enum AlphaBlending {
	ALPHA_TESTING,
	ALPHA_DITHERING,
	ALPHA_BLENDING
};

enum Wrap {
	REPEAT,
	MIRRORED_REPEAT,
	CLAMP_TO_EDGE,
	CLAMP_TO_BORDER
};

class Image2D
{
public:
	// Colour programmes
	typedef std::function<glm::fvec4(const glm::ivec2&)> ColourProgrammer;
	typedef std::function<glm::fvec4(const glm::ivec2&, const glm::fvec4&)> ColourProgrammer2;
	typedef std::function<glm::fvec4(const glm::fvec2&)> ColourProgrammer3;
	typedef std::function<glm::fvec4(const glm::fvec2&, const glm::fvec4&)> ColourProgrammer4;
	// Colour iterators
	typedef std::function<void(const glm::ivec2&, const glm::fvec4&)> ColourIterator;
	typedef std::function<void(const glm::fvec2&, const glm::fvec4&)> ColourIterator2;
protected:
	unsigned width, height, stride;
	float widthF, heightF;
	float widthR, heightR;
	void recalculateDimensions();
public:
	virtual ~Image2D() = default;
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
	void sample(const glm::fvec2& pos, const glm::ivec2& screenpos, glm::fvec4& colourKernel, TextureFiltering filteringType = NEAREST_NEIGHBOUR, Wrap wrap = REPEAT) const;
	inline glm::fvec4 sample(const glm::fvec2& pos, const glm::ivec2& screenpos, TextureFiltering filteringType = NEAREST_NEIGHBOUR, Wrap wrap = REPEAT) const {
		glm::fvec4 tmp;
		sample(pos,screenpos,tmp,filteringType,wrap);
		return tmp;
	}
	virtual void getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel, Wrap wrap = REPEAT) const = 0;
	inline glm::fvec4 getPixel(const glm::ivec2& pos, Wrap wrap = REPEAT) const {
		glm::fvec4 tmp;
		getPixel(pos,tmp,wrap);
		return tmp;
	}
	// Editing
	virtual void setPixel(const glm::ivec2& pos, const glm::fvec4& colourKernel) = 0;
	virtual void setPixelDithered(const glm::ivec2& pos, const glm::fvec4& colourKernel) = 0;
	bool setPixelWithBlending(const glm::ivec2& pos,const glm::fvec4& colourKernel, AlphaBlending blendingType);
	virtual void* getRawPixels() = 0;
	virtual const void* getRawPixels() const = 0;
	virtual bool resize(int newWidth, int newHeight) = 0;
	virtual void iterateOverPixels(const ColourIterator& program) const = 0;
	virtual void iterateOverPixels(const ColourIterator2& program) const = 0;
	virtual void iterateOverPixels(const ColourIterator& program, const glm::ivec2 offset, const glm::ivec2& dimensions) const = 0;
	virtual void iterateOverPixels(const ColourIterator2& program, const glm::ivec2 offset, const glm::ivec2& dimensions) const = 0;
	virtual void clearToColour(const glm::fvec4& colourKernel) = 0;
	virtual void clearToColour(const ColourProgrammer& program) = 0;
	virtual void clearToColour(const ColourProgrammer2& program) = 0;
	virtual void clearToColour(const ColourProgrammer3& program) = 0;
	virtual void clearToColour(const ColourProgrammer4& program) = 0;
	virtual void clearToColour(const glm::fvec4& colourKernel, const glm::ivec2 offset, const glm::ivec2& dimensions) = 0;
	virtual void clearToColour(const ColourProgrammer& program, const glm::ivec2 offset, const glm::ivec2& dimensions) = 0;
	virtual void clearToColour(const ColourProgrammer2& program, const glm::ivec2 offset, const glm::ivec2& dimensions) = 0;
	virtual void clearToColour(const ColourProgrammer3& program, const glm::ivec2 offset, const glm::ivec2& dimensions) = 0;
	virtual void clearToColour(const ColourProgrammer4& program, const glm::ivec2 offset, const glm::ivec2& dimensions) = 0;
	virtual void blit(const Image2D& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) ;
	virtual void blit(const Image2D& cpy, const glm::ivec2 offset);
	virtual void blit(const std::span<const std::byte>& data, Format format, const glm::ivec2 offset, const glm::ivec2& dimensions) = 0;
	// For saving into files
	void saveInto(DecodeTarget& destination) const;
	// Produce 1-bit alpha map!
	template <class Allocator> inline void produceAlphaMap(std::vector<bool,Allocator>& alphas, float threshhold) const {
		const int width = getWidth();
		alphas.resize(getWidth() * getHeight());
		iterateOverPixels((ColourIterator)[&alphas,threshhold,width](const glm::ivec2& pos, const glm::fvec4& clr) {
			int index = (pos.y*width)+pos.x;
			alphas[index] = clr.a >= threshhold;
		});
	}
};

}
}
}
#endif // EUPHIMAGE_HPP
