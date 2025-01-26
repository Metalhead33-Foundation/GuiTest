#ifndef EUPHSTANDARDIMAGE_HPP
#define EUPHSTANDARDIMAGE_HPP
#include <Euphemy/Media/Image/EuphImage.hpp>
#include <Euphemy/Media/Image/EuphPixelFormat.hpp>
#include <Elvavena/Util/ElvSpanHelpers.hpp>
#include <vector>
#include <algorithm>

namespace Euph {
namespace Media {
namespace Image {

template <PixelConcept Pixel> struct ImageImplementationHelpers {
	typedef std::span<const Pixel> ReadOnlyPixelStorage;
	typedef std::span<Pixel> PixelStorage;
	static inline void getPixel(const ReadOnlyPixelStorage& pixels, unsigned width, unsigned height, const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap)
	{
		unsigned x,y;
		switch (wrap) {
		case Euph::Media::Image::REPEAT:
		{
			x = pos.x % width;
			y = pos.y % height;
			break;
		}
		case Euph::Media::Image::MIRRORED_REPEAT:
		{
			const unsigned doubleWidth = 2 * width;
			const unsigned doubleHeight = 2 * height;
			const unsigned modulo_double_x = pos.x % doubleWidth;
			const unsigned modulo_double_y = pos.y % doubleHeight;
			x = (modulo_double_x < width)
					? pos.x % width
					: (doubleWidth - 1 - (pos.x % width));
			y = (modulo_double_y < height)
					? pos.y % height
					: (doubleHeight - 1 - (pos.y % height));

			break;
		}
		case Euph::Media::Image::CLAMP_TO_BORDER: // Can't tell the difference between the two
		case Euph::Media::Image::CLAMP_TO_EDGE:
		{
			x = std::min(pos.x,width-1);
			y = std::min(pos.y,height-1);
			break;
		}
		}
		const Pixel& pxl = pixels[toLinearIndex(width,x,y)];
		pxl.toKernel (colourKernel);
	}
	static inline void setPixel(const PixelStorage& pixels, unsigned width, const glm::uvec2& pos, const glm::fvec4& colourKernel)
	{
		Pixel& pxl = pixels[toLinearIndex(width,pos.x,pos.y)];
		pxl.fromKernel(colourKernel);
	}
	static inline void setPixelDithered(const PixelStorage& pixels, unsigned width, const glm::uvec2& pos, const glm::fvec4& colourKernel)
	{
		Pixel& pxl = pixels[toLinearIndex(width,pos.x,pos.y)];
		pxl.fromKernelDithered(colourKernel,pos);
	}
	static inline void iterateOverPixels(const ReadOnlyPixelStorage& pixels, unsigned width, unsigned height, const ReadOnlyImage2D::ColourIterator& program)
	{
		Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program](const Pixel& pxl, const glm::uvec2& pos) {
			glm::fvec4 kernel;
			pxl.toKernel(kernel);
			program(pos,kernel);
		});
	}
	static inline void iterateOverPixels(const ReadOnlyPixelStorage& pixels, unsigned width, unsigned height, float widthR, float heightR, const ReadOnlyImage2D::ColourIterator2& program)
	{
		Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program,width,height,widthR,heightR](const Pixel& pxl, const glm::uvec2& pos) {
			glm::fvec4 kernel;
			pxl.toKernel(kernel);
			program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR),kernel);
		});
	}
	static inline void iterateOverPixels(const ReadOnlyPixelStorage& pixels, unsigned width, unsigned height, const ReadOnlyImage2D::ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions)
	{
		Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program](const Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 kernel;
				pxl.toKernel(kernel);
				program(pos,kernel);
			},offset,dimensions);
	}
	static inline void iterateOverPixels(const ReadOnlyPixelStorage& pixels, unsigned width, unsigned height, float widthR, float heightR, const ReadOnlyImage2D::ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions)
	{
		Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program,width,height,widthR,heightR](const Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 kernel;
				pxl.toKernel(kernel);
				program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR),kernel);
			},offset,dimensions);
	}
	static inline void clearToColour(const PixelStorage& pixels, unsigned width, unsigned height, const glm::fvec4& colourKernel, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&colourKernel](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernelDithered(colourKernel, pos);
			});
		} else {
			Pixel pxl;
			pxl.fromKernel(colourKernel);
			std::fill(std::begin(pixels),std::end(pixels),pxl);
		}
	}
	static inline void clearToColour(const PixelStorage& pixels, unsigned width, unsigned height, const Image2D::ColourProgrammer& program, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernelDithered(program(pos), pos);
			});
		} else {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernel(program(pos));
			});
		}
	}
	static inline void clearToColour(const PixelStorage& pixels, unsigned width, unsigned height, const Image2D::ColourProgrammer2& program, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program](Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 preExisting;
				pxl.toKernel(preExisting);
				pxl.fromKernelDithered(program(pos,preExisting), pos);
			});
		} else {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program](Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 preExisting;
				pxl.toKernel(preExisting);
				pxl.fromKernel(program(pos,preExisting));
			});
		}
	}
	static inline void clearToColour(const PixelStorage& pixels, unsigned width, unsigned height, float widthR, float heightR, const Image2D::ColourProgrammer3& program, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program,width,height,widthR,heightR](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernelDithered(program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR)), pos);
			});
		} else {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program,width,height,widthR,heightR](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernel(program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR)));
			});
		}
	}
	static inline void clearToColour(const PixelStorage& pixels,unsigned width, unsigned height, float widthR, float heightR, const Image2D::ColourProgrammer4& program, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program,width,height,widthR,heightR](Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 preExisting;
				pxl.toKernel(preExisting);
				pxl.fromKernelDithered(program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR),preExisting), pos);
			});
		} else {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program,width,height,widthR,heightR](Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 preExisting;
				pxl.toKernel(preExisting);
				pxl.fromKernel(program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR),preExisting));
			});
		}
	}
	static inline void clearToColour(const PixelStorage& pixels,unsigned width, unsigned height, const glm::fvec4& colourKernel, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&colourKernel](Pixel& pxl, const glm::uvec2& pos) {
					pxl.fromKernelDithered(colourKernel, pos);
				},offset,dimensions);
		} else {
			Pixel srcPxl;
			srcPxl.fromKernel(colourKernel);
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[srcPxl](Pixel& dstPxl, const glm::uvec2& pos) {
					dstPxl = srcPxl;
				},offset,dimensions);
		}
	}
	static inline void clearToColour(const PixelStorage& pixels, unsigned width, unsigned height, const Image2D::ColourProgrammer& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernelDithered(program(pos), pos);
			}, offset, dimensions);
		} else {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernel(program(pos));
			}, offset, dimensions);
		}
	}
	static inline void clearToColour(const PixelStorage& pixels, unsigned width, unsigned height, const Image2D::ColourProgrammer2& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program](Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 preExisting;
				pxl.toKernel(preExisting);
				pxl.fromKernelDithered(program(pos,preExisting), pos);
			}, offset, dimensions);
		} else {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program](Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 preExisting;
				pxl.toKernel(preExisting);
				pxl.fromKernel(program(pos,preExisting));
			}, offset, dimensions);
		}
	}
	static inline void clearToColour(const PixelStorage& pixels,unsigned width, unsigned height, float widthR, float heightR, const Image2D::ColourProgrammer3& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program,width,height,widthR,heightR](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernelDithered(program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR)), pos);
			}, offset, dimensions);
		} else {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program,width,height,widthR,heightR](Pixel& pxl, const glm::uvec2& pos) {
				pxl.fromKernel(program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR)));
			}, offset, dimensions);
		}
	}
	static inline void clearToColour(const PixelStorage& pixels,unsigned width, unsigned height, float widthR, float heightR, const Image2D::ColourProgrammer4& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither)
	{
		if(dither) {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program,width,height,widthR,heightR](Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 preExisting;
				pxl.toKernel(preExisting);
				pxl.fromKernelDithered(program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR),preExisting), pos);
			}, offset, dimensions);
		} else {
			Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program,width,height,widthR,heightR](Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 preExisting;
				pxl.toKernel(preExisting);
				pxl.fromKernel(program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR),preExisting));
			}, offset, dimensions);
		}
	}
};

