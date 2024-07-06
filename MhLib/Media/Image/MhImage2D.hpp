#ifndef MHIMAGE2D_HPP
#define MHIMAGE2D_HPP
#include <MhLib/Media/Image/MhDecodeTarget.hpp>
#include <MhLib/Media/Image/MhImageType.hpp>
#include <glm/glm.hpp>
#include <span>
#include <MhLib/Io/MhIoDevice.hpp>

namespace MH33 {
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
DEFINE_CLASS(Image2D);
class Image2D {
public:
	// Colour programmes
	typedef std::function<glm::fvec4(const glm::ivec2&)> ColourProgrammer;
	typedef std::function<glm::fvec4(const glm::ivec2&, const glm::fvec4&)> ColourProgrammer2;
	typedef std::function<glm::fvec4(const glm::fvec2&)> ColourProgrammer3;
	typedef std::function<glm::fvec4(const glm::fvec2&, const glm::fvec4&)> ColourProgrammer4;
	// Colour iterators
	typedef std::function<void(const glm::ivec2&, const glm::fvec4&)> ColourIterator;
	typedef std::function<void(const glm::fvec2&, const glm::fvec4&)> ColourIterator2;
public:
	virtual ~Image2D() = default;
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
	// Dimensions
	virtual unsigned getWidth() const = 0;
	virtual float getWidthF() const = 0;
	virtual float getWidthR() const = 0;
	virtual unsigned getHeight() const = 0;
	virtual float getHeightF() const = 0;
	virtual float getHeightR() const = 0;
	// Stride
	virtual unsigned getStride() const = 0;
	virtual Format getFormat() const = 0;
	// Produce 1-bit alpha map!
	void produceAlphaMap(std::vector<bool>& alphas, float threshhold) const;
	// Debug save
	void save(Io::Device& iodev) const;
};

}
}

#endif // MHIMAGE2D_HPP
