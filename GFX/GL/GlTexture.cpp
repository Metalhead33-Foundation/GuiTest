#include "GlTexture.hpp"

namespace GL {

GLenum translateToInternalFormat(MH33::GFX::TextureFormat format)
{
	switch (format) {
		case MH33::GFX::TextureFormat::R8U: return GL_R8;
		case MH33::GFX::TextureFormat::R8S: return GL_R8_SNORM;
		case MH33::GFX::TextureFormat::R16U: return GL_R16;
		case MH33::GFX::TextureFormat::R16S: return GL_R16_SNORM;
		case MH33::GFX::TextureFormat::R16F: return GL_R16F;
		case MH33::GFX::TextureFormat::R32U: return -1;
		case MH33::GFX::TextureFormat::R32S: return -1;
		case MH33::GFX::TextureFormat::R32F: return GL_R32F;
		case MH33::GFX::TextureFormat::R64F: return -1;

		case MH33::GFX::TextureFormat::RG8U: return GL_RG8;
		case MH33::GFX::TextureFormat::RG8S: return GL_RG8_SNORM;
		case MH33::GFX::TextureFormat::RG16U: return GL_RG16;
		case MH33::GFX::TextureFormat::RG16S: return GL_RG16_SNORM;
		case MH33::GFX::TextureFormat::RG16F: return GL_RG16F;
		case MH33::GFX::TextureFormat::RG32U: return -1;
		case MH33::GFX::TextureFormat::RG32S: return -1;
		case MH33::GFX::TextureFormat::RG32F: return GL_RG32F;
		case MH33::GFX::TextureFormat::RG64F: return -1;

		case MH33::GFX::TextureFormat::RGB8U: return GL_RGB8;
		case MH33::GFX::TextureFormat::RGB8S: return GL_RGB8_SNORM;
		case MH33::GFX::TextureFormat::RGB16U: return GL_RGB16;
		case MH33::GFX::TextureFormat::RGB16S: return GL_RGB16_SNORM;
		case MH33::GFX::TextureFormat::RGB16F: return GL_RGB16F;
		case MH33::GFX::TextureFormat::RGB32U: return -1;
		case MH33::GFX::TextureFormat::RGB32S: return -1;
		case MH33::GFX::TextureFormat::RGB32F: return GL_RGB32F;
		case MH33::GFX::TextureFormat::RGB64F: return -1;

		case MH33::GFX::TextureFormat::BGR8U: return GL_RGB8;
		case MH33::GFX::TextureFormat::BGR8S: return GL_RGB8_SNORM;
		case MH33::GFX::TextureFormat::BGR16U: return GL_RGB16;
		case MH33::GFX::TextureFormat::BGR16S: return GL_RGB16_SNORM;
		case MH33::GFX::TextureFormat::BGR16F: return GL_RGB16F;
		case MH33::GFX::TextureFormat::BGR32U: return -1;
		case MH33::GFX::TextureFormat::BGR32S: return -1;
		case MH33::GFX::TextureFormat::BGR32F: return GL_RGB32F;
		case MH33::GFX::TextureFormat::BGR64F: return -1;

		case MH33::GFX::TextureFormat::RGBA8U: return GL_RGBA8;
		case MH33::GFX::TextureFormat::RGBA8S: return GL_RGBA8_SNORM;
		case MH33::GFX::TextureFormat::RGBA16U: return GL_RGBA16;
		case MH33::GFX::TextureFormat::RGBA16S: return GL_RGBA16_SNORM;
		case MH33::GFX::TextureFormat::RGBA16F: return GL_RGBA16F;
		case MH33::GFX::TextureFormat::RGBA32U: return -1;
		case MH33::GFX::TextureFormat::RGBA32S: return -1;
		case MH33::GFX::TextureFormat::RGBA32F: return GL_RGBA32F;
		case MH33::GFX::TextureFormat::RGBA64F: return -1;

		case MH33::GFX::TextureFormat::BGRA8U: return GL_RGBA8;
		case MH33::GFX::TextureFormat::BGRA8S: return GL_RGBA8_SNORM;
		case MH33::GFX::TextureFormat::BGRA16U: return GL_RGBA16;
		case MH33::GFX::TextureFormat::BGRA16S: return GL_RGBA16_SNORM;
		case MH33::GFX::TextureFormat::BGRA16F: return GL_RGBA16F;
		case MH33::GFX::TextureFormat::BGRA32U: return -1;
		case MH33::GFX::TextureFormat::BGRA32S: return -1;
		case MH33::GFX::TextureFormat::BGRA32F: return GL_RGBA32F;
		case MH33::GFX::TextureFormat::BGRA64F: return -1;

		case MH33::GFX::TextureFormat::ARGB8U: return GL_RGBA8;
		case MH33::GFX::TextureFormat::ARGB8S: return GL_RGBA8_SNORM;
		case MH33::GFX::TextureFormat::ARGB16U: return GL_RGBA16;
		case MH33::GFX::TextureFormat::ARGB16S: return GL_RGBA16_SNORM;
		case MH33::GFX::TextureFormat::ARGB16F: return GL_RGBA16F;
		case MH33::GFX::TextureFormat::ARGB32U: return -1;
		case MH33::GFX::TextureFormat::ARGB32S: return -1;
		case MH33::GFX::TextureFormat::ARGB32F: return GL_RGBA32F;
		case MH33::GFX::TextureFormat::ARGB64F: return -1;

		case MH33::GFX::TextureFormat::INDEXED: return -1;
		case MH33::GFX::TextureFormat::RGB565: return GL_RGB;
		case MH33::GFX::TextureFormat::RGB555: return GL_RGB5;
		case MH33::GFX::TextureFormat::RGB444: return GL_RGB4;
		case MH33::GFX::TextureFormat::RGBA444: return GL_RGBA4;
		case MH33::GFX::TextureFormat::DEPTH_COMPONENT16: return GL_DEPTH_COMPONENT;
		case MH33::GFX::TextureFormat::DEPTH_COMPONENT24: return GL_DEPTH_COMPONENT;
		case MH33::GFX::TextureFormat::DEPTH_COMPONENT32: return GL_DEPTH_COMPONENT;
		case MH33::GFX::TextureFormat::DEPTH_COMPONENT32F: return GL_DEPTH_COMPONENT;
		case MH33::GFX::TextureFormat::DEPTH24_STENCIL8: return GL_DEPTH_STENCIL;
		case MH33::GFX::TextureFormat::DEPTH32F_STENCIL8: return GL_DEPTH_STENCIL;
		case MH33::GFX::TextureFormat::STENCIL_INDEX8: return GL_STENCIL_INDEX;

		case MH33::GFX::TextureFormat::B_DXT1: return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		case MH33::GFX::TextureFormat::B_DXT3: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		case MH33::GFX::TextureFormat::B_DXT5: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		default: return -1;
	}
}

GLenum translateToFormat(MH33::GFX::TextureFormat format)
{
	GLenum toReturn = -1;
	switch (format) {
		case MH33::GFX::TextureFormat::RGB555: toReturn = GL_RGB; break;
		case MH33::GFX::TextureFormat::RGB565: toReturn = GL_RGB; break;
		case MH33::GFX::TextureFormat::RGB444: toReturn = GL_RGB; break;
		case MH33::GFX::TextureFormat::RGBA444: toReturn = GL_RGBA; break;
		case MH33::GFX::TextureFormat::DEPTH_COMPONENT16: toReturn = GL_DEPTH_COMPONENT; break;
		case MH33::GFX::TextureFormat::DEPTH_COMPONENT24: toReturn = GL_DEPTH_COMPONENT; break;
		case MH33::GFX::TextureFormat::DEPTH_COMPONENT32: toReturn = GL_DEPTH_COMPONENT; break;
		case MH33::GFX::TextureFormat::DEPTH_COMPONENT32F: toReturn = GL_DEPTH_COMPONENT; break;
		case MH33::GFX::TextureFormat::STENCIL_INDEX8: toReturn = GL_STENCIL_INDEX; break;
		case MH33::GFX::TextureFormat::DEPTH24_STENCIL8: toReturn = GL_DEPTH_STENCIL; break;
		case MH33::GFX::TextureFormat::DEPTH32F_STENCIL8: toReturn = GL_DEPTH_STENCIL; break;
		default: break;
	}
	if(toReturn == GLenum(-1)) {
		switch (unsigned(format) | unsigned(MH33::GFX::TextureFormat::COLOR_COMPONENT_MASK)) {
			case unsigned(MH33::GFX::TextureFormat::ST_GREYSCALE): return GL_RED;
			case unsigned(MH33::GFX::TextureFormat::ST_RG): return GL_RG;
			case unsigned(MH33::GFX::TextureFormat::ST_RGB): return GL_RGB;
			case unsigned(MH33::GFX::TextureFormat::ST_BGR): return GL_BGR;
			case unsigned(MH33::GFX::TextureFormat::ST_RGBA): return GL_RGBA;
			case unsigned(MH33::GFX::TextureFormat::ST_BGRA): return GL_BGRA;
			default: return -1;
		}
	} else return toReturn;
}

GLenum translateToType(MH33::GFX::TextureFormat format)
{
	GLenum toReturn = -1;
	switch (format) {
		case MH33::GFX::TextureFormat::RGB555: toReturn =  GL_UNSIGNED_SHORT_5_5_5_1; break;
		case MH33::GFX::TextureFormat::RGB565: toReturn =  GL_UNSIGNED_SHORT_5_6_5; break;
		case MH33::GFX::TextureFormat::RGB444: toReturn =  GL_UNSIGNED_SHORT_4_4_4_4; break;
		case MH33::GFX::TextureFormat::RGBA444: toReturn =  GL_UNSIGNED_SHORT_4_4_4_4; break;
		case MH33::GFX::TextureFormat::DEPTH_COMPONENT16: toReturn = GL_UNSIGNED_SHORT; break;
		case MH33::GFX::TextureFormat::DEPTH_COMPONENT24: toReturn = GL_UNSIGNED_INT_24_8; break;
		case MH33::GFX::TextureFormat::DEPTH_COMPONENT32: toReturn = GL_UNSIGNED_INT; break;
		case MH33::GFX::TextureFormat::DEPTH_COMPONENT32F: toReturn = GL_FLOAT; break;
		case MH33::GFX::TextureFormat::STENCIL_INDEX8: toReturn = GL_UNSIGNED_BYTE; break;
		case MH33::GFX::TextureFormat::DEPTH24_STENCIL8: toReturn = GL_UNSIGNED_INT_24_8; break;
		case MH33::GFX::TextureFormat::DEPTH32F_STENCIL8: toReturn = GL_FLOAT_32_UNSIGNED_INT_24_8_REV; break;
		default: break;
	}
	if(toReturn == GLenum(-1)) {
		switch (unsigned(format) | unsigned(MH33::GFX::TextureFormat::COLOR_RESOLUTION_MASK)) {
			case unsigned(MH33::GFX::TextureFormat::SC_UINT8): return  GL_UNSIGNED_BYTE;
			case unsigned(MH33::GFX::TextureFormat::SC_UINT16): return  GL_UNSIGNED_SHORT;
			case unsigned(MH33::GFX::TextureFormat::SC_UINT32): return  GL_UNSIGNED_INT;
			case unsigned(MH33::GFX::TextureFormat::SC_INT8): return  GL_BYTE;
			case unsigned(MH33::GFX::TextureFormat::SC_INT16): return  GL_SHORT;
			case unsigned(MH33::GFX::TextureFormat::SC_INT32): return  GL_INT;
			case unsigned(MH33::GFX::TextureFormat::SC_F16): return  GL_HALF_FLOAT;
			case unsigned(MH33::GFX::TextureFormat::SC_F32): return  GL_FLOAT;
			default: return -1;
		}
	} else return toReturn;
}

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

Texture2D::~Texture2D()
{
	glDeleteTextures(1,&textureVar);
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
	glBindImageTexture(GL_IMAGE_2D, textureVar, 0, false, 0, access, translateToInternalFormat(format));
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

void prevalidateGlTexture(const MH33::Image::DecodeTarget& source, GLenum& internalFormat, GLenum& format, GLenum& type)
{
	if(!source.frames.size()) throw std::runtime_error("Image without frames!");
	internalFormat = translateToInternalFormat(source.format);
	if(internalFormat == GLenum(-1)) {
		throw std::runtime_error("Invalid internal format! Possibly unsupported texture type?");
	}
	format = translateToFormat(source.format);
	if(format == GLenum(-1)) {
		throw std::runtime_error("Invalid format! Possibly unsupported texture type?");
	}
	type = translateToType(source.format);
	if(type == GLenum(-1)) {
		throw std::runtime_error("Invalid type! Possibly unsupported texture type?");
	}
}
void glInitializeTexture2D(const MH33::Image::DecodeTarget& source, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& stride)
{
	GLenum internalFormat, format, type;
	prevalidateGlTexture(source, internalFormat, format, type);
	width = source.frames[0].width;
	height = source.frames[0].height;
	stride = source.frames[0].stride;
	uint8_t mipmapCount = 0;
	for(size_t i = 0; i < source.frames.size(); ++i) {
		switch (source.format) {
			case MH33::GFX::TextureFormat::B_DXT1:
			case MH33::GFX::TextureFormat::B_DXT3:
			case MH33::GFX::TextureFormat::B_DXT5: glCompressedTexImage2D(GL_TEXTURE_2D,i,internalFormat,source.frames[i].width,source.frames[i].height,0,source.frames[i].imageData.size(),source.frames[i].imageData.data()); break;
			default: glTexImage2D(GL_TEXTURE_2D,i,internalFormat,source.frames[i].width,source.frames[i].height,0,format,type,source.frames[i].imageData.data()); break;
		}
		++mipmapCount;
	}
	if(mipmapCount < wantedMipmaps) {
		glGenerateMipmap(GL_TEXTURE_2D);
	}
}

void glInitializeTexture3D(const MH33::Image::DecodeTarget& source, bool isArray, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& depth, unsigned& stride)
{
	GLenum internalFormat, format, type;
	prevalidateGlTexture(source, internalFormat, format, type);
	GLenum unit = isArray ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_3D;
	width = source.frames[0].width;
	height = source.frames[0].height;
	stride = source.frames[0].stride;
	depth = source.frames.size();
	switch (source.format) {
		case MH33::GFX::TextureFormat::B_DXT1:
		case MH33::GFX::TextureFormat::B_DXT3:
		case MH33::GFX::TextureFormat::B_DXT5: glCompressedTexImage3D(unit,0,internalFormat,source.frames[0].width,source.frames[0].height,source.frames.size(),0,0,nullptr); break;
		default: glTexImage3D(unit,0,internalFormat,source.frames[0].width,source.frames[0].height,source.frames.size(),0,format,type,nullptr); break;
	}
	for(size_t i = 0; i < source.frames.size(); ++i) {
		switch (source.format) {
			case MH33::GFX::TextureFormat::B_DXT1:
			case MH33::GFX::TextureFormat::B_DXT3:
			case MH33::GFX::TextureFormat::B_DXT5: glCompressedTexSubImage3D(unit,0,0,0,i,source.frames[i].width,source.frames[i].height,1,format,source.frames[i].imageData.size(),source.frames[i].imageData.data()); break;
			default: glTexSubImage3D(unit,0,0,0,i,source.frames[i].width,source.frames[i].height,1,format,type,source.frames[i].imageData.data()); break;
		}
	}
	if(wantedMipmaps) {
		glGenerateMipmap(unit);
	}
}

void glInitializeTextureCubemnap(const MH33::Image::DecodeTarget& source, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& stride)
{
	GLenum internalFormat, format, type;
	prevalidateGlTexture(source, internalFormat, format, type);
	width = source.frames[0].width;
	height = source.frames[0].height;
	stride = source.frames[0].stride;
	for(size_t i = 0; i < source.frames.size(); ++i) {
		switch (source.format) {
			case MH33::GFX::TextureFormat::B_DXT1:
			case MH33::GFX::TextureFormat::B_DXT3:
			case MH33::GFX::TextureFormat::B_DXT5: glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,0,internalFormat,source.frames[i].width,source.frames[i].height,0,source.frames[i].imageData.size(),source.frames[i].imageData.data()); break;
			default: glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i ,0,internalFormat,source.frames[i].width,source.frames[i].height,0,format,type,source.frames[i].imageData.data()); break;
		}
	}
	if(wantedMipmaps) {
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	}
}