template <PixelConcept Pixel> struct PalettedImageImplementationHelpers {
	typedef std::span<const Pixel> Palette;
	typedef std::span<const uint8_t> PixelStorage;
	static inline void getPixel(const Palette& palette, const PixelStorage& pixels, unsigned width, unsigned height,
								const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap)
	{
		unsigned x,y;
		switch (wrap) {
		case Euph::Media::Image::REPEAT:
		{
			x = pos.x % width;
			y = pos.y % height;
			break;
		}
		case Euph::Media::Image::MIRRORED_REPEAT:
		{
			const unsigned doubleWidth = 2 * width;
			const unsigned doubleHeight = 2 * height;
			const unsigned modulo_double_x = pos.x % doubleWidth;
			const unsigned modulo_double_y = pos.y % doubleHeight;
			x = (modulo_double_x < width)
					? pos.x % width
					: (doubleWidth - 1 - (pos.x % width));
			y = (modulo_double_y < height)
					? pos.y % height
					: (doubleHeight - 1 - (pos.y % height));

			break;
		}
		case Euph::Media::Image::CLAMP_TO_BORDER: // Can't tell the difference between the two
		case Euph::Media::Image::CLAMP_TO_EDGE:
		{
			x = std::min(pos.x,width-1);
			y = std::min(pos.y,height-1);
			break;
		}
		}
		const Pixel& pxl = palette[pixels[toLinearIndex(width,x,y)]];
		pxl.toKernel(colourKernel);
	}
	static inline void iterateOverPixels(const Palette& palette, const PixelStorage& pixels, unsigned width, unsigned height, const ReadOnlyImage2D::ColourIterator& program)
	{
		Elv::Util::span_wrappers<uint8_t>::over_2d_span(pixels,glm::uvec2(width,height),[&program,&palette](const uint8_t& pxl, const glm::uvec2& pos) {
			glm::fvec4 kernel;
			palette[pxl].toKernel(kernel);
			program(pos,kernel);
		});
	}
	static inline void iterateOverPixels(const Palette& palette, const PixelStorage& pixels, unsigned width, unsigned height, float widthR, float heightR, const ReadOnlyImage2D::ColourIterator2& program)
	{
		Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program,&palette,widthR,heightR](const Pixel& pxl, const glm::uvec2& pos) {
			glm::fvec4 kernel;
			palette[pxl].toKernel(kernel);
			program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR),kernel);
		});
	}
	static inline void iterateOverPixels(const Palette& palette, const PixelStorage& pixels, unsigned width, unsigned height,
										 const ReadOnlyImage2D::ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions)
	{
		Elv::Util::span_wrappers<uint8_t>::over_2d_span(pixels,glm::uvec2(width,height),[&program,&palette](const uint8_t& pxl, const glm::uvec2& pos) {
				glm::fvec4 kernel;
				palette[pxl].toKernel(kernel);
				program(pos,kernel);
			}, offset,dimensions);
	}
	static inline void iterateOverPixels(const Palette& palette, const PixelStorage& pixels, unsigned width, unsigned height, float widthR, float heightR, const ReadOnlyImage2D::ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions)
	{
		Elv::Util::span_wrappers<Pixel>::over_2d_span(pixels,glm::uvec2(width,height),[&program,&palette,widthR,heightR](const Pixel& pxl, const glm::uvec2& pos) {
				glm::fvec4 kernel;
				palette[pxl].toKernel(kernel);
				program(glm::fvec2(static_cast<float>(pos.x) * widthR,static_cast<float>(pos.y) * heightR),kernel);
			}, offset,dimensions);
	}
};

