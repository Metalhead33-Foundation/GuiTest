#ifndef STANDARDTEXTURE_HPP
#define STANDARDTEXTURE_HPP
#include "Texture.hpp"
#include <MhLib/Util/PixelFormat.hpp>
#include <vector>
#include <cstring>
#include <span>

namespace SoftwareRenderer {
template <typename PixelType, MH33::GFX::TextureFormat fmt> class StandardTexture;
template <typename PixelType, MH33::GFX::TextureFormat fmt> class ReferenceTexture;

template <typename PixelType, MH33::GFX::TextureFormat fmt> class StandardTexture : public Texture {
private:
	std::vector<PixelType> pixels;
	int width,height,stride;
	float widthF,heightF,widthR,heightR;
public:
	friend class ReferenceTexture<PixelType,fmt>;
	virtual MH33::GFX::TextureFormat getFormat() const override { return fmt; }
	StandardTexture(const StandardTexture& cpy);
	StandardTexture(StandardTexture&& mov);
	StandardTexture& operator=(const StandardTexture& cpy);
	StandardTexture& operator=(StandardTexture&& mov);
	StandardTexture(int width, int height);
	StandardTexture(const PixelType* pixelsToCopy, int width, int height);
	StandardTexture(const std::span<PixelType> pixelsToCopy, int width, int height);
	StandardTexture(std::vector<PixelType>&& mov, int width, int height);
	StandardTexture(const ReferenceTexture<PixelType,fmt>& cpy);
	StandardTexture& operator=(const ReferenceTexture<PixelType,fmt>& cpy);
	bool resize(int newWidth, int newHeight) override;
	void blit(const PixelType* cpy, const glm::ivec2 offset, const glm::ivec2& dimensions);
	void blit(const Texture& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void blit(const Texture& cpy, const glm::ivec2 offset) override;
	int getWidth() const override { return width; }
	float getWidthF() const override { return widthF; }
	float getWidthR() const override { return widthR; }
	int getHeight() const override { return height; }
	float getHeightF() const override { return heightF; }
	float getHeightR() const override { return heightR; }
	int getStride() const override { return stride; }
	void getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override;
	void setPixel(const glm::ivec2& pos, const glm::fvec4& colourKernel) override;
	void setPixelDithered(const glm::ivec2& pos, const glm::fvec4& colourKernel) override;
	void* getRawPixels() override;
	const void* getRawPixels() const override;
	void clearToColour(const glm::fvec4& colourKernel) override;
	void clearToColour(const ColourProgrammer& program) override;
	void clearToColour(const ColourProgrammer2& program) override;
	void clearToColour(const ColourProgrammer3& program);
	void clearToColour(const ColourProgrammer4& program);
	void iterateOverPixels(const ColourIterator& program) const;
	void iterateOverPixels(const ColourIterator2& program) const;
};

typedef StandardTexture<PixelGreyscale_U8, MH33::GFX::TextureFormat::R8U> TexGreyscale_U8;
typedef StandardTexture<PixelGreyscale_U16, MH33::GFX::TextureFormat::R16U> TexGreyscale_U16;
typedef StandardTexture<PixelGreyscale_U32, MH33::GFX::TextureFormat::R32U> TexGreyscale_U32;
typedef StandardTexture<PixelRG_U8, MH33::GFX::TextureFormat::RG8U> TexRG_U8;
typedef StandardTexture<PixelRG_U16, MH33::GFX::TextureFormat::RG16U> TexRG_U16;
typedef StandardTexture<PixelRG_U32, MH33::GFX::TextureFormat::RG32U> TexRG_U32;
typedef StandardTexture<PixelRGB_U8, MH33::GFX::TextureFormat::RGB8U> TexRGB_U8;
typedef StandardTexture<PixelRGB_U16, MH33::GFX::TextureFormat::RGB16U> TexRGB_U16;
typedef StandardTexture<PixelRGB_U32, MH33::GFX::TextureFormat::RGB32U> TexRGB_U32;
typedef StandardTexture<PixelBGR_U8, MH33::GFX::TextureFormat::BGR8U> TexBGR_U8;
typedef StandardTexture<PixelBGR_U16, MH33::GFX::TextureFormat::BGR16U> TexBGR_U16;
typedef StandardTexture<PixelBGR_U32, MH33::GFX::TextureFormat::BGR32U> TexBGR_U32;
typedef StandardTexture<PixelRGBA_U8, MH33::GFX::TextureFormat::RGBA8U> TexRGBA_U8;
typedef StandardTexture<PixelRGBA_U16, MH33::GFX::TextureFormat::RGBA16U> TexRGBA_U16;
typedef StandardTexture<PixelRGBA_U32, MH33::GFX::TextureFormat::RGBA32U> TexRGBA_U32;
typedef StandardTexture<PixelBGRA_U8, MH33::GFX::TextureFormat::BGRA8U> TexBGRA_U8;
typedef StandardTexture<PixelBGRA_U16, MH33::GFX::TextureFormat::BGRA16U> TexBGRA_U16;
typedef StandardTexture<PixelBGRA_U32, MH33::GFX::TextureFormat::BGRA32U> TexBGRA_U32;
typedef StandardTexture<PixelARGB_U8, MH33::GFX::TextureFormat::ARGB8U> TexARGB_U8;
typedef StandardTexture<PixelARGB_U16, MH33::GFX::TextureFormat::ARGB16U> TexARGB_U16;
typedef StandardTexture<PixelARGB_U32, MH33::GFX::TextureFormat::ARGB32U> TexARGB_U32;
typedef StandardTexture<PixelABGR_U8, MH33::GFX::TextureFormat::INVALID> TexABGR_U8;
typedef StandardTexture<PixelABGR_U16, MH33::GFX::TextureFormat::INVALID> TexABGR_U16;
typedef StandardTexture<PixelABGR_U32, MH33::GFX::TextureFormat::INVALID> TexABGR_U32;
typedef StandardTexture<PixelRGB332, MH33::GFX::TextureFormat::INVALID> TexRGB332;
typedef StandardTexture<PixelRGB444, MH33::GFX::TextureFormat::RGB444> TexRGB444;
typedef StandardTexture<PixelRGB555, MH33::GFX::TextureFormat::RGB555> TexRGB555;
typedef StandardTexture<PixelRGB565, MH33::GFX::TextureFormat::RGB565> TexRGB565;
typedef StandardTexture<PixelBGR555, MH33::GFX::TextureFormat::INVALID> TexBGR555;
typedef StandardTexture<PixelBGR565, MH33::GFX::TextureFormat::INVALID> TexBGR565;
typedef StandardTexture<PixelARGB4444, MH33::GFX::TextureFormat::INVALID> TexARGB4444;
typedef StandardTexture<PixelBGRA4444, MH33::GFX::TextureFormat::INVALID> TexBGRA4444;
typedef StandardTexture<PixelRGBA4444, MH33::GFX::TextureFormat::RGBA444> TexRGBA4444;
typedef StandardTexture<PixelABGR4444, MH33::GFX::TextureFormat::INVALID> TexABGR4444;
typedef StandardTexture<PixelARGB1555, MH33::GFX::TextureFormat::INVALID> TexARGB1555;
typedef StandardTexture<PixelRGBA5551, MH33::GFX::TextureFormat::INVALID> TexRGBA5551;
typedef StandardTexture<PixelABGR1555, MH33::GFX::TextureFormat::INVALID> TexABGR1555;
typedef StandardTexture<PixelBGRA5551, MH33::GFX::TextureFormat::INVALID> TexBGRA5551;
typedef StandardTexture<PixelRGBX8888, MH33::GFX::TextureFormat::INVALID> TexRGBX8888;
typedef StandardTexture<PixelBGRX8888, MH33::GFX::TextureFormat::INVALID> TexBGRX8888;
typedef StandardTexture<PixelRGB888, MH33::GFX::TextureFormat::RGB8U> TexRGB888;
typedef StandardTexture<PixelBGR888, MH33::GFX::TextureFormat::BGR8U> TexBGR888;
typedef StandardTexture<PixelARGB8888, MH33::GFX::TextureFormat::ARGB8U> TexARGB8888;
typedef StandardTexture<PixelRGBA8888, MH33::GFX::TextureFormat::RGBA8U> TexRGBA8888;
typedef StandardTexture<PixelABGR8888, MH33::GFX::TextureFormat::INVALID> TexABGR8888;
typedef StandardTexture<PixelBGRA8888, MH33::GFX::TextureFormat::BGRA8U> TexBGRA8888;

template <typename PixelType, MH33::GFX::TextureFormat fmt> class ReferenceTexture : public Texture {
private:
	std::span<PixelType> pixels;
	int width,height,stride;
	float widthF,heightF,widthR,heightR;
public:
	friend class StandardTexture<PixelType,fmt>;
	virtual MH33::GFX::TextureFormat getFormat() const override { return fmt; }
	ReferenceTexture(const ReferenceTexture& cpy);
	ReferenceTexture& operator=(const ReferenceTexture& cpy);
	ReferenceTexture(PixelType* pixelsPointing, int width, int height);
	ReferenceTexture(const std::span<PixelType> pixelsToCopy, int width, int height);
	ReferenceTexture(std::span<PixelType>&& mov, int width, int height);
	ReferenceTexture(const StandardTexture<PixelType,fmt>& cpy);
	ReferenceTexture& operator=(const StandardTexture<PixelType,fmt>& cpy);
	bool resize(int newWidth, int newHeight) override;
	void blit(const PixelType* cpy, const glm::ivec2 offset, const glm::ivec2& dimensions);
	void blit(const Texture& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) override;
	void blit(const Texture& cpy, const glm::ivec2 offset) override;
	int getWidth() const override { return width; }
	float getWidthF() const override { return widthF; }
	float getWidthR() const override { return widthR; }
	int getHeight() const override { return height; }
	float getHeightF() const override { return heightF; }
	float getHeightR() const override { return heightR; }
	int getStride() const override { return stride; }
	void getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override;
	void setPixel(const glm::ivec2& pos, const glm::fvec4& colourKernel) override;
	void setPixelDithered(const glm::ivec2& pos, const glm::fvec4& colourKernel) override;
	void* getRawPixels() override;
	const void* getRawPixels() const override;
	void clearToColour(const glm::fvec4& colourKernel) override;
	void clearToColour(const ColourProgrammer& program) override;
	void clearToColour(const ColourProgrammer2& program) override;
	void clearToColour(const ColourProgrammer3& program);
	void clearToColour(const ColourProgrammer4& program);
	void iterateOverPixels(const ColourIterator& program) const;
	void iterateOverPixels(const ColourIterator2& program) const;
};

typedef ReferenceTexture<PixelGreyscale_U8, MH33::GFX::TextureFormat::R8U> RefTexGreyscale_U8;
typedef ReferenceTexture<PixelGreyscale_U16, MH33::GFX::TextureFormat::R16U> RefTexGreyscale_U16;
typedef ReferenceTexture<PixelGreyscale_U32, MH33::GFX::TextureFormat::R32U> RefTexGreyscale_U32;
typedef ReferenceTexture<PixelRG_U8, MH33::GFX::TextureFormat::RG8U> RefTexRG_U8;
typedef ReferenceTexture<PixelRG_U16, MH33::GFX::TextureFormat::RG16U> RefTexRG_U16;
typedef ReferenceTexture<PixelRG_U32, MH33::GFX::TextureFormat::RG32U> RefTexRG_U32;
typedef ReferenceTexture<PixelRGB_U8, MH33::GFX::TextureFormat::RGB8U> RefTexRGB_U8;
typedef ReferenceTexture<PixelRGB_U16, MH33::GFX::TextureFormat::RGB16U> RefTexRGB_U16;
typedef ReferenceTexture<PixelRGB_U32, MH33::GFX::TextureFormat::RGB32U> RefTexRGB_U32;
typedef ReferenceTexture<PixelBGR_U8, MH33::GFX::TextureFormat::BGR8U> RefTexBGR_U8;
typedef ReferenceTexture<PixelBGR_U16, MH33::GFX::TextureFormat::BGR16U> RefTexBGR_U16;
typedef ReferenceTexture<PixelBGR_U32, MH33::GFX::TextureFormat::BGR32U> RefTexBGR_U32;
typedef ReferenceTexture<PixelRGBA_U8, MH33::GFX::TextureFormat::RGBA8U> RefTexRGBA_U8;
typedef ReferenceTexture<PixelRGBA_U16, MH33::GFX::TextureFormat::RGBA16U> RefTexRGBA_U16;
typedef ReferenceTexture<PixelRGBA_U32, MH33::GFX::TextureFormat::RGBA32U> RefTexRGBA_U32;
typedef ReferenceTexture<PixelBGRA_U8, MH33::GFX::TextureFormat::BGRA8U> RefTexBGRA_U8;
typedef ReferenceTexture<PixelBGRA_U16, MH33::GFX::TextureFormat::BGRA16U> RefTexBGRA_U16;
typedef ReferenceTexture<PixelBGRA_U32, MH33::GFX::TextureFormat::BGRA32U> RefTexBGRA_U32;
typedef ReferenceTexture<PixelARGB_U8, MH33::GFX::TextureFormat::ARGB8U> RefTexARGB_U8;
typedef ReferenceTexture<PixelARGB_U16, MH33::GFX::TextureFormat::ARGB16U> RefTexARGB_U16;
typedef ReferenceTexture<PixelARGB_U32, MH33::GFX::TextureFormat::ARGB32U> RefTexARGB_U32;
typedef ReferenceTexture<PixelABGR_U8, MH33::GFX::TextureFormat::INVALID> RefTexABGR_U8;
typedef ReferenceTexture<PixelABGR_U16, MH33::GFX::TextureFormat::INVALID> RefTexABGR_U16;
typedef ReferenceTexture<PixelABGR_U32, MH33::GFX::TextureFormat::INVALID> RefTexABGR_U32;
typedef ReferenceTexture<PixelRGB332, MH33::GFX::TextureFormat::INVALID> RefTexRGB332;
typedef ReferenceTexture<PixelRGB444, MH33::GFX::TextureFormat::RGB444> RefTexRGB444;
typedef ReferenceTexture<PixelRGB555, MH33::GFX::TextureFormat::RGB555> RefTexRGB555;
typedef ReferenceTexture<PixelRGB565, MH33::GFX::TextureFormat::RGB565> RefTexRGB565;
typedef ReferenceTexture<PixelBGR555, MH33::GFX::TextureFormat::INVALID> RefTexBGR555;
typedef ReferenceTexture<PixelBGR565, MH33::GFX::TextureFormat::INVALID> RefTexBGR565;
typedef ReferenceTexture<PixelARGB4444, MH33::GFX::TextureFormat::INVALID> RefTexARGB4444;
typedef ReferenceTexture<PixelBGRA4444, MH33::GFX::TextureFormat::INVALID> RefTexBGRA4444;
typedef ReferenceTexture<PixelRGBA4444, MH33::GFX::TextureFormat::RGBA444> RefTexRGBA4444;
typedef ReferenceTexture<PixelABGR4444, MH33::GFX::TextureFormat::INVALID> RefTexABGR4444;
typedef ReferenceTexture<PixelARGB1555, MH33::GFX::TextureFormat::INVALID> RefTexARGB1555;
typedef ReferenceTexture<PixelRGBA5551, MH33::GFX::TextureFormat::INVALID> RefTexRGBA5551;
typedef ReferenceTexture<PixelABGR1555, MH33::GFX::TextureFormat::INVALID> RefTexABGR1555;
typedef ReferenceTexture<PixelBGRA5551, MH33::GFX::TextureFormat::INVALID> RefTexBGRA5551;
typedef ReferenceTexture<PixelRGBX8888, MH33::GFX::TextureFormat::INVALID> RefTexRGBX8888;
typedef ReferenceTexture<PixelBGRX8888, MH33::GFX::TextureFormat::INVALID> RefTexBGRX8888;
typedef ReferenceTexture<PixelRGB888, MH33::GFX::TextureFormat::RGB8U> RefTexRGB888;
typedef ReferenceTexture<PixelBGR888, MH33::GFX::TextureFormat::BGR8U> RefTexBGR888;
typedef ReferenceTexture<PixelARGB8888, MH33::GFX::TextureFormat::ARGB8U> RefTexARGB8888;
typedef ReferenceTexture<PixelRGBA8888, MH33::GFX::TextureFormat::RGBA8U> RefTexRGBA8888;
typedef ReferenceTexture<PixelABGR8888, MH33::GFX::TextureFormat::INVALID> RefTexABGR8888;
typedef ReferenceTexture<PixelBGRA8888, MH33::GFX::TextureFormat::BGRA8U> RefTexBGRA8888;

#include "StandardTexture.ipp"
}

#endif // STANDARDTEXTURE_HPP
