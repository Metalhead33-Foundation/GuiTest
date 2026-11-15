#include "GlUtil.hpp"

#ifdef GLES_IMPLEMENTATION
namespace GLES {
#else
namespace GL {
#endif

GLenum translateToInternalFormat(Euph::Media::Image::Format format)
{
	switch (format) {
		case Euph::Media::Image::Format::R8U: return GL_R8;
		case Euph::Media::Image::Format::R8S: return GL_R8_SNORM;
		case Euph::Media::Image::Format::R16U: return GL_R16;
		case Euph::Media::Image::Format::R16S: return GL_R16_SNORM;
		case Euph::Media::Image::Format::R16F: return GL_R16F;
		case Euph::Media::Image::Format::R32U: return -1;
		case Euph::Media::Image::Format::R32S: return -1;
		case Euph::Media::Image::Format::R32F: return GL_R32F;
		case Euph::Media::Image::Format::R64F: return -1;

		case Euph::Media::Image::Format::RG8U: return GL_RG8;
		case Euph::Media::Image::Format::RG8S: return GL_RG8_SNORM;
		case Euph::Media::Image::Format::RG16U: return GL_RG16;
		case Euph::Media::Image::Format::RG16S: return GL_RG16_SNORM;
		case Euph::Media::Image::Format::RG16F: return GL_RG16F;
		case Euph::Media::Image::Format::RG32U: return -1;
		case Euph::Media::Image::Format::RG32S: return -1;
		case Euph::Media::Image::Format::RG32F: return GL_RG32F;
		case Euph::Media::Image::Format::RG64F: return -1;

		case Euph::Media::Image::Format::RGB8U: return GL_RGB8;
		case Euph::Media::Image::Format::RGB8S: return GL_RGB8_SNORM;
		case Euph::Media::Image::Format::RGB16U: return GL_RGB16;
		case Euph::Media::Image::Format::RGB16S: return GL_RGB16_SNORM;
		case Euph::Media::Image::Format::RGB16F: return GL_RGB16F;
		case Euph::Media::Image::Format::RGB32U: return -1;
		case Euph::Media::Image::Format::RGB32S: return -1;
		case Euph::Media::Image::Format::RGB32F: return GL_RGB32F;
		case Euph::Media::Image::Format::RGB64F: return -1;

		case Euph::Media::Image::Format::BGR8U: return GL_RGB8;
		case Euph::Media::Image::Format::BGR8S: return GL_RGB8_SNORM;
		case Euph::Media::Image::Format::BGR16U: return GL_RGB16;
		case Euph::Media::Image::Format::BGR16S: return GL_RGB16_SNORM;
		case Euph::Media::Image::Format::BGR16F: return GL_RGB16F;
		case Euph::Media::Image::Format::BGR32U: return -1;
		case Euph::Media::Image::Format::BGR32S: return -1;
		case Euph::Media::Image::Format::BGR32F: return GL_RGB32F;
		case Euph::Media::Image::Format::BGR64F: return -1;

		case Euph::Media::Image::Format::RGBA8U: return GL_RGBA8;
		case Euph::Media::Image::Format::RGBA8S: return GL_RGBA8_SNORM;
		case Euph::Media::Image::Format::RGBA16U: return GL_RGBA16;
		case Euph::Media::Image::Format::RGBA16S: return GL_RGBA16_SNORM;
		case Euph::Media::Image::Format::RGBA16F: return GL_RGBA16F;
		case Euph::Media::Image::Format::RGBA32U: return -1;
		case Euph::Media::Image::Format::RGBA32S: return -1;
		case Euph::Media::Image::Format::RGBA32F: return GL_RGBA32F;
		case Euph::Media::Image::Format::RGBA64F: return -1;

		case Euph::Media::Image::Format::BGRA8U: return GL_RGBA8;
		case Euph::Media::Image::Format::BGRA8S: return GL_RGBA8_SNORM;
		case Euph::Media::Image::Format::BGRA16U: return GL_RGBA16;
		case Euph::Media::Image::Format::BGRA16S: return GL_RGBA16_SNORM;
		case Euph::Media::Image::Format::BGRA16F: return GL_RGBA16F;
		case Euph::Media::Image::Format::BGRA32U: return -1;
		case Euph::Media::Image::Format::BGRA32S: return -1;
		case Euph::Media::Image::Format::BGRA32F: return GL_RGBA32F;
		case Euph::Media::Image::Format::BGRA64F: return -1;

		case Euph::Media::Image::Format::ARGB8U: return GL_RGBA8;
		case Euph::Media::Image::Format::ARGB8S: return GL_RGBA8_SNORM;
		case Euph::Media::Image::Format::ARGB16U: return GL_RGBA16;
		case Euph::Media::Image::Format::ARGB16S: return GL_RGBA16_SNORM;
		case Euph::Media::Image::Format::ARGB16F: return GL_RGBA16F;
		case Euph::Media::Image::Format::ARGB32U: return -1;
		case Euph::Media::Image::Format::ARGB32S: return -1;
		case Euph::Media::Image::Format::ARGB32F: return GL_RGBA32F;
		case Euph::Media::Image::Format::ARGB64F: return -1;

		case Euph::Media::Image::Format::INDEXED: return -1;
		case Euph::Media::Image::Format::RGB565: return GL_RGB;
		case Euph::Media::Image::Format::RGB555: return GL_RGB5;
		case Euph::Media::Image::Format::RGB444: return GL_RGB4;
		case Euph::Media::Image::Format::RGBA444: return GL_RGBA4;
		case Euph::Media::Image::Format::DEPTH_COMPONENT16: return GL_DEPTH_COMPONENT;
		case Euph::Media::Image::Format::DEPTH_COMPONENT24: return GL_DEPTH_COMPONENT;
		case Euph::Media::Image::Format::DEPTH_COMPONENT32: return GL_DEPTH_COMPONENT;
		case Euph::Media::Image::Format::DEPTH_COMPONENT32F: return GL_DEPTH_COMPONENT;
		case Euph::Media::Image::Format::DEPTH24_STENCIL8: return GL_DEPTH_STENCIL;
		case Euph::Media::Image::Format::DEPTH32F_STENCIL8: return GL_DEPTH_STENCIL;
		case Euph::Media::Image::Format::STENCIL_INDEX8: return GL_STENCIL_INDEX;

		case Euph::Media::Image::Format::B_DXT1: return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		case Euph::Media::Image::Format::B_DXT3: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		case Euph::Media::Image::Format::B_DXT5: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		default: return -1;
	}
}

GLenum translateToFormat(Euph::Media::Image::Format format)
{
	GLenum toReturn = -1;
	switch (format) {
		case Euph::Media::Image::Format::RGB555: toReturn = GL_RGB; break;
		case Euph::Media::Image::Format::RGB565: toReturn = GL_RGB; break;
		case Euph::Media::Image::Format::RGB444: toReturn = GL_RGB; break;
		case Euph::Media::Image::Format::RGBA444: toReturn = GL_RGBA; break;
		case Euph::Media::Image::Format::DEPTH_COMPONENT16: toReturn = GL_DEPTH_COMPONENT; break;
		case Euph::Media::Image::Format::DEPTH_COMPONENT24: toReturn = GL_DEPTH_COMPONENT; break;
		case Euph::Media::Image::Format::DEPTH_COMPONENT32: toReturn = GL_DEPTH_COMPONENT; break;
		case Euph::Media::Image::Format::DEPTH_COMPONENT32F: toReturn = GL_DEPTH_COMPONENT; break;
		case Euph::Media::Image::Format::STENCIL_INDEX8: toReturn = GL_STENCIL_INDEX; break;
		case Euph::Media::Image::Format::DEPTH24_STENCIL8: toReturn = GL_DEPTH_STENCIL; break;
		case Euph::Media::Image::Format::DEPTH32F_STENCIL8: toReturn = GL_DEPTH_STENCIL; break;
		default: break;
	}
	if(toReturn == GLenum(-1)) {
		switch (unsigned(format) & unsigned(Euph::Media::Image::Format::COLOR_COMPONENT_MASK)) {
			case unsigned(Euph::Media::Image::Format::ST_GREYSCALE): return GL_RED;
			case unsigned(Euph::Media::Image::Format::ST_RG): return GL_RG;
			case unsigned(Euph::Media::Image::Format::ST_RGB): return GL_RGB;
			case unsigned(Euph::Media::Image::Format::ST_BGR): return GL_BGR;
			case unsigned(Euph::Media::Image::Format::ST_RGBA): return GL_RGBA;
			case unsigned(Euph::Media::Image::Format::ST_BGRA): return GL_BGRA;
			default: return -1;
		}
	} else return toReturn;
}

GLenum translateToType(Euph::Media::Image::Format format)
{
	GLenum toReturn = -1;
	switch (format) {
		case Euph::Media::Image::Format::RGB555: toReturn =  GL_UNSIGNED_SHORT_5_5_5_1; break;
		case Euph::Media::Image::Format::RGB565: toReturn =  GL_UNSIGNED_SHORT_5_6_5; break;
		case Euph::Media::Image::Format::RGB444: toReturn =  GL_UNSIGNED_SHORT_4_4_4_4; break;
		case Euph::Media::Image::Format::RGBA444: toReturn =  GL_UNSIGNED_SHORT_4_4_4_4; break;
		case Euph::Media::Image::Format::DEPTH_COMPONENT16: toReturn = GL_UNSIGNED_SHORT; break;
		case Euph::Media::Image::Format::DEPTH_COMPONENT24: toReturn = GL_UNSIGNED_INT_24_8; break;
		case Euph::Media::Image::Format::DEPTH_COMPONENT32: toReturn = GL_UNSIGNED_INT; break;
		case Euph::Media::Image::Format::DEPTH_COMPONENT32F: toReturn = GL_FLOAT; break;
		case Euph::Media::Image::Format::STENCIL_INDEX8: toReturn = GL_UNSIGNED_BYTE; break;
		case Euph::Media::Image::Format::DEPTH24_STENCIL8: toReturn = GL_UNSIGNED_INT_24_8; break;
		case Euph::Media::Image::Format::DEPTH32F_STENCIL8: toReturn = GL_FLOAT_32_UNSIGNED_INT_24_8_REV; break;
		default: break;
	}
	if(toReturn == GLenum(-1)) {
		switch (unsigned(format) & unsigned(Euph::Media::Image::Format::COLOR_RESOLUTION_MASK)) {
			case unsigned(Euph::Media::Image::Format::SC_UINT8): return  GL_UNSIGNED_BYTE;
			case unsigned(Euph::Media::Image::Format::SC_UINT16): return  GL_UNSIGNED_SHORT;
			case unsigned(Euph::Media::Image::Format::SC_UINT32): return  GL_UNSIGNED_INT;
			case unsigned(Euph::Media::Image::Format::SC_INT8): return  GL_BYTE;
			case unsigned(Euph::Media::Image::Format::SC_INT16): return  GL_SHORT;
			case unsigned(Euph::Media::Image::Format::SC_INT32): return  GL_INT;
			case unsigned(Euph::Media::Image::Format::SC_F16): return  GL_HALF_FLOAT;
			case unsigned(Euph::Media::Image::Format::SC_F32): return  GL_FLOAT;
			default: return -1;
		}
	} else return toReturn;
}

void prevalidateGlTexture(const Euph::Media::Image::DecodeTarget& source, GLenum& internalFormat, GLenum& format, GLenum& type)
{
	if(!source.getFrames().size()) throw std::runtime_error("Image without frames!");
	internalFormat = translateToInternalFormat(source.getFormat());
	if(internalFormat == GLenum(-1)) {
		throw std::runtime_error("Invalid internal format! Possibly unsupported texture type?");
	}
	format = translateToFormat(source.getFormat());
	if(format == GLenum(-1)) {
		throw std::runtime_error("Invalid format! Possibly unsupported texture type?");
	}
	type = translateToType(source.getFormat());
	if(type == GLenum(-1)) {
		throw std::runtime_error("Invalid type! Possibly unsupported texture type?");
	}
}
void prevalidateGlTexture(const Euph::Media::Image::ImageView& source, GLenum& internalFormat, GLenum& format, GLenum& type)
{
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
void glInitializeTexture2D(const Euph::Media::Image::DecodeTarget& source, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& stride)
{
	GLenum internalFormat, format, type;
	prevalidateGlTexture(source, internalFormat, format, type);
	const auto& frm = source.getFrame(0);
	width = frm.width;
	height = frm.height;
	stride = frm.stride;
	uint8_t mipmapCount = 0;
	for(size_t i = 0; i < source.getFrames().size(); ++i) {
		const auto& frm = source.getFrame(i);
		switch (source.getFormat()) {
			case Euph::Media::Image::Format::B_DXT1:
			case Euph::Media::Image::Format::B_DXT3:
			case Euph::Media::Image::Format::B_DXT5: glCompressedTexImage2D(GL_TEXTURE_2D,i,internalFormat,frm.width,frm.height,0,frm.data.size(),frm.data.data()); break;
			default: glTexImage2D(GL_TEXTURE_2D,i,internalFormat,frm.width,frm.height,0,format,type,frm.data.data()); break;
		}
		++mipmapCount;
	}
	if(mipmapCount < wantedMipmaps) {
		glGenerateMipmap(GL_TEXTURE_2D);
	}
}
void glInitializeTexture2D(const Euph::Media::Image::ImageView& source, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& stride)
{
	GLenum internalFormat, format, type;
	prevalidateGlTexture(source, internalFormat, format, type);
	width = source.width;
	height = source.height;
	stride = source.stride;
	glTexImage2D(GL_TEXTURE_2D,0,internalFormat,width,height,0,format,type,source.data);
	if(wantedMipmaps) {
		glGenerateMipmap(GL_TEXTURE_2D);
	}
}

void glInitializeTexture3D(const Euph::Media::Image::DecodeTarget& source, bool isArray, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& depth, unsigned& stride)
{
	GLenum internalFormat, format, type;
	prevalidateGlTexture(source, internalFormat, format, type);
	GLenum unit = isArray ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_3D;
	const auto& frm = source.getFrame(0);
	width = frm.width;
	height = frm.height;
	stride = frm.stride;
	depth = source.getFrames().size();
	switch (source.getFormat()) {
		case Euph::Media::Image::Format::B_DXT1:
		case Euph::Media::Image::Format::B_DXT3:
		case Euph::Media::Image::Format::B_DXT5: glCompressedTexImage3D(unit,0,internalFormat,frm.width,frm.height,source.getFrames().size(),0,0,nullptr); break;
		default: glTexImage3D(unit,0,internalFormat,frm.width,frm.height,source.getFrames().size(),0,format,type,nullptr); break;
	}
	for(size_t i = 0; i < source.getFrames().size(); ++i) {
		const auto& frm = source.getFrame(i);
		switch (source.getFormat()) {
			case Euph::Media::Image::Format::B_DXT1:
			case Euph::Media::Image::Format::B_DXT3:
			case Euph::Media::Image::Format::B_DXT5: glCompressedTexSubImage3D(unit,0,0,0,i,frm.width,frm.height,1,format,frm.data.size(),frm.data.data()); break;
			default: glTexSubImage3D(unit,0,0,0,i,frm.width,frm.height,1,format,type,frm.data.data()); break;
		}
	}
	if(wantedMipmaps) {
		glGenerateMipmap(unit);
	}
}

void glInitializeTextureCubemnap(const Euph::Media::Image::DecodeTarget& source, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& stride)
{
	GLenum internalFormat, format, type;
	prevalidateGlTexture(source, internalFormat, format, type);
	const auto& frm = source.getFrame(0);
	width = frm.width;
	height = frm.height;
	stride = frm.stride;
	for(size_t i = 0; i < source.getFrames().size(); ++i) {
		const auto& frm = source.getFrame(i);
		switch (source.getFormat()) {
			case Euph::Media::Image::Format::B_DXT1:
			case Euph::Media::Image::Format::B_DXT3:
			case Euph::Media::Image::Format::B_DXT5: glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,0,internalFormat,frm.width,frm.height,0,frm.data.size(),frm.data.data()); break;
			default: glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i ,0,internalFormat,frm.width,frm.height,0,format,type,frm.data.data()); break;
		}
	}
	if(wantedMipmaps) {
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	}
}

/*GLenum translateBindingType(MH33::GFX::ImageBindingType bindingType)
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
}*/

void glProcessVertexDescriptor(const Kld::VertexDescriptor& vertexDescriptor)
{
	for (size_t i = 0; i < vertexDescriptor.descriptors.size(); ++i) {
		const auto& attr = vertexDescriptor.descriptors[i];
		GLint size = attr.type & Kld::PrimitiveType::SIZE_MASK;
		GLenum type;
		GLboolean normalized = GL_FALSE;

		switch (attr.type & Kld::PrimitiveType::TYPE_MASK) {
			case Kld::PrimitiveType::TYPE_HALF_FLOAT: type = GL_HALF_FLOAT; break;
			case Kld::PrimitiveType::TYPE_FLOAT: type = GL_FLOAT; break;
			case Kld::PrimitiveType::TYPE_DOUBLE: type = GL_DOUBLE; break;
			case Kld::PrimitiveType::TYPE_UBYTE: type = GL_UNSIGNED_BYTE; break;
			case Kld::PrimitiveType::TYPE_SBYTE: type = GL_BYTE; break;
			case Kld::PrimitiveType::TYPE_USHORT: type = GL_UNSIGNED_SHORT; break;
			case Kld::PrimitiveType::TYPE_SSHORT: type = GL_SHORT; break;
			case Kld::PrimitiveType::TYPE_UINT: type = GL_UNSIGNED_INT; break;
			case Kld::PrimitiveType::TYPE_SINT: type = GL_INT; break;
			default: type = GL_INVALID_ENUM; break;
		}

		if (attr.type & Kld::PrimitiveType::IS_NORMALIZED) {
			normalized = GL_TRUE;
		}

		if (type != GL_INVALID_ENUM) {
			glVertexAttribPointer(
				static_cast<GLuint>(i),
				size,
				type,
				normalized,
				static_cast<GLsizei>(vertexDescriptor.stride),
				reinterpret_cast<const void*>(attr.offset)
				);
			glEnableVertexAttribArray(static_cast<GLuint>(i));
		}
	}
}
GLbitfield generateGlMappedBitfield(bool needsToRead, bool entireBuffer) {
	if(needsToRead) {
		return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT;
	} else {
		if(entireBuffer) {
			return GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_FLUSH_EXPLICIT_BIT;
		} else {
			return GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT;
		}
	}
}

void prevalidateGlTextureFormat(const Euph::Media::Image::Format& sourceformat, GLenum& internalFormat, GLenum& format, GLenum& type)
{
	internalFormat = translateToInternalFormat(sourceformat);
	if(internalFormat == GLenum(-1)) {
		throw std::runtime_error("Invalid internal format! Possibly unsupported texture type?");
	}
	format = translateToFormat(sourceformat);
	if(format == GLenum(-1)) {
		throw std::runtime_error("Invalid format! Possibly unsupported texture type?");
	}
	type = translateToType(sourceformat);
	if(type == GLenum(-1)) {
		throw std::runtime_error("Invalid type! Possibly unsupported texture type?");
	}
}


}