template <PixelConcept Pixel> class ReadOnlyPalettedImage : public ReadOnlyImage2D {
public:
	typedef std::span<const Pixel> Palette;
private:
	Palette palette;
	std::span<const uint8_t> pixels;
public:
	Format getFormat() const override
	{
		return Format::INDEXED;
	}
	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override
	{
		PalettedImageImplementationHelpers<Pixel>::getPixel(palette,pixels,width,height,pos,colourKernel,wrap);
	}
	const void* getRawPixels() const override
	{
		return pixels.data();
	}
	void iterateOverPixels(const ColourIterator& program) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,program);
	}
	void iterateOverPixels(const ColourIterator2& program) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,widthR,heightR,program);
	}
	void iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,program,offset,dimensions);
	}
	void iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,widthR,heightR,program,offset,dimensions);
	}
	const Palette& getPalette() const { return palette; }
	Palette& getPalette() { return palette; }
	void setPalette(const Palette& nPalette) { palette = nPalette; }
	std::span<const uint8_t> getPixels() const { return pixels; }
	ReadOnlyPalettedImage(const Palette& palette, const std::span<const uint8_t>& pixels, unsigned width, unsigned height)
		: palette(palette), pixels(pixels) {
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	ReadOnlyPalettedImage(const ReadOnlyPalettedImage& cpy)
		: palette(cpy.palette), pixels(cpy.pixels) {
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
	}
	ReadOnlyPalettedImage& operator=(const ReadOnlyPalettedImage& cpy)
	{
		this->palette = cpy.palette;
		this->pixels = cpy.pixels;
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
		return *this;
	}
};
template <PixelConcept Pixel> class ReferencePalettedImage : public ReadOnlyImage2D {
public:
	typedef std::span<const Pixel> Palette;
private:
	Palette palette;
	std::span<uint8_t> pixels;
public:
	Format getFormat() const override
	{
		return Format::INDEXED;
	}
	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override
	{
		PalettedImageImplementationHelpers<Pixel>::getPixel(palette,pixels,width,height,pos,colourKernel,wrap);
	}
	const void* getRawPixels() const override
	{
		return pixels.data();
	}
	void iterateOverPixels(const ColourIterator& program) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,program);
	}
	void iterateOverPixels(const ColourIterator2& program) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,widthR,heightR,program);
	}
	void iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,program,offset,dimensions);
	}
	void iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,widthR,heightR,program,offset,dimensions);
	}
	const Palette& getPalette() const { return palette; }
	Palette& getPalette() { return palette; }
	void setPalette(const Palette& nPalette) { palette = nPalette; }
	std::span<uint8_t> getPixels() { return pixels; }
	std::span<const uint8_t> getPixels() const { return pixels; }
	ReferencePalettedImage(const Palette& palette, const std::span<uint8_t>& pixels, unsigned width, unsigned height)
		: palette(palette), pixels(pixels) {
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	ReferencePalettedImage(const ReferencePalettedImage& cpy)
		: palette(cpy.palette), pixels(cpy.pixels) {
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
	}
	ReferencePalettedImage& operator=(const ReferencePalettedImage& cpy)
	{
		this->palette = cpy.palette;
		this->pixels = cpy.pixels;
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
		return *this;
	}
};
template <PixelConcept Pixel> class PalettedImage : public ReadOnlyImage2D {
public:
	typedef std::span<const Pixel> Palette;
private:
	Palette palette;
	std::pmr::vector<uint8_t> pixels;
	std::pmr::memory_resource* memRes;
public:
	Format getFormat() const override
	{
		return Format::INDEXED;
	}
	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override
	{
		PalettedImageImplementationHelpers<Pixel>::getPixel(palette,pixels,width,height,pos,colourKernel,wrap);
	}
	const void* getRawPixels() const override
	{
		return pixels.data();
	}
	void iterateOverPixels(const ColourIterator& program) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,program);
	}
	void iterateOverPixels(const ColourIterator2& program) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,widthR,heightR,program);
	}
	void iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,program,offset,dimensions);
	}
	void iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,widthR,heightR,program,offset,dimensions);
	}
	const Palette& getPalette() const { return palette; }
	Palette& getPalette() { return palette; }
	void setPalette(const Palette& nPalette) { palette = nPalette; }
	std::pmr::vector<uint8_t>& getPixels() { return pixels; }
	const std::pmr::vector<uint8_t>& getPixels() const { return pixels; }
	std::pmr::memory_resource* getMemRes() const { return memRes; }
	PalettedImage(unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: pixels(memRes), memRes(memRes) {
		this->pixels.resize(width * height);
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	PalettedImage(const Palette& palette, unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: palette(palette), pixels(memRes), memRes(memRes) {
		this->pixels.resize(width * height);
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	PalettedImage(const Palette& palette, const std::span<const uint8_t>& pixels, unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: palette(palette), pixels(memRes), memRes(memRes) {
		this->pixels.resize(pixels.size());
		std::memcpy(this->pixels.data(),pixels.data(),pixels.size_bytes());
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	PalettedImage(const PalettedImage& cpy)
		: palette(cpy.palette), pixels(cpy.pixels), memRes(cpy.memRes) {
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
	}
	PalettedImage& operator=(const PalettedImage& cpy)
	{
		this->memRes = cpy.memRes;
		this->palette = cpy.palette;
		this->pixels = cpy.pixels;
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
		return *this;
	}
	PalettedImage(PalettedImage&& mov)
		: palette(mov.palette), pixels(std::move(mov.pixels)), memRes(mov.memRes) {
		this->width = mov.width;
		this->height = mov.height;
		recalculateDimensions();
	}
	PalettedImage& operator=(PalettedImage&& mov)
	{
		this->memRes = mov.memRes;
		this->palette = mov.palette;
		this->pixels = std::move(mov.pixels);
		this->width = mov.width;
		this->height = mov.height;
		recalculateDimensions();
		return *this;
	}
};
template <PixelConcept Pixel> class FullPalettedImage : public ReadOnlyImage2D {
public:
	typedef std::pmr::vector<Pixel> Palette;
private:
	Palette palette;
	std::pmr::vector<uint8_t> pixels;
	std::pmr::memory_resource* memRes;
public:
	Format getFormat() const override
	{
		return Format::INDEXED;
	}
	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override
	{
		PalettedImageImplementationHelpers<Pixel>::getPixel(palette,pixels,width,height,pos,colourKernel,wrap);
	}
	const void* getRawPixels() const override
	{
		return pixels.data();
	}
	void iterateOverPixels(const ColourIterator& program) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,program);
	}
	void iterateOverPixels(const ColourIterator2& program) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,widthR,heightR,program);
	}
	void iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,program,offset,dimensions);
	}
	void iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		PalettedImageImplementationHelpers<Pixel>::iterateOverPixels(palette,pixels,width,height,widthR,heightR,program,offset,dimensions);
	}
	const Palette& getPalette() const { return palette; }
	Palette& getPalette() { return palette; }
	void setPalette(const Palette& nPalette) { palette = nPalette; }
	std::pmr::vector<uint8_t>& getPixels() { return pixels; }
	const std::pmr::vector<uint8_t>& getPixels() const { return pixels; }
	std::pmr::memory_resource* getMemRes() const { return memRes; }
	FullPalettedImage(unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: pixels(memRes), memRes(memRes) {
		this->pixels.resize(width * height);
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	FullPalettedImage(const std::span<const Pixel>& palette, unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: palette(memRes), pixels(memRes), memRes(memRes) {
		this->palette.resize(palette.size());
		std::memcpy(this->palette.data(),palette.data(),palette.size_bytes());
		this->pixels.resize(width * height);
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	FullPalettedImage(const std::span<const Pixel>& palette, const std::span<const uint8_t>& pixels, unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: palette(memRes), pixels(memRes), memRes(memRes) {
		this->palette.resize(palette.size());
		std::memcpy(this->palette.data(),palette.data(),palette.size_bytes());
		this->pixels.resize(pixels.size());
		std::memcpy(this->pixels.data(),pixels.data(),pixels.size_bytes());
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	FullPalettedImage(const FullPalettedImage& cpy)
		: palette(cpy.palette), pixels(cpy.pixels), memRes(cpy.memRes) {
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
	}
	FullPalettedImage& operator=(const FullPalettedImage& cpy)
	{
		this->memRes = cpy.memRes;
		this->palette = cpy.palette;
		this->pixels = cpy.pixels;
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
		return *this;
	}
	FullPalettedImage(FullPalettedImage&& mov)
		: palette(std::move(mov.palette)), pixels(std::move(mov.pixels)), memRes(mov.memRes) {
		this->width = mov.width;
		this->height = mov.height;
		recalculateDimensions();
	}
	FullPalettedImage& operator=(FullPalettedImage&& mov)
	{
		this->memRes = mov.memRes;
		this->palette = std::move(mov.palette);
		this->pixels = std::move(mov.pixels);
		this->width = mov.width;
		this->height = mov.height;
		recalculateDimensions();
		return *this;
	}
};

template <PixelConcept Pixel> class ReadOnlyReferenceImage : public ReadOnlyImage2D {
private:
	std::span<const Pixel> pixels;
public:
	Format getFormat() const override
	{
		return Pixel::FMT_ID;
	}
	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override
	{
		ImageImplementationHelpers<Pixel>::getPixel(pixels,width,height,pos,colourKernel,wrap);
	}
	const void* getRawPixels() const override
	{
		return pixels.data();
	}
	void iterateOverPixels(const ColourIterator& program) const override
	{
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels,width,height,program);
	}
	void iterateOverPixels(const ColourIterator2& program) const override
	{
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels,width,height,widthR,heightR,program);
	}
	void iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels,width,height,program,offset,dimensions);
	}
	void iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels,width,height,widthR,heightR,program,offset,dimensions);
	}
	ReadOnlyReferenceImage(std::span<const Pixel> pixels, unsigned width, unsigned height)
	: pixels(pixels) {
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	ReadOnlyReferenceImage(const ReadOnlyReferenceImage& cpy)
		: pixels(cpy.pixels) {
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
	}
	ReadOnlyReferenceImage& operator=(const ReadOnlyReferenceImage& cpy)
	{
		this->pixels = cpy.pixels;
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
		return *this;
	}
};

