#ifndef GLTEXTURE2D_HPP
#define GLTEXTURE2D_HPP
#include <Pipeline/ITexture.hpp>
#include <GL/GlTexture.hpp>
#include <Texture/Texture.hpp>

namespace GL {

class AcceleratedTexture : public SYS::ITexture
{
private:
	std::unique_ptr<SoftwareRenderer::Texture> softTex;
	Texture tex;
	GLint internalFormat;
	GLenum format;
	GLenum type;
public:
	AcceleratedTexture(std::unique_ptr<SoftwareRenderer::Texture>&& mov, GLint internalFormat, GLenum format, GLenum type);
	int getWidth() const override;
	float getWidthF() const override;
	float getWidthR() const override;
	int getHeight() const override;
	float getHeightF() const override;
	int getStride() const override;
	float getHeightR() const override;
	bool resize(int newWidth, int newHeight) override;
	Texture& getTex();
	const Texture& getTex() const;
	void update();
	GLint getInternalFormat() const;
	GLenum getGlFormat() const;
	GLenum getType() const;
	void blit(const SYS::ITexture& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void blit(const SYS::ITexture& cpy, const glm::ivec2 offset) override;
	void iterateOverPixels(const ColourIterator& program) const override;
	void iterateOverPixels(const ColourIterator2& program) const override;
	void getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel) const override;
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::TextureFormat getFormat() const override;
};

}

#endif // GLTEXTURE2D_HPP
