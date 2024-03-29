#ifndef SOFTTEXTURE_HPP
#define SOFTTEXTURE_HPP
#include <MhLib/Media/GFX/MhTexture.hpp>

namespace Renderer {
namespace Software {

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

struct Sampler {
	Wrap wrap;
	TextureFiltering filtering;
};

DEFINE_CLASS(Texture)
class Texture : public MH33::GFX::WriteableTexture2D
{
public:
	virtual ~Texture() = default;
	// Colour programmes
	typedef std::function<glm::fvec4(const glm::ivec2&)> ColourProgrammer;
	typedef std::function<glm::fvec4(const glm::ivec2&, const glm::fvec4&)> ColourProgrammer2;
	typedef std::function<glm::fvec4(const glm::fvec2&)> ColourProgrammer3;
	typedef std::function<glm::fvec4(const glm::fvec2&, const glm::fvec4&)> ColourProgrammer4;
	// Colour iterators
	typedef std::function<void(const glm::ivec2&, const glm::fvec4&)> ColourIterator;
	typedef std::function<void(const glm::fvec2&, const glm::fvec4&)> ColourIterator2;
	virtual void iterateOverPixels(const ColourIterator& program) const = 0;
	virtual void iterateOverPixels(const ColourIterator2& program) const = 0;
	virtual void getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel, Wrap wrap) const = 0;
	void getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel) const {
		getPixel(pos,colourKernel,REPEAT);
	}
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
	void update() override;
	void blit(const WriteableTexture2D& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions);
	void blit(const WriteableTexture2D& cpy, const glm::ivec2 offset);
	void blit(const Texture& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions);
	void blit(const Texture& cpy, const glm::ivec2 offset);
	MH33::GFX::Handle getNativeHandle();
	MH33::GFX::ConstHandle getNativeHandle() const;
};
}
}

#endif // SOFTTEXTURE_HPP
