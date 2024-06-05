#include "MhStandardImage2D.hpp"

namespace MH33 {
namespace Image {

#ifdef MH33_IMG_LEAN_AND_MEAN
#include "MhStandardImage2D.ipp"
#endif

Image2D* createImage2D(const Frame& source, Format format)
{
	switch (format) {
		case MH33::Image::Format::R8U:
			return new ImgGreyscale_U8(reinterpret_cast<const MH33::Util::PixelGreyscale<uint8_t>*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::R8S:
			return new ImgGreyscale_S8(reinterpret_cast<const MH33::Util::PixelGreyscale<int8_t>*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::R16U:
			return new ImgGreyscale_U16(reinterpret_cast<const MH33::Util::PixelGreyscale<uint16_t>*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::R16S:
			return new ImgGreyscale_S16(reinterpret_cast<const MH33::Util::PixelGreyscale<int16_t>*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::R32U:
			return new ImgGreyscale_U32(reinterpret_cast<const MH33::Util::PixelGreyscale<uint32_t>*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::R32S:
			return new ImgGreyscale_U32(reinterpret_cast<const MH33::Util::PixelGreyscale<uint32_t>*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RG8U:
				return new ImgRG_U8(reinterpret_cast<const MH33::Util::PixelRG_U8*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RG8S:
				return new ImgRG_S8(reinterpret_cast<const MH33::Util::PixelRG_S8*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RG16U:
				return new ImgRG_U16(reinterpret_cast<const MH33::Util::PixelRG_U16*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RG16S:
				return new ImgRG_S16(reinterpret_cast<const MH33::Util::PixelRG_S16*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RG32U:
				return new ImgRG_U32(reinterpret_cast<const MH33::Util::PixelRG_U32*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RG32S:
				return new ImgRG_S32(reinterpret_cast<const MH33::Util::PixelRG_S32*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RGB8U:
				return new ImgRGB_U8(reinterpret_cast<const MH33::Util::PixelRGB_U8*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RGB8S:
				return new ImgRGB_S8(reinterpret_cast<const MH33::Util::PixelRGB_S8*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RGB16U:
				return new ImgRGB_U16(reinterpret_cast<const MH33::Util::PixelRGB_U16*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RGB16S:
				return new ImgRGB_S16(reinterpret_cast<const MH33::Util::PixelRGB_S16*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RGB32U:
				return new ImgRGB_U32(reinterpret_cast<const MH33::Util::PixelRGB_U32*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RGB32S:
				return new ImgRGB_S32(reinterpret_cast<const MH33::Util::PixelRGB_S32*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::BGR8U:
				return new ImgBGR_U8(reinterpret_cast<const MH33::Util::PixelBGR_U8*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::BGR8S:
				return new ImgBGR_S8(reinterpret_cast<const MH33::Util::PixelBGR_S8*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::BGR16U:
				return new ImgBGR_U16(reinterpret_cast<const MH33::Util::PixelBGR_U16*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::BGR16S:
				return new ImgBGR_S16(reinterpret_cast<const MH33::Util::PixelBGR_S16*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::BGR32U:
				return new ImgBGR_U32(reinterpret_cast<const MH33::Util::PixelBGR_U32*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::BGR32S:
				return new ImgBGR_S32(reinterpret_cast<const MH33::Util::PixelBGR_S32*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RGBA8U:
				return new ImgRGBA_U8(reinterpret_cast<const MH33::Util::PixelRGBA_U8*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RGBA8S:
				return new ImgRGBA_S8(reinterpret_cast<const MH33::Util::PixelRGBA_S8*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RGBA16U:
				return new ImgRGBA_U16(reinterpret_cast<const MH33::Util::PixelRGBA_U16*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RGBA16S:
				return new ImgRGBA_S16(reinterpret_cast<const MH33::Util::PixelRGBA_S16*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RGBA32U:
				return new ImgRGBA_U32(reinterpret_cast<const MH33::Util::PixelRGBA_U32*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RGBA32S:
				return new ImgRGBA_S32(reinterpret_cast<const MH33::Util::PixelRGBA_S32*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::BGRA8U:
				return new ImgBGRA_U8(reinterpret_cast<const MH33::Util::PixelBGRA_U8*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::BGRA8S:
				return new ImgBGRA_S8(reinterpret_cast<const MH33::Util::PixelBGRA_S8*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::BGRA16U:
				return new ImgBGRA_U16(reinterpret_cast<const MH33::Util::PixelBGRA_U16*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::BGRA16S:
				return new ImgBGRA_S16(reinterpret_cast<const MH33::Util::PixelBGRA_S16*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::BGRA32U:
				return new ImgBGRA_U32(reinterpret_cast<const MH33::Util::PixelBGRA_U32*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::BGRA32S:
				return new ImgBGRA_S32(reinterpret_cast<const MH33::Util::PixelBGRA_S32*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::ARGB8U:
				return new ImgARGB_U8(reinterpret_cast<const MH33::Util::PixelARGB_U8*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::ARGB8S:
				return new ImgARGB_S8(reinterpret_cast<const MH33::Util::PixelARGB_S8*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::ARGB16U:
				return new ImgARGB_U16(reinterpret_cast<const MH33::Util::PixelARGB_U16*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::ARGB16S:
				return new ImgARGB_S16(reinterpret_cast<const MH33::Util::PixelARGB_S16*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::ARGB32U:
				return new ImgARGB_U32(reinterpret_cast<const MH33::Util::PixelARGB_U32*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::ARGB32S:
				return new ImgARGB_S32(reinterpret_cast<const MH33::Util::PixelARGB_S32*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RGB565:
			return new ImgRGB565(reinterpret_cast<const MH33::Util::PixelRGB565*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RGB555:
			return new ImgRGB555(reinterpret_cast<const MH33::Util::PixelRGB555*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RGB444:
			return new ImgRGB444(reinterpret_cast<const MH33::Util::PixelRGB444*>(source.imageData.data()),source.width,source.height);
		case MH33::Image::Format::RGBA444:
			return new ImgRGBA4444(reinterpret_cast<const MH33::Util::PixelRGBA4444*>(source.imageData.data()),source.width,source.height);
		default: return nullptr;
	}
}

Image2D* createImage2D(Format format, unsigned width, unsigned height)
{
	switch (format) {
		case MH33::Image::Format::R8U:
			return new ImgGreyscale_U8(width,height);
		case MH33::Image::Format::R8S:
			return new ImgGreyscale_S8(width,height);
		case MH33::Image::Format::R16U:
			return new ImgGreyscale_U16(width,height);
		case MH33::Image::Format::R16S:
			return new ImgGreyscale_S16(width,height);
		case MH33::Image::Format::R32U:
			return new ImgGreyscale_U32(width,height);
		case MH33::Image::Format::R32S:
			return new ImgGreyscale_U32(width,height);
		case MH33::Image::Format::RG8U:
				return new ImgRG_U8(width,height);
		case MH33::Image::Format::RG8S:
				return new ImgRG_S8(width,height);
		case MH33::Image::Format::RG16U:
				return new ImgRG_U16(width,height);
		case MH33::Image::Format::RG16S:
				return new ImgRG_S16(width,height);
		case MH33::Image::Format::RG32U:
				return new ImgRG_U32(width,height);
		case MH33::Image::Format::RG32S:
				return new ImgRG_S32(width,height);
		case MH33::Image::Format::RGB8U:
				return new ImgRGB_U8(width,height);
		case MH33::Image::Format::RGB8S:
				return new ImgRGB_S8(width,height);
		case MH33::Image::Format::RGB16U:
				return new ImgRGB_U16(width,height);
		case MH33::Image::Format::RGB16S:
				return new ImgRGB_S16(width,height);
		case MH33::Image::Format::RGB32U:
				return new ImgRGB_U32(width,height);
		case MH33::Image::Format::RGB32S:
				return new ImgRGB_S32(width,height);
		case MH33::Image::Format::BGR8U:
				return new ImgBGR_U8(width,height);
		case MH33::Image::Format::BGR8S:
				return new ImgBGR_S8(width,height);
		case MH33::Image::Format::BGR16U:
				return new ImgBGR_U16(width,height);
		case MH33::Image::Format::BGR16S:
				return new ImgBGR_S16(width,height);
		case MH33::Image::Format::BGR32U:
				return new ImgBGR_U32(width,height);
		case MH33::Image::Format::BGR32S:
				return new ImgBGR_S32(width,height);
		case MH33::Image::Format::RGBA8U:
				return new ImgRGBA_U8(width,height);
		case MH33::Image::Format::RGBA8S:
				return new ImgRGBA_S8(width,height);
		case MH33::Image::Format::RGBA16U:
				return new ImgRGBA_U16(width,height);
		case MH33::Image::Format::RGBA16S:
				return new ImgRGBA_S16(width,height);
		case MH33::Image::Format::RGBA32U:
				return new ImgRGBA_U32(width,height);
		case MH33::Image::Format::RGBA32S:
				return new ImgRGBA_S32(width,height);
		case MH33::Image::Format::BGRA8U:
				return new ImgBGRA_U8(width,height);
		case MH33::Image::Format::BGRA8S:
				return new ImgBGRA_S8(width,height);
		case MH33::Image::Format::BGRA16U:
				return new ImgBGRA_U16(width,height);
		case MH33::Image::Format::BGRA16S:
				return new ImgBGRA_S16(width,height);
		case MH33::Image::Format::BGRA32U:
				return new ImgBGRA_U32(width,height);
		case MH33::Image::Format::BGRA32S:
				return new ImgBGRA_S32(width,height);
		case MH33::Image::Format::ARGB8U:
				return new ImgARGB_U8(width,height);
		case MH33::Image::Format::ARGB8S:
				return new ImgARGB_S8(width,height);
		case MH33::Image::Format::ARGB16U:
				return new ImgARGB_U16(width,height);
		case MH33::Image::Format::ARGB16S:
				return new ImgARGB_S16(width,height);
		case MH33::Image::Format::ARGB32U:
				return new ImgARGB_U32(width,height);
		case MH33::Image::Format::ARGB32S:
				return new ImgARGB_S32(width,height);
		case MH33::Image::Format::RGB565:
			return new ImgRGB565(width,height);
		case MH33::Image::Format::RGB555:
			return new ImgRGB555(width,height);
		case MH33::Image::Format::RGB444:
			return new ImgRGB444(width,height);
		case MH33::Image::Format::RGBA444:
			return new ImgRGBA4444(width,height);
		default: return nullptr;
	}
}

Image2D* createImageReference2D(void* pixels, Format format, unsigned width, unsigned height)
{
	switch (format) {
		case MH33::Image::Format::R8U:
			return new RefImgGreyscale_U8(static_cast<MH33::Util::PixelGreyscale<uint8_t>*>(pixels),width,height);
		case MH33::Image::Format::R8S:
			return new RefImgGreyscale_S8(static_cast<MH33::Util::PixelGreyscale<int8_t>*>(pixels),width,height);
		case MH33::Image::Format::R16U:
			return new RefImgGreyscale_U16(static_cast<MH33::Util::PixelGreyscale<uint16_t>*>(pixels),width,height);
		case MH33::Image::Format::R16S:
			return new RefImgGreyscale_S16(static_cast<MH33::Util::PixelGreyscale<int16_t>*>(pixels),width,height);
		case MH33::Image::Format::R32U:
			return new RefImgGreyscale_U32(static_cast<MH33::Util::PixelGreyscale<uint32_t>*>(pixels),width,height);
		case MH33::Image::Format::R32S:
			return new RefImgGreyscale_U32(static_cast<MH33::Util::PixelGreyscale<uint32_t>*>(pixels),width,height);
		case MH33::Image::Format::RG8U:
				return new RefImgRG_U8(static_cast<MH33::Util::PixelRG_U8*>(pixels),width,height);
		case MH33::Image::Format::RG8S:
				return new RefImgRG_S8(static_cast<MH33::Util::PixelRG_S8*>(pixels),width,height);
		case MH33::Image::Format::RG16U:
				return new RefImgRG_U16(static_cast<MH33::Util::PixelRG_U16*>(pixels),width,height);
		case MH33::Image::Format::RG16S:
				return new RefImgRG_S16(static_cast<MH33::Util::PixelRG_S16*>(pixels),width,height);
		case MH33::Image::Format::RG32U:
				return new RefImgRG_U32(static_cast<MH33::Util::PixelRG_U32*>(pixels),width,height);
		case MH33::Image::Format::RG32S:
				return new RefImgRG_S32(static_cast<MH33::Util::PixelRG_S32*>(pixels),width,height);
		case MH33::Image::Format::RGB8U:
				return new RefImgRGB_U8(static_cast<MH33::Util::PixelRGB_U8*>(pixels),width,height);
		case MH33::Image::Format::RGB8S:
				return new RefImgRGB_S8(static_cast<MH33::Util::PixelRGB_S8*>(pixels),width,height);
		case MH33::Image::Format::RGB16U:
				return new RefImgRGB_U16(static_cast<MH33::Util::PixelRGB_U16*>(pixels),width,height);
		case MH33::Image::Format::RGB16S:
				return new RefImgRGB_S16(static_cast<MH33::Util::PixelRGB_S16*>(pixels),width,height);
		case MH33::Image::Format::RGB32U:
				return new RefImgRGB_U32(static_cast<MH33::Util::PixelRGB_U32*>(pixels),width,height);
		case MH33::Image::Format::RGB32S:
				return new RefImgRGB_S32(static_cast<MH33::Util::PixelRGB_S32*>(pixels),width,height);
		case MH33::Image::Format::BGR8U:
				return new RefImgBGR_U8(static_cast<MH33::Util::PixelBGR_U8*>(pixels),width,height);
		case MH33::Image::Format::BGR8S:
				return new RefImgBGR_S8(static_cast<MH33::Util::PixelBGR_S8*>(pixels),width,height);
		case MH33::Image::Format::BGR16U:
				return new RefImgBGR_U16(static_cast<MH33::Util::PixelBGR_U16*>(pixels),width,height);
		case MH33::Image::Format::BGR16S:
				return new RefImgBGR_S16(static_cast<MH33::Util::PixelBGR_S16*>(pixels),width,height);
		case MH33::Image::Format::BGR32U:
				return new RefImgBGR_U32(static_cast<MH33::Util::PixelBGR_U32*>(pixels),width,height);
		case MH33::Image::Format::BGR32S:
				return new RefImgBGR_S32(static_cast<MH33::Util::PixelBGR_S32*>(pixels),width,height);
		case MH33::Image::Format::RGBA8U:
				return new RefImgRGBA_U8(static_cast<MH33::Util::PixelRGBA_U8*>(pixels),width,height);
		case MH33::Image::Format::RGBA8S:
				return new RefImgRGBA_S8(static_cast<MH33::Util::PixelRGBA_S8*>(pixels),width,height);
		case MH33::Image::Format::RGBA16U:
				return new RefImgRGBA_U16(static_cast<MH33::Util::PixelRGBA_U16*>(pixels),width,height);
		case MH33::Image::Format::RGBA16S:
				return new RefImgRGBA_S16(static_cast<MH33::Util::PixelRGBA_S16*>(pixels),width,height);
		case MH33::Image::Format::RGBA32U:
				return new RefImgRGBA_U32(static_cast<MH33::Util::PixelRGBA_U32*>(pixels),width,height);
		case MH33::Image::Format::RGBA32S:
				return new RefImgRGBA_S32(static_cast<MH33::Util::PixelRGBA_S32*>(pixels),width,height);
		case MH33::Image::Format::BGRA8U:
				return new RefImgBGRA_U8(static_cast<MH33::Util::PixelBGRA_U8*>(pixels),width,height);
		case MH33::Image::Format::BGRA8S:
				return new RefImgBGRA_S8(static_cast<MH33::Util::PixelBGRA_S8*>(pixels),width,height);
		case MH33::Image::Format::BGRA16U:
				return new RefImgBGRA_U16(static_cast<MH33::Util::PixelBGRA_U16*>(pixels),width,height);
		case MH33::Image::Format::BGRA16S:
				return new RefImgBGRA_S16(static_cast<MH33::Util::PixelBGRA_S16*>(pixels),width,height);
		case MH33::Image::Format::BGRA32U:
				return new RefImgBGRA_U32(static_cast<MH33::Util::PixelBGRA_U32*>(pixels),width,height);
		case MH33::Image::Format::BGRA32S:
				return new RefImgBGRA_S32(static_cast<MH33::Util::PixelBGRA_S32*>(pixels),width,height);
		case MH33::Image::Format::ARGB8U:
				return new RefImgARGB_U8(static_cast<MH33::Util::PixelARGB_U8*>(pixels),width,height);
		case MH33::Image::Format::ARGB8S:
				return new RefImgARGB_S8(static_cast<MH33::Util::PixelARGB_S8*>(pixels),width,height);
		case MH33::Image::Format::ARGB16U:
				return new RefImgARGB_U16(static_cast<MH33::Util::PixelARGB_U16*>(pixels),width,height);
		case MH33::Image::Format::ARGB16S:
				return new RefImgARGB_S16(static_cast<MH33::Util::PixelARGB_S16*>(pixels),width,height);
		case MH33::Image::Format::ARGB32U:
				return new RefImgARGB_U32(static_cast<MH33::Util::PixelARGB_U32*>(pixels),width,height);
		case MH33::Image::Format::ARGB32S:
				return new RefImgARGB_S32(static_cast<MH33::Util::PixelARGB_S32*>(pixels),width,height);
		case MH33::Image::Format::RGB565:
			return new RefImgRGB565(static_cast<MH33::Util::PixelRGB565*>(pixels),width,height);
		case MH33::Image::Format::RGB555:
			return new RefImgRGB555(static_cast<MH33::Util::PixelRGB555*>(pixels),width,height);
		case MH33::Image::Format::RGB444:
			return new RefImgRGB444(static_cast<MH33::Util::PixelRGB444*>(pixels),width,height);
		case MH33::Image::Format::RGBA444:
			return new RefImgRGBA4444(static_cast<MH33::Util::PixelRGBA4444*>(pixels),width,height);
		default: return nullptr;
	}
}

}
}
