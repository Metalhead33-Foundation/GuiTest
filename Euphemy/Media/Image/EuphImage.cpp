#include "EuphImage.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <Elvavena/Util/ElvAllocatorBasic.hpp>

namespace Euph {
namespace Media {
namespace Image {

std::optional<AnyImage2D> fromDecodeTargetToImage(const DecodeTarget& source, std::pmr::memory_resource* memRes)
{
	if(source.getFrames().empty()) {
		return std::nullopt;
	}

	const Frame& frame = source.getFrames()[0];

#define MAKE_IMAGE(PixelT) \
	{ \
		Image2D<PixelT> img(frame.width, frame.height, memRes); \
		const size_t imageBytes = img.getPixels().size() * sizeof(PixelT); \
		const size_t copyBytes = std::min(frame.data.size(), imageBytes); \
		if(copyBytes > 0) { \
			std::memcpy(img.getPixels().data(), frame.data.data(), copyBytes); \
		} \
		return img; \
	}

#define MAKE_PALETTED(PixelT) \
	{ \
		auto imgPalette = Elv::Util::pmr_make_shared<std::array<PixelT, 256>>(memRes); \
		const size_t paletteBytes = sizeof(*imgPalette); \
		const size_t paletteCopyBytes = std::min(palette.data.size(), paletteBytes); \
		std::memset(imgPalette->data(), 0, paletteBytes); \
		if(paletteCopyBytes > 0) { \
			std::memcpy(imgPalette->data(), palette.data.data(), paletteCopyBytes); \
		} \
		PalettedImage2D<PixelT> img(frame.width, frame.height, imgPalette, palette.transparentColorIndex, memRes); \
		const size_t imageBytes = img.getPixels().size() * sizeof(uint8_t); \
		const size_t copyBytes = std::min(frame.data.size(), imageBytes); \
		if(copyBytes > 0) { \
			std::memcpy(img.getPixels().data(), frame.data.data(), copyBytes); \
		} \
		return img; \
	}

	switch(source.getFormat()) {
		case Format::R8U: MAKE_IMAGE(PixelGreyscale_U8)
		case Format::R16U: MAKE_IMAGE(PixelGreyscale_U16)
		case Format::R32U: MAKE_IMAGE(PixelGreyscale_U32)
		case Format::R8S: MAKE_IMAGE(PixelGreyscale_S8)
		case Format::R16S: MAKE_IMAGE(PixelGreyscale_S16)
		case Format::R32S: MAKE_IMAGE(PixelGreyscale_S32)
		case Format::R16F: MAKE_IMAGE(PixelGreyscale_F16)
		case Format::R32F: MAKE_IMAGE(PixelGreyscale_F32)
		case Format::R64F: MAKE_IMAGE(PixelGreyscale_F64)
		case Format::RG8U: MAKE_IMAGE(PixelRG_U8)
		case Format::RG16U: MAKE_IMAGE(PixelRG_U16)
		case Format::RG32U: MAKE_IMAGE(PixelRG_U32)
		case Format::RG8S: MAKE_IMAGE(PixelRG_S8)
		case Format::RG16S: MAKE_IMAGE(PixelRG_S16)
		case Format::RG32S: MAKE_IMAGE(PixelRG_S32)
		case Format::RG16F: MAKE_IMAGE(PixelRG_F16)
		case Format::RG32F: MAKE_IMAGE(PixelRG_F32)
		case Format::RG64F: MAKE_IMAGE(PixelRG_F64)
		case Format::RGB8U: MAKE_IMAGE(PixelRGB_U8)
		case Format::RGB16U: MAKE_IMAGE(PixelRGB_U16)
		case Format::RGB32U: MAKE_IMAGE(PixelRGB_U32)
		case Format::RGB8S: MAKE_IMAGE(PixelRGB_S8)
		case Format::RGB16S: MAKE_IMAGE(PixelRGB_S16)
		case Format::RGB32S: MAKE_IMAGE(PixelRGB_S32)
		case Format::RGB16F: MAKE_IMAGE(PixelRGB_F16)
		case Format::RGB32F: MAKE_IMAGE(PixelRGB_F32)
		case Format::RGB64F: MAKE_IMAGE(PixelRGB_F64)
		case Format::BGR8U: MAKE_IMAGE(PixelBGR_U8)
		case Format::BGR16U: MAKE_IMAGE(PixelBGR_U16)
		case Format::BGR32U: MAKE_IMAGE(PixelBGR_U32)
		case Format::BGR8S: MAKE_IMAGE(PixelBGR_S8)
		case Format::BGR16S: MAKE_IMAGE(PixelBGR_S16)
		case Format::BGR32S: MAKE_IMAGE(PixelBGR_S32)
		case Format::BGR16F: MAKE_IMAGE(PixelBGR_F16)
		case Format::BGR32F: MAKE_IMAGE(PixelBGR_F32)
		case Format::BGR64F: MAKE_IMAGE(PixelBGR_F64)
		case Format::RGBA8U: MAKE_IMAGE(PixelRGBA_U8)
		case Format::RGBA16U: MAKE_IMAGE(PixelRGBA_U16)
		case Format::RGBA32U: MAKE_IMAGE(PixelRGBA_U32)
		case Format::RGBA8S: MAKE_IMAGE(PixelRGBA_S8)
		case Format::RGBA16S: MAKE_IMAGE(PixelRGBA_S16)
		case Format::RGBA32S: MAKE_IMAGE(PixelRGBA_S32)
		case Format::RGBA16F: MAKE_IMAGE(PixelRGBA_F16)
		case Format::RGBA32F: MAKE_IMAGE(PixelRGBA_F32)
		case Format::RGBA64F: MAKE_IMAGE(PixelRGBA_F64)
		case Format::BGRA8U: MAKE_IMAGE(PixelBGRA_U8)
		case Format::BGRA16U: MAKE_IMAGE(PixelBGRA_U16)
		case Format::BGRA32U: MAKE_IMAGE(PixelBGRA_U32)
		case Format::BGRA8S: MAKE_IMAGE(PixelBGRA_S8)
		case Format::BGRA16S: MAKE_IMAGE(PixelBGRA_S16)
		case Format::BGRA32S: MAKE_IMAGE(PixelBGRA_S32)
		case Format::BGRA16F: MAKE_IMAGE(PixelBGRA_F16)
		case Format::BGRA32F: MAKE_IMAGE(PixelBGRA_F32)
		case Format::BGRA64F: MAKE_IMAGE(PixelBGRA_F64)
		case Format::ARGB8U: MAKE_IMAGE(PixelARGB_U8)
		case Format::ARGB16U: MAKE_IMAGE(PixelARGB_U16)
		case Format::ARGB32U: MAKE_IMAGE(PixelARGB_U32)
		case Format::ARGB8S: MAKE_IMAGE(PixelARGB_S8)
		case Format::ARGB16S: MAKE_IMAGE(PixelARGB_S16)
		case Format::ARGB32S: MAKE_IMAGE(PixelARGB_S32)
		case Format::ARGB16F: MAKE_IMAGE(PixelARGB_F16)
		case Format::ARGB32F: MAKE_IMAGE(PixelARGB_F32)
		case Format::ARGB64F: MAKE_IMAGE(PixelARGB_F64)
		case Format::RGB444: MAKE_IMAGE(PixelRGB444)
		case Format::RGB555: MAKE_IMAGE(PixelRGB555)
		case Format::RGB565: MAKE_IMAGE(PixelRGB565)
		case Format::INDEXED: {
			if(!source.getPalette().has_value()) {
				throw std::runtime_error("Expected a palette, has none!");
			}
			const auto& palette = source.getPalette().value();
			switch(palette.format) {
				case Format::RGB8U: MAKE_PALETTED(PixelRGB_U8)
				case Format::RGB16U: MAKE_PALETTED(PixelRGB_U16)
				case Format::RGB32U: MAKE_PALETTED(PixelRGB_U32)
				case Format::RGB8S: MAKE_PALETTED(PixelRGB_S8)
				case Format::RGB16S: MAKE_PALETTED(PixelRGB_S16)
				case Format::RGB32S: MAKE_PALETTED(PixelRGB_S32)
				case Format::RGB16F: MAKE_PALETTED(PixelRGB_F16)
				case Format::RGB32F: MAKE_PALETTED(PixelRGB_F32)
				case Format::RGB64F: MAKE_PALETTED(PixelRGB_F64)
				case Format::BGR8U: MAKE_PALETTED(PixelBGR_U8)
				case Format::BGR16U: MAKE_PALETTED(PixelBGR_U16)
				case Format::BGR32U: MAKE_PALETTED(PixelBGR_U32)
				case Format::BGR8S: MAKE_PALETTED(PixelBGR_S8)
				case Format::BGR16S: MAKE_PALETTED(PixelBGR_S16)
				case Format::BGR32S: MAKE_PALETTED(PixelBGR_S32)
				case Format::BGR16F: MAKE_PALETTED(PixelBGR_F16)
				case Format::BGR32F: MAKE_PALETTED(PixelBGR_F32)
				case Format::BGR64F: MAKE_PALETTED(PixelBGR_F64)
				case Format::RGBA8U: MAKE_PALETTED(PixelRGBA_U8)
				case Format::RGBA16U: MAKE_PALETTED(PixelRGBA_U16)
				case Format::RGBA32U: MAKE_PALETTED(PixelRGBA_U32)
				case Format::RGBA8S: MAKE_PALETTED(PixelRGBA_S8)
				case Format::RGBA16S: MAKE_PALETTED(PixelRGBA_S16)
				case Format::RGBA32S: MAKE_PALETTED(PixelRGBA_S32)
				case Format::RGBA16F: MAKE_PALETTED(PixelRGBA_F16)
				case Format::RGBA32F: MAKE_PALETTED(PixelRGBA_F32)
				case Format::RGBA64F: MAKE_PALETTED(PixelRGBA_F64)
				case Format::BGRA8U: MAKE_PALETTED(PixelBGRA_U8)
				case Format::BGRA16U: MAKE_PALETTED(PixelBGRA_U16)
				case Format::BGRA32U: MAKE_PALETTED(PixelBGRA_U32)
				case Format::BGRA8S: MAKE_PALETTED(PixelBGRA_S8)
				case Format::BGRA16S: MAKE_PALETTED(PixelBGRA_S16)
				case Format::BGRA32S: MAKE_PALETTED(PixelBGRA_S32)
				case Format::BGRA16F: MAKE_PALETTED(PixelBGRA_F16)
				case Format::BGRA32F: MAKE_PALETTED(PixelBGRA_F32)
				case Format::BGRA64F: MAKE_PALETTED(PixelBGRA_F64)
				case Format::ARGB8U: MAKE_PALETTED(PixelARGB_U8)
				case Format::ARGB16U: MAKE_PALETTED(PixelARGB_U16)
				case Format::ARGB32U: MAKE_PALETTED(PixelARGB_U32)
				case Format::ARGB8S: MAKE_PALETTED(PixelARGB_S8)
				case Format::ARGB16S: MAKE_PALETTED(PixelARGB_S16)
				case Format::ARGB32S: MAKE_PALETTED(PixelARGB_S32)
				case Format::ARGB16F: MAKE_PALETTED(PixelARGB_F16)
				case Format::ARGB32F: MAKE_PALETTED(PixelARGB_F32)
				case Format::ARGB64F: MAKE_PALETTED(PixelARGB_F64)
				case Format::RGB444: MAKE_PALETTED(PixelRGB444)
				case Format::RGB555: MAKE_PALETTED(PixelRGB555)
				case Format::RGB565: MAKE_PALETTED(PixelRGB565)
				default:
					return std::nullopt;
			}
		}
		default:
			return std::nullopt;
	}

#undef MAKE_IMAGE
#undef MAKE_PALETTED
}

}
}
}
