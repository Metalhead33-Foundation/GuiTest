#ifndef MHTEXTURE_H
#define MHTEXTURE_H
#include <MhLib/Media/Image/MhImageType.hpp>
#include <MhLib/Media/GFX/MhGFXHandle.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <functional>
#include <span>

namespace MH33 {
namespace GFX {
typedef Image::Format TextureFormat;

DEFINE_CLASS(Texture2D)
class Texture2D {
public:
	virtual ~Texture2D() = default;
	virtual Handle getNativeHandle() = 0;
	virtual ConstHandle getNativeHandle() const = 0;
	virtual TextureFormat getFormat() const = 0;
	/// Data getters
	// Width
	virtual int getWidth() const = 0;
	virtual float getWidthF() const = 0;
	virtual float getWidthR() const = 0;
	// Height
	virtual int getHeight() const = 0;
	virtual float getHeightF() const = 0;
	virtual float getHeightR() const = 0;
	// Stride
	virtual int getStride() const = 0;
};

DEFINE_CLASS(WriteableTexture2D)
class WriteableTexture2D : public Texture2D {
public:
	virtual ~WriteableTexture2D() override = default;
	virtual bool resize(int newWidth, int newHeight) = 0;
	virtual void blit(const WriteableTexture2D& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) = 0;
	virtual void blit(const WriteableTexture2D& cpy, const glm::ivec2 offset) = 0;
	virtual void blit(const std::span<const std::byte>& data, TextureFormat format, const glm::ivec2 offset, const glm::ivec2& dimensions) = 0;
	virtual void update() = 0;
	/*
	virtual void getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel) const = 0;
	// Colour programmes
	typedef std::function<glm::fvec4(const glm::ivec2&)> ColourProgrammer;
	typedef std::function<glm::fvec4(const glm::ivec2&, const glm::fvec4&)> ColourProgrammer2;
	typedef std::function<glm::fvec4(const glm::fvec2&)> ColourProgrammer3;
	typedef std::function<glm::fvec4(const glm::fvec2&, const glm::fvec4&)> ColourProgrammer4;
	// Colour iterators
	typedef std::function<void(const glm::ivec2&, const glm::fvec4&)> ColourIterator;
	typedef std::function<void(const glm::fvec2&, const glm::fvec4&)> ColourIterator2;
	virtual void iterateOverPixels(const ColourIterator& program) const = 0;
	virtual void iterateOverPixels(const ColourIterator2& program) const = 0;*/
};

DEFINE_CLASS(ArrayTexture2D)
class ArrayTexture2D {
public:
	virtual ~ArrayTexture2D() = default;
	virtual Handle getNativeHandle() = 0;
	virtual ConstHandle getNativeHandle() const = 0;
	virtual TextureFormat getFormat() const = 0;
	/// Data getters
	// Width
	virtual int getWidth() const = 0;
	virtual float getWidthF() const = 0;
	virtual float getWidthR() const = 0;
	// Height
	virtual int getHeight() const = 0;
	virtual float getHeightF() const = 0;
	virtual float getHeightR() const = 0;
	// Depth
	virtual int getDepth() const = 0;
	virtual float getDepthF() const = 0;
	virtual float getDepthR() const = 0;
	// Stride
	virtual int getStride() const = 0;
};

DEFINE_CLASS(Texture3D)
class Texture3D {
public:
	virtual ~Texture3D() = default;
	virtual Handle getNativeHandle() = 0;
	virtual ConstHandle getNativeHandle() const = 0;
	virtual TextureFormat getFormat() const = 0;
	/// Data getters
	// Width
	virtual int getWidth() const = 0;
	virtual float getWidthF() const = 0;
	virtual float getWidthR() const = 0;
	// Height
	virtual int getHeight() const = 0;
	virtual float getHeightF() const = 0;
	virtual float getHeightR() const = 0;
	// Depth
	virtual int getDepth() const = 0;
	virtual float getDepthF() const = 0;
	virtual float getDepthR() const = 0;
	// Stride
	virtual int getStride() const = 0;
};

}
}

#endif // MHTEXTURE_H
