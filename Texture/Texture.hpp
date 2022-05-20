#ifndef TEXTURE_HPP
#define TEXTURE_HPP
#include <glm/glm.hpp>
#include <functional>

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
	typedef std::function<glm::vec4(const glm::ivec2&)> ColourProgrammer;
	typedef std::function<glm::vec4(const glm::ivec2&, const glm::vec4&)> ColourProgrammer2;
	virtual ~Texture() = default;
	// Data getters
	virtual int getWidth() const = 0;
	virtual float getWidthF() const = 0;
	virtual int getHeight() const = 0;
	virtual float getHeightF() const = 0;
	virtual int getStride() const = 0;
	// Pixel manipulation
	virtual void getPixel(const glm::ivec2& pos, glm::vec4& colourKernel, Wrap wrap = REPEAT) const = 0;
	inline glm::vec4 getPixel(const glm::ivec2& pos, Wrap wrap = REPEAT) const {
		glm::vec4 tmp;
		getPixel(pos,tmp,wrap);
		return tmp;
	}
	virtual void setPixel(const glm::ivec2& pos, const glm::vec4& colourKernel) = 0;
	virtual void setPixelDithered(const glm::ivec2& pos, const glm::vec4& colourKernel) = 0;
	bool setPixelWithBlending(const glm::ivec2& pos,const glm::vec4& colourKernel, AlphaBlending blendingType);
	virtual void* getRawPixels() = 0;
	virtual const void* getRawPixels() const = 0;

	virtual void sample(const glm::vec2& pos, const glm::ivec2& screenpos, glm::vec4& colourKernel, TextureFiltering filteringType = NEAREST_NEIGHBOUR, Wrap wrap = REPEAT) const;
	inline glm::vec4 sample(const glm::vec2& pos, const glm::ivec2& screenpos, TextureFiltering filteringType = NEAREST_NEIGHBOUR, Wrap wrap = REPEAT) const {
		glm::vec4 tmp;
		sample(pos,screenpos,tmp,filteringType,wrap);
		return tmp;
	}
	inline void sample(const glm::vec2& pos, const glm::ivec2& screenpos, glm::vec4& colourKernel, const Sampler& sampler) const {
		sample(pos,screenpos,colourKernel,sampler.filtering,sampler.wrap);
	}
	inline glm::vec4 sample(const glm::vec2& pos, const glm::ivec2& screenpos, const Sampler& sampler) const {
		glm::vec4 tmp;
		sample(pos,screenpos,tmp,sampler.filtering,sampler.wrap);
		return tmp;
	}
	virtual void clearToColour(const glm::vec4& colourKernel) = 0;
	virtual void clearToColour(const ColourProgrammer& program) = 0;
	virtual void clearToColour(const ColourProgrammer2& program) = 0;
};

#endif // TEXTURE_HPP
