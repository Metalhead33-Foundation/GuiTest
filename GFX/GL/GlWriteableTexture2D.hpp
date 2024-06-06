#ifndef WRITEABLETEXTURE2D_HPP
#define WRITEABLETEXTURE2D_HPP
#include <GFX/Abstract/Texture.hpp>
#include <NativeGfxApi/glad/glwrap.h>

namespace GL {
class WriteableTexture2D : public MH33::GFX::WriteableTexture2D
{
private:
	GLuint textureVar;
	MH33::Image::pImage2D softTexture;
	WriteableTexture2D(const WriteableTexture2D& cpy) = delete;
	WriteableTexture2D& operator=(const WriteableTexture2D& cpy) = delete;
	glm::ivec2 topLeftUpdated, bottomRightUpdated;
	bool wasResized;
	unsigned clampXWitthinBounds(unsigned x) const;
	unsigned clampYWitthinBounds(unsigned y) const;
	void clearUpdatedRegion();
public:
	typedef std::function<MH33::Image::pImage2D(void)> SoftwareSideImageCreator;
	WriteableTexture2D(const SoftwareSideImageCreator& imageCreator);
	~WriteableTexture2D();
	WriteableTexture2D(WriteableTexture2D&& mov);
	WriteableTexture2D& operator=(WriteableTexture2D&& mov);
	void onRegionUpdate(const glm::ivec2& topleft, const glm::ivec2& bottomright) override;
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
	void* getPixels() override;
	const void* getPixels() const override;
	bool resize(int newWidth, int newHeight) override;
	void iterateOverPixels(const ColourIterator& program) const override;
	void iterateOverPixels(const ColourIterator2& program) const override;
	void iterateOverPixels(const ColourIterator& program, const glm::ivec2 offset, const glm::ivec2& dimensions) const override;
	void iterateOverPixels(const ColourIterator2& program, const glm::ivec2 offset, const glm::ivec2& dimensions) const override;
	void clearToColour(const glm::fvec4& colourKernel) override;
	void clearToColour(const ColourProgrammer& program) override;
	void clearToColour(const ColourProgrammer2& program) override;
	void clearToColour(const ColourProgrammer3& program) override;
	void clearToColour(const ColourProgrammer4& program) override;
	void clearToColour(const glm::fvec4& colourKernel, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void clearToColour(const ColourProgrammer& program, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void clearToColour(const ColourProgrammer2& program, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void clearToColour(const ColourProgrammer3& program, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void clearToColour(const ColourProgrammer4& program, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void blit(const MH33::Image::Image2D& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void blit(const MH33::Image::Image2D& cpy, const glm::ivec2 offset) override;
	void blit(const std::span<const std::byte>& data, MH33::GFX::TextureFormat format, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void update() override;
};
}
#endif // WRITEABLETEXTURE2D_HPP
