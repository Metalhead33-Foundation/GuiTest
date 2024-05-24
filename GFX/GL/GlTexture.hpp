#ifndef GLTEXTURE_HPP
#define GLTEXTURE_HPP
#include <GFX/Abstract/Texture.hpp>
#include <NativeGfxApi/glad/glwrap.h>
#include <MhLib/Media/Image/MhDecodeTarget.hpp>

namespace GL {

GLenum translateBindingType(MH33::GFX::ImageBindingType bindingType);
GLenum translateToInternalFormat(MH33::GFX::TextureFormat format);
GLenum translateToFormat(MH33::GFX::TextureFormat format);
GLenum translateToType(MH33::GFX::TextureFormat format);
void glInitializeTexture2D(const MH33::Image::DecodeTarget& source, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& stride);
void glInitializeTexture3D(const MH33::Image::DecodeTarget& source, bool isArray, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& depth, unsigned& stride);
void glInitializeTextureCubemnap(const MH33::Image::DecodeTarget& source, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& stride);


class Texture2D : public MH33::GFX::Texture2D
{
private:
	GLuint textureVar;
	MH33::GFX::TextureFormat format;
	unsigned width, height, stride;
	float widthF, widthR, heightF, heightR;
public:
	Texture2D();
	Texture2D(const MH33::Image::DecodeTarget& source, uint8_t wantedMipmaps = 0);
	~Texture2D();
	void reinitialize(const MH33::Image::DecodeTarget& source, uint8_t wantedMipmaps = 0);
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	// Texture2D interface
	MH33::GFX::TextureFormat getFormat() const override;
	void bind(uint8_t unit) const override;
	void bindAsImage(uint8_t unit, MH33::GFX::ImageBindingType bindingType) const override;
	unsigned getWidth() const override;
	float getWidthF() const override;
	float getWidthR() const override;
	unsigned getHeight() const override;
	float getHeightF() const override;
	float getHeightR() const override;
	unsigned getStride() const override;
};
class Texture3D : public MH33::GFX::Texture3D {
private:
	GLuint textureVar;
	MH33::GFX::TextureFormat format;
	unsigned width, height, depth, stride;
	float widthF, widthR, heightF, heightR, depthF, depthR;
public:
	Texture3D();
	Texture3D(const MH33::Image::DecodeTarget& source, uint8_t wantedMipmaps = 0);
	~Texture3D();
	void reinitialize(const MH33::Image::DecodeTarget& source, uint8_t wantedMipmaps = 0);
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	MH33::GFX::TextureFormat getFormat() const override;
	void bind(uint8_t unit) const override;
	void bindAsImage(uint8_t unit, MH33::GFX::ImageBindingType bindingType) const override;
	void bindAsImage(uint8_t unit, int layer, MH33::GFX::ImageBindingType bindingType) const override;
	unsigned getWidth() const override;
	float getWidthF() const override;
	float getWidthR() const override;
	unsigned getHeight() const override;
	float getHeightF() const override;
	float getHeightR() const override;
	unsigned getDepth() const override;
	float getDepthF() const override;
	float getDepthR() const override;
	unsigned getStride() const override;
};
class TextureArray2D : public MH33::GFX::TextureArray2D {
private:
	GLuint textureVar;
	MH33::GFX::TextureFormat format;
	unsigned width, height, stride, textureCount;
	float widthF, widthR, heightF, heightR;
public:
	TextureArray2D();
	TextureArray2D(const MH33::Image::DecodeTarget& source, uint8_t wantedMipmaps = 0);
	~TextureArray2D();
	void reinitialize(const MH33::Image::DecodeTarget& source, uint8_t wantedMipmaps = 0);
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	MH33::GFX::TextureFormat getFormat() const override;
	void bind(uint8_t unit) const override;
	void bindAsImage(uint8_t unit, MH33::GFX::ImageBindingType bindingType) const override;
	void bindAsImage(uint8_t unit, int layer, MH33::GFX::ImageBindingType bindingType) const override;
	unsigned getWidth() const override;
	float getWidthF() const override;
	float getWidthR() const override;
	unsigned getHeight() const override;
	float getHeightF() const override;
	float getHeightR() const override;
	unsigned getTextureCount() const override;
	unsigned getStride() const override;
};
class Cubemap : public MH33::GFX::Cubemap {
private:
	GLuint textureVar;
	MH33::GFX::TextureFormat format;
	unsigned width, height, stride;
	float widthF, widthR, heightF, heightR;
public:
	Cubemap();
	Cubemap(const MH33::Image::DecodeTarget& source, uint8_t wantedMipmaps = 0);
	~Cubemap();
	void reinitialize(const MH33::Image::DecodeTarget& source, uint8_t wantedMipmaps = 0);
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	MH33::GFX::TextureFormat getFormat() const override;
	void bind(uint8_t unit) const override;
	void bindAsImage(uint8_t unit, MH33::GFX::ImageBindingType bindingType) const override;
	void bindAsImage(uint8_t unit, uint8_t layer, MH33::GFX::ImageBindingType bindingType) const override;
	unsigned getWidth() const override;
	float getWidthF() const override;
	float getWidthR() const override;
	unsigned getHeight() const override;
	float getHeightF() const override;
	float getHeightR() const override;
	unsigned getStride() const override;
};

}

#endif // GLTEXTURE_HPP
