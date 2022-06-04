#ifndef TEXTURE_HPP
#define TEXTURE_HPP
#include <glm/glm.hpp>
#include <functional>
#include <memory>

enum TextureFiltering {
	NEAREST_NEIGHBOUR,
	DITHERED,
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

struct Sampler {
	Wrap wrap;
	TextureFiltering filtering;
};

class Texture
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
	virtual ~Texture() = default;
	// Data getters
	virtual int getWidth() const = 0;
	virtual float getWidthF() const = 0;
	virtual float getWidthR() const = 0;
	virtual int getHeight() const = 0;
	virtual float getHeightF() const = 0;
	virtual int getStride() const = 0;
	virtual float getHeightR() const = 0;
	// Pixel manipulation
	virtual bool resize(int newWidth, int newHeight) = 0;
	virtual void blit(const Texture& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions);
	virtual void blit(const Texture& cpy, const glm::ivec2 offset);
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

	virtual void sample(const glm::fvec2& pos, const glm::ivec2& screenpos, glm::fvec4& colourKernel, TextureFiltering filteringType = NEAREST_NEIGHBOUR, Wrap wrap = REPEAT) const;
	inline glm::fvec4 sample(const glm::fvec2& pos, const glm::ivec2& screenpos, TextureFiltering filteringType = NEAREST_NEIGHBOUR, Wrap wrap = REPEAT) const {
		glm::fvec4 tmp;
		sample(pos,screenpos,tmp,filteringType,wrap);
		return tmp;
	}
	inline void sample(const glm::fvec2& pos, const glm::ivec2& screenpos, glm::fvec4& colourKernel, const Sampler& sampler) const {
		sample(pos,screenpos,colourKernel,sampler.filtering,sampler.wrap);
	}
	inline glm::fvec4 sample(const glm::fvec2& pos, const glm::ivec2& screenpos, const Sampler& sampler) const {
		glm::fvec4 tmp;
		sample(pos,screenpos,tmp,sampler.filtering,sampler.wrap);
		return tmp;
	}
	virtual void clearToColour(const glm::fvec4& colourKernel) = 0;
	virtual void clearToColour(const ColourProgrammer& program) = 0;
	virtual void clearToColour(const ColourProgrammer2& program) = 0;
	virtual void clearToColour(const ColourProgrammer3& program) = 0;
	virtual void clearToColour(const ColourProgrammer4& program) = 0;
	virtual void iterateOverPixels(const ColourIterator& program) const = 0;
	virtual void iterateOverPixels(const ColourIterator2& program) const = 0;
};
typedef std::shared_ptr<Texture> sTexture;

#endif // TEXTURE_HPP
