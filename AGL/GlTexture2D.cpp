#include "GlTexture2D.hpp"
#include <GL/GlValidate.hpp>

namespace GL {

const Texture& AcceleratedTexture::getTex() const
{
	return tex;
}

void AcceleratedTexture::update()
{
	glGetError();
	tex.bind();
	tex.pixelStorei(GL_UNPACK_ALIGNMENT,1);
	tex.pixelStorei(GL_PACK_ALIGNMENT,1);
	tex.image2D(0,internalFormat,softTex->getWidth(),softTex->getHeight(),0,format,type, softTex->getRawPixels());
	tex.setWrapS(GL_MIRRORED_REPEAT);
	tex.setWrapT(GL_MIRRORED_REPEAT);
	tex.setMinFilter(GL_NEAREST);
	tex.setMagFilter(GL_LINEAR);
}

GLint AcceleratedTexture::getInternalFormat() const
{
	return internalFormat;
}

GLenum AcceleratedTexture::getFormat() const
{
	return format;
}

GLenum AcceleratedTexture::getType() const
{
	return type;
}

void AcceleratedTexture::blit(const ITexture& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions)
{
	softTex->blit(cpy,offset,dimensions);
}

void AcceleratedTexture::blit(const ITexture& cpy, const glm::ivec2 offset)
{
	softTex->blit(cpy,offset);
}

AcceleratedTexture::AcceleratedTexture(std::unique_ptr<SoftwareRenderer::Texture>&& mov, GLint internalFormat, GLenum format, GLenum type)
	: softTex(std::move(mov)), tex(GL_TEXTURE_2D), internalFormat(internalFormat), format(format), type(type)
{
	glGetError();
	tex.bind();
	tex.pixelStorei(GL_UNPACK_ALIGNMENT,1);
	tex.pixelStorei(GL_PACK_ALIGNMENT,1);
	tex.image2D(0,internalFormat,softTex->getWidth(),softTex->getHeight(),0,format,type, softTex->getRawPixels());
	tex.setWrapS(GL_MIRRORED_REPEAT);
	tex.setWrapT(GL_MIRRORED_REPEAT);
	tex.setMinFilter(GL_NEAREST);
	tex.setMagFilter(GL_LINEAR);
	Validate::validate();
}

int AcceleratedTexture::getWidth() const
{
	return softTex->getWidth();
}

float AcceleratedTexture::getWidthF() const
{
	return softTex->getWidthF();
}

float AcceleratedTexture::getWidthR() const
{
	return softTex->getWidthR();
}

int AcceleratedTexture::getHeight() const
{
	return softTex->getHeight();
}

float AcceleratedTexture::getHeightF() const
{
	return softTex->getHeightF();
}

int AcceleratedTexture::getStride() const
{
	return softTex->getStride();
}

float AcceleratedTexture::getHeightR() const
{
	return softTex->getHeightR();
}

bool AcceleratedTexture::resize(int newWidth, int newHeight)
{
	softTex->resize(newWidth,newHeight);
	return true;
}

Texture& AcceleratedTexture::getTex()
{
	return tex;
}
void AcceleratedTexture::iterateOverPixels(const ColourIterator& program) const
{
	softTex->iterateOverPixels(program);
}

void AcceleratedTexture::iterateOverPixels(const ColourIterator2& program) const
{
	softTex->iterateOverPixels(program);
}
}

void GL::AcceleratedTexture::getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel) const
{
	softTex->getPixel(pos,colourKernel);
}
