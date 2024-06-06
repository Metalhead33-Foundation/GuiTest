#ifndef GLFRAMEBUFFER_HPP
#define GLFRAMEBUFFER_HPP
#include <GFX/Abstract/Framebuffer.hpp>
#include <NativeGfxApi/glad/glwrap.h>
#include <MhLib/Util/MhStackAllocator.hpp>
#include <vector>

namespace GL {

class ReferenceTexture2D : public MH33::GFX::Texture2D {
private:
	GLuint textureVar;
	MH33::GFX::TextureFormat format;
	unsigned width, height, stride;
	float widthF, widthR, heightF, heightR;
public:
	ReferenceTexture2D();
	ReferenceTexture2D(const ReferenceTexture2D& cpy);
	ReferenceTexture2D& operator=(const ReferenceTexture2D& cpy);
	ReferenceTexture2D(uint8_t unit, GLuint tex, MH33::GFX::TextureFormat format, unsigned width, unsigned height, unsigned stride, bool initialize = false);
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
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

class Framebuffer : public MH33::GFX::Framebuffer
{
private:
	GLuint fbo;
	std::vector<GLuint, MH33::Util::StackAllocator<GLuint,32>> textureAttachments;
	std::vector<ReferenceTexture2D, MH33::Util::StackAllocator<ReferenceTexture2D,32>> textureReferences;
	unsigned width, height;
	float widthF, widthR, heightF, heightR;
	Framebuffer(const Framebuffer& cpy) = delete;
	Framebuffer& operator=(const Framebuffer& cpy) = delete;
public:
	Framebuffer(const std::span<const MH33::GFX::TextureFormat>& attachmentFormats, unsigned width, unsigned height);
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	void bind() const override;
	void unbind() const override;
	uint8_t getAttachmentCount() const override;
	unsigned getWidth() const override;
	float getWidthF() const override;
	float getWidthR() const override;
	unsigned getHeight() const override;
	float getHeightF() const override;
	float getHeightR() const override;
	MH33::GFX::Texture2D& getTextureAttachment2D(uint8_t unit) override;
	const MH33::GFX::Texture2D& getTextureAttachment2D(uint8_t unit) const override;
};
}
#endif // GLFRAMEBUFFER_HPP
