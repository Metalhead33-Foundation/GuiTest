#ifndef TEXTURE_H
#define TEXTURE_H
#include <cstdint>
#include <cstddef>
#include <functional>
#include <span>
#include <glm/glm.hpp>
#include "NativeHandle.hpp"
#include "ImageType.hpp"
namespace MH33 {
namespace GFX {
typedef Image::Format TextureFormat;

class Texture2D : public GfxResource {
public:
	virtual ~Texture2D() = default;
	virtual TextureFormat getFormat() const = 0;
	virtual void bind(uint8_t unit) const = 0;
	// Dimensions
	virtual unsigned getWidth() const = 0;
	virtual float getWidthF() const = 0;
	virtual float getWidthR() const = 0;
	virtual unsigned getHeight() const = 0;
	virtual float getHeightF() const = 0;
	virtual float getHeightR() const = 0;
	// Stride
	virtual unsigned getStride() const = 0;
};
class WriteableTexture2D : public Texture2D {
public:
	// Colour programmes
	typedef std::function<glm::fvec4(const glm::ivec2&)> ColourProgrammer;
	typedef std::function<glm::fvec4(const glm::ivec2&, const glm::fvec4&)> ColourProgrammer2;
	typedef std::function<glm::fvec4(const glm::fvec2&)> ColourProgrammer3;
	typedef std::function<glm::fvec4(const glm::fvec2&, const glm::fvec4&)> ColourProgrammer4;
	// Colour iterators
	typedef std::function<void(const glm::ivec2&, const glm::fvec4&)> ColourIterator;
	typedef std::function<void(const glm::fvec2&, const glm::fvec4&)> ColourIterator2;
	// Now for the functions
	virtual ~WriteableTexture2D() = default;
	virtual void* getPixels() = 0;
	virtual const void* getPixels() const = 0;
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
	virtual void blit(const WriteableTexture2D& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) = 0;
	virtual void blit(const WriteableTexture2D& cpy, const glm::ivec2 offset) = 0;
	virtual void blit(const std::span<const std::byte>& data, TextureFormat format, const glm::ivec2 offset, const glm::ivec2& dimensions) = 0;
	virtual void update() = 0;
};
class TextureArray2D : public GfxResource {
public:
	virtual ~TextureArray2D() = default;
	virtual TextureFormat getFormat() const = 0;
	virtual void bind(uint8_t unit) const = 0;
	// Dimensions
	virtual unsigned getWidth() const = 0;
	virtual float getWidthF() const = 0;
	virtual float getWidthR() const = 0;
	virtual unsigned getHeight() const = 0;
	virtual float getHeightF() const = 0;
	virtual float getHeightR() const = 0;
	virtual unsigned getTextureCount() const = 0;
	// Stride
	virtual unsigned getStride() const = 0;
};
class Texture3D : public GfxResource {
public:
	virtual ~Texture3D() = default;
	virtual TextureFormat getFormat() const = 0;
	virtual void bind(uint8_t unit) const = 0;
	// Dimensions
	virtual unsigned getWidth() const = 0;
	virtual float getWidthF() const = 0;
	virtual float getWidthR() const = 0;
	virtual unsigned getHeight() const = 0;
	virtual float getHeightF() const = 0;
	virtual float getHeightR() const = 0;
	virtual unsigned getDepth() const = 0;
	virtual float getDepthF() const = 0;
	virtual float getDepthR() const = 0;
	// Stride
	virtual unsigned getStride() const = 0;
};
class Cubemap : public GfxResource {
public:
	virtual ~Cubemap() = default;
	virtual TextureFormat getFormat() const = 0;
	virtual void bind(uint8_t unit) const = 0;
	// Dimensions
	virtual unsigned getWidth() const = 0;
	virtual float getWidthF() const = 0;
	virtual float getWidthR() const = 0;
	virtual unsigned getHeight() const = 0;
	virtual float getHeightF() const = 0;
	virtual float getHeightR() const = 0;
	// Stride
	virtual unsigned getStride() const = 0;
};

class Framebuffer : public GfxResource {
public:
	virtual ~Framebuffer() = default;
	virtual void bind() const = 0;
	virtual void unbind() const = 0;
	// Dimensions
	virtual unsigned getWidth() = 0;
	virtual float getWidthF() = 0;
	virtual float getWidthR() = 0;
	virtual unsigned getHeight() = 0;
	virtual float getHeightF() = 0;
	virtual float getHeightR() = 0;
	// Get attachment
	virtual Texture2D& getTextureAttachment2D(uint8_t unit) const = 0;
};
class CubemapFramebuffer : public GfxResource {
public:
	virtual ~CubemapFramebuffer() = default;
	virtual void bind() const = 0;
	virtual void unbind() const = 0;
	// Dimensions
	virtual unsigned getWidth() = 0;
	virtual float getWidthF() = 0;
	virtual float getWidthR() = 0;
	virtual unsigned getHeight() = 0;
	virtual float getHeightF() = 0;
	virtual float getHeightR() = 0;
	// Get attachment
	virtual const Cubemap& getCubemap() const = 0;
	virtual const Texture2D& getDepthAttachment(uint8_t unit) const = 0;
};

}
}
#endif // TEXTURE_H
