#include "GlFramebuffer.hpp"
#include "GlUtil.hpp"
namespace GL {

ReferenceTexture2D::ReferenceTexture2D()
{

}

/*
	GLuint textureVar;
	MH33::GFX::TextureFormat format;
	unsigned width, height, stride;
	float widthF, widthR, heightF, heightR;
*/

ReferenceTexture2D::ReferenceTexture2D(const ReferenceTexture2D& cpy)
	: textureVar(cpy.textureVar), format(cpy.format), width(cpy.width), height(cpy.height), stride(cpy.stride),
	  widthF(cpy.widthF), widthR(cpy.widthR), heightF(cpy.heightF), heightR(cpy.heightR)
{

}

ReferenceTexture2D& ReferenceTexture2D::operator=(const ReferenceTexture2D& cpy)
{
	this->textureVar = cpy.textureVar;
	this->format = cpy.format;
	this->width = cpy.width;
	this->height = cpy.height;
	this->stride = cpy.stride;
	this->widthF = cpy.widthF;
	this->widthR = cpy.widthR;
	this->heightF = cpy.heightF;
	this->heightR = cpy.heightR;
}

ReferenceTexture2D::ReferenceTexture2D(uint8_t unit, GLuint tex, MH33::GFX::TextureFormat format, unsigned width, unsigned height, unsigned stride, bool initialize)
	: textureVar(tex), format(format), width(width), height(height), stride(stride),
	  widthF(static_cast<float>(width - 1)), widthR(1.0f / static_cast<float>(width - 1)),
	  heightF(static_cast<float>(height - 1)), heightR(1.0f / static_cast<float>(height - 1))
{
	if(initialize) {
		GLenum glinternalFormat, glFormat, gltype, attachmentType;
		prevalidateGlTextureFormat(format,glinternalFormat,glFormat,gltype);
		glBindTexture(GL_TEXTURE_2D, textureVar);
		glTexImage2D(GL_TEXTURE_2D, 0, glinternalFormat, width, height, 0, glFormat, gltype, nullptr);
		/*
		DEPTH_COMPONENT16 = RGBA444 + 1,
		DEPTH_COMPONENT24 = DEPTH_COMPONENT16 + 1,
		DEPTH_COMPONENT32 = DEPTH_COMPONENT24 + 1,
		DEPTH_COMPONENT32F = DEPTH_COMPONENT32 + 1,
		DEPTH24_STENCIL8 = DEPTH_COMPONENT32F + 1,
		DEPTH32F_STENCIL8 = DEPTH24_STENCIL8 + 1,
		STENCIL_INDEX8 = DEPTH32F_STENCIL8 + 1
		*/
		switch (format) {
			case MH33::GFX::TextureFormat::DEPTH24_STENCIL8: attachmentType = GL_DEPTH_STENCIL_ATTACHMENT; break;
			case MH33::GFX::TextureFormat::DEPTH32F_STENCIL8: attachmentType = GL_DEPTH_STENCIL_ATTACHMENT; break;
			case MH33::GFX::TextureFormat::DEPTH_COMPONENT16: attachmentType = GL_DEPTH_ATTACHMENT; break;
			case MH33::GFX::TextureFormat::DEPTH_COMPONENT24: attachmentType = GL_DEPTH_ATTACHMENT; break;
			case MH33::GFX::TextureFormat::DEPTH_COMPONENT32: attachmentType = GL_DEPTH_ATTACHMENT; break;
			case MH33::GFX::TextureFormat::DEPTH_COMPONENT32F: attachmentType = GL_DEPTH_ATTACHMENT; break;
			case MH33::GFX::TextureFormat::STENCIL_INDEX8: attachmentType = GL_STENCIL_ATTACHMENT; break;
			default: attachmentType = GL_COLOR_ATTACHMENT0 + unit; break;
		}
		glFramebufferTexture2D(GL_FRAMEBUFFER,attachmentType,GL_TEXTURE_2D,textureVar,0);
	}
}

MH33::GFX::Handle ReferenceTexture2D::getNativeHandle()
{
	return { .oglUint = textureVar };
}

MH33::GFX::ConstHandle ReferenceTexture2D::getNativeHandle() const
{
	return { .oglUint = textureVar };
}

MH33::GFX::TextureFormat ReferenceTexture2D::getFormat() const
{
	return format;
}

void ReferenceTexture2D::bind(uint8_t unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, textureVar);
}

void ReferenceTexture2D::bindAsImage(uint8_t unit, MH33::GFX::ImageBindingType bindingType) const
{
	GLenum access = translateBindingType(bindingType);
	glBindImageTexture(unit, textureVar, 0, false, 0, access, translateToInternalFormat(format));
}

unsigned ReferenceTexture2D::getWidth() const
{
	return width;
}

float ReferenceTexture2D::getWidthF() const
{
	return widthF;
}

float ReferenceTexture2D::getWidthR() const
{
	return widthR;
}

unsigned ReferenceTexture2D::getHeight() const
{
	return height;
}

float ReferenceTexture2D::getHeightF() const
{
	return heightF;
}

float ReferenceTexture2D::getHeightR() const
{
	return heightR;
}

unsigned ReferenceTexture2D::getStride() const
{
	return stride;
}

Framebuffer::Framebuffer(const std::span<const MH33::GFX::TextureFormat>& attachmentFormats, unsigned width, unsigned height)
	: width(width), height(height),
	  widthF(static_cast<float>(width - 1)), widthR(1.0f / static_cast<float>(width - 1)),
	  heightF(static_cast<float>(height - 1)), heightR(1.0f / static_cast<float>(height - 1))
{
	glGenFramebuffers(1,&fbo);
	glBindFramebuffer(GL_FRAMEBUFFER,fbo);
	textureAttachments.resize(attachmentFormats.size());
	glGenTextures(attachmentFormats.size(),textureAttachments.data());
}

MH33::GFX::Handle Framebuffer::getNativeHandle()
{
	return { .oglUint = fbo };
}

MH33::GFX::ConstHandle Framebuffer::getNativeHandle() const
{
	return { .oglUint = fbo };
}

void Framebuffer::bind() const
{
	glBindFramebuffer(GL_FRAMEBUFFER,fbo);
}

void Framebuffer::unbind() const
{
	glBindFramebuffer(GL_FRAMEBUFFER,0);
}

uint8_t Framebuffer::getAttachmentCount() const
{
	return textureAttachments.size();
}

unsigned Framebuffer::getWidth() const
{
	return width;
}

float Framebuffer::getWidthF() const
{
	return widthF;
}

float Framebuffer::getWidthR() const
{
	return widthR;
}

unsigned Framebuffer::getHeight() const
{
	return height;
}

float Framebuffer::getHeightF() const
{
	return heightF;
}

float Framebuffer::getHeightR() const
{
	return heightR;
}

MH33::GFX::Texture2D& Framebuffer::getTextureAttachment2D(uint8_t unit)
{
	return textureReferences[unit];
}

const MH33::GFX::Texture2D& Framebuffer::getTextureAttachment2D(uint8_t unit) const
{
	return textureReferences[unit];
}

}
