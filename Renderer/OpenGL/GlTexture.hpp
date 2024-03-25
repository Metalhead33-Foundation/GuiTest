#ifndef GLTEXTURE_ADV_HPP
#define GLTEXTURE_ADV_HPP
#include <MhLib/Media/GFX/MhTexture.hpp>
#include <MhLib/Media/Image/MhDecodeTarget.hpp>
#include <GL/GlTex.hpp>
namespace GL {

class Texture2D : public MH33::GFX::Texture2D {
private:
	Texture texHndl;
	int width, height, stride;
	float widthF, widthR, heightF, heightR;
	MH33::GFX::TextureFormat format;
	void updateDim(int nwidth, int nheight, int npixxisze);
public:
	Texture2D();
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	const Texture& getTexHndl() const;
	Texture& getTexHndl();
	MH33::GFX::TextureFormat getFormat() const override;
	int getWidth() const override;
	float getWidthF() const override;
	float getWidthR() const override;
	int getHeight() const override;
	float getHeightF() const override;
	float getHeightR() const override;
	int getStride() const override;
};
class WriteableTexture2D : public MH33::GFX::WriteableTexture2D {
private:
	Texture texHndl;
	int width, height, stride;
	float widthF, widthR, heightF, heightR;
	MH33::GFX::TextureFormat format;
	void updateDim(int nwidth, int nheight, int npixxisze);
public:
	WriteableTexture2D();
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	const Texture& getTexHndl() const;
	Texture& getTexHndl();
	MH33::GFX::TextureFormat getFormat() const override;
	int getWidth() const override;
	float getWidthF() const override;
	float getWidthR() const override;
	int getHeight() const override;
	float getHeightF() const override;
	float getHeightR() const override;
	int getStride() const override;
	bool resize(int newWidth, int newHeight) override;
	void blit(const MH33::GFX::WriteableTexture2D& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void blit(const MH33::GFX::WriteableTexture2D& cpy, const glm::ivec2 offset) override;
	void blit(const std::span<const std::byte>& data, MH33::GFX::TextureFormat format, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void update() override;
};
class ArrayTexture2D : public MH33::GFX::ArrayTexture2D {
private:
	Texture texHndl;
	int width, height, depth, stride;
	float widthF, widthR, heightF, heightR, depthF, depthR;
	MH33::GFX::TextureFormat format;
	void updateDim(int nwidth, int nheight, int ndepth, int npixxisze);
public:
	ArrayTexture2D();
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	const Texture& getTexHndl() const;
	Texture& getTexHndl();
	MH33::GFX::TextureFormat getFormat() const override;
	int getWidth() const override;
	float getWidthF() const override;
	float getWidthR() const override;
	int getHeight() const override;
	float getHeightF() const override;
	float getHeightR() const override;
	int getDepth() const override;
	float getDepthF() const override;
	float getDepthR() const override;
	int getStride() const override;
};
class Texture3D : public MH33::GFX::Texture3D {
private:
	Texture texHndl;
	int width, height, depth, stride;
	float widthF, widthR, heightF, heightR, depthF, depthR;
	MH33::GFX::TextureFormat format;
	void updateDim(int nwidth, int nheight, int ndepth, int npixxisze);
public:
	Texture3D();
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	const Texture& getTexHndl() const;
	Texture& getTexHndl();
	MH33::GFX::TextureFormat getFormat() const override;
	int getWidth() const override;
	float getWidthF() const override;
	float getWidthR() const override;
	int getHeight() const override;
	float getHeightF() const override;
	float getHeightR() const override;
	int getDepth() const override;
	float getDepthF() const override;
	float getDepthR() const override;
	int getStride() const override;
};

}

#endif // GLTEXTURE_ADV_HPP