template <PixelConcept Pixel> class ReferenceImage : public Image2D {
private:
	std::span<Pixel> pixels;
public:
	Format getFormat() const override
	{
		return Pixel::FMT_ID;
	}
	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override
	{
		ImageImplementationHelpers<Pixel>::getPixel(pixels,width,height,pos,colourKernel,wrap);
	}
	void setPixel(const glm::uvec2& pos, const glm::fvec4& colourKernel) override
	{
		Pixel& pxl = pixels[toLinearIndex(width,pos.x,pos.y)];
		pxl.fromKernel(colourKernel);
	}
	void setPixelDithered(const glm::uvec2& pos, const glm::fvec4& colourKernel) override
	{
		Pixel& pxl = pixels[toLinearIndex(width,pos.x,pos.y)];
		pxl.fromKernelDithered(colourKernel,pos);
	}
	void* getRawPixels() override
	{
		return pixels.data();
	}
	const void* getRawPixels() const override
	{
		return pixels.data();
	}
	void iterateOverPixels(const ColourIterator& program) const override
	{
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels,width,height,program);
	}
	void iterateOverPixels(const ColourIterator2& program) const override
	{
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels,width,height,widthR,heightR,program);
	}
	void iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels,width,height,program,offset,dimensions);
	}
	void iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels,width,height,widthR,heightR,program,offset,dimensions);
	}
	void clearToColour(const glm::fvec4& colourKernel, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,colourKernel,dither);
	}
	void clearToColour(const ColourProgrammer& program, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,program,dither);
	}
	void clearToColour(const ColourProgrammer2& program, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,program,dither);
	}
	void clearToColour(const ColourProgrammer3& program, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,widthR,heightR,program,dither);
	}
	void clearToColour(const ColourProgrammer4& program, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,widthR,heightR,program,dither);
	}
	void clearToColour(const glm::fvec4& colourKernel, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,colourKernel,offset,dimensions,dither);
	}
	void clearToColour(const ColourProgrammer& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,program,offset,dimensions,dither);
	}
	void clearToColour(const ColourProgrammer2& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,program,offset,dimensions,dither);
	}
	void clearToColour(const ColourProgrammer3& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,widthR,heightR,program,offset,dimensions,dither);
	}
	void clearToColour(const ColourProgrammer4& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,widthR,heightR,program,offset,dimensions,dither);
	}
	ReferenceImage(std::span<Pixel> pixels, unsigned width, unsigned height)
		: pixels(pixels) {
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	ReferenceImage(const ReferenceImage& cpy)
		: pixels(cpy.pixels) {
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
	}
	ReferenceImage& operator=(const ReferenceImage& cpy)
	{
		this->pixels = cpy.pixels;
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
		return *this;
	}
};

