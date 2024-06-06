#ifndef GLUTIL_HPP
#define GLUTIL_HPP
#include <NativeGfxApi/glad/glwrap.h>
#include <GFX/Abstract/Texture.hpp>
#include <MhLib/Media/Image/MhDecodeTarget.hpp>
#include <GFX/Abstract/VertexFormat.hpp>

namespace GL {

GLenum translateBindingType(MH33::GFX::ImageBindingType bindingType);
GLenum translateToInternalFormat(MH33::GFX::TextureFormat format);
GLenum translateToFormat(MH33::GFX::TextureFormat format);
GLenum translateToType(MH33::GFX::TextureFormat format);
void glInitializeTexture2D(const MH33::Image::DecodeTarget& source, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& stride);
void glInitializeTexture2D(const MH33::Image::Image2D& source, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& stride);
void glInitializeTexture3D(const MH33::Image::DecodeTarget& source, bool isArray, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& depth, unsigned& stride);
void glInitializeTextureCubemnap(const MH33::Image::DecodeTarget& source, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& stride);
void prevalidateGlTextureFormat(const MH33::Image::Format& sourceformat, GLenum& internalFormat, GLenum& format, GLenum& type);
void prevalidateGlTexture(const MH33::Image::DecodeTarget& source, GLenum& internalFormat, GLenum& format, GLenum& type);
void prevalidateGlTexture(const MH33::Image::Image2D& source, GLenum& internalFormat, GLenum& format, GLenum& type);
void prevalidateGlTexture(const MH33::Image::Image2D& source, GLenum& internalFormat, GLenum& format, GLenum& type);
void glProcessVertexDescriptor(const MH33::GFX::VertexDescriptor& vertexDescriptor);
GLbitfield generateGlMappedBitfield(bool needsToRead, bool entireBuffer);

}

#endif // GLUTIL_HPP
