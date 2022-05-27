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
	float widthF,heightF;
public:
	StandardTexture(const StandardTexture& cpy)
		: pixels(cpy.pixels), width(cpy.width), height(cpy.height), stride(cpy.stride), widthF(cpy.widthF), heightF(cpy.heightF) {

	}
	StandardTexture(StandardTexture&& mov)
		: pixels(std::move(mov.pixels)), width(mov.width), height(mov.height), stride(mov.stride), widthF(mov.widthF), heightF(mov.heightF) {
		mov.width = 0;
		mov.height = 0;
		mov.stride = 0;
		mov.widthF = 0.0f;
		mov.heightF = 0.0f;
	}
	StandardTexture& operator=(const StandardTexture& cpy) {
		this->pixels = cpy.pixels;
		this->width = cpy.width;
		this->height = cpy.height;
		this->stride = cpy.stride;
		this->widthF = cpy.widthF;
		this->heightF = cpy.heightF;
		return *this;
	}
	StandardTexture& operator=(StandardTexture&& mov) {
		this->pixels = std::move(mov.pixels);
		this->width = mov.width;
		this->height = mov.height;
		this->stride = mov.stride;
		this->widthF = mov.widthF;
		this->heightF = mov.heightF;
		mov.width = 0;
		mov.height = 0;
		mov.stride = 0;
		mov.widthF = 0.0f;
		mov.heightF = 0.0f;
		return *this;
	}
	StandardTexture(int width, int height)
		: pixels(width*height), width(width), height(height), stride(width*sizeof(PixelType)), widthF(width-1), heightF(height-1)
	{
		std::memset(pixels.data(),0,sizeof(PixelType)*pixels.size());
	}
	StandardTexture(const PixelType* pixelsToCopy, int width, int height)
		: pixels(width*height), width(width), height(height), stride(width*sizeof(PixelType)), widthF(width-1), heightF(height-1)
	{
		std::memcpy(pixels.data(),pixelsToCopy,sizeof(PixelType)*pixels.size());
	}
	StandardTexture(const std::span<PixelType> pixelsToCopy, int width, int height)
		: pixels(width*height), width(width), height(height), stride(width*sizeof(PixelType)), widthF(width-1), heightF(height-1)
	{
		std::memcpy(pixels.data(),pixelsToCopy.data(),sizeof(PixelType)*pixels.size());
	}
	StandardTexture(std::vector<PixelType>&& mov, int width, int height)
		: pixels(std::move(mov)), width(width), height(height), stride(width*sizeof(PixelType)), widthF(width-1), heightF(height-1)
	{

	}

	int getWidth() const override {
		return width;
	}
	float getWidthF() const override {
		return widthF;
	}
	int getHeight() const override {
		return height;
	}
	float getHeightF() const override {
		return heightF;
	}
	int getStride() const override {
		return stride;
	}
	void getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override {
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
	void setPixel(const glm::ivec2& pos, const glm::fvec4& colourKernel) override {
		pixels[ (pos.y * width) + pos.x ].fromKernel(colourKernel);
	}
	void setPixelDithered(const glm::ivec2& pos, const glm::fvec4& colourKernel) override {
		pixels[ (pos.y * width) + pos.x ].fromKernelDithered(colourKernel,pos);
	}
	void* getRawPixels() override {
		return pixels.data();
	}
	const void* getRawPixels() const override {
		return pixels.data();
	}
	void clearToColour(const glm::fvec4& colourKernel) override {
		PixelType p;
		p.fromKernel(colourKernel);
		for(int y = 0; y < height; ++y) {
			PixelType* const scanline = &pixels[y*width];
			for(int x = 0; x < width; ++x) {
				scanline[x] = p;
			}
		}
	}
	void clearToColour(const ColourProgrammer& program) override {
		for(int y = 0; y < height; ++y) {
			PixelType* const scanline = &pixels[y*width];
			for(int x = 0; x < width; ++x) {
				glm::ivec2 pos = glm::ivec2(x,y);
				scanline[x].fromKernelDithered(program(pos),pos);
			}
		}
	}
	void clearToColour(const ColourProgrammer2& program) override {
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
	void clearToColour(const ColourProgrammer3& program)
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
	void clearToColour(const ColourProgrammer4& program)
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
	void iterateOverPixels(const ColourIterator& program) const
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
	void iterateOverPixels(const ColourIterator2& program) const
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
	float widthF,heightF;
public:
	ReferenceTexture(const ReferenceTexture& cpy)
		: pixels(cpy.pixels), width(cpy.width), height(cpy.height), stride(cpy.stride), widthF(cpy.widthF), heightF(cpy.heightF) {

	}
	ReferenceTexture& operator=(const ReferenceTexture& cpy) {
		this->pixels = cpy.pixels;
		this->width = cpy.width;
		this->height = cpy.height;
		this->stride = cpy.stride;
		this->widthF = cpy.widthF;
		this->heightF = cpy.heightF;
		return *this;
	}
	ReferenceTexture(PixelType* pixelsPointing, int width, int height)
		: pixels(pixelsPointing,width*height), width(width), height(height), stride(width*sizeof(PixelType)), widthF(width-1), heightF(height-1)
	{

	}
	ReferenceTexture(const std::span<PixelType> pixelsToCopy, int width, int height)
		: pixels(pixelsToCopy), width(width), height(height), stride(width*sizeof(PixelType)), widthF(width-1), heightF(height-1)
	{

	}
	ReferenceTexture(std::span<PixelType>&& mov, int width, int height)
		: pixels(std::move(mov)), width(width), height(height), stride(width*sizeof(PixelType)), widthF(width-1), heightF(height-1)
	{

	}

	int getWidth() const override {
		return width;
	}
	float getWidthF() const override {
		return widthF;
	}
	int getHeight() const override {
		return height;
	}
	float getHeightF() const override {
		return heightF;
	}
	int getStride() const override {
		return stride;
	}
	void getPixel(const glm::ivec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override {
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
	void setPixel(const glm::ivec2& pos, const glm::fvec4& colourKernel) override {
		pixels[ (pos.y * width) + pos.x ].fromKernel(colourKernel);
	}
	void setPixelDithered(const glm::ivec2& pos, const glm::fvec4& colourKernel) override {
		pixels[ (pos.y * width) + pos.x ].fromKernelDithered(colourKernel,pos);
	}
	void* getRawPixels() override {
		return pixels.data();
	}
	const void* getRawPixels() const override {
		return pixels.data();
	}
	void clearToColour(const glm::fvec4& colourKernel) override {
		PixelType p;
		p.fromKernel(colourKernel);
		for(int y = 0; y < height; ++y) {
			PixelType* const scanline = &pixels[y*width];
			for(int x = 0; x < width; ++x) {
				scanline[x] = p;
			}
		}
	}
	void clearToColour(const ColourProgrammer& program) override {
		for(int y = 0; y < height; ++y) {
			PixelType* const scanline = &pixels[y*width];
			for(int x = 0; x < width; ++x) {
				glm::ivec2 pos = glm::ivec2(x,y);
				scanline[x].fromKernelDithered(program(pos),pos);
			}
		}
	}
	void clearToColour(const ColourProgrammer2& program) override {
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
	void clearToColour(const ColourProgrammer3& program)
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
	void clearToColour(const ColourProgrammer4& program)
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
	void iterateOverPixels(const ColourIterator& program) const
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
	void iterateOverPixels(const ColourIterator2& program) const
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

#endif // STANDARDTEXTURE_HPP
