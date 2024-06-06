#ifndef TEXTURE_H
#define TEXTURE_H
#include <MhLib/Util/MhGlobals.hpp>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <span>
#include <glm/glm.hpp>
#include "NativeHandle.hpp"
#include <MhLib/Media/Image/MhImageType.hpp>
#include <MhLib/Media/Image/MhImage2D.hpp>
namespace MH33 {
namespace GFX {

typedef Image::Format TextureFormat;
enum class ImageBindingType {
	READ_ONLY = 1,
	WRITE_ONLY = 2,
	READ_WRITE = READ_ONLY | WRITE_ONLY
};

DEFINE_CLASS(Texture2D)
class Texture2D : public GfxResource {
public:
	virtual ~Texture2D() = default;
	virtual TextureFormat getFormat() const = 0;
	virtual void bind(uint8_t unit) const = 0;
	virtual void bindAsImage(uint8_t unit, ImageBindingType bindingType) const = 0;
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

DEFINE_CLASS(WriteableTexture2D)
class WriteableTexture2D : public Texture2D {
public:
	// Colour programmes
	typedef MH33::Image::Image2D::ColourProgrammer ColourProgrammer;
	typedef MH33::Image::Image2D::ColourProgrammer2 ColourProgrammer2;
	typedef MH33::Image::Image2D::ColourProgrammer3 ColourProgrammer3;
	typedef MH33::Image::Image2D::ColourProgrammer4 ColourProgrammer4;
	// Colour iterators
	typedef MH33::Image::Image2D::ColourIterator ColourIterator;
	typedef MH33::Image::Image2D::ColourIterator2 ColourIterator2;
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
	virtual void blit(const MH33::Image::Image2D& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) = 0;
	virtual void blit(const MH33::Image::Image2D& cpy, const glm::ivec2 offset) = 0;
	virtual void blit(const std::span<const std::byte>& data, TextureFormat format, const glm::ivec2 offset, const glm::ivec2& dimensions) = 0;
	virtual void onRegionUpdate(const glm::ivec2& topleft, const glm::ivec2& bottomright) = 0;
	virtual void update() = 0;
};

DEFINE_CLASS(TextureArray2D)
class TextureArray2D : public GfxResource {
public:
	virtual ~TextureArray2D() = default;
	virtual TextureFormat getFormat() const = 0;
	virtual void bind(uint8_t unit) const = 0;
	virtual void bindAsImage(uint8_t unit, ImageBindingType bindingType) const = 0;
	virtual void bindAsImage(uint8_t unit, int layer, ImageBindingType bindingType) const = 0;
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

DEFINE_CLASS(Texture3D)
class Texture3D : public GfxResource {
public:
	virtual ~Texture3D() = default;
	virtual TextureFormat getFormat() const = 0;
	virtual void bind(uint8_t unit) const = 0;
	virtual void bindAsImage(uint8_t unit, ImageBindingType bindingType) const = 0;
	virtual void bindAsImage(uint8_t unit, int layer, ImageBindingType bindingType) const = 0;
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

DEFINE_CLASS(Cubemap)
class Cubemap : public GfxResource {
public:
	virtual ~Cubemap() = default;
	virtual TextureFormat getFormat() const = 0;
	virtual void bind(uint8_t unit) const = 0;
	virtual void bindAsImage(uint8_t unit, ImageBindingType bindingType) const = 0;
	virtual void bindAsImage(uint8_t unit, uint8_t layer, ImageBindingType bindingType) const = 0;
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

}
}
#endif // TEXTURE_H
