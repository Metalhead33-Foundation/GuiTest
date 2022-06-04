#ifndef STANDARDTEXTURE_HPP
#define STANDARDTEXTURE_HPP
#include "Texture.hpp"
#include "../Util/PixelFormat.hpp"
#include <vector>
#include <cstring>
#include <span>

template <typename PixelType> class StandardTexture : public Texture {
private:
	std::vector<PixelType> pixels;
	int width,height,stride;
	float widthF,heightF,widthR,heightR;
public:
	StandardTexture(const StandardTexture& cpy);
	StandardTexture(StandardTexture&& mov);
	StandardTexture& operator=(const StandardTexture& cpy);
	StandardTexture& operator=(StandardTexture&& mov);
	StandardTexture(int width, int height);
	StandardTexture(const PixelType* pixelsToCopy, int width, int height);
	StandardTexture(const std::span<PixelType> pixelsToCopy, int width, int height);
	StandardTexture(std::vector<PixelType>&& mov, int width, int height);
	bool resize(int newWidth, int newHeight) override;
	void blit(const PixelType* cpy, const glm::ivec2 offset, const glm::ivec2& dimensions);
	void blit(const StandardTexture& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions);
	void blit(const StandardTexture& cpy, const glm::ivec2 offset);
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

typedef StandardTexture<PixelGreyscale_U8> TexGreyscale_U8;
typedef StandardTexture<PixelGreyscale_U16> TexGreyscale_U16;
typedef StandardTexture<PixelGreyscale_U32> TexGreyscale_U32;
typedef StandardTexture<PixelRG_U8> TexRG_U8;
typedef StandardTexture<PixelRG_U16> TexRG_U16;
typedef StandardTexture<PixelRG_U32> TexRG_U32;
typedef StandardTexture<PixelRGB_U8> TexRGB_U8;
typedef StandardTexture<PixelRGB_U16> TexRGB_U16;
typedef StandardTexture<PixelRGB_U32> TexRGB_U32;
typedef StandardTexture<PixelBGR_U8> TexBGR_U8;
typedef StandardTexture<PixelBGR_U16> TexBGR_U16;
typedef StandardTexture<PixelBGR_U32> TexBGR_U32;
typedef StandardTexture<PixelRGBA_U8> TexRGBA_U8;
typedef StandardTexture<PixelRGBA_U16> TexRGBA_U16;
typedef StandardTexture<PixelRGBA_U32> TexRGBA_U32;
typedef StandardTexture<PixelBGRA_U8> TexBGRA_U8;
typedef StandardTexture<PixelBGRA_U16> TexBGRA_U16;
typedef StandardTexture<PixelBGRA_U32> TexBGRA_U32;
typedef StandardTexture<PixelARGB_U8> TexARGB_U8;
typedef StandardTexture<PixelARGB_U16> TexARGB_U16;
typedef StandardTexture<PixelARGB_U32> TexARGB_U32;
typedef StandardTexture<PixelABGR_U8> TexABGR_U8;
typedef StandardTexture<PixelABGR_U16> TexABGR_U16;
typedef StandardTexture<PixelABGR_U32> TexABGR_U32;
typedef StandardTexture<PixelRGB332> TexRGB332;
typedef StandardTexture<PixelRGB444> TexRGB444;
typedef StandardTexture<PixelRGB555> TexRGB555;
typedef StandardTexture<PixelRGB565> TexRGB565;
typedef StandardTexture<PixelBGR555> TexBGR555;
typedef StandardTexture<PixelBGR565> TexBGR565;
typedef StandardTexture<PixelARGB4444> TexARGB4444;
typedef StandardTexture<PixelBGRA4444> TexBGRA4444;
typedef StandardTexture<PixelRGBA4444> TexRGBA4444;
typedef StandardTexture<PixelABGR4444> TexABGR4444;
typedef StandardTexture<PixelARGB1555> TexARGB1555;
typedef StandardTexture<PixelRGBA5551> TexRGBA5551;
typedef StandardTexture<PixelABGR1555> TexABGR1555;
typedef StandardTexture<PixelBGRA5551> TexBGRA5551;
typedef StandardTexture<PixelRGBX8888> TexRGBX8888;
typedef StandardTexture<PixelBGRX8888> TexBGRX8888;
typedef StandardTexture<PixelRGB888> TexRGB888;
typedef StandardTexture<PixelBGR888> TexBGR888;
typedef StandardTexture<PixelARGB8888> TexARGB8888;
typedef StandardTexture<PixelRGBA8888> TexRGBA8888;
typedef StandardTexture<PixelABGR8888> TexABGR8888;
typedef StandardTexture<PixelBGRA8888> TexBGRA8888;

template <typename PixelType> class ReferenceTexture : public Texture {
private:
	std::span<PixelType> pixels;
	int width,height,stride;
	float widthF,heightF,widthR,heightR;
public:
	ReferenceTexture(const ReferenceTexture& cpy);
	ReferenceTexture& operator=(const ReferenceTexture& cpy);
	ReferenceTexture(PixelType* pixelsPointing, int width, int height);
	ReferenceTexture(const std::span<PixelType> pixelsToCopy, int width, int height);
	ReferenceTexture(std::span<PixelType>&& mov, int width, int height);
	bool resize(int newWidth, int newHeight) override;
	void blit(const PixelType* cpy, const glm::ivec2 offset, const glm::ivec2& dimensions);
	void blit(const ReferenceTexture& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions);
	void blit(const ReferenceTexture& cpy, const glm::ivec2 offset);
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

typedef ReferenceTexture<PixelGreyscale_U8> RefTexGreyscale_U8;
typedef ReferenceTexture<PixelGreyscale_U16> RefTexGreyscale_U16;
typedef ReferenceTexture<PixelGreyscale_U32> RefTexGreyscale_U32;
typedef ReferenceTexture<PixelRG_U8> RefTexRG_U8;
typedef ReferenceTexture<PixelRG_U16> RefTexRG_U16;
typedef ReferenceTexture<PixelRG_U32> RefTexRG_U32;
typedef ReferenceTexture<PixelRGB_U8> RefTexRGB_U8;
typedef ReferenceTexture<PixelRGB_U16> RefTexRGB_U16;
typedef ReferenceTexture<PixelRGB_U32> RefTexRGB_U32;
typedef ReferenceTexture<PixelBGR_U8> RefTexBGR_U8;
typedef ReferenceTexture<PixelBGR_U16> RefTexBGR_U16;
typedef ReferenceTexture<PixelBGR_U32> RefTexBGR_U32;
typedef ReferenceTexture<PixelRGBA_U8> RefTexRGBA_U8;
typedef ReferenceTexture<PixelRGBA_U16> RefTexRGBA_U16;
typedef ReferenceTexture<PixelRGBA_U32> RefTexRGBA_U32;
typedef ReferenceTexture<PixelBGRA_U8> RefTexBGRA_U8;
typedef ReferenceTexture<PixelBGRA_U16> RefTexBGRA_U16;
typedef ReferenceTexture<PixelBGRA_U32> RefTexBGRA_U32;
typedef ReferenceTexture<PixelARGB_U8> RefTexARGB_U8;
typedef ReferenceTexture<PixelARGB_U16> RefTexARGB_U16;
typedef ReferenceTexture<PixelARGB_U32> RefTexARGB_U32;
typedef ReferenceTexture<PixelABGR_U8> RefTexABGR_U8;
typedef ReferenceTexture<PixelABGR_U16> RefTexABGR_U16;
typedef ReferenceTexture<PixelABGR_U32> RefTexABGR_U32;
typedef ReferenceTexture<PixelRGB332> RefTexRGB332;
typedef ReferenceTexture<PixelRGB444> RefTexRGB444;
typedef ReferenceTexture<PixelRGB555> RefTexRGB555;
typedef ReferenceTexture<PixelRGB565> RefTexRGB565;
typedef ReferenceTexture<PixelBGR555> RefTexBGR555;
typedef ReferenceTexture<PixelBGR565> RefTexBGR565;
typedef ReferenceTexture<PixelARGB4444> RefTexARGB4444;
typedef ReferenceTexture<PixelBGRA4444> RefTexBGRA4444;
typedef ReferenceTexture<PixelRGBA4444> RefTexRGBA4444;
typedef ReferenceTexture<PixelABGR4444> RefTexABGR4444;
typedef ReferenceTexture<PixelARGB1555> RefTexARGB1555;
typedef ReferenceTexture<PixelRGBA5551> RefTexRGBA5551;
typedef ReferenceTexture<PixelABGR1555> RefTexABGR1555;
typedef ReferenceTexture<PixelBGRA5551> RefTexBGRA5551;
typedef ReferenceTexture<PixelRGBX8888> RefTexRGBX8888;
typedef ReferenceTexture<PixelBGRX8888> RefTexBGRX8888;
typedef ReferenceTexture<PixelRGB888> RefTexRGB888;
typedef ReferenceTexture<PixelBGR888> RefTexBGR888;
typedef ReferenceTexture<PixelARGB8888> RefTexARGB8888;
typedef ReferenceTexture<PixelRGBA8888> RefTexRGBA8888;
typedef ReferenceTexture<PixelABGR8888> RefTexABGR8888;
typedef ReferenceTexture<PixelBGRA8888> RefTexBGRA8888;

#include "StandardTexture.ipp"

#endif // STANDARDTEXTURE_HPP
