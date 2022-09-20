#ifndef ITEXTURE_HPP
#define ITEXTURE_HPP
#include <glm/glm.hpp>
#include <memory>
#include <functional>
namespace SYS {
class ITexture {
public:
	// Colour programmes
	typedef std::function<glm::fvec4(const glm::ivec2&)> ColourProgrammer;
	typedef std::function<glm::fvec4(const glm::ivec2&, const glm::fvec4&)> ColourProgrammer2;
	typedef std::function<glm::fvec4(const glm::fvec2&)> ColourProgrammer3;
	typedef std::function<glm::fvec4(const glm::fvec2&, const glm::fvec4&)> ColourProgrammer4;
	// Colour iterators
	typedef std::function<void(const glm::ivec2&, const glm::fvec4&)> ColourIterator;
	typedef std::function<void(const glm::fvec2&, const glm::fvec4&)> ColourIterator2;
	virtual ~ITexture() = default;
	virtual int getWidth() const = 0;
	virtual float getWidthF() const = 0;
	virtual float getWidthR() const = 0;
	virtual int getHeight() const = 0;
	virtual float getHeightF() const = 0;
	virtual int getStride() const = 0;
	virtual float getHeightR() const = 0;
	virtual bool resize(int newWidth, int newHeight) = 0;
	virtual void blit(const ITexture& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) = 0;
	virtual void blit(const ITexture& cpy, const glm::ivec2 offset) = 0;
	virtual void update() = 0;
	virtual void iterateOverPixels(const ColourIterator& program) const = 0;
	virtual void iterateOverPixels(const ColourIterator2& program) const = 0;
	virtual void getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel) const = 0;
};
typedef std::shared_ptr<ITexture> sTexture;
}
#endif // ITEXTURE_HPP
