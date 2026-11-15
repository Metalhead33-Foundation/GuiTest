#ifndef GLUTIL_HPP
#define GLUTIL_HPP
// TODO: We'll need to put this somewhere else, so this can be shared by OpenGL and OpenGL ES
#ifdef GLES_IMPLEMENTATION
#include <glad/gles2.h>
#else
#include <glad/gl.h>
#endif
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Kaldi/KldVertexFormat.hpp>

#ifdef GLES_IMPLEMENTATION
namespace GLES {
#else
namespace GL {
#endif

// TODO: Reintroduce ImageBindingType in either Kaldi or Euphemy
/*
enum class ImageBindingType {
	READ_ONLY = 1,
	WRITE_ONLY = 2,
	READ_WRITE = READ_ONLY | WRITE_ONLY
};
*/
//GLenum translateBindingType(MH33::GFX::ImageBindingType bindingType);
GLenum translateToInternalFormat(Euph::Media::Image::Format format);
GLenum translateToFormat(Euph::Media::Image::Format format);
GLenum translateToType(Euph::Media::Image::Format format);
void glInitializeTexture2D(const Euph::Media::Image::DecodeTarget& source, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& stride);
void glInitializeTexture2D(const Euph::Media::Image::ImageView& source, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& stride);
void glInitializeTexture3D(const Euph::Media::Image::DecodeTarget& source, bool isArray, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& depth, unsigned& stride);
void glInitializeTextureCubemnap(const Euph::Media::Image::DecodeTarget& source, GLuint texVar, uint8_t wantedMipmaps, unsigned& width, unsigned& height, unsigned& stride);
void prevalidateGlTextureFormat(const Euph::Media::Image::Format& sourceformat, GLenum& internalFormat, GLenum& format, GLenum& type);
void prevalidateGlTexture(const Euph::Media::Image::DecodeTarget& source, GLenum& internalFormat, GLenum& format, GLenum& type);
void prevalidateGlTexture(const Euph::Media::Image::ImageView& source, GLenum& internalFormat, GLenum& format, GLenum& type);
void prevalidateGlTexture(const Euph::Media::Image::ImageView& source, GLenum& internalFormat, GLenum& format, GLenum& type);
void glProcessVertexDescriptor(const Kld::VertexDescriptor& vertexDescriptor);
GLbitfield generateGlMappedBitfield(bool needsToRead, bool entireBuffer);

}

#endif // GLUTIL_HPP