template <PixelConcept Pixel> class StandardImage : public ResizeableImage2D {
private:
	std::pmr::vector<Pixel> pixels;
	std::pmr::memory_resource* memRes;
public:
	Format getFormat() const override
	{
		return Pixel::FMT_ID;
	}
	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const override
	{
		ImageImplementationHelpers<Pixel>::getPixel(pixels,width,height,pos,colourKernel,wrap);
	}
	void setPixel(const glm::uvec2& pos, const glm::fvec4& colourKernel) override
	{
		Pixel& pxl = pixels[toLinearIndex(width,pos.x,pos.y)];
		pxl.fromKernel(colourKernel);
	}
	void setPixelDithered(const glm::uvec2& pos, const glm::fvec4& colourKernel) override
	{
		Pixel& pxl = pixels[toLinearIndex(width,pos.x,pos.y)];
		pxl.fromKernelDithered(colourKernel,pos);
	}
	void* getRawPixels() override
	{
		return pixels.data();
	}
	const void* getRawPixels() const override
	{
		return pixels.data();
	}
	void iterateOverPixels(const ColourIterator& program) const override
	{
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels,width,height,program);
	}
	void iterateOverPixels(const ColourIterator2& program) const override
	{
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels,width,height,widthR,heightR,program);
	}
	void iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels,width,height,program,offset,dimensions);
	}
	void iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const override
	{
		ImageImplementationHelpers<Pixel>::iterateOverPixels(pixels,width,height,widthR,heightR,program,offset,dimensions);
	}
	void clearToColour(const glm::fvec4& colourKernel, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,colourKernel,dither);
	}
	void clearToColour(const ColourProgrammer& program, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,program,dither);
	}
	void clearToColour(const ColourProgrammer2& program, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,program,dither);
	}
	void clearToColour(const ColourProgrammer3& program, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,widthR,heightR,program,dither);
	}
	void clearToColour(const ColourProgrammer4& program, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,widthR,heightR,program,dither);
	}
	void clearToColour(const glm::fvec4& colourKernel, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,colourKernel,offset,dimensions,dither);
	}
	void clearToColour(const ColourProgrammer& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,program,offset,dimensions,dither);
	}
	void clearToColour(const ColourProgrammer2& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,program,offset,dimensions,dither);
	}
	void clearToColour(const ColourProgrammer3& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,widthR,heightR,program,offset,dimensions,dither);
	}
	void clearToColour(const ColourProgrammer4& program, const glm::uvec2& offset, const glm::uvec2& dimensions, bool dither) override
	{
		ImageImplementationHelpers<Pixel>::clearToColour(pixels,width,height,widthR,heightR,program,offset,dimensions,dither);
	}
	// ResizeableImage2D interface
	bool resize(unsigned newWidth, unsigned newHeight) override
	{
		std::pmr::vector<Pixel> newPixels(memRes);
		newPixels.resize(newWidth*newHeight);
		unsigned pixelsToCopy_X = std::min(width,newWidth);
		unsigned pixelsToCopy_Y = std::min(height,newHeight);
		for(unsigned y = 0; y < pixelsToCopy_Y; ++y) {
			std::memcpy(&newPixels[y*newWidth],&pixels[y*width],pixelsToCopy_X*pixelByteSize(Pixel::FMT_ID));
		}
		pixels = std::move(newPixels);
		width = newWidth;
		height = newHeight;
		recalculateDimensions();
		return true;
	}
	std::pmr::vector<Pixel>& getPixels() { return pixels; }
	const std::pmr::vector<Pixel>& getPixels() const { return pixels; }
	std::pmr::memory_resource* getMemRes() const { return memRes; }
	StandardImage(unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: pixels(memRes), memRes(memRes) {
		this->pixels.resize(height * width);
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	StandardImage(std::span<Pixel> pixels, unsigned width, unsigned height, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: pixels(memRes), memRes(memRes) {
		this->pixels.resize(pixels.size());
		std::memcpy(this->pixels.data(),pixels.data(),pixels.size_bytes());
		this->width = width;
		this->height = height;
		recalculateDimensions();
	}
	StandardImage(const StandardImage& cpy)
		: pixels(cpy.pixels) {
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
	}
	StandardImage& operator=(const StandardImage& cpy)
	{
		this->pixels = cpy.pixels;
		this->width = cpy.width;
		this->height = cpy.height;
		recalculateDimensions();
		return *this;
	}
	StandardImage(StandardImage&& mov)
		: pixels(std::move(mov.pixels)) {
		this->width = mov.width;
		this->height = mov.height;
		recalculateDimensions();
	}
	StandardImage& operator=(StandardImage&& mov)
	{
		this->pixels = std::move(mov.pixels);
		this->width = mov.width;
		this->height = mov.height;
		recalculateDimensions();
		return *this;
	}
};

}
}
}
#endif // EUPHSTANDARDIMAGE_HPP
