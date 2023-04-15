#ifndef SOFTSTANDARDTEXTURE_HPP
#define SOFTSTANDARDTEXTURE_HPP
#include <Renderer/Software/SoftTexture.hpp>
#include <MhLib/Util/PixelFormat.hpp>
#include <vector>
#include <cstring>
#include <span>

namespace Renderer {
namespace Software {

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
	void blit(const std::span<const std::byte>& data, MH33::GFX::TextureFormat format, const glm::ivec2 offset, const glm::ivec2& dimensions)
	{
		if(format != fmt) return;
		int linesToCopy = std::min(dimensions.y,height - offset.y);
		int pixelsPerLines = std::min(dimensions.x,width - offset.x);
		if(linesToCopy <= 0 || pixelsPerLines <= 0) return;
		const PixelType * const inPixels = reinterpret_cast<const PixelType*>(data.data());
		for(int y = 0; y < linesToCopy; ++y) {
			PixelType * const destLinePtr = &(pixels[(offset.y + y) * width + offset.x]);
			const PixelType * const inPixelLine = &(inPixels[y * dimensions.x]);
			memcpy(destLinePtr,inPixelLine,pixelsPerLines * sizeof(PixelType));
		}
	}
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
	void blit(const std::span<const std::byte>& data, MH33::GFX::TextureFormat format, const glm::ivec2 offset, const glm::ivec2& dimensions)
	{
		if(format != fmt) return;
		int linesToCopy = std::min(dimensions.y,height - offset.y);
		int pixelsPerLines = std::min(dimensions.x,width - offset.x);
		if(linesToCopy <= 0 || pixelsPerLines <= 0) return;
		const PixelType * const inPixels = reinterpret_cast<const PixelType*>(data.data());
		for(int y = 0; y < linesToCopy; ++y) {
			PixelType * const destLinePtr = &(pixels[(offset.y + y) * width + offset.x]);
			const PixelType * const inPixelLine = &(inPixels[y * dimensions.x]);
			memcpy(destLinePtr,inPixelLine,pixelsPerLines * sizeof(PixelType));
		}
	}
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

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline StandardTexture<PixelType,fmt>::StandardTexture(const StandardTexture& cpy)
	: pixels(cpy.pixels), width(cpy.width), height(cpy.height), stride(cpy.stride),
	  widthF(cpy.widthF), heightF(cpy.heightF), widthR(cpy.widthR), heightR(cpy.heightR) {

}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline StandardTexture<PixelType,fmt>::StandardTexture(StandardTexture&& mov)
	: pixels(std::move(mov.pixels)), width(mov.width), height(mov.height), stride(mov.stride),
	  widthF(mov.widthF), heightF(mov.heightF), widthR(mov.widthR), heightR(mov.heightR) {
	mov.width = 0;
	mov.height = 0;
	mov.stride = 0;
	mov.widthF = 0.0f;
	mov.heightF = 0.0f;
	mov.widthR = 0.0f;
	mov.heightR = 0.0f;
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline StandardTexture<PixelType,fmt>& StandardTexture<PixelType,fmt>::operator=(const StandardTexture& cpy) {
	this->pixels = cpy.pixels;
	this->width = cpy.width;
	this->height = cpy.height;
	this->stride = cpy.stride;
	this->widthF = cpy.widthF;
	this->heightF = cpy.heightF;
	this->widthR = cpy.widthR;
	this->heightR = cpy.heightR;
	return *this;
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline StandardTexture<PixelType,fmt>& StandardTexture<PixelType,fmt>::operator=(StandardTexture&& mov) {
	this->pixels = std::move(mov.pixels);
	this->width = mov.width;
	this->height = mov.height;
	this->stride = mov.stride;
	this->widthF = mov.widthF;
	this->heightF = mov.heightF;
	this->widthR = mov.widthR;
	this->heightR = mov.heightR;
	mov.width = 0;
	mov.height = 0;
	mov.stride = 0;
	mov.widthF = 0.0f;
	mov.heightF = 0.0f;
	mov.widthR = 0.0f;
	mov.heightR = 0.0f;
	return *this;
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline StandardTexture<PixelType,fmt>::StandardTexture(int width, int height)
	: pixels(width*height), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{
	std::memset(pixels.data(),0,sizeof(PixelType)*pixels.size());
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline StandardTexture<PixelType,fmt>::StandardTexture(const PixelType* pixelsToCopy, int width, int height)
	: pixels(width*height), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{
	std::memcpy(pixels.data(),pixelsToCopy,sizeof(PixelType)*pixels.size());
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline StandardTexture<PixelType,fmt>::StandardTexture(const std::span<PixelType> pixelsToCopy, int width, int height)
	: pixels(width*height), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{
	std::memcpy(pixels.data(),pixelsToCopy.data(),sizeof(PixelType)*pixels.size());
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline StandardTexture<PixelType,fmt>::StandardTexture(std::vector<PixelType>&& mov, int width, int height)
	: pixels(std::move(mov)), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{

}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline bool StandardTexture<PixelType,fmt>::resize(int newWidth, int newHeight) {
	const int rowsToRunThrough = std::min(this->height,newHeight);
	const size_t bytesPerRow = std::min(this->width,newWidth) * sizeof(PixelType);
	std::vector<PixelType> newPixels(newWidth * newHeight);
	std::memset(newPixels.data(),0,sizeof(PixelType) * newPixels.size());
	for(int y = 0; y < rowsToRunThrough; ++y) {
		const PixelType* const originalRow = &pixels[y*width];
		PixelType* const newRow = &newPixels[y*newWidth];
		memcpy(newRow,originalRow,bytesPerRow);
	}
	this->pixels = std::move(newPixels);
	this->width = newWidth;
	this->height = newHeight;
	this->widthF = static_cast<float>(newWidth - 1);
	this->heightF = static_cast<float>(newHeight - 1);
	this->widthR = 1.0f / static_cast<float>(newWidth);
	this->heightR = 1.0f / static_cast<float>(newHeight);
	return true;
}

/*template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::blit(const PixelType* cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) {
	if(offset.x >= width || offset.y >= height) return;
	const int rowsToCopy = std::min(dimensions.y,height - offset.y);
	const int columnsToCopy = std::min(dimensions.x,width - offset.x);
	const size_t bytesPerRow = columnsToCopy * sizeof(PixelType);
	for(int y = 0; y < rowsToCopy; ++y) {
		const PixelType* const originalRow = cpy + (y*dimensions.x);
		PixelType* const newRow = pixels.data() + (((offset.y+y)*width)+offset.x);
		memcpy(newRow,originalRow,bytesPerRow);
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::blit(const Texture& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) {
	const StandardTexture* standardTexture = dynamic_cast<const StandardTexture*>(&cpy);
	if(standardTexture) blit(standardTexture->pixels.data(),offset,dimensions);
	else {
		const ReferenceTexture<PixelType,fmt>* referenceTexture = dynamic_cast<const ReferenceTexture<PixelType,fmt>*>(&cpy);
		if(referenceTexture) blit(referenceTexture->pixels.data(),offset,dimensions);
		else Texture::blit(cpy,offset,dimensions);
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::blit(const Texture& cpy, const glm::ivec2 offset) {
	const StandardTexture* standardTexture = dynamic_cast<const StandardTexture*>(&cpy);
	if(standardTexture) blit(standardTexture->pixels.data(),offset,glm::ivec2(standardTexture->width,standardTexture->height));
	else {
		const ReferenceTexture<PixelType,fmt>* referenceTexture = dynamic_cast<const ReferenceTexture<PixelType,fmt>*>(&cpy);
		if(referenceTexture) blit(referenceTexture->pixels.data(),offset,glm::ivec2(referenceTexture->width,referenceTexture->height));
		else Texture::blit(cpy,offset);
	}
}*/

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel, Wrap wrap) const {
	int x;
	int y;
	switch (wrap) {
	case Wrap::REPEAT:
		x = (pos.x >= 0) ? (pos.x % width) : ((pos.x % width) + width);
		y = (pos.y >= 0) ? (pos.y % height) : ((pos.y % height) + height);
		break;
	case MIRRORED_REPEAT:
		x = std::abs(pos.x % width);
		y = std::abs(pos.y % height);
		break;
	case CLAMP_TO_EDGE:
		x = std::clamp(pos.x,0,width);
		y = std::clamp(pos.y,0,height);
		break;
	case CLAMP_TO_BORDER:
		if(pos.x > 0 && pos.x < width && pos.y > 0 && pos.y < height) { x = pos.x; y = pos.y; }
		else { std::memset(&colourKernel,0,sizeof(glm::fvec4)); return; }
		break;
	}
	pixels[ (y * width) + x ].toKernel(colourKernel);
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::setPixel(const glm::ivec2& pos, const glm::fvec4& colourKernel) {
	pixels[ (pos.y * width) + pos.x ].fromKernel(colourKernel);
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::setPixelDithered(const glm::ivec2& pos, const glm::fvec4& colourKernel) {
	pixels[ (pos.y * width) + pos.x ].fromKernelDithered(colourKernel,pos);
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void* StandardTexture<PixelType,fmt>::getRawPixels() {
	return pixels.data();
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline const void* StandardTexture<PixelType,fmt>::getRawPixels() const {
	return pixels.data();
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::clearToColour(const glm::fvec4& colourKernel) {
	PixelType p;
	p.fromKernel(colourKernel);
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			scanline[x] = p;
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::clearToColour(const ColourProgrammer& program) {
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			scanline[x].fromKernelDithered(program(pos),pos);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::clearToColour(const ColourProgrammer2& program) {
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			scanline[x].fromKernelDithered(program(pos,kernel),pos);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::clearToColour(const ColourProgrammer3& program)
{
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			scanline[x].fromKernelDithered(program(fpos),pos);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::clearToColour(const ColourProgrammer4& program)
{
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			scanline[x].fromKernelDithered(program(fpos,kernel),pos);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::iterateOverPixels(const ColourIterator& program) const
{
	for(int y = 0; y < height; ++y) {
		const PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			program(pos,kernel);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void StandardTexture<PixelType,fmt>::iterateOverPixels(const ColourIterator2& program) const
{
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = 0; y < height; ++y) {
		const PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			program(fpos,kernel);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline ReferenceTexture<PixelType,fmt>::ReferenceTexture(const ReferenceTexture& cpy)
	: pixels(cpy.pixels), width(cpy.width), height(cpy.height), stride(cpy.stride),
	  widthF(cpy.widthF), heightF(cpy.heightF), widthR(cpy.widthR), heightR(cpy.heightR)  {

}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline ReferenceTexture<PixelType,fmt>& ReferenceTexture<PixelType,fmt>::operator=(const ReferenceTexture& cpy) {
	this->pixels = cpy.pixels;
	this->width = cpy.width;
	this->height = cpy.height;
	this->stride = cpy.stride;
	this->widthF = cpy.widthF;
	this->heightF = cpy.heightF;
	this->widthR = cpy.widthR;
	this->heightR = cpy.heightR;
	return *this;
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline ReferenceTexture<PixelType,fmt>::ReferenceTexture(PixelType* pixelsPointing, int width, int height)
	: pixels(pixelsPointing,width*height), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{

}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline ReferenceTexture<PixelType,fmt>::ReferenceTexture(const std::span<PixelType> pixelsToCopy, int width, int height)
	: pixels(pixelsToCopy), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{

}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline ReferenceTexture<PixelType,fmt>::ReferenceTexture(std::span<PixelType>&& mov, int width, int height)
	: pixels(std::move(mov)), width(width), height(height), stride(width*sizeof(PixelType)),
	  widthF(width-1), heightF(height-1), widthR(1.0f / static_cast<float>(width)), heightR(1.0f / static_cast<float>(height))
{

}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline bool ReferenceTexture<PixelType,fmt>::resize(int newWidth, int newHeight) {
	(void)newWidth;
	(void)newHeight;
	return false;
}

/*template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::blit(const PixelType* cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) {
	if(offset.x >= width || offset.y >= height) return;
	const int rowsToCopy = std::min(dimensions.y,height - offset.y);
	const int columnsToCopy = std::min(dimensions.x,width - offset.x);
	const size_t bytesPerRow = columnsToCopy * sizeof(PixelType);
	for(int y = 0; y < rowsToCopy; ++y) {
		const PixelType* const originalRow = cpy + (y*dimensions.x);
		PixelType* const newRow = pixels.data() + (((offset.y+y)*width)+offset.x);
		memcpy(newRow,originalRow,bytesPerRow);
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::blit(const Texture& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions) {
	const ReferenceTexture* refTxt = dynamic_cast<const ReferenceTexture*>(&cpy);
	if(refTxt) blit(refTxt->pixels.data(),offset,dimensions);
	else {
		const StandardTexture<PixelType,fmt>* standardTexture = dynamic_cast<const StandardTexture<PixelType,fmt>*>(&cpy);
		if(standardTexture) blit(standardTexture->pixels.data(),offset,dimensions);
		else Texture::blit(cpy,offset,dimensions);
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::blit(const Texture& cpy, const glm::ivec2 offset) {
	const ReferenceTexture* refTxt = dynamic_cast<const ReferenceTexture*>(&cpy);
	if(refTxt) blit(refTxt->pixels.data(),offset,glm::ivec2(refTxt->width,refTxt->height));
	else {
		const StandardTexture<PixelType,fmt>* standardTexture = dynamic_cast<const StandardTexture<PixelType,fmt>*>(&cpy);
		if(standardTexture) blit(standardTexture->pixels.data(),offset,glm::ivec2(standardTexture->width,standardTexture->height));
		else Texture::blit(cpy,offset);
	}
}*/

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel, Wrap wrap) const {
	int x;
	int y;
	switch (wrap) {
	case Wrap::REPEAT:
		x = (pos.x >= 0) ? (pos.x % width) : ((pos.x % width) + width);
		y = (pos.y >= 0) ? (pos.y % height) : ((pos.y % height) + height);
		break;
	case MIRRORED_REPEAT:
		x = std::abs(pos.x % width);
		y = std::abs(pos.y % height);
		break;
	case CLAMP_TO_EDGE:
		x = std::clamp(pos.x,0,width);
		y = std::clamp(pos.y,0,height);
		break;
	case CLAMP_TO_BORDER:
		if(pos.x > 0 && pos.x < width && pos.y > 0 && pos.y < height) { x = pos.x; y = pos.y; }
		else { std::memset(&colourKernel,0,sizeof(glm::fvec4)); return; }
		break;
	}
	pixels[ (y * width) + x ].toKernel(colourKernel);
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::setPixel(const glm::ivec2& pos, const glm::fvec4& colourKernel) {
	pixels[ (pos.y * width) + pos.x ].fromKernel(colourKernel);
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::setPixelDithered(const glm::ivec2& pos, const glm::fvec4& colourKernel) {
	pixels[ (pos.y * width) + pos.x ].fromKernelDithered(colourKernel,pos);
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void* ReferenceTexture<PixelType,fmt>::getRawPixels() {
	return pixels.data();
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline const void* ReferenceTexture<PixelType,fmt>::getRawPixels() const {
	return pixels.data();
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::clearToColour(const glm::fvec4& colourKernel) {
	PixelType p;
	p.fromKernel(colourKernel);
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			scanline[x] = p;
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::clearToColour(const ColourProgrammer& program) {
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			scanline[x].fromKernelDithered(program(pos),pos);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::clearToColour(const ColourProgrammer2& program) {
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			scanline[x].fromKernelDithered(program(pos,kernel),pos);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::clearToColour(const ColourProgrammer3& program)
{
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			scanline[x].fromKernelDithered(program(fpos),pos);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::clearToColour(const ColourProgrammer4& program)
{
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = 0; y < height; ++y) {
		PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			scanline[x].fromKernelDithered(program(fpos,kernel),pos);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::iterateOverPixels(const ColourIterator& program) const
{
	for(int y = 0; y < height; ++y) {
		const PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::ivec2 pos = glm::ivec2(x,y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			program(pos,kernel);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt>
inline void ReferenceTexture<PixelType,fmt>::iterateOverPixels(const ColourIterator2& program) const
{
	const glm::fvec2 reciprocal = glm::fvec2( 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) );
	for(int y = 0; y < height; ++y) {
		const PixelType* const scanline = &pixels[y*width];
		for(int x = 0; x < width; ++x) {
			glm::fvec2 fpos = glm::fvec2(static_cast<float>(x) * reciprocal.x, static_cast<float>(y) * reciprocal.y);
			glm::fvec4 kernel;
			scanline[x].toKernel(kernel);
			program(fpos,kernel);
		}
	}
}

template <typename PixelType, MH33::GFX::TextureFormat fmt> StandardTexture<PixelType,fmt>::StandardTexture(const ReferenceTexture<PixelType,fmt>& cpy) {
	this->pixels.resize(cpy.pixels.size());
	std::memcpy(this->pixels.data(),cpy.pixels.data(),cpy.pixels.size() * sizeof(PixelType));
	this->width = cpy.width;
	this->height = cpy.height;
	this->stride = cpy.stride;
	this->widthF = cpy.widthF;
	this->heightF = cpy.heightF;
	this->widthR = cpy.widthR;
	this->heightR = cpy.heightR;
}
template <typename PixelType, MH33::GFX::TextureFormat fmt> StandardTexture<PixelType,fmt>& StandardTexture<PixelType,fmt>::operator=(const ReferenceTexture<PixelType,fmt>& cpy) {
	this->pixels.resize(cpy.pixels.size());
	std::memcpy(this->pixels.data(),cpy.pixels.data(),cpy.pixels.size() * sizeof(PixelType));
	this->width = cpy.width;
	this->height = cpy.height;
	this->stride = cpy.stride;
	this->widthF = cpy.widthF;
	this->heightF = cpy.heightF;
	this->widthR = cpy.widthR;
	this->heightR = cpy.heightR;
	return *this;
}
template <typename PixelType, MH33::GFX::TextureFormat fmt> ReferenceTexture<PixelType,fmt>::ReferenceTexture(const StandardTexture<PixelType,fmt>& cpy) {
	this->pixels = cpy.pixels;
	this->width = cpy.width;
	this->height = cpy.height;
	this->stride = cpy.stride;
	this->widthF = cpy.widthF;
	this->heightF = cpy.heightF;
	this->widthR = cpy.widthR;
	this->heightR = cpy.heightR;
}
template <typename PixelType, MH33::GFX::TextureFormat fmt> ReferenceTexture<PixelType,fmt>& ReferenceTexture<PixelType,fmt>::operator=(const StandardTexture<PixelType,fmt>& cpy) {
	this->pixels = cpy.pixels;
	this->width = cpy.width;
	this->height = cpy.height;
	this->stride = cpy.stride;
	this->widthF = cpy.widthF;
	this->heightF = cpy.heightF;
	this->widthR = cpy.widthR;
	this->heightR = cpy.heightR;
	return *this;
}

}
}

#endif // SOFTSTANDARDTEXTURE_HPP
