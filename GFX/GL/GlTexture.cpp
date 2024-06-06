#include "GlTexture.hpp"
#include "GlUtil.hpp"

namespace GL {

Texture2D::Texture2D()
	: format(MH33::GFX::TextureFormat::INVALID), width(0), height(0), stride(0), widthF(0), widthR(0), heightF(0), heightR(0)
{
	glGenTextures(1,&textureVar);
}

Texture2D::Texture2D(const MH33::Image::DecodeTarget& source, uint8_t wantedMipmaps)
{
	glGenTextures(1,&textureVar);
	reinitialize(source,wantedMipmaps);
}

Texture2D::Texture2D(const MH33::Image::Image2D& source, uint8_t wantedMipmaps)
{
	glGenTextures(1,&textureVar);
	reinitialize(source,wantedMipmaps);
}

Texture2D::~Texture2D()
{
	if(textureVar) glDeleteTextures(1,&textureVar);
}

void Texture2D::reinitialize(const MH33::Image::DecodeTarget& source, uint8_t wantedMipmaps)
{
	format = source.format;
	glBindTexture(GL_TEXTURE_2D,textureVar);
	glInitializeTexture2D(source,textureVar,wantedMipmaps,width,height,stride);
	widthF = static_cast<float>(width - 1);
	widthR = 1.0f / widthF;
	heightF = static_cast<float>(height - 1);
	heightR = 1.0f / heightF;
}

void Texture2D::reinitialize(const MH33::Image::Image2D& source, uint8_t wantedMipmaps)
{
	format = source.getFormat();
	glBindTexture(GL_TEXTURE_2D,textureVar);
	glInitializeTexture2D(source,textureVar,wantedMipmaps,width,height,stride);
	widthF = static_cast<float>(width - 1);
	widthR = 1.0f / widthF;
	heightF = static_cast<float>(height - 1);
	heightR = 1.0f / heightF;
}


MH33::GFX::Handle Texture2D::getNativeHandle()
{
	return { .oglUint = textureVar };
}

MH33::GFX::ConstHandle Texture2D::getNativeHandle() const
{
	return { .oglUint = textureVar };
}

MH33::GFX::TextureFormat Texture2D::getFormat() const
{
	return format;
}

void Texture2D::bind(uint8_t unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, textureVar);
}

void Texture2D::bindAsImage(uint8_t unit, MH33::GFX::ImageBindingType bindingType) const
{
	GLenum access = translateBindingType(bindingType);
	glBindImageTexture(unit, textureVar, 0, false, 0, access, translateToInternalFormat(format));
}

unsigned Texture2D::getWidth() const
{
	return width;
}

float Texture2D::getWidthF() const
{
	return widthF;
}

float Texture2D::getWidthR() const
{
	return widthR;
}

unsigned Texture2D::getHeight() const
{
	return height;
}

float Texture2D::getHeightF() const
{
	return heightF;
}

float Texture2D::getHeightR() const
{
	return heightR;
}

unsigned Texture2D::getStride() const
{
	return stride;
}


Texture3D::Texture3D(const MH33::Image::DecodeTarget& source, uint8_t wantedMipmaps)
{
	glGenTextures(1,&textureVar);
	reinitialize(source,wantedMipmaps);
}

Texture3D::~Texture3D()
{
	if(textureVar) glDeleteTextures(1,&textureVar);
}

Texture3D::Texture3D()
	: format(MH33::GFX::TextureFormat::INVALID), width(0), height(0), depth(0), stride(0), widthF(0), widthR(0), heightF(0), heightR(0), depthF(0), depthR(0)
{
	glGenTextures(1,&textureVar);
}

void Texture3D::reinitialize(const MH33::Image::DecodeTarget& source, uint8_t wantedMipmaps)
{
	format = source.format;
	glBindTexture(GL_TEXTURE_3D,textureVar);
	glInitializeTexture3D(source,false,textureVar,wantedMipmaps,width,height,depth,stride);
	widthF = static_cast<float>(width - 1);
	widthR = 1.0f / widthF;
	heightF = static_cast<float>(height - 1);
	heightR = 1.0f / heightF;
	depthF = static_cast<float>(depth - 1);
	depthR = 1.0f / depthF;
}

MH33::GFX::Handle Texture3D::getNativeHandle()
{
	return { .oglUint = textureVar };
}

MH33::GFX::ConstHandle Texture3D::getNativeHandle() const
{
	return { .oglUint = textureVar };
}

MH33::GFX::TextureFormat Texture3D::getFormat() const
{
	return format;
}

void Texture3D::bind(uint8_t unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_3D, textureVar);
}

void Texture3D::bindAsImage(uint8_t unit, MH33::GFX::ImageBindingType bindingType) const
{
	GLenum access = translateBindingType(bindingType);
	glBindImageTexture(unit, textureVar, 0, false, 0, access, translateToInternalFormat(format));
}

void Texture3D::bindAsImage(uint8_t unit, int layer, MH33::GFX::ImageBindingType bindingType) const
{
	GLenum access = translateBindingType(bindingType);
	glBindImageTexture(unit, textureVar, 0, false, layer, access, translateToInternalFormat(format));
}

unsigned Texture3D::getWidth() const
{
	return width;
}

float Texture3D::getWidthF() const
{
	return widthF;
}

float Texture3D::getWidthR() const
{
	return widthR;
}

unsigned Texture3D::getHeight() const
{
	return height;
}

float Texture3D::getHeightF() const
{
	return heightF;
}