GLenum translateBindingType(MH33::GFX::ImageBindingType bindingType)
{
	switch (bindingType) {
		case MH33::GFX::ImageBindingType::READ_ONLY:
			return GL_READ_ONLY;
			break;
		case MH33::GFX::ImageBindingType::WRITE_ONLY:
			return GL_WRITE_ONLY;
			break;
		case MH33::GFX::ImageBindingType::READ_WRITE:
			return GL_READ_WRITE;
			break;
	}
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
	glBindImageTexture(GL_IMAGE_3D, textureVar, 0, false, 0, access, translateToInternalFormat(format));
}

void Texture3D::bindAsImage(uint8_t unit, int layer, MH33::GFX::ImageBindingType bindingType) const
{
	GLenum access = translateBindingType(bindingType);
	glBindImageTexture(GL_IMAGE_3D, textureVar, 0, false, layer, access, translateToInternalFormat(format));
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
	glBindImageTexture(GL_IMAGE_2D_ARRAY, textureVar, 0, true, 0, access, translateToInternalFormat(format));
}

void TextureArray2D::bindAsImage(uint8_t unit, int layer, MH33::GFX::ImageBindingType bindingType) const
{
	GLenum access = translateBindingType(bindingType);
	glBindImageTexture(GL_IMAGE_2D_ARRAY, textureVar, 0, false, layer, access, translateToInternalFormat(format));
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
	glBindImageTexture(GL_IMAGE_CUBE, textureVar, 0, true, 0, access, translateToInternalFormat(format));
}

void Cubemap::bindAsImage(uint8_t unit, uint8_t layer, MH33::GFX::ImageBindingType bindingType) const
{
	GLenum access = translateBindingType(bindingType);
	glBindImageTexture(GL_IMAGE_CUBE, textureVar, 0, false, layer, access, translateToInternalFormat(format));
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