float Texture3D::getHeightR() const
{
	return heightR;
}

unsigned Texture3D::getDepth() const
{
	return depth;
}

float Texture3D::getDepthF() const
{
	return depthF;
}

float Texture3D::getDepthR() const
{
	return depthR;
}

unsigned Texture3D::getStride() const
{
	return stride;
}

TextureArray2D::TextureArray2D(const MH33::Image::DecodeTarget& source, uint8_t wantedMipmaps)
{
	glGenTextures(1,&textureVar);
	reinitialize(source,wantedMipmaps);
}

TextureArray2D::~TextureArray2D()
{
	if(textureVar) glDeleteTextures(1,&textureVar);
}

TextureArray2D::TextureArray2D()
	: format(MH33::GFX::TextureFormat::INVALID), width(0), height(0), stride(0), textureCount(0), widthF(0), widthR(0), heightF(0), heightR(0)
{
	glGenTextures(1,&textureVar);
}

void TextureArray2D::reinitialize(const MH33::Image::DecodeTarget& source, uint8_t wantedMipmaps)
{
	format = source.format;
	glBindTexture(GL_TEXTURE_2D_ARRAY,textureVar);
	glInitializeTexture3D(source,true,textureVar,wantedMipmaps,width,height,textureCount,stride);
	widthF = static_cast<float>(width - 1);
	widthR = 1.0f / widthF;
	heightF = static_cast<float>(height - 1);
	heightR = 1.0f / heightF;
}

MH33::GFX::Handle TextureArray2D::getNativeHandle()
{
	return { .oglUint = textureVar };
}

MH33::GFX::ConstHandle TextureArray2D::getNativeHandle() const
{
	return { .oglUint = textureVar };
}

MH33::GFX::TextureFormat TextureArray2D::getFormat() const
{
	return format;
}

void TextureArray2D::bind(uint8_t unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureVar);
}

void TextureArray2D::bindAsImage(uint8_t unit, MH33::GFX::ImageBindingType bindingType) const
{
	GLenum access = translateBindingType(bindingType);
	glBindImageTexture(unit, textureVar, 0, true, 0, access, translateToInternalFormat(format));
}

void TextureArray2D::bindAsImage(uint8_t unit, int layer, MH33::GFX::ImageBindingType bindingType) const
{
	GLenum access = translateBindingType(bindingType);
	glBindImageTexture(unit, textureVar, 0, false, layer, access, translateToInternalFormat(format));
}

unsigned TextureArray2D::getWidth() const
{
	return width;
}

float TextureArray2D::getWidthF() const
{
	return widthF;
}

float TextureArray2D::getWidthR() const
{
	return widthR;
}

unsigned TextureArray2D::getHeight() const
{
	return height;
}

float TextureArray2D::getHeightF() const
{
	return heightF;
}

float TextureArray2D::getHeightR() const
{
	return heightR;
}

unsigned TextureArray2D::getTextureCount() const
{
	return textureCount;
}

unsigned TextureArray2D::getStride() const
{
	return stride;
}

Cubemap::Cubemap(const MH33::Image::DecodeTarget& source, uint8_t wantedMipmaps)
{
	glGenTextures(1,&textureVar);
	reinitialize(source,wantedMipmaps);
}

Cubemap::~Cubemap()
{
	if(textureVar) glDeleteTextures(1,&textureVar);
}

void Cubemap::reinitialize(const MH33::Image::DecodeTarget& source, uint8_t wantedMipmaps)
{
	format = source.format;
	glBindTexture(GL_TEXTURE_2D_ARRAY,textureVar);
	glInitializeTextureCubemnap(source,textureVar,wantedMipmaps,width,height,stride);
	widthF = static_cast<float>(width - 1);
	widthR = 1.0f / widthF;
	heightF = static_cast<float>(height - 1);
	heightR = 1.0f / heightF;
}

Cubemap::Cubemap()
	: format(MH33::GFX::TextureFormat::INVALID), width(0), height(0), stride(0), widthF(0), widthR(0), heightF(0), heightR(0)
{
	glGenTextures(1,&textureVar);
}

MH33::GFX::Handle Cubemap::getNativeHandle()
{
	return { .oglUint = textureVar };
}

MH33::GFX::ConstHandle Cubemap::getNativeHandle() const
{
	return { .oglUint = textureVar };
}

MH33::GFX::TextureFormat Cubemap::getFormat() const
{
	return format;
}

void Cubemap::bind(uint8_t unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureVar);
}

void Cubemap::bindAsImage(uint8_t unit, MH33::GFX::ImageBindingType bindingType) const
{
	GLenum access = translateBindingType(bindingType);
	glBindImageTexture(unit, textureVar, 0, true, 0, access, translateToInternalFormat(format));
}

void Cubemap::bindAsImage(uint8_t unit, uint8_t layer, MH33::GFX::ImageBindingType bindingType) const
{
	GLenum access = translateBindingType(bindingType);
	glBindImageTexture(unit, textureVar, 0, false, layer, access, translateToInternalFormat(format));
}

unsigned Cubemap::getWidth() const
{
	return width;
}

float Cubemap::getWidthF() const
{
	return widthF;
}

float Cubemap::getWidthR() const
{
	return widthR;
}

unsigned Cubemap::getHeight() const
{
	return height;
}

float Cubemap::getHeightF() const
{
	return heightF;
}

float Cubemap::getHeightR() const
{
	return heightR;
}

unsigned Cubemap::getStride() const
{
	return stride;
}


}
