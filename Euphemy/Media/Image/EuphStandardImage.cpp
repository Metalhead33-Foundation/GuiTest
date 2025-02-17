#include "EuphStandardImage.hpp"
namespace Euph {
namespace Media {
namespace Image {

uIResizeableImage2D createUptrResizeableImage2D(const std::span<const std::byte>& bytes, Format format, unsigned width, unsigned height, std::pmr::memory_resource* memRes)
{
	switch (format) {
	case Format::R8U: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_U8>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U8>(bytes), width, height, memRes);
	case Format::R16U: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_U16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U16>(bytes), width, height, memRes);
	case Format::R32U: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_U32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U32>(bytes), width, height, memRes);
	case Format::R8S: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_S8>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S8>(bytes), width, height, memRes);
	case Format::R16S: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_S16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S16>(bytes), width, height, memRes);
	case Format::R32S: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_S32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S32>(bytes), width, height, memRes);
	case Format::R16F: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_F16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F16>(bytes), width, height, memRes);
	case Format::R32F: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_F32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F32>(bytes), width, height, memRes);
	case Format::R64F: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_F64>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F64>(bytes), width, height, memRes);
	case Format::RG8U: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_U8>>(memRes, Elv::Util::as_const_span<PixelRG_U8>(bytes), width, height, memRes);
	case Format::RG16U: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_U16>>(memRes, Elv::Util::as_const_span<PixelRG_U16>(bytes), width, height, memRes);
	case Format::RG32U: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_U32>>(memRes, Elv::Util::as_const_span<PixelRG_U32>(bytes), width, height, memRes);
	case Format::RG8S: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_S8>>(memRes, Elv::Util::as_const_span<PixelRG_S8>(bytes), width, height, memRes);
	case Format::RG16S: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_S16>>(memRes, Elv::Util::as_const_span<PixelRG_S16>(bytes), width, height, memRes);
	case Format::RG32S: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_S32>>(memRes, Elv::Util::as_const_span<PixelRG_S32>(bytes), width, height, memRes);
	case Format::RG16F: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_F16>>(memRes, Elv::Util::as_const_span<PixelRG_F16>(bytes), width, height, memRes);
	case Format::RG32F: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_F32>>(memRes, Elv::Util::as_const_span<PixelRG_F32>(bytes), width, height, memRes);
	case Format::RG64F: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_F64>>(memRes, Elv::Util::as_const_span<PixelRG_F64>(bytes), width, height, memRes);
	case Format::RGB8U: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_U8>>(memRes, Elv::Util::as_const_span<PixelRGB_U8>(bytes), width, height, memRes);
	case Format::RGB16U: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_U16>>(memRes, Elv::Util::as_const_span<PixelRGB_U16>(bytes), width, height, memRes);
	case Format::RGB32U: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_U32>>(memRes, Elv::Util::as_const_span<PixelRGB_U32>(bytes), width, height, memRes);
	case Format::RGB8S: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_S8>>(memRes, Elv::Util::as_const_span<PixelRGB_S8>(bytes), width, height, memRes);
	case Format::RGB16S: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_S16>>(memRes, Elv::Util::as_const_span<PixelRGB_S16>(bytes), width, height, memRes);
	case Format::RGB32S: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_S32>>(memRes, Elv::Util::as_const_span<PixelRGB_S32>(bytes), width, height, memRes);
	case Format::RGB16F: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_F16>>(memRes, Elv::Util::as_const_span<PixelRGB_F16>(bytes), width, height, memRes);
	case Format::RGB32F: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_F32>>(memRes, Elv::Util::as_const_span<PixelRGB_F32>(bytes), width, height, memRes);
	case Format::RGB64F: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_F64>>(memRes, Elv::Util::as_const_span<PixelRGB_F64>(bytes), width, height, memRes);
	case Format::BGR8U: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_U8>>(memRes, Elv::Util::as_const_span<PixelBGR_U8>(bytes), width, height, memRes);
	case Format::BGR16U: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_U16>>(memRes, Elv::Util::as_const_span<PixelBGR_U16>(bytes), width, height, memRes);
	case Format::BGR32U: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_U32>>(memRes, Elv::Util::as_const_span<PixelBGR_U32>(bytes), width, height, memRes);
	case Format::BGR8S: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_S8>>(memRes, Elv::Util::as_const_span<PixelBGR_S8>(bytes), width, height, memRes);
	case Format::BGR16S: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_S16>>(memRes, Elv::Util::as_const_span<PixelBGR_S16>(bytes), width, height, memRes);
	case Format::BGR32S: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_S32>>(memRes, Elv::Util::as_const_span<PixelBGR_S32>(bytes), width, height, memRes);
	case Format::BGR16F: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_F16>>(memRes, Elv::Util::as_const_span<PixelBGR_F16>(bytes), width, height, memRes);
	case Format::BGR32F: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_F32>>(memRes, Elv::Util::as_const_span<PixelBGR_F32>(bytes), width, height, memRes);
	case Format::BGR64F: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_F64>>(memRes, Elv::Util::as_const_span<PixelBGR_F64>(bytes), width, height, memRes);
	case Format::RGBA8U: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_U8>>(memRes, Elv::Util::as_const_span<PixelRGBA_U8>(bytes), width, height, memRes);
	case Format::RGBA16U: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_U16>>(memRes, Elv::Util::as_const_span<PixelRGBA_U16>(bytes), width, height, memRes);
	case Format::RGBA32U: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_U32>>(memRes, Elv::Util::as_const_span<PixelRGBA_U32>(bytes), width, height, memRes);
	case Format::RGBA8S: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_S8>>(memRes, Elv::Util::as_const_span<PixelRGBA_S8>(bytes), width, height, memRes);
	case Format::RGBA16S: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_S16>>(memRes, Elv::Util::as_const_span<PixelRGBA_S16>(bytes), width, height, memRes);
	case Format::RGBA32S: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_S32>>(memRes, Elv::Util::as_const_span<PixelRGBA_S32>(bytes), width, height, memRes);
	case Format::RGBA16F: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_F16>>(memRes, Elv::Util::as_const_span<PixelRGBA_F16>(bytes), width, height, memRes);
	case Format::RGBA32F: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_F32>>(memRes, Elv::Util::as_const_span<PixelRGBA_F32>(bytes), width, height, memRes);
	case Format::RGBA64F: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_F64>>(memRes, Elv::Util::as_const_span<PixelRGBA_F64>(bytes), width, height, memRes);
	case Format::BGRA8U: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_U8>>(memRes, Elv::Util::as_const_span<PixelBGRA_U8>(bytes), width, height, memRes);
	case Format::BGRA16U: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_U16>>(memRes, Elv::Util::as_const_span<PixelBGRA_U16>(bytes), width, height, memRes);
	case Format::BGRA32U: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_U32>>(memRes, Elv::Util::as_const_span<PixelBGRA_U32>(bytes), width, height, memRes);
	case Format::BGRA8S: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_S8>>(memRes, Elv::Util::as_const_span<PixelBGRA_S8>(bytes), width, height, memRes);
	case Format::BGRA16S: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_S16>>(memRes, Elv::Util::as_const_span<PixelBGRA_S16>(bytes), width, height, memRes);
	case Format::BGRA32S: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_S32>>(memRes, Elv::Util::as_const_span<PixelBGRA_S32>(bytes), width, height, memRes);
	case Format::BGRA16F: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_F16>>(memRes, Elv::Util::as_const_span<PixelBGRA_F16>(bytes), width, height, memRes);
	case Format::BGRA32F: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_F32>>(memRes, Elv::Util::as_const_span<PixelBGRA_F32>(bytes), width, height, memRes);
	case Format::BGRA64F: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_F64>>(memRes, Elv::Util::as_const_span<PixelBGRA_F64>(bytes), width, height, memRes);
	case Format::ARGB8U: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_U8>>(memRes, Elv::Util::as_const_span<PixelARGB_U8>(bytes), width, height, memRes);
	case Format::ARGB16U: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_U16>>(memRes, Elv::Util::as_const_span<PixelARGB_U16>(bytes), width, height, memRes);
	case Format::ARGB32U: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_U32>>(memRes, Elv::Util::as_const_span<PixelARGB_U32>(bytes), width, height, memRes);
	case Format::ARGB8S: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_S8>>(memRes, Elv::Util::as_const_span<PixelARGB_S8>(bytes), width, height, memRes);
	case Format::ARGB16S: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_S16>>(memRes, Elv::Util::as_const_span<PixelARGB_S16>(bytes), width, height, memRes);
	case Format::ARGB32S: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_S32>>(memRes, Elv::Util::as_const_span<PixelARGB_S32>(bytes), width, height, memRes);
	case Format::ARGB16F: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_F16>>(memRes, Elv::Util::as_const_span<PixelARGB_F16>(bytes), width, height, memRes);
	case Format::ARGB32F: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_F32>>(memRes, Elv::Util::as_const_span<PixelARGB_F32>(bytes), width, height, memRes);
	case Format::ARGB64F: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_F64>>(memRes, Elv::Util::as_const_span<PixelARGB_F64>(bytes), width, height, memRes);
	case Format::RGB444: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB444>>(memRes, Elv::Util::as_const_span<PixelRGB444>(bytes), width, height, memRes);
	case Format::RGB555: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB555>>(memRes, Elv::Util::as_const_span<PixelRGB555>(bytes), width, height, memRes);
	case Format::RGB565: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB565>>(memRes, Elv::Util::as_const_span<PixelRGB565>(bytes), width, height, memRes);
	default:
		throw std::runtime_error("Invalid pixel type!");
	}
}

uIResizeableImage2D createUptrResizeableImage2D(const Frame& frame, Format format, std::pmr::memory_resource* memRes)
{
	return createUptrResizeableImage2D(frame.data,format,frame.width,frame.height,memRes);
}

uIResizeableImage2D createUptrResizeableImage2D(Format format, unsigned width, unsigned height, std::pmr::memory_resource* memRes)
{
	switch (format) {
	case Format::R8U: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_U8>>(memRes, width, height, memRes);
	case Format::R16U: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_U16>>(memRes, width, height, memRes);
	case Format::R32U: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_U32>>(memRes, width, height, memRes);
	case Format::R8S: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_S8>>(memRes, width, height, memRes);
	case Format::R16S: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_S16>>(memRes, width, height, memRes);
	case Format::R32S: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_S32>>(memRes, width, height, memRes);
	case Format::R16F: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_F16>>(memRes, width, height, memRes);
	case Format::R32F: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_F32>>(memRes, width, height, memRes);
	case Format::R64F: return Elv::Util::pmr_make_unique<StandardImage<PixelGreyscale_F64>>(memRes, width, height, memRes);
	case Format::RG8U: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_U8>>(memRes, width, height, memRes);
	case Format::RG16U: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_U16>>(memRes, width, height, memRes);
	case Format::RG32U: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_U32>>(memRes, width, height, memRes);
	case Format::RG8S: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_S8>>(memRes, width, height, memRes);
	case Format::RG16S: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_S16>>(memRes, width, height, memRes);
	case Format::RG32S: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_S32>>(memRes, width, height, memRes);
	case Format::RG16F: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_F16>>(memRes, width, height, memRes);
	case Format::RG32F: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_F32>>(memRes, width, height, memRes);
	case Format::RG64F: return Elv::Util::pmr_make_unique<StandardImage<PixelRG_F64>>(memRes, width, height, memRes);
	case Format::RGB8U: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_U8>>(memRes, width, height, memRes);
	case Format::RGB16U: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_U16>>(memRes, width, height, memRes);
	case Format::RGB32U: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_U32>>(memRes, width, height, memRes);
	case Format::RGB8S: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_S8>>(memRes, width, height, memRes);
	case Format::RGB16S: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_S16>>(memRes, width, height, memRes);
	case Format::RGB32S: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_S32>>(memRes, width, height, memRes);
	case Format::RGB16F: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_F16>>(memRes, width, height, memRes);
	case Format::RGB32F: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_F32>>(memRes, width, height, memRes);
	case Format::RGB64F: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB_F64>>(memRes, width, height, memRes);
	case Format::BGR8U: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_U8>>(memRes, width, height, memRes);
	case Format::BGR16U: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_U16>>(memRes, width, height, memRes);
	case Format::BGR32U: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_U32>>(memRes, width, height, memRes);
	case Format::BGR8S: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_S8>>(memRes, width, height, memRes);
	case Format::BGR16S: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_S16>>(memRes, width, height, memRes);
	case Format::BGR32S: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_S32>>(memRes, width, height, memRes);
	case Format::BGR16F: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_F16>>(memRes, width, height, memRes);
	case Format::BGR32F: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_F32>>(memRes, width, height, memRes);
	case Format::BGR64F: return Elv::Util::pmr_make_unique<StandardImage<PixelBGR_F64>>(memRes, width, height, memRes);
	case Format::RGBA8U: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_U8>>(memRes, width, height, memRes);
	case Format::RGBA16U: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_U16>>(memRes, width, height, memRes);
	case Format::RGBA32U: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_U32>>(memRes, width, height, memRes);
	case Format::RGBA8S: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_S8>>(memRes, width, height, memRes);
	case Format::RGBA16S: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_S16>>(memRes, width, height, memRes);
	case Format::RGBA32S: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_S32>>(memRes, width, height, memRes);
	case Format::RGBA16F: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_F16>>(memRes, width, height, memRes);
	case Format::RGBA32F: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_F32>>(memRes, width, height, memRes);
	case Format::RGBA64F: return Elv::Util::pmr_make_unique<StandardImage<PixelRGBA_F64>>(memRes, width, height, memRes);
	case Format::BGRA8U: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_U8>>(memRes, width, height, memRes);
	case Format::BGRA16U: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_U16>>(memRes, width, height, memRes);
	case Format::BGRA32U: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_U32>>(memRes, width, height, memRes);
	case Format::BGRA8S: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_S8>>(memRes, width, height, memRes);
	case Format::BGRA16S: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_S16>>(memRes, width, height, memRes);
	case Format::BGRA32S: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_S32>>(memRes, width, height, memRes);
	case Format::BGRA16F: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_F16>>(memRes, width, height, memRes);
	case Format::BGRA32F: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_F32>>(memRes, width, height, memRes);
	case Format::BGRA64F: return Elv::Util::pmr_make_unique<StandardImage<PixelBGRA_F64>>(memRes, width, height, memRes);
	case Format::ARGB8U: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_U8>>(memRes, width, height, memRes);
	case Format::ARGB16U: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_U16>>(memRes, width, height, memRes);
	case Format::ARGB32U: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_U32>>(memRes, width, height, memRes);
	case Format::ARGB8S: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_S8>>(memRes, width, height, memRes);
	case Format::ARGB16S: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_S16>>(memRes, width, height, memRes);
	case Format::ARGB32S: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_S32>>(memRes, width, height, memRes);
	case Format::ARGB16F: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_F16>>(memRes, width, height, memRes);
	case Format::ARGB32F: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_F32>>(memRes, width, height, memRes);
	case Format::ARGB64F: return Elv::Util::pmr_make_unique<StandardImage<PixelARGB_F64>>(memRes, width, height, memRes);
	case Format::RGB444: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB444>>(memRes, width, height, memRes);
	case Format::RGB555: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB555>>(memRes, width, height, memRes);
	case Format::RGB565: return Elv::Util::pmr_make_unique<StandardImage<PixelRGB565>>(memRes, width, height, memRes);
	default:
		throw std::runtime_error("Invalid pixel type!");
	}
}

sIResizeableImage2D createSptrResizeableImage2D(const std::span<const std::byte>& bytes, Format format, unsigned width, unsigned height, std::pmr::memory_resource* memRes)
{
	switch (format) {
	case Format::R8U: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_U8>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U8>(bytes), width, height, memRes);
	case Format::R16U: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_U16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U16>(bytes), width, height, memRes);
	case Format::R32U: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_U32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U32>(bytes), width, height, memRes);
	case Format::R8S: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_S8>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S8>(bytes), width, height, memRes);
	case Format::R16S: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_S16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S16>(bytes), width, height, memRes);
	case Format::R32S: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_S32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S32>(bytes), width, height, memRes);
	case Format::R16F: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_F16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F16>(bytes), width, height, memRes);
	case Format::R32F: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_F32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F32>(bytes), width, height, memRes);
	case Format::R64F: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_F64>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F64>(bytes), width, height, memRes);
	case Format::RG8U: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_U8>>(memRes, Elv::Util::as_const_span<PixelRG_U8>(bytes), width, height, memRes);
	case Format::RG16U: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_U16>>(memRes, Elv::Util::as_const_span<PixelRG_U16>(bytes), width, height, memRes);
	case Format::RG32U: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_U32>>(memRes, Elv::Util::as_const_span<PixelRG_U32>(bytes), width, height, memRes);
	case Format::RG8S: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_S8>>(memRes, Elv::Util::as_const_span<PixelRG_S8>(bytes), width, height, memRes);
	case Format::RG16S: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_S16>>(memRes, Elv::Util::as_const_span<PixelRG_S16>(bytes), width, height, memRes);
	case Format::RG32S: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_S32>>(memRes, Elv::Util::as_const_span<PixelRG_S32>(bytes), width, height, memRes);
	case Format::RG16F: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_F16>>(memRes, Elv::Util::as_const_span<PixelRG_F16>(bytes), width, height, memRes);
	case Format::RG32F: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_F32>>(memRes, Elv::Util::as_const_span<PixelRG_F32>(bytes), width, height, memRes);
	case Format::RG64F: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_F64>>(memRes, Elv::Util::as_const_span<PixelRG_F64>(bytes), width, height, memRes);
	case Format::RGB8U: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_U8>>(memRes, Elv::Util::as_const_span<PixelRGB_U8>(bytes), width, height, memRes);
	case Format::RGB16U: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_U16>>(memRes, Elv::Util::as_const_span<PixelRGB_U16>(bytes), width, height, memRes);
	case Format::RGB32U: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_U32>>(memRes, Elv::Util::as_const_span<PixelRGB_U32>(bytes), width, height, memRes);
	case Format::RGB8S: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_S8>>(memRes, Elv::Util::as_const_span<PixelRGB_S8>(bytes), width, height, memRes);
	case Format::RGB16S: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_S16>>(memRes, Elv::Util::as_const_span<PixelRGB_S16>(bytes), width, height, memRes);
	case Format::RGB32S: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_S32>>(memRes, Elv::Util::as_const_span<PixelRGB_S32>(bytes), width, height, memRes);
	case Format::RGB16F: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_F16>>(memRes, Elv::Util::as_const_span<PixelRGB_F16>(bytes), width, height, memRes);
	case Format::RGB32F: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_F32>>(memRes, Elv::Util::as_const_span<PixelRGB_F32>(bytes), width, height, memRes);
	case Format::RGB64F: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_F64>>(memRes, Elv::Util::as_const_span<PixelRGB_F64>(bytes), width, height, memRes);
	case Format::BGR8U: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_U8>>(memRes, Elv::Util::as_const_span<PixelBGR_U8>(bytes), width, height, memRes);
	case Format::BGR16U: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_U16>>(memRes, Elv::Util::as_const_span<PixelBGR_U16>(bytes), width, height, memRes);
	case Format::BGR32U: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_U32>>(memRes, Elv::Util::as_const_span<PixelBGR_U32>(bytes), width, height, memRes);
	case Format::BGR8S: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_S8>>(memRes, Elv::Util::as_const_span<PixelBGR_S8>(bytes), width, height, memRes);
	case Format::BGR16S: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_S16>>(memRes, Elv::Util::as_const_span<PixelBGR_S16>(bytes), width, height, memRes);
	case Format::BGR32S: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_S32>>(memRes, Elv::Util::as_const_span<PixelBGR_S32>(bytes), width, height, memRes);
	case Format::BGR16F: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_F16>>(memRes, Elv::Util::as_const_span<PixelBGR_F16>(bytes), width, height, memRes);
	case Format::BGR32F: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_F32>>(memRes, Elv::Util::as_const_span<PixelBGR_F32>(bytes), width, height, memRes);
	case Format::BGR64F: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_F64>>(memRes, Elv::Util::as_const_span<PixelBGR_F64>(bytes), width, height, memRes);
	case Format::RGBA8U: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_U8>>(memRes, Elv::Util::as_const_span<PixelRGBA_U8>(bytes), width, height, memRes);
	case Format::RGBA16U: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_U16>>(memRes, Elv::Util::as_const_span<PixelRGBA_U16>(bytes), width, height, memRes);
	case Format::RGBA32U: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_U32>>(memRes, Elv::Util::as_const_span<PixelRGBA_U32>(bytes), width, height, memRes);
	case Format::RGBA8S: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_S8>>(memRes, Elv::Util::as_const_span<PixelRGBA_S8>(bytes), width, height, memRes);
	case Format::RGBA16S: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_S16>>(memRes, Elv::Util::as_const_span<PixelRGBA_S16>(bytes), width, height, memRes);
	case Format::RGBA32S: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_S32>>(memRes, Elv::Util::as_const_span<PixelRGBA_S32>(bytes), width, height, memRes);
	case Format::RGBA16F: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_F16>>(memRes, Elv::Util::as_const_span<PixelRGBA_F16>(bytes), width, height, memRes);
	case Format::RGBA32F: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_F32>>(memRes, Elv::Util::as_const_span<PixelRGBA_F32>(bytes), width, height, memRes);
	case Format::RGBA64F: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_F64>>(memRes, Elv::Util::as_const_span<PixelRGBA_F64>(bytes), width, height, memRes);
	case Format::BGRA8U: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_U8>>(memRes, Elv::Util::as_const_span<PixelBGRA_U8>(bytes), width, height, memRes);
	case Format::BGRA16U: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_U16>>(memRes, Elv::Util::as_const_span<PixelBGRA_U16>(bytes), width, height, memRes);
	case Format::BGRA32U: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_U32>>(memRes, Elv::Util::as_const_span<PixelBGRA_U32>(bytes), width, height, memRes);
	case Format::BGRA8S: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_S8>>(memRes, Elv::Util::as_const_span<PixelBGRA_S8>(bytes), width, height, memRes);
	case Format::BGRA16S: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_S16>>(memRes, Elv::Util::as_const_span<PixelBGRA_S16>(bytes), width, height, memRes);
	case Format::BGRA32S: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_S32>>(memRes, Elv::Util::as_const_span<PixelBGRA_S32>(bytes), width, height, memRes);
	case Format::BGRA16F: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_F16>>(memRes, Elv::Util::as_const_span<PixelBGRA_F16>(bytes), width, height, memRes);
	case Format::BGRA32F: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_F32>>(memRes, Elv::Util::as_const_span<PixelBGRA_F32>(bytes), width, height, memRes);
	case Format::BGRA64F: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_F64>>(memRes, Elv::Util::as_const_span<PixelBGRA_F64>(bytes), width, height, memRes);
	case Format::ARGB8U: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_U8>>(memRes, Elv::Util::as_const_span<PixelARGB_U8>(bytes), width, height, memRes);
	case Format::ARGB16U: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_U16>>(memRes, Elv::Util::as_const_span<PixelARGB_U16>(bytes), width, height, memRes);
	case Format::ARGB32U: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_U32>>(memRes, Elv::Util::as_const_span<PixelARGB_U32>(bytes), width, height, memRes);
	case Format::ARGB8S: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_S8>>(memRes, Elv::Util::as_const_span<PixelARGB_S8>(bytes), width, height, memRes);
	case Format::ARGB16S: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_S16>>(memRes, Elv::Util::as_const_span<PixelARGB_S16>(bytes), width, height, memRes);
	case Format::ARGB32S: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_S32>>(memRes, Elv::Util::as_const_span<PixelARGB_S32>(bytes), width, height, memRes);
	case Format::ARGB16F: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_F16>>(memRes, Elv::Util::as_const_span<PixelARGB_F16>(bytes), width, height, memRes);
	case Format::ARGB32F: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_F32>>(memRes, Elv::Util::as_const_span<PixelARGB_F32>(bytes), width, height, memRes);
	case Format::ARGB64F: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_F64>>(memRes, Elv::Util::as_const_span<PixelARGB_F64>(bytes), width, height, memRes);
	case Format::RGB444: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB444>>(memRes, Elv::Util::as_const_span<PixelRGB444>(bytes), width, height, memRes);
	case Format::RGB555: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB555>>(memRes, Elv::Util::as_const_span<PixelRGB555>(bytes), width, height, memRes);
	case Format::RGB565: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB565>>(memRes, Elv::Util::as_const_span<PixelRGB565>(bytes), width, height, memRes);
	default:
		throw std::runtime_error("Invalid pixel type!");
	}
}

sIResizeableImage2D createSptrResizeableImage2D(const Frame& frame, Format format, std::pmr::memory_resource* memRes)
{
	return createSptrResizeableImage2D(frame.data,format,frame.width,frame.height,memRes);
}

sIResizeableImage2D createSptrResizeableImage2D(Format format, unsigned width, unsigned height, std::pmr::memory_resource* memRes)
{
	switch (format) {
	case Format::R8U: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_U8>>(memRes, width, height, memRes);
	case Format::R16U: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_U16>>(memRes, width, height, memRes);
	case Format::R32U: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_U32>>(memRes, width, height, memRes);
	case Format::R8S: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_S8>>(memRes, width, height, memRes);
	case Format::R16S: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_S16>>(memRes, width, height, memRes);
	case Format::R32S: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_S32>>(memRes, width, height, memRes);
	case Format::R16F: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_F16>>(memRes, width, height, memRes);
	case Format::R32F: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_F32>>(memRes, width, height, memRes);
	case Format::R64F: return Elv::Util::pmr_make_shared<StandardImage<PixelGreyscale_F64>>(memRes, width, height, memRes);
	case Format::RG8U: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_U8>>(memRes, width, height, memRes);
	case Format::RG16U: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_U16>>(memRes, width, height, memRes);
	case Format::RG32U: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_U32>>(memRes, width, height, memRes);
	case Format::RG8S: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_S8>>(memRes, width, height, memRes);
	case Format::RG16S: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_S16>>(memRes, width, height, memRes);
	case Format::RG32S: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_S32>>(memRes, width, height, memRes);
	case Format::RG16F: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_F16>>(memRes, width, height, memRes);
	case Format::RG32F: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_F32>>(memRes, width, height, memRes);
	case Format::RG64F: return Elv::Util::pmr_make_shared<StandardImage<PixelRG_F64>>(memRes, width, height, memRes);
	case Format::RGB8U: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_U8>>(memRes, width, height, memRes);
	case Format::RGB16U: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_U16>>(memRes, width, height, memRes);
	case Format::RGB32U: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_U32>>(memRes, width, height, memRes);
	case Format::RGB8S: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_S8>>(memRes, width, height, memRes);
	case Format::RGB16S: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_S16>>(memRes, width, height, memRes);
	case Format::RGB32S: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_S32>>(memRes, width, height, memRes);
	case Format::RGB16F: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_F16>>(memRes, width, height, memRes);
	case Format::RGB32F: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_F32>>(memRes, width, height, memRes);
	case Format::RGB64F: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB_F64>>(memRes, width, height, memRes);
	case Format::BGR8U: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_U8>>(memRes, width, height, memRes);
	case Format::BGR16U: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_U16>>(memRes, width, height, memRes);
	case Format::BGR32U: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_U32>>(memRes, width, height, memRes);
	case Format::BGR8S: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_S8>>(memRes, width, height, memRes);
	case Format::BGR16S: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_S16>>(memRes, width, height, memRes);
	case Format::BGR32S: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_S32>>(memRes, width, height, memRes);
	case Format::BGR16F: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_F16>>(memRes, width, height, memRes);
	case Format::BGR32F: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_F32>>(memRes, width, height, memRes);
	case Format::BGR64F: return Elv::Util::pmr_make_shared<StandardImage<PixelBGR_F64>>(memRes, width, height, memRes);
	case Format::RGBA8U: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_U8>>(memRes, width, height, memRes);
	case Format::RGBA16U: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_U16>>(memRes, width, height, memRes);
	case Format::RGBA32U: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_U32>>(memRes, width, height, memRes);
	case Format::RGBA8S: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_S8>>(memRes, width, height, memRes);
	case Format::RGBA16S: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_S16>>(memRes, width, height, memRes);
	case Format::RGBA32S: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_S32>>(memRes, width, height, memRes);
	case Format::RGBA16F: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_F16>>(memRes, width, height, memRes);
	case Format::RGBA32F: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_F32>>(memRes, width, height, memRes);
	case Format::RGBA64F: return Elv::Util::pmr_make_shared<StandardImage<PixelRGBA_F64>>(memRes, width, height, memRes);
	case Format::BGRA8U: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_U8>>(memRes, width, height, memRes);
	case Format::BGRA16U: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_U16>>(memRes, width, height, memRes);
	case Format::BGRA32U: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_U32>>(memRes, width, height, memRes);
	case Format::BGRA8S: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_S8>>(memRes, width, height, memRes);
	case Format::BGRA16S: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_S16>>(memRes, width, height, memRes);
	case Format::BGRA32S: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_S32>>(memRes, width, height, memRes);
	case Format::BGRA16F: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_F16>>(memRes, width, height, memRes);
	case Format::BGRA32F: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_F32>>(memRes, width, height, memRes);
	case Format::BGRA64F: return Elv::Util::pmr_make_shared<StandardImage<PixelBGRA_F64>>(memRes, width, height, memRes);
	case Format::ARGB8U: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_U8>>(memRes, width, height, memRes);
	case Format::ARGB16U: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_U16>>(memRes, width, height, memRes);
	case Format::ARGB32U: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_U32>>(memRes, width, height, memRes);
	case Format::ARGB8S: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_S8>>(memRes, width, height, memRes);
	case Format::ARGB16S: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_S16>>(memRes, width, height, memRes);
	case Format::ARGB32S: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_S32>>(memRes, width, height, memRes);
	case Format::ARGB16F: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_F16>>(memRes, width, height, memRes);
	case Format::ARGB32F: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_F32>>(memRes, width, height, memRes);
	case Format::ARGB64F: return Elv::Util::pmr_make_shared<StandardImage<PixelARGB_F64>>(memRes, width, height, memRes);
	case Format::RGB444: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB444>>(memRes, width, height, memRes);
	case Format::RGB555: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB555>>(memRes, width, height, memRes);
	case Format::RGB565: return Elv::Util::pmr_make_shared<StandardImage<PixelRGB565>>(memRes, width, height, memRes);
	default:
		throw std::runtime_error("Invalid pixel type!");
	}
}

uIMutableImage2D createUptrReferenceImage2D(std::span<std::byte> bytes, Format format, unsigned width, unsigned height, std::pmr::memory_resource* memRes)
{
	switch (format) {
	case Format::R8U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelGreyscale_U8>>(memRes, Elv::Util::as_span<PixelGreyscale_U8>(bytes), width, height);
	case Format::R16U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelGreyscale_U16>>(memRes, Elv::Util::as_span<PixelGreyscale_U16>(bytes), width, height);
	case Format::R32U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelGreyscale_U32>>(memRes, Elv::Util::as_span<PixelGreyscale_U32>(bytes), width, height);
	case Format::R8S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelGreyscale_S8>>(memRes, Elv::Util::as_span<PixelGreyscale_S8>(bytes), width, height);
	case Format::R16S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelGreyscale_S16>>(memRes, Elv::Util::as_span<PixelGreyscale_S16>(bytes), width, height);
	case Format::R32S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelGreyscale_S32>>(memRes, Elv::Util::as_span<PixelGreyscale_S32>(bytes), width, height);
	case Format::R16F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelGreyscale_F16>>(memRes, Elv::Util::as_span<PixelGreyscale_F16>(bytes), width, height);
	case Format::R32F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelGreyscale_F32>>(memRes, Elv::Util::as_span<PixelGreyscale_F32>(bytes), width, height);
	case Format::R64F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelGreyscale_F64>>(memRes, Elv::Util::as_span<PixelGreyscale_F64>(bytes), width, height);
	case Format::RG8U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRG_U8>>(memRes, Elv::Util::as_span<PixelRG_U8>(bytes), width, height);
	case Format::RG16U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRG_U16>>(memRes, Elv::Util::as_span<PixelRG_U16>(bytes), width, height);
	case Format::RG32U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRG_U32>>(memRes, Elv::Util::as_span<PixelRG_U32>(bytes), width, height);
	case Format::RG8S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRG_S8>>(memRes, Elv::Util::as_span<PixelRG_S8>(bytes), width, height);
	case Format::RG16S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRG_S16>>(memRes, Elv::Util::as_span<PixelRG_S16>(bytes), width, height);
	case Format::RG32S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRG_S32>>(memRes, Elv::Util::as_span<PixelRG_S32>(bytes), width, height);
	case Format::RG16F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRG_F16>>(memRes, Elv::Util::as_span<PixelRG_F16>(bytes), width, height);
	case Format::RG32F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRG_F32>>(memRes, Elv::Util::as_span<PixelRG_F32>(bytes), width, height);
	case Format::RG64F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRG_F64>>(memRes, Elv::Util::as_span<PixelRG_F64>(bytes), width, height);
	case Format::RGB8U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGB_U8>>(memRes, Elv::Util::as_span<PixelRGB_U8>(bytes), width, height);
	case Format::RGB16U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGB_U16>>(memRes, Elv::Util::as_span<PixelRGB_U16>(bytes), width, height);
	case Format::RGB32U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGB_U32>>(memRes, Elv::Util::as_span<PixelRGB_U32>(bytes), width, height);
	case Format::RGB8S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGB_S8>>(memRes, Elv::Util::as_span<PixelRGB_S8>(bytes), width, height);
	case Format::RGB16S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGB_S16>>(memRes, Elv::Util::as_span<PixelRGB_S16>(bytes), width, height);
	case Format::RGB32S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGB_S32>>(memRes, Elv::Util::as_span<PixelRGB_S32>(bytes), width, height);
	case Format::RGB16F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGB_F16>>(memRes, Elv::Util::as_span<PixelRGB_F16>(bytes), width, height);
	case Format::RGB32F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGB_F32>>(memRes, Elv::Util::as_span<PixelRGB_F32>(bytes), width, height);
	case Format::RGB64F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGB_F64>>(memRes, Elv::Util::as_span<PixelRGB_F64>(bytes), width, height);
	case Format::BGR8U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGR_U8>>(memRes, Elv::Util::as_span<PixelBGR_U8>(bytes), width, height);
	case Format::BGR16U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGR_U16>>(memRes, Elv::Util::as_span<PixelBGR_U16>(bytes), width, height);
	case Format::BGR32U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGR_U32>>(memRes, Elv::Util::as_span<PixelBGR_U32>(bytes), width, height);
	case Format::BGR8S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGR_S8>>(memRes, Elv::Util::as_span<PixelBGR_S8>(bytes), width, height);
	case Format::BGR16S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGR_S16>>(memRes, Elv::Util::as_span<PixelBGR_S16>(bytes), width, height);
	case Format::BGR32S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGR_S32>>(memRes, Elv::Util::as_span<PixelBGR_S32>(bytes), width, height);
	case Format::BGR16F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGR_F16>>(memRes, Elv::Util::as_span<PixelBGR_F16>(bytes), width, height);
	case Format::BGR32F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGR_F32>>(memRes, Elv::Util::as_span<PixelBGR_F32>(bytes), width, height);
	case Format::BGR64F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGR_F64>>(memRes, Elv::Util::as_span<PixelBGR_F64>(bytes), width, height);
	case Format::RGBA8U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGBA_U8>>(memRes, Elv::Util::as_span<PixelRGBA_U8>(bytes), width, height);
	case Format::RGBA16U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGBA_U16>>(memRes, Elv::Util::as_span<PixelRGBA_U16>(bytes), width, height);
	case Format::RGBA32U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGBA_U32>>(memRes, Elv::Util::as_span<PixelRGBA_U32>(bytes), width, height);
	case Format::RGBA8S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGBA_S8>>(memRes, Elv::Util::as_span<PixelRGBA_S8>(bytes), width, height);
	case Format::RGBA16S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGBA_S16>>(memRes, Elv::Util::as_span<PixelRGBA_S16>(bytes), width, height);
	case Format::RGBA32S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGBA_S32>>(memRes, Elv::Util::as_span<PixelRGBA_S32>(bytes), width, height);
	case Format::RGBA16F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGBA_F16>>(memRes, Elv::Util::as_span<PixelRGBA_F16>(bytes), width, height);
	case Format::RGBA32F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGBA_F32>>(memRes, Elv::Util::as_span<PixelRGBA_F32>(bytes), width, height);
	case Format::RGBA64F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGBA_F64>>(memRes, Elv::Util::as_span<PixelRGBA_F64>(bytes), width, height);
	case Format::BGRA8U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGRA_U8>>(memRes, Elv::Util::as_span<PixelBGRA_U8>(bytes), width, height);
	case Format::BGRA16U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGRA_U16>>(memRes, Elv::Util::as_span<PixelBGRA_U16>(bytes), width, height);
	case Format::BGRA32U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGRA_U32>>(memRes, Elv::Util::as_span<PixelBGRA_U32>(bytes), width, height);
	case Format::BGRA8S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGRA_S8>>(memRes, Elv::Util::as_span<PixelBGRA_S8>(bytes), width, height);
	case Format::BGRA16S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGRA_S16>>(memRes, Elv::Util::as_span<PixelBGRA_S16>(bytes), width, height);
	case Format::BGRA32S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGRA_S32>>(memRes, Elv::Util::as_span<PixelBGRA_S32>(bytes), width, height);
	case Format::BGRA16F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGRA_F16>>(memRes, Elv::Util::as_span<PixelBGRA_F16>(bytes), width, height);
	case Format::BGRA32F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGRA_F32>>(memRes, Elv::Util::as_span<PixelBGRA_F32>(bytes), width, height);
	case Format::BGRA64F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelBGRA_F64>>(memRes, Elv::Util::as_span<PixelBGRA_F64>(bytes), width, height);
	case Format::ARGB8U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelARGB_U8>>(memRes, Elv::Util::as_span<PixelARGB_U8>(bytes), width, height);
	case Format::ARGB16U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelARGB_U16>>(memRes, Elv::Util::as_span<PixelARGB_U16>(bytes), width, height);
	case Format::ARGB32U: return Elv::Util::pmr_make_unique<ReferenceImage<PixelARGB_U32>>(memRes, Elv::Util::as_span<PixelARGB_U32>(bytes), width, height);
	case Format::ARGB8S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelARGB_S8>>(memRes, Elv::Util::as_span<PixelARGB_S8>(bytes), width, height);
	case Format::ARGB16S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelARGB_S16>>(memRes, Elv::Util::as_span<PixelARGB_S16>(bytes), width, height);
	case Format::ARGB32S: return Elv::Util::pmr_make_unique<ReferenceImage<PixelARGB_S32>>(memRes, Elv::Util::as_span<PixelARGB_S32>(bytes), width, height);
	case Format::ARGB16F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelARGB_F16>>(memRes, Elv::Util::as_span<PixelARGB_F16>(bytes), width, height);
	case Format::ARGB32F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelARGB_F32>>(memRes, Elv::Util::as_span<PixelARGB_F32>(bytes), width, height);
	case Format::ARGB64F: return Elv::Util::pmr_make_unique<ReferenceImage<PixelARGB_F64>>(memRes, Elv::Util::as_span<PixelARGB_F64>(bytes), width, height);
	case Format::RGB444: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGB444>>(memRes, Elv::Util::as_span<PixelRGB444>(bytes), width, height);
	case Format::RGB555: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGB555>>(memRes, Elv::Util::as_span<PixelRGB555>(bytes), width, height);
	case Format::RGB565: return Elv::Util::pmr_make_unique<ReferenceImage<PixelRGB565>>(memRes, Elv::Util::as_span<PixelRGB565>(bytes), width, height);
	default:
		throw std::runtime_error("Invalid pixel type!");
	}
}

uIMutableImage2D createUptrReferenceImage2D(Frame& frame, Format format, std::pmr::memory_resource* memRes)
{
	return createUptrReferenceImage2D(frame.data, format, frame.width, frame.height, memRes);
}

sIMutableImage2D createSptrReferenceImage2D(std::span<std::byte> bytes, Format format, unsigned width, unsigned height, std::pmr::memory_resource* memRes)
{
	switch (format) {
	case Format::R8U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelGreyscale_U8>>(memRes, Elv::Util::as_span<PixelGreyscale_U8>(bytes), width, height);
	case Format::R16U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelGreyscale_U16>>(memRes, Elv::Util::as_span<PixelGreyscale_U16>(bytes), width, height);
	case Format::R32U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelGreyscale_U32>>(memRes, Elv::Util::as_span<PixelGreyscale_U32>(bytes), width, height);
	case Format::R8S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelGreyscale_S8>>(memRes, Elv::Util::as_span<PixelGreyscale_S8>(bytes), width, height);
	case Format::R16S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelGreyscale_S16>>(memRes, Elv::Util::as_span<PixelGreyscale_S16>(bytes), width, height);
	case Format::R32S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelGreyscale_S32>>(memRes, Elv::Util::as_span<PixelGreyscale_S32>(bytes), width, height);
	case Format::R16F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelGreyscale_F16>>(memRes, Elv::Util::as_span<PixelGreyscale_F16>(bytes), width, height);
	case Format::R32F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelGreyscale_F32>>(memRes, Elv::Util::as_span<PixelGreyscale_F32>(bytes), width, height);
	case Format::R64F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelGreyscale_F64>>(memRes, Elv::Util::as_span<PixelGreyscale_F64>(bytes), width, height);
	case Format::RG8U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRG_U8>>(memRes, Elv::Util::as_span<PixelRG_U8>(bytes), width, height);
	case Format::RG16U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRG_U16>>(memRes, Elv::Util::as_span<PixelRG_U16>(bytes), width, height);
	case Format::RG32U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRG_U32>>(memRes, Elv::Util::as_span<PixelRG_U32>(bytes), width, height);
	case Format::RG8S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRG_S8>>(memRes, Elv::Util::as_span<PixelRG_S8>(bytes), width, height);
	case Format::RG16S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRG_S16>>(memRes, Elv::Util::as_span<PixelRG_S16>(bytes), width, height);
	case Format::RG32S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRG_S32>>(memRes, Elv::Util::as_span<PixelRG_S32>(bytes), width, height);
	case Format::RG16F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRG_F16>>(memRes, Elv::Util::as_span<PixelRG_F16>(bytes), width, height);
	case Format::RG32F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRG_F32>>(memRes, Elv::Util::as_span<PixelRG_F32>(bytes), width, height);
	case Format::RG64F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRG_F64>>(memRes, Elv::Util::as_span<PixelRG_F64>(bytes), width, height);
	case Format::RGB8U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGB_U8>>(memRes, Elv::Util::as_span<PixelRGB_U8>(bytes), width, height);
	case Format::RGB16U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGB_U16>>(memRes, Elv::Util::as_span<PixelRGB_U16>(bytes), width, height);
	case Format::RGB32U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGB_U32>>(memRes, Elv::Util::as_span<PixelRGB_U32>(bytes), width, height);
	case Format::RGB8S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGB_S8>>(memRes, Elv::Util::as_span<PixelRGB_S8>(bytes), width, height);
	case Format::RGB16S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGB_S16>>(memRes, Elv::Util::as_span<PixelRGB_S16>(bytes), width, height);
	case Format::RGB32S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGB_S32>>(memRes, Elv::Util::as_span<PixelRGB_S32>(bytes), width, height);
	case Format::RGB16F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGB_F16>>(memRes, Elv::Util::as_span<PixelRGB_F16>(bytes), width, height);
	case Format::RGB32F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGB_F32>>(memRes, Elv::Util::as_span<PixelRGB_F32>(bytes), width, height);
	case Format::RGB64F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGB_F64>>(memRes, Elv::Util::as_span<PixelRGB_F64>(bytes), width, height);
	case Format::BGR8U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGR_U8>>(memRes, Elv::Util::as_span<PixelBGR_U8>(bytes), width, height);
	case Format::BGR16U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGR_U16>>(memRes, Elv::Util::as_span<PixelBGR_U16>(bytes), width, height);
	case Format::BGR32U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGR_U32>>(memRes, Elv::Util::as_span<PixelBGR_U32>(bytes), width, height);
	case Format::BGR8S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGR_S8>>(memRes, Elv::Util::as_span<PixelBGR_S8>(bytes), width, height);
	case Format::BGR16S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGR_S16>>(memRes, Elv::Util::as_span<PixelBGR_S16>(bytes), width, height);
	case Format::BGR32S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGR_S32>>(memRes, Elv::Util::as_span<PixelBGR_S32>(bytes), width, height);
	case Format::BGR16F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGR_F16>>(memRes, Elv::Util::as_span<PixelBGR_F16>(bytes), width, height);
	case Format::BGR32F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGR_F32>>(memRes, Elv::Util::as_span<PixelBGR_F32>(bytes), width, height);
	case Format::BGR64F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGR_F64>>(memRes, Elv::Util::as_span<PixelBGR_F64>(bytes), width, height);
	case Format::RGBA8U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGBA_U8>>(memRes, Elv::Util::as_span<PixelRGBA_U8>(bytes), width, height);
	case Format::RGBA16U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGBA_U16>>(memRes, Elv::Util::as_span<PixelRGBA_U16>(bytes), width, height);
	case Format::RGBA32U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGBA_U32>>(memRes, Elv::Util::as_span<PixelRGBA_U32>(bytes), width, height);
	case Format::RGBA8S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGBA_S8>>(memRes, Elv::Util::as_span<PixelRGBA_S8>(bytes), width, height);
	case Format::RGBA16S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGBA_S16>>(memRes, Elv::Util::as_span<PixelRGBA_S16>(bytes), width, height);
	case Format::RGBA32S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGBA_S32>>(memRes, Elv::Util::as_span<PixelRGBA_S32>(bytes), width, height);
	case Format::RGBA16F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGBA_F16>>(memRes, Elv::Util::as_span<PixelRGBA_F16>(bytes), width, height);
	case Format::RGBA32F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGBA_F32>>(memRes, Elv::Util::as_span<PixelRGBA_F32>(bytes), width, height);
	case Format::RGBA64F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGBA_F64>>(memRes, Elv::Util::as_span<PixelRGBA_F64>(bytes), width, height);
	case Format::BGRA8U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGRA_U8>>(memRes, Elv::Util::as_span<PixelBGRA_U8>(bytes), width, height);
	case Format::BGRA16U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGRA_U16>>(memRes, Elv::Util::as_span<PixelBGRA_U16>(bytes), width, height);
	case Format::BGRA32U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGRA_U32>>(memRes, Elv::Util::as_span<PixelBGRA_U32>(bytes), width, height);
	case Format::BGRA8S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGRA_S8>>(memRes, Elv::Util::as_span<PixelBGRA_S8>(bytes), width, height);
	case Format::BGRA16S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGRA_S16>>(memRes, Elv::Util::as_span<PixelBGRA_S16>(bytes), width, height);
	case Format::BGRA32S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGRA_S32>>(memRes, Elv::Util::as_span<PixelBGRA_S32>(bytes), width, height);
	case Format::BGRA16F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGRA_F16>>(memRes, Elv::Util::as_span<PixelBGRA_F16>(bytes), width, height);
	case Format::BGRA32F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGRA_F32>>(memRes, Elv::Util::as_span<PixelBGRA_F32>(bytes), width, height);
	case Format::BGRA64F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelBGRA_F64>>(memRes, Elv::Util::as_span<PixelBGRA_F64>(bytes), width, height);
	case Format::ARGB8U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelARGB_U8>>(memRes, Elv::Util::as_span<PixelARGB_U8>(bytes), width, height);
	case Format::ARGB16U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelARGB_U16>>(memRes, Elv::Util::as_span<PixelARGB_U16>(bytes), width, height);
	case Format::ARGB32U: return Elv::Util::pmr_make_shared<ReferenceImage<PixelARGB_U32>>(memRes, Elv::Util::as_span<PixelARGB_U32>(bytes), width, height);
	case Format::ARGB8S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelARGB_S8>>(memRes, Elv::Util::as_span<PixelARGB_S8>(bytes), width, height);
	case Format::ARGB16S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelARGB_S16>>(memRes, Elv::Util::as_span<PixelARGB_S16>(bytes), width, height);
	case Format::ARGB32S: return Elv::Util::pmr_make_shared<ReferenceImage<PixelARGB_S32>>(memRes, Elv::Util::as_span<PixelARGB_S32>(bytes), width, height);
	case Format::ARGB16F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelARGB_F16>>(memRes, Elv::Util::as_span<PixelARGB_F16>(bytes), width, height);
	case Format::ARGB32F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelARGB_F32>>(memRes, Elv::Util::as_span<PixelARGB_F32>(bytes), width, height);
	case Format::ARGB64F: return Elv::Util::pmr_make_shared<ReferenceImage<PixelARGB_F64>>(memRes, Elv::Util::as_span<PixelARGB_F64>(bytes), width, height);
	case Format::RGB444: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGB444>>(memRes, Elv::Util::as_span<PixelRGB444>(bytes), width, height);
	case Format::RGB555: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGB555>>(memRes, Elv::Util::as_span<PixelRGB555>(bytes), width, height);
	case Format::RGB565: return Elv::Util::pmr_make_shared<ReferenceImage<PixelRGB565>>(memRes, Elv::Util::as_span<PixelRGB565>(bytes), width, height);
	default:
		throw std::runtime_error("Invalid pixel type!");
	}
}

sIMutableImage2D createSptrReferenceImage2D(Frame& frame, Format format, std::pmr::memory_resource* memRes)
{
	return createSptrReferenceImage2D(frame.data, format, frame.width, frame.height, memRes);
}

uIReadOnlyImage2D createUptrReadOnlyReferenceImage2D(const std::span<const std::byte>& bytes, Format format, unsigned width, unsigned height, std::pmr::memory_resource* memRes)
{
	switch (format) {
	case Format::R8U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelGreyscale_U8>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U8>(bytes), width, height);
	case Format::R16U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelGreyscale_U16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U16>(bytes), width, height);
	case Format::R32U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelGreyscale_U32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U32>(bytes), width, height);
	case Format::R8S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelGreyscale_S8>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S8>(bytes), width, height);
	case Format::R16S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelGreyscale_S16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S16>(bytes), width, height);
	case Format::R32S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelGreyscale_S32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S32>(bytes), width, height);
	case Format::R16F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelGreyscale_F16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F16>(bytes), width, height);
	case Format::R32F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelGreyscale_F32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F32>(bytes), width, height);
	case Format::R64F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelGreyscale_F64>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F64>(bytes), width, height);
	case Format::RG8U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRG_U8>>(memRes, Elv::Util::as_const_span<PixelRG_U8>(bytes), width, height);
	case Format::RG16U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRG_U16>>(memRes, Elv::Util::as_const_span<PixelRG_U16>(bytes), width, height);
	case Format::RG32U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRG_U32>>(memRes, Elv::Util::as_const_span<PixelRG_U32>(bytes), width, height);
	case Format::RG8S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRG_S8>>(memRes, Elv::Util::as_const_span<PixelRG_S8>(bytes), width, height);
	case Format::RG16S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRG_S16>>(memRes, Elv::Util::as_const_span<PixelRG_S16>(bytes), width, height);
	case Format::RG32S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRG_S32>>(memRes, Elv::Util::as_const_span<PixelRG_S32>(bytes), width, height);
	case Format::RG16F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRG_F16>>(memRes, Elv::Util::as_const_span<PixelRG_F16>(bytes), width, height);
	case Format::RG32F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRG_F32>>(memRes, Elv::Util::as_const_span<PixelRG_F32>(bytes), width, height);
	case Format::RG64F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRG_F64>>(memRes, Elv::Util::as_const_span<PixelRG_F64>(bytes), width, height);
	case Format::RGB8U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGB_U8>>(memRes, Elv::Util::as_const_span<PixelRGB_U8>(bytes), width, height);
	case Format::RGB16U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGB_U16>>(memRes, Elv::Util::as_const_span<PixelRGB_U16>(bytes), width, height);
	case Format::RGB32U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGB_U32>>(memRes, Elv::Util::as_const_span<PixelRGB_U32>(bytes), width, height);
	case Format::RGB8S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGB_S8>>(memRes, Elv::Util::as_const_span<PixelRGB_S8>(bytes), width, height);
	case Format::RGB16S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGB_S16>>(memRes, Elv::Util::as_const_span<PixelRGB_S16>(bytes), width, height);
	case Format::RGB32S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGB_S32>>(memRes, Elv::Util::as_const_span<PixelRGB_S32>(bytes), width, height);
	case Format::RGB16F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGB_F16>>(memRes, Elv::Util::as_const_span<PixelRGB_F16>(bytes), width, height);
	case Format::RGB32F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGB_F32>>(memRes, Elv::Util::as_const_span<PixelRGB_F32>(bytes), width, height);
	case Format::RGB64F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGB_F64>>(memRes, Elv::Util::as_const_span<PixelRGB_F64>(bytes), width, height);
	case Format::BGR8U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGR_U8>>(memRes, Elv::Util::as_const_span<PixelBGR_U8>(bytes), width, height);
	case Format::BGR16U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGR_U16>>(memRes, Elv::Util::as_const_span<PixelBGR_U16>(bytes), width, height);
	case Format::BGR32U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGR_U32>>(memRes, Elv::Util::as_const_span<PixelBGR_U32>(bytes), width, height);
	case Format::BGR8S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGR_S8>>(memRes, Elv::Util::as_const_span<PixelBGR_S8>(bytes), width, height);
	case Format::BGR16S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGR_S16>>(memRes, Elv::Util::as_const_span<PixelBGR_S16>(bytes), width, height);
	case Format::BGR32S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGR_S32>>(memRes, Elv::Util::as_const_span<PixelBGR_S32>(bytes), width, height);
	case Format::BGR16F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGR_F16>>(memRes, Elv::Util::as_const_span<PixelBGR_F16>(bytes), width, height);
	case Format::BGR32F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGR_F32>>(memRes, Elv::Util::as_const_span<PixelBGR_F32>(bytes), width, height);
	case Format::BGR64F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGR_F64>>(memRes, Elv::Util::as_const_span<PixelBGR_F64>(bytes), width, height);
	case Format::RGBA8U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGBA_U8>>(memRes, Elv::Util::as_const_span<PixelRGBA_U8>(bytes), width, height);
	case Format::RGBA16U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGBA_U16>>(memRes, Elv::Util::as_const_span<PixelRGBA_U16>(bytes), width, height);
	case Format::RGBA32U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGBA_U32>>(memRes, Elv::Util::as_const_span<PixelRGBA_U32>(bytes), width, height);
	case Format::RGBA8S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGBA_S8>>(memRes, Elv::Util::as_const_span<PixelRGBA_S8>(bytes), width, height);
	case Format::RGBA16S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGBA_S16>>(memRes, Elv::Util::as_const_span<PixelRGBA_S16>(bytes), width, height);
	case Format::RGBA32S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGBA_S32>>(memRes, Elv::Util::as_const_span<PixelRGBA_S32>(bytes), width, height);
	case Format::RGBA16F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGBA_F16>>(memRes, Elv::Util::as_const_span<PixelRGBA_F16>(bytes), width, height);
	case Format::RGBA32F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGBA_F32>>(memRes, Elv::Util::as_const_span<PixelRGBA_F32>(bytes), width, height);
	case Format::RGBA64F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGBA_F64>>(memRes, Elv::Util::as_const_span<PixelRGBA_F64>(bytes), width, height);
	case Format::BGRA8U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGRA_U8>>(memRes, Elv::Util::as_const_span<PixelBGRA_U8>(bytes), width, height);
	case Format::BGRA16U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGRA_U16>>(memRes, Elv::Util::as_const_span<PixelBGRA_U16>(bytes), width, height);
	case Format::BGRA32U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGRA_U32>>(memRes, Elv::Util::as_const_span<PixelBGRA_U32>(bytes), width, height);
	case Format::BGRA8S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGRA_S8>>(memRes, Elv::Util::as_const_span<PixelBGRA_S8>(bytes), width, height);
	case Format::BGRA16S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGRA_S16>>(memRes, Elv::Util::as_const_span<PixelBGRA_S16>(bytes), width, height);
	case Format::BGRA32S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGRA_S32>>(memRes, Elv::Util::as_const_span<PixelBGRA_S32>(bytes), width, height);
	case Format::BGRA16F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGRA_F16>>(memRes, Elv::Util::as_const_span<PixelBGRA_F16>(bytes), width, height);
	case Format::BGRA32F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGRA_F32>>(memRes, Elv::Util::as_const_span<PixelBGRA_F32>(bytes), width, height);
	case Format::BGRA64F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelBGRA_F64>>(memRes, Elv::Util::as_const_span<PixelBGRA_F64>(bytes), width, height);
	case Format::ARGB8U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelARGB_U8>>(memRes, Elv::Util::as_const_span<PixelARGB_U8>(bytes), width, height);
	case Format::ARGB16U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelARGB_U16>>(memRes, Elv::Util::as_const_span<PixelARGB_U16>(bytes), width, height);
	case Format::ARGB32U: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelARGB_U32>>(memRes, Elv::Util::as_const_span<PixelARGB_U32>(bytes), width, height);
	case Format::ARGB8S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelARGB_S8>>(memRes, Elv::Util::as_const_span<PixelARGB_S8>(bytes), width, height);
	case Format::ARGB16S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelARGB_S16>>(memRes, Elv::Util::as_const_span<PixelARGB_S16>(bytes), width, height);
	case Format::ARGB32S: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelARGB_S32>>(memRes, Elv::Util::as_const_span<PixelARGB_S32>(bytes), width, height);
	case Format::ARGB16F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelARGB_F16>>(memRes, Elv::Util::as_const_span<PixelARGB_F16>(bytes), width, height);
	case Format::ARGB32F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelARGB_F32>>(memRes, Elv::Util::as_const_span<PixelARGB_F32>(bytes), width, height);
	case Format::ARGB64F: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelARGB_F64>>(memRes, Elv::Util::as_const_span<PixelARGB_F64>(bytes), width, height);
	case Format::RGB444: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGB444>>(memRes, Elv::Util::as_const_span<PixelRGB444>(bytes), width, height);
	case Format::RGB555: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGB555>>(memRes, Elv::Util::as_const_span<PixelRGB555>(bytes), width, height);
	case Format::RGB565: return Elv::Util::pmr_make_unique<ReadOnlyReferenceImage<PixelRGB565>>(memRes, Elv::Util::as_const_span<PixelRGB565>(bytes), width, height);
	default:
		throw std::runtime_error("Invalid pixel type!");
	}
}

uIReadOnlyImage2D createUptrRReadOnlyeferenceImage2D(const Frame& frame, Format format, std::pmr::memory_resource* memRes)
{
	return createUptrReadOnlyReferenceImage2D(frame.data, format, frame.width, frame.height, memRes);
}

sIReadOnlyImage2D createSptrReadOnlyReferenceImage2D(const std::span<const std::byte>& bytes, Format format, unsigned width, unsigned height, std::pmr::memory_resource* memRes)
{
	switch (format) {
	case Format::R8U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelGreyscale_U8>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U8>(bytes), width, height);
	case Format::R16U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelGreyscale_U16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U16>(bytes), width, height);
	case Format::R32U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelGreyscale_U32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U32>(bytes), width, height);
	case Format::R8S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelGreyscale_S8>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S8>(bytes), width, height);
	case Format::R16S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelGreyscale_S16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S16>(bytes), width, height);
	case Format::R32S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelGreyscale_S32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S32>(bytes), width, height);
	case Format::R16F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelGreyscale_F16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F16>(bytes), width, height);
	case Format::R32F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelGreyscale_F32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F32>(bytes), width, height);
	case Format::R64F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelGreyscale_F64>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F64>(bytes), width, height);
	case Format::RG8U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRG_U8>>(memRes, Elv::Util::as_const_span<PixelRG_U8>(bytes), width, height);
	case Format::RG16U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRG_U16>>(memRes, Elv::Util::as_const_span<PixelRG_U16>(bytes), width, height);
	case Format::RG32U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRG_U32>>(memRes, Elv::Util::as_const_span<PixelRG_U32>(bytes), width, height);
	case Format::RG8S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRG_S8>>(memRes, Elv::Util::as_const_span<PixelRG_S8>(bytes), width, height);
	case Format::RG16S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRG_S16>>(memRes, Elv::Util::as_const_span<PixelRG_S16>(bytes), width, height);
	case Format::RG32S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRG_S32>>(memRes, Elv::Util::as_const_span<PixelRG_S32>(bytes), width, height);
	case Format::RG16F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRG_F16>>(memRes, Elv::Util::as_const_span<PixelRG_F16>(bytes), width, height);
	case Format::RG32F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRG_F32>>(memRes, Elv::Util::as_const_span<PixelRG_F32>(bytes), width, height);
	case Format::RG64F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRG_F64>>(memRes, Elv::Util::as_const_span<PixelRG_F64>(bytes), width, height);
	case Format::RGB8U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGB_U8>>(memRes, Elv::Util::as_const_span<PixelRGB_U8>(bytes), width, height);
	case Format::RGB16U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGB_U16>>(memRes, Elv::Util::as_const_span<PixelRGB_U16>(bytes), width, height);
	case Format::RGB32U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGB_U32>>(memRes, Elv::Util::as_const_span<PixelRGB_U32>(bytes), width, height);
	case Format::RGB8S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGB_S8>>(memRes, Elv::Util::as_const_span<PixelRGB_S8>(bytes), width, height);
	case Format::RGB16S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGB_S16>>(memRes, Elv::Util::as_const_span<PixelRGB_S16>(bytes), width, height);
	case Format::RGB32S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGB_S32>>(memRes, Elv::Util::as_const_span<PixelRGB_S32>(bytes), width, height);
	case Format::RGB16F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGB_F16>>(memRes, Elv::Util::as_const_span<PixelRGB_F16>(bytes), width, height);
	case Format::RGB32F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGB_F32>>(memRes, Elv::Util::as_const_span<PixelRGB_F32>(bytes), width, height);
	case Format::RGB64F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGB_F64>>(memRes, Elv::Util::as_const_span<PixelRGB_F64>(bytes), width, height);
	case Format::BGR8U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGR_U8>>(memRes, Elv::Util::as_const_span<PixelBGR_U8>(bytes), width, height);
	case Format::BGR16U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGR_U16>>(memRes, Elv::Util::as_const_span<PixelBGR_U16>(bytes), width, height);
	case Format::BGR32U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGR_U32>>(memRes, Elv::Util::as_const_span<PixelBGR_U32>(bytes), width, height);
	case Format::BGR8S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGR_S8>>(memRes, Elv::Util::as_const_span<PixelBGR_S8>(bytes), width, height);
	case Format::BGR16S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGR_S16>>(memRes, Elv::Util::as_const_span<PixelBGR_S16>(bytes), width, height);
	case Format::BGR32S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGR_S32>>(memRes, Elv::Util::as_const_span<PixelBGR_S32>(bytes), width, height);
	case Format::BGR16F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGR_F16>>(memRes, Elv::Util::as_const_span<PixelBGR_F16>(bytes), width, height);
	case Format::BGR32F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGR_F32>>(memRes, Elv::Util::as_const_span<PixelBGR_F32>(bytes), width, height);
	case Format::BGR64F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGR_F64>>(memRes, Elv::Util::as_const_span<PixelBGR_F64>(bytes), width, height);
	case Format::RGBA8U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGBA_U8>>(memRes, Elv::Util::as_const_span<PixelRGBA_U8>(bytes), width, height);
	case Format::RGBA16U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGBA_U16>>(memRes, Elv::Util::as_const_span<PixelRGBA_U16>(bytes), width, height);
	case Format::RGBA32U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGBA_U32>>(memRes, Elv::Util::as_const_span<PixelRGBA_U32>(bytes), width, height);
	case Format::RGBA8S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGBA_S8>>(memRes, Elv::Util::as_const_span<PixelRGBA_S8>(bytes), width, height);
	case Format::RGBA16S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGBA_S16>>(memRes, Elv::Util::as_const_span<PixelRGBA_S16>(bytes), width, height);
	case Format::RGBA32S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGBA_S32>>(memRes, Elv::Util::as_const_span<PixelRGBA_S32>(bytes), width, height);
	case Format::RGBA16F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGBA_F16>>(memRes, Elv::Util::as_const_span<PixelRGBA_F16>(bytes), width, height);
	case Format::RGBA32F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGBA_F32>>(memRes, Elv::Util::as_const_span<PixelRGBA_F32>(bytes), width, height);
	case Format::RGBA64F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGBA_F64>>(memRes, Elv::Util::as_const_span<PixelRGBA_F64>(bytes), width, height);
	case Format::BGRA8U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGRA_U8>>(memRes, Elv::Util::as_const_span<PixelBGRA_U8>(bytes), width, height);
	case Format::BGRA16U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGRA_U16>>(memRes, Elv::Util::as_const_span<PixelBGRA_U16>(bytes), width, height);
	case Format::BGRA32U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGRA_U32>>(memRes, Elv::Util::as_const_span<PixelBGRA_U32>(bytes), width, height);
	case Format::BGRA8S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGRA_S8>>(memRes, Elv::Util::as_const_span<PixelBGRA_S8>(bytes), width, height);
	case Format::BGRA16S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGRA_S16>>(memRes, Elv::Util::as_const_span<PixelBGRA_S16>(bytes), width, height);
	case Format::BGRA32S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGRA_S32>>(memRes, Elv::Util::as_const_span<PixelBGRA_S32>(bytes), width, height);
	case Format::BGRA16F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGRA_F16>>(memRes, Elv::Util::as_const_span<PixelBGRA_F16>(bytes), width, height);
	case Format::BGRA32F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGRA_F32>>(memRes, Elv::Util::as_const_span<PixelBGRA_F32>(bytes), width, height);
	case Format::BGRA64F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelBGRA_F64>>(memRes, Elv::Util::as_const_span<PixelBGRA_F64>(bytes), width, height);
	case Format::ARGB8U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelARGB_U8>>(memRes, Elv::Util::as_const_span<PixelARGB_U8>(bytes), width, height);
	case Format::ARGB16U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelARGB_U16>>(memRes, Elv::Util::as_const_span<PixelARGB_U16>(bytes), width, height);
	case Format::ARGB32U: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelARGB_U32>>(memRes, Elv::Util::as_const_span<PixelARGB_U32>(bytes), width, height);
	case Format::ARGB8S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelARGB_S8>>(memRes, Elv::Util::as_const_span<PixelARGB_S8>(bytes), width, height);
	case Format::ARGB16S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelARGB_S16>>(memRes, Elv::Util::as_const_span<PixelARGB_S16>(bytes), width, height);
	case Format::ARGB32S: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelARGB_S32>>(memRes, Elv::Util::as_const_span<PixelARGB_S32>(bytes), width, height);
	case Format::ARGB16F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelARGB_F16>>(memRes, Elv::Util::as_const_span<PixelARGB_F16>(bytes), width, height);
	case Format::ARGB32F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelARGB_F32>>(memRes, Elv::Util::as_const_span<PixelARGB_F32>(bytes), width, height);
	case Format::ARGB64F: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelARGB_F64>>(memRes, Elv::Util::as_const_span<PixelARGB_F64>(bytes), width, height);
	case Format::RGB444: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGB444>>(memRes, Elv::Util::as_const_span<PixelRGB444>(bytes), width, height);
	case Format::RGB555: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGB555>>(memRes, Elv::Util::as_const_span<PixelRGB555>(bytes), width, height);
	case Format::RGB565: return Elv::Util::pmr_make_shared<ReadOnlyReferenceImage<PixelRGB565>>(memRes, Elv::Util::as_const_span<PixelRGB565>(bytes), width, height);
	default:
		throw std::runtime_error("Invalid pixel type!");
	}
}

sIReadOnlyImage2D createSptrReadOnlyReferenceImage2D(const Frame& frame, Format format, std::pmr::memory_resource* memRes)
{
	return createSptrReadOnlyReferenceImage2D(frame.data, format, frame.width, frame.height, memRes);
}

uIMutablePalette createUptrFullPalette(Format format, std::pmr::memory_resource* memRes)
{
	switch (format) {
		case Format::R8U: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_U8>>(memRes);
		case Format::R16U: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_U16>>(memRes);
		case Format::R32U: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_U32>>(memRes);
		case Format::R8S: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_S8>>(memRes);
		case Format::R16S: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_S16>>(memRes);
		case Format::R32S: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_S32>>(memRes);
		case Format::R16F: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_F16>>(memRes);
		case Format::R32F: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_F32>>(memRes);
		case Format::R64F: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_F64>>(memRes);
		case Format::RG8U: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_U8>>(memRes);
		case Format::RG16U: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_U16>>(memRes);
		case Format::RG32U: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_U32>>(memRes);
		case Format::RG8S: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_S8>>(memRes);
		case Format::RG16S: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_S16>>(memRes);
		case Format::RG32S: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_S32>>(memRes);
		case Format::RG16F: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_F16>>(memRes);
		case Format::RG32F: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_F32>>(memRes);
		case Format::RG64F: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_F64>>(memRes);
		case Format::RGB8U: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_U8>>(memRes);
		case Format::RGB16U: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_U16>>(memRes);
		case Format::RGB32U: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_U32>>(memRes);
		case Format::RGB8S: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_S8>>(memRes);
		case Format::RGB16S: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_S16>>(memRes);
		case Format::RGB32S: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_S32>>(memRes);
		case Format::RGB16F: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_F16>>(memRes);
		case Format::RGB32F: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_F32>>(memRes);
		case Format::RGB64F: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_F64>>(memRes);
		case Format::BGR8U: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_U8>>(memRes);
		case Format::BGR16U: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_U16>>(memRes);
		case Format::BGR32U: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_U32>>(memRes);
		case Format::BGR8S: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_S8>>(memRes);
		case Format::BGR16S: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_S16>>(memRes);
		case Format::BGR32S: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_S32>>(memRes);
		case Format::BGR16F: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_F16>>(memRes);
		case Format::BGR32F: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_F32>>(memRes);
		case Format::BGR64F: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_F64>>(memRes);
		case Format::RGBA8U: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_U8>>(memRes);
		case Format::RGBA16U: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_U16>>(memRes);
		case Format::RGBA32U: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_U32>>(memRes);
		case Format::RGBA8S: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_S8>>(memRes);
		case Format::RGBA16S: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_S16>>(memRes);
		case Format::RGBA32S: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_S32>>(memRes);
		case Format::RGBA16F: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_F16>>(memRes);
		case Format::RGBA32F: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_F32>>(memRes);
		case Format::RGBA64F: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_F64>>(memRes);
		case Format::BGRA8U: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_U8>>(memRes);
		case Format::BGRA16U: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_U16>>(memRes);
		case Format::BGRA32U: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_U32>>(memRes);
		case Format::BGRA8S: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_S8>>(memRes);
		case Format::BGRA16S: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_S16>>(memRes);
		case Format::BGRA32S: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_S32>>(memRes);
		case Format::BGRA16F: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_F16>>(memRes);
		case Format::BGRA32F: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_F32>>(memRes);
		case Format::BGRA64F: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_F64>>(memRes);
		case Format::ARGB8U: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_U8>>(memRes);
		case Format::ARGB16U: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_U16>>(memRes);
		case Format::ARGB32U: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_U32>>(memRes);
		case Format::ARGB8S: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_S8>>(memRes);
		case Format::ARGB16S: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_S16>>(memRes);
		case Format::ARGB32S: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_S32>>(memRes);
		case Format::ARGB16F: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_F16>>(memRes);
		case Format::ARGB32F: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_F32>>(memRes);
		case Format::ARGB64F: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_F64>>(memRes);
		case Format::RGB444: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB444>>(memRes);
		case Format::RGB555: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB555>>(memRes);
		case Format::RGB565: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB565>>(memRes);
		default:
			throw std::runtime_error("Invalid pixel type!");
	}
}

sIMutablePalette createSptrFullPalette(Format format, std::pmr::memory_resource* memRes)
{
	switch (format) {
		case Format::R8U: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_U8>>(memRes);
		case Format::R16U: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_U16>>(memRes);
		case Format::R32U: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_U32>>(memRes);
		case Format::R8S: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_S8>>(memRes);
		case Format::R16S: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_S16>>(memRes);
		case Format::R32S: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_S32>>(memRes);
		case Format::R16F: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_F16>>(memRes);
		case Format::R32F: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_F32>>(memRes);
		case Format::R64F: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_F64>>(memRes);
		case Format::RG8U: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_U8>>(memRes);
		case Format::RG16U: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_U16>>(memRes);
		case Format::RG32U: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_U32>>(memRes);
		case Format::RG8S: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_S8>>(memRes);
		case Format::RG16S: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_S16>>(memRes);
		case Format::RG32S: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_S32>>(memRes);
		case Format::RG16F: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_F16>>(memRes);
		case Format::RG32F: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_F32>>(memRes);
		case Format::RG64F: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_F64>>(memRes);
		case Format::RGB8U: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_U8>>(memRes);
		case Format::RGB16U: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_U16>>(memRes);
		case Format::RGB32U: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_U32>>(memRes);
		case Format::RGB8S: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_S8>>(memRes);
		case Format::RGB16S: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_S16>>(memRes);
		case Format::RGB32S: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_S32>>(memRes);
		case Format::RGB16F: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_F16>>(memRes);
		case Format::RGB32F: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_F32>>(memRes);
		case Format::RGB64F: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_F64>>(memRes);
		case Format::BGR8U: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_U8>>(memRes);
		case Format::BGR16U: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_U16>>(memRes);
		case Format::BGR32U: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_U32>>(memRes);
		case Format::BGR8S: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_S8>>(memRes);
		case Format::BGR16S: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_S16>>(memRes);
		case Format::BGR32S: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_S32>>(memRes);
		case Format::BGR16F: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_F16>>(memRes);
		case Format::BGR32F: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_F32>>(memRes);
		case Format::BGR64F: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_F64>>(memRes);
		case Format::RGBA8U: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_U8>>(memRes);
		case Format::RGBA16U: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_U16>>(memRes);
		case Format::RGBA32U: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_U32>>(memRes);
		case Format::RGBA8S: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_S8>>(memRes);
		case Format::RGBA16S: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_S16>>(memRes);
		case Format::RGBA32S: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_S32>>(memRes);
		case Format::RGBA16F: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_F16>>(memRes);
		case Format::RGBA32F: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_F32>>(memRes);
		case Format::RGBA64F: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_F64>>(memRes);
		case Format::BGRA8U: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_U8>>(memRes);
		case Format::BGRA16U: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_U16>>(memRes);
		case Format::BGRA32U: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_U32>>(memRes);
		case Format::BGRA8S: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_S8>>(memRes);
		case Format::BGRA16S: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_S16>>(memRes);
		case Format::BGRA32S: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_S32>>(memRes);
		case Format::BGRA16F: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_F16>>(memRes);
		case Format::BGRA32F: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_F32>>(memRes);
		case Format::BGRA64F: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_F64>>(memRes);
		case Format::ARGB8U: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_U8>>(memRes);
		case Format::ARGB16U: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_U16>>(memRes);
		case Format::ARGB32U: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_U32>>(memRes);
		case Format::ARGB8S: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_S8>>(memRes);
		case Format::ARGB16S: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_S16>>(memRes);
		case Format::ARGB32S: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_S32>>(memRes);
		case Format::ARGB16F: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_F16>>(memRes);
		case Format::ARGB32F: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_F32>>(memRes);
		case Format::ARGB64F: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_F64>>(memRes);
		case Format::RGB444: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB444>>(memRes);
		case Format::RGB555: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB555>>(memRes);
		case Format::RGB565: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB565>>(memRes);
		default:
			throw std::runtime_error("Invalid pixel type!");
	}
}

uIMutablePalette createUptrFullPalette(const std::span<const std::byte>& bytes, Format format, std::pmr::memory_resource* memRes)
{
	switch (format) {
		case Format::R8U: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_U8>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U8>(bytes));
		case Format::R16U: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_U16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U16>(bytes));
		case Format::R32U: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_U32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U32>(bytes));
		case Format::R8S: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_S8>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S8>(bytes));
		case Format::R16S: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_S16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S16>(bytes));
		case Format::R32S: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_S32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S32>(bytes));
		case Format::R16F: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_F16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F16>(bytes));
		case Format::R32F: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_F32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F32>(bytes));
		case Format::R64F: return Elv::Util::pmr_make_unique<FullPalette<PixelGreyscale_F64>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F64>(bytes));
		case Format::RG8U: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_U8>>(memRes, Elv::Util::as_const_span<PixelRG_U8>(bytes));
		case Format::RG16U: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_U16>>(memRes, Elv::Util::as_const_span<PixelRG_U16>(bytes));
		case Format::RG32U: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_U32>>(memRes, Elv::Util::as_const_span<PixelRG_U32>(bytes));
		case Format::RG8S: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_S8>>(memRes, Elv::Util::as_const_span<PixelRG_S8>(bytes));
		case Format::RG16S: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_S16>>(memRes, Elv::Util::as_const_span<PixelRG_S16>(bytes));
		case Format::RG32S: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_S32>>(memRes, Elv::Util::as_const_span<PixelRG_S32>(bytes));
		case Format::RG16F: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_F16>>(memRes, Elv::Util::as_const_span<PixelRG_F16>(bytes));
		case Format::RG32F: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_F32>>(memRes, Elv::Util::as_const_span<PixelRG_F32>(bytes));
		case Format::RG64F: return Elv::Util::pmr_make_unique<FullPalette<PixelRG_F64>>(memRes, Elv::Util::as_const_span<PixelRG_F64>(bytes));
		case Format::RGB8U: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_U8>>(memRes, Elv::Util::as_const_span<PixelRGB_U8>(bytes));
		case Format::RGB16U: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_U16>>(memRes, Elv::Util::as_const_span<PixelRGB_U16>(bytes));
		case Format::RGB32U: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_U32>>(memRes, Elv::Util::as_const_span<PixelRGB_U32>(bytes));
		case Format::RGB8S: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_S8>>(memRes, Elv::Util::as_const_span<PixelRGB_S8>(bytes));
		case Format::RGB16S: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_S16>>(memRes, Elv::Util::as_const_span<PixelRGB_S16>(bytes));
		case Format::RGB32S: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_S32>>(memRes, Elv::Util::as_const_span<PixelRGB_S32>(bytes));
		case Format::RGB16F: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_F16>>(memRes, Elv::Util::as_const_span<PixelRGB_F16>(bytes));
		case Format::RGB32F: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_F32>>(memRes, Elv::Util::as_const_span<PixelRGB_F32>(bytes));
		case Format::RGB64F: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB_F64>>(memRes, Elv::Util::as_const_span<PixelRGB_F64>(bytes));
		case Format::BGR8U: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_U8>>(memRes, Elv::Util::as_const_span<PixelBGR_U8>(bytes));
		case Format::BGR16U: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_U16>>(memRes, Elv::Util::as_const_span<PixelBGR_U16>(bytes));
		case Format::BGR32U: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_U32>>(memRes, Elv::Util::as_const_span<PixelBGR_U32>(bytes));
		case Format::BGR8S: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_S8>>(memRes, Elv::Util::as_const_span<PixelBGR_S8>(bytes));
		case Format::BGR16S: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_S16>>(memRes, Elv::Util::as_const_span<PixelBGR_S16>(bytes));
		case Format::BGR32S: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_S32>>(memRes, Elv::Util::as_const_span<PixelBGR_S32>(bytes));
		case Format::BGR16F: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_F16>>(memRes, Elv::Util::as_const_span<PixelBGR_F16>(bytes));
		case Format::BGR32F: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_F32>>(memRes, Elv::Util::as_const_span<PixelBGR_F32>(bytes));
		case Format::BGR64F: return Elv::Util::pmr_make_unique<FullPalette<PixelBGR_F64>>(memRes, Elv::Util::as_const_span<PixelBGR_F64>(bytes));
		case Format::RGBA8U: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_U8>>(memRes, Elv::Util::as_const_span<PixelRGBA_U8>(bytes));
		case Format::RGBA16U: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_U16>>(memRes, Elv::Util::as_const_span<PixelRGBA_U16>(bytes));
		case Format::RGBA32U: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_U32>>(memRes, Elv::Util::as_const_span<PixelRGBA_U32>(bytes));
		case Format::RGBA8S: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_S8>>(memRes, Elv::Util::as_const_span<PixelRGBA_S8>(bytes));
		case Format::RGBA16S: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_S16>>(memRes, Elv::Util::as_const_span<PixelRGBA_S16>(bytes));
		case Format::RGBA32S: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_S32>>(memRes, Elv::Util::as_const_span<PixelRGBA_S32>(bytes));
		case Format::RGBA16F: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_F16>>(memRes, Elv::Util::as_const_span<PixelRGBA_F16>(bytes));
		case Format::RGBA32F: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_F32>>(memRes, Elv::Util::as_const_span<PixelRGBA_F32>(bytes));
		case Format::RGBA64F: return Elv::Util::pmr_make_unique<FullPalette<PixelRGBA_F64>>(memRes, Elv::Util::as_const_span<PixelRGBA_F64>(bytes));
		case Format::BGRA8U: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_U8>>(memRes, Elv::Util::as_const_span<PixelBGRA_U8>(bytes));
		case Format::BGRA16U: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_U16>>(memRes, Elv::Util::as_const_span<PixelBGRA_U16>(bytes));
		case Format::BGRA32U: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_U32>>(memRes, Elv::Util::as_const_span<PixelBGRA_U32>(bytes));
		case Format::BGRA8S: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_S8>>(memRes, Elv::Util::as_const_span<PixelBGRA_S8>(bytes));
		case Format::BGRA16S: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_S16>>(memRes, Elv::Util::as_const_span<PixelBGRA_S16>(bytes));
		case Format::BGRA32S: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_S32>>(memRes, Elv::Util::as_const_span<PixelBGRA_S32>(bytes));
		case Format::BGRA16F: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_F16>>(memRes, Elv::Util::as_const_span<PixelBGRA_F16>(bytes));
		case Format::BGRA32F: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_F32>>(memRes, Elv::Util::as_const_span<PixelBGRA_F32>(bytes));
		case Format::BGRA64F: return Elv::Util::pmr_make_unique<FullPalette<PixelBGRA_F64>>(memRes, Elv::Util::as_const_span<PixelBGRA_F64>(bytes));
		case Format::ARGB8U: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_U8>>(memRes, Elv::Util::as_const_span<PixelARGB_U8>(bytes));
		case Format::ARGB16U: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_U16>>(memRes, Elv::Util::as_const_span<PixelARGB_U16>(bytes));
		case Format::ARGB32U: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_U32>>(memRes, Elv::Util::as_const_span<PixelARGB_U32>(bytes));
		case Format::ARGB8S: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_S8>>(memRes, Elv::Util::as_const_span<PixelARGB_S8>(bytes));
		case Format::ARGB16S: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_S16>>(memRes, Elv::Util::as_const_span<PixelARGB_S16>(bytes));
		case Format::ARGB32S: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_S32>>(memRes, Elv::Util::as_const_span<PixelARGB_S32>(bytes));
		case Format::ARGB16F: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_F16>>(memRes, Elv::Util::as_const_span<PixelARGB_F16>(bytes));
		case Format::ARGB32F: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_F32>>(memRes, Elv::Util::as_const_span<PixelARGB_F32>(bytes));
		case Format::ARGB64F: return Elv::Util::pmr_make_unique<FullPalette<PixelARGB_F64>>(memRes, Elv::Util::as_const_span<PixelARGB_F64>(bytes));
		case Format::RGB444: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB444>>(memRes, Elv::Util::as_const_span<PixelRGB444>(bytes));
		case Format::RGB555: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB555>>(memRes, Elv::Util::as_const_span<PixelRGB555>(bytes));
		case Format::RGB565: return Elv::Util::pmr_make_unique<FullPalette<PixelRGB565>>(memRes, Elv::Util::as_const_span<PixelRGB565>(bytes));
		default:
			throw std::runtime_error("Invalid pixel type!");
	}
}

sIMutablePalette createSptrFullPalette(const std::span<const std::byte>& bytes, Format format, std::pmr::memory_resource* memRes)
{
	switch (format) {
		case Format::R8U: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_U8>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U8>(bytes));
		case Format::R16U: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_U16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U16>(bytes));
		case Format::R32U: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_U32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U32>(bytes));
		case Format::R8S: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_S8>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S8>(bytes));
		case Format::R16S: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_S16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S16>(bytes));
		case Format::R32S: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_S32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S32>(bytes));
		case Format::R16F: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_F16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F16>(bytes));
		case Format::R32F: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_F32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F32>(bytes));
		case Format::R64F: return Elv::Util::pmr_make_shared<FullPalette<PixelGreyscale_F64>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F64>(bytes));
		case Format::RG8U: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_U8>>(memRes, Elv::Util::as_const_span<PixelRG_U8>(bytes));
		case Format::RG16U: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_U16>>(memRes, Elv::Util::as_const_span<PixelRG_U16>(bytes));
		case Format::RG32U: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_U32>>(memRes, Elv::Util::as_const_span<PixelRG_U32>(bytes));
		case Format::RG8S: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_S8>>(memRes, Elv::Util::as_const_span<PixelRG_S8>(bytes));
		case Format::RG16S: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_S16>>(memRes, Elv::Util::as_const_span<PixelRG_S16>(bytes));
		case Format::RG32S: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_S32>>(memRes, Elv::Util::as_const_span<PixelRG_S32>(bytes));
		case Format::RG16F: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_F16>>(memRes, Elv::Util::as_const_span<PixelRG_F16>(bytes));
		case Format::RG32F: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_F32>>(memRes, Elv::Util::as_const_span<PixelRG_F32>(bytes));
		case Format::RG64F: return Elv::Util::pmr_make_shared<FullPalette<PixelRG_F64>>(memRes, Elv::Util::as_const_span<PixelRG_F64>(bytes));
		case Format::RGB8U: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_U8>>(memRes, Elv::Util::as_const_span<PixelRGB_U8>(bytes));
		case Format::RGB16U: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_U16>>(memRes, Elv::Util::as_const_span<PixelRGB_U16>(bytes));
		case Format::RGB32U: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_U32>>(memRes, Elv::Util::as_const_span<PixelRGB_U32>(bytes));
		case Format::RGB8S: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_S8>>(memRes, Elv::Util::as_const_span<PixelRGB_S8>(bytes));
		case Format::RGB16S: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_S16>>(memRes, Elv::Util::as_const_span<PixelRGB_S16>(bytes));
		case Format::RGB32S: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_S32>>(memRes, Elv::Util::as_const_span<PixelRGB_S32>(bytes));
		case Format::RGB16F: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_F16>>(memRes, Elv::Util::as_const_span<PixelRGB_F16>(bytes));
		case Format::RGB32F: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_F32>>(memRes, Elv::Util::as_const_span<PixelRGB_F32>(bytes));
		case Format::RGB64F: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB_F64>>(memRes, Elv::Util::as_const_span<PixelRGB_F64>(bytes));
		case Format::BGR8U: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_U8>>(memRes, Elv::Util::as_const_span<PixelBGR_U8>(bytes));
		case Format::BGR16U: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_U16>>(memRes, Elv::Util::as_const_span<PixelBGR_U16>(bytes));
		case Format::BGR32U: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_U32>>(memRes, Elv::Util::as_const_span<PixelBGR_U32>(bytes));
		case Format::BGR8S: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_S8>>(memRes, Elv::Util::as_const_span<PixelBGR_S8>(bytes));
		case Format::BGR16S: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_S16>>(memRes, Elv::Util::as_const_span<PixelBGR_S16>(bytes));
		case Format::BGR32S: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_S32>>(memRes, Elv::Util::as_const_span<PixelBGR_S32>(bytes));
		case Format::BGR16F: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_F16>>(memRes, Elv::Util::as_const_span<PixelBGR_F16>(bytes));
		case Format::BGR32F: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_F32>>(memRes, Elv::Util::as_const_span<PixelBGR_F32>(bytes));
		case Format::BGR64F: return Elv::Util::pmr_make_shared<FullPalette<PixelBGR_F64>>(memRes, Elv::Util::as_const_span<PixelBGR_F64>(bytes));
		case Format::RGBA8U: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_U8>>(memRes, Elv::Util::as_const_span<PixelRGBA_U8>(bytes));
		case Format::RGBA16U: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_U16>>(memRes, Elv::Util::as_const_span<PixelRGBA_U16>(bytes));
		case Format::RGBA32U: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_U32>>(memRes, Elv::Util::as_const_span<PixelRGBA_U32>(bytes));
		case Format::RGBA8S: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_S8>>(memRes, Elv::Util::as_const_span<PixelRGBA_S8>(bytes));
		case Format::RGBA16S: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_S16>>(memRes, Elv::Util::as_const_span<PixelRGBA_S16>(bytes));
		case Format::RGBA32S: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_S32>>(memRes, Elv::Util::as_const_span<PixelRGBA_S32>(bytes));
		case Format::RGBA16F: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_F16>>(memRes, Elv::Util::as_const_span<PixelRGBA_F16>(bytes));
		case Format::RGBA32F: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_F32>>(memRes, Elv::Util::as_const_span<PixelRGBA_F32>(bytes));
		case Format::RGBA64F: return Elv::Util::pmr_make_shared<FullPalette<PixelRGBA_F64>>(memRes, Elv::Util::as_const_span<PixelRGBA_F64>(bytes));
		case Format::BGRA8U: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_U8>>(memRes, Elv::Util::as_const_span<PixelBGRA_U8>(bytes));
		case Format::BGRA16U: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_U16>>(memRes, Elv::Util::as_const_span<PixelBGRA_U16>(bytes));
		case Format::BGRA32U: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_U32>>(memRes, Elv::Util::as_const_span<PixelBGRA_U32>(bytes));
		case Format::BGRA8S: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_S8>>(memRes, Elv::Util::as_const_span<PixelBGRA_S8>(bytes));
		case Format::BGRA16S: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_S16>>(memRes, Elv::Util::as_const_span<PixelBGRA_S16>(bytes));
		case Format::BGRA32S: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_S32>>(memRes, Elv::Util::as_const_span<PixelBGRA_S32>(bytes));
		case Format::BGRA16F: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_F16>>(memRes, Elv::Util::as_const_span<PixelBGRA_F16>(bytes));
		case Format::BGRA32F: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_F32>>(memRes, Elv::Util::as_const_span<PixelBGRA_F32>(bytes));
		case Format::BGRA64F: return Elv::Util::pmr_make_shared<FullPalette<PixelBGRA_F64>>(memRes, Elv::Util::as_const_span<PixelBGRA_F64>(bytes));
		case Format::ARGB8U: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_U8>>(memRes, Elv::Util::as_const_span<PixelARGB_U8>(bytes));
		case Format::ARGB16U: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_U16>>(memRes, Elv::Util::as_const_span<PixelARGB_U16>(bytes));
		case Format::ARGB32U: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_U32>>(memRes, Elv::Util::as_const_span<PixelARGB_U32>(bytes));
		case Format::ARGB8S: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_S8>>(memRes, Elv::Util::as_const_span<PixelARGB_S8>(bytes));
		case Format::ARGB16S: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_S16>>(memRes, Elv::Util::as_const_span<PixelARGB_S16>(bytes));
		case Format::ARGB32S: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_S32>>(memRes, Elv::Util::as_const_span<PixelARGB_S32>(bytes));
		case Format::ARGB16F: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_F16>>(memRes, Elv::Util::as_const_span<PixelARGB_F16>(bytes));
		case Format::ARGB32F: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_F32>>(memRes, Elv::Util::as_const_span<PixelARGB_F32>(bytes));
		case Format::ARGB64F: return Elv::Util::pmr_make_shared<FullPalette<PixelARGB_F64>>(memRes, Elv::Util::as_const_span<PixelARGB_F64>(bytes));
		case Format::RGB444: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB444>>(memRes, Elv::Util::as_const_span<PixelRGB444>(bytes));
		case Format::RGB555: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB555>>(memRes, Elv::Util::as_const_span<PixelRGB555>(bytes));
		case Format::RGB565: return Elv::Util::pmr_make_shared<FullPalette<PixelRGB565>>(memRes, Elv::Util::as_const_span<PixelRGB565>(bytes));
		default:
			throw std::runtime_error("Invalid pixel type!");
	}
}

uIMutablePalette createUptrMutableReferencePalette(std::span<std::byte> bytes, Format format, std::pmr::memory_resource* memRes)
{
	switch (format) {
		case Format::R8U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelGreyscale_U8>>(memRes, Elv::Util::as_span<PixelGreyscale_U8>(bytes));
		case Format::R16U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelGreyscale_U16>>(memRes, Elv::Util::as_span<PixelGreyscale_U16>(bytes));
		case Format::R32U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelGreyscale_U32>>(memRes, Elv::Util::as_span<PixelGreyscale_U32>(bytes));
		case Format::R8S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelGreyscale_S8>>(memRes, Elv::Util::as_span<PixelGreyscale_S8>(bytes));
		case Format::R16S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelGreyscale_S16>>(memRes, Elv::Util::as_span<PixelGreyscale_S16>(bytes));
		case Format::R32S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelGreyscale_S32>>(memRes, Elv::Util::as_span<PixelGreyscale_S32>(bytes));
		case Format::R16F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelGreyscale_F16>>(memRes, Elv::Util::as_span<PixelGreyscale_F16>(bytes));
		case Format::R32F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelGreyscale_F32>>(memRes, Elv::Util::as_span<PixelGreyscale_F32>(bytes));
		case Format::R64F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelGreyscale_F64>>(memRes, Elv::Util::as_span<PixelGreyscale_F64>(bytes));
		case Format::RG8U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRG_U8>>(memRes, Elv::Util::as_span<PixelRG_U8>(bytes));
		case Format::RG16U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRG_U16>>(memRes, Elv::Util::as_span<PixelRG_U16>(bytes));
		case Format::RG32U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRG_U32>>(memRes, Elv::Util::as_span<PixelRG_U32>(bytes));
		case Format::RG8S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRG_S8>>(memRes, Elv::Util::as_span<PixelRG_S8>(bytes));
		case Format::RG16S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRG_S16>>(memRes, Elv::Util::as_span<PixelRG_S16>(bytes));
		case Format::RG32S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRG_S32>>(memRes, Elv::Util::as_span<PixelRG_S32>(bytes));
		case Format::RG16F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRG_F16>>(memRes, Elv::Util::as_span<PixelRG_F16>(bytes));
		case Format::RG32F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRG_F32>>(memRes, Elv::Util::as_span<PixelRG_F32>(bytes));
		case Format::RG64F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRG_F64>>(memRes, Elv::Util::as_span<PixelRG_F64>(bytes));
		case Format::RGB8U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGB_U8>>(memRes, Elv::Util::as_span<PixelRGB_U8>(bytes));
		case Format::RGB16U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGB_U16>>(memRes, Elv::Util::as_span<PixelRGB_U16>(bytes));
		case Format::RGB32U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGB_U32>>(memRes, Elv::Util::as_span<PixelRGB_U32>(bytes));
		case Format::RGB8S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGB_S8>>(memRes, Elv::Util::as_span<PixelRGB_S8>(bytes));
		case Format::RGB16S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGB_S16>>(memRes, Elv::Util::as_span<PixelRGB_S16>(bytes));
		case Format::RGB32S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGB_S32>>(memRes, Elv::Util::as_span<PixelRGB_S32>(bytes));
		case Format::RGB16F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGB_F16>>(memRes, Elv::Util::as_span<PixelRGB_F16>(bytes));
		case Format::RGB32F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGB_F32>>(memRes, Elv::Util::as_span<PixelRGB_F32>(bytes));
		case Format::RGB64F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGB_F64>>(memRes, Elv::Util::as_span<PixelRGB_F64>(bytes));
		case Format::BGR8U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGR_U8>>(memRes, Elv::Util::as_span<PixelBGR_U8>(bytes));
		case Format::BGR16U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGR_U16>>(memRes, Elv::Util::as_span<PixelBGR_U16>(bytes));
		case Format::BGR32U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGR_U32>>(memRes, Elv::Util::as_span<PixelBGR_U32>(bytes));
		case Format::BGR8S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGR_S8>>(memRes, Elv::Util::as_span<PixelBGR_S8>(bytes));
		case Format::BGR16S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGR_S16>>(memRes, Elv::Util::as_span<PixelBGR_S16>(bytes));
		case Format::BGR32S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGR_S32>>(memRes, Elv::Util::as_span<PixelBGR_S32>(bytes));
		case Format::BGR16F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGR_F16>>(memRes, Elv::Util::as_span<PixelBGR_F16>(bytes));
		case Format::BGR32F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGR_F32>>(memRes, Elv::Util::as_span<PixelBGR_F32>(bytes));
		case Format::BGR64F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGR_F64>>(memRes, Elv::Util::as_span<PixelBGR_F64>(bytes));
		case Format::RGBA8U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGBA_U8>>(memRes, Elv::Util::as_span<PixelRGBA_U8>(bytes));
		case Format::RGBA16U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGBA_U16>>(memRes, Elv::Util::as_span<PixelRGBA_U16>(bytes));
		case Format::RGBA32U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGBA_U32>>(memRes, Elv::Util::as_span<PixelRGBA_U32>(bytes));
		case Format::RGBA8S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGBA_S8>>(memRes, Elv::Util::as_span<PixelRGBA_S8>(bytes));
		case Format::RGBA16S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGBA_S16>>(memRes, Elv::Util::as_span<PixelRGBA_S16>(bytes));
		case Format::RGBA32S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGBA_S32>>(memRes, Elv::Util::as_span<PixelRGBA_S32>(bytes));
		case Format::RGBA16F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGBA_F16>>(memRes, Elv::Util::as_span<PixelRGBA_F16>(bytes));
		case Format::RGBA32F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGBA_F32>>(memRes, Elv::Util::as_span<PixelRGBA_F32>(bytes));
		case Format::RGBA64F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGBA_F64>>(memRes, Elv::Util::as_span<PixelRGBA_F64>(bytes));
		case Format::BGRA8U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGRA_U8>>(memRes, Elv::Util::as_span<PixelBGRA_U8>(bytes));
		case Format::BGRA16U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGRA_U16>>(memRes, Elv::Util::as_span<PixelBGRA_U16>(bytes));
		case Format::BGRA32U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGRA_U32>>(memRes, Elv::Util::as_span<PixelBGRA_U32>(bytes));
		case Format::BGRA8S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGRA_S8>>(memRes, Elv::Util::as_span<PixelBGRA_S8>(bytes));
		case Format::BGRA16S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGRA_S16>>(memRes, Elv::Util::as_span<PixelBGRA_S16>(bytes));
		case Format::BGRA32S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGRA_S32>>(memRes, Elv::Util::as_span<PixelBGRA_S32>(bytes));
		case Format::BGRA16F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGRA_F16>>(memRes, Elv::Util::as_span<PixelBGRA_F16>(bytes));
		case Format::BGRA32F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGRA_F32>>(memRes, Elv::Util::as_span<PixelBGRA_F32>(bytes));
		case Format::BGRA64F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelBGRA_F64>>(memRes, Elv::Util::as_span<PixelBGRA_F64>(bytes));
		case Format::ARGB8U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelARGB_U8>>(memRes, Elv::Util::as_span<PixelARGB_U8>(bytes));
		case Format::ARGB16U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelARGB_U16>>(memRes, Elv::Util::as_span<PixelARGB_U16>(bytes));
		case Format::ARGB32U: return Elv::Util::pmr_make_unique<ReferencePalette<PixelARGB_U32>>(memRes, Elv::Util::as_span<PixelARGB_U32>(bytes));
		case Format::ARGB8S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelARGB_S8>>(memRes, Elv::Util::as_span<PixelARGB_S8>(bytes));
		case Format::ARGB16S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelARGB_S16>>(memRes, Elv::Util::as_span<PixelARGB_S16>(bytes));
		case Format::ARGB32S: return Elv::Util::pmr_make_unique<ReferencePalette<PixelARGB_S32>>(memRes, Elv::Util::as_span<PixelARGB_S32>(bytes));
		case Format::ARGB16F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelARGB_F16>>(memRes, Elv::Util::as_span<PixelARGB_F16>(bytes));
		case Format::ARGB32F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelARGB_F32>>(memRes, Elv::Util::as_span<PixelARGB_F32>(bytes));
		case Format::ARGB64F: return Elv::Util::pmr_make_unique<ReferencePalette<PixelARGB_F64>>(memRes, Elv::Util::as_span<PixelARGB_F64>(bytes));
		case Format::RGB444: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGB444>>(memRes, Elv::Util::as_span<PixelRGB444>(bytes));
		case Format::RGB555: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGB555>>(memRes, Elv::Util::as_span<PixelRGB555>(bytes));
		case Format::RGB565: return Elv::Util::pmr_make_unique<ReferencePalette<PixelRGB565>>(memRes, Elv::Util::as_span<PixelRGB565>(bytes));
		default:
			throw std::runtime_error("Invalid pixel type!");
	}
}

sIMutablePalette createSptrMutableReferencePalette(std::span<std::byte> bytes, Format format, std::pmr::memory_resource* memRes)
{
	switch (format) {
		case Format::R8U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelGreyscale_U8>>(memRes, Elv::Util::as_span<PixelGreyscale_U8>(bytes));
		case Format::R16U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelGreyscale_U16>>(memRes, Elv::Util::as_span<PixelGreyscale_U16>(bytes));
		case Format::R32U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelGreyscale_U32>>(memRes, Elv::Util::as_span<PixelGreyscale_U32>(bytes));
		case Format::R8S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelGreyscale_S8>>(memRes, Elv::Util::as_span<PixelGreyscale_S8>(bytes));
		case Format::R16S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelGreyscale_S16>>(memRes, Elv::Util::as_span<PixelGreyscale_S16>(bytes));
		case Format::R32S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelGreyscale_S32>>(memRes, Elv::Util::as_span<PixelGreyscale_S32>(bytes));
		case Format::R16F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelGreyscale_F16>>(memRes, Elv::Util::as_span<PixelGreyscale_F16>(bytes));
		case Format::R32F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelGreyscale_F32>>(memRes, Elv::Util::as_span<PixelGreyscale_F32>(bytes));
		case Format::R64F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelGreyscale_F64>>(memRes, Elv::Util::as_span<PixelGreyscale_F64>(bytes));
		case Format::RG8U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRG_U8>>(memRes, Elv::Util::as_span<PixelRG_U8>(bytes));
		case Format::RG16U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRG_U16>>(memRes, Elv::Util::as_span<PixelRG_U16>(bytes));
		case Format::RG32U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRG_U32>>(memRes, Elv::Util::as_span<PixelRG_U32>(bytes));
		case Format::RG8S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRG_S8>>(memRes, Elv::Util::as_span<PixelRG_S8>(bytes));
		case Format::RG16S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRG_S16>>(memRes, Elv::Util::as_span<PixelRG_S16>(bytes));
		case Format::RG32S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRG_S32>>(memRes, Elv::Util::as_span<PixelRG_S32>(bytes));
		case Format::RG16F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRG_F16>>(memRes, Elv::Util::as_span<PixelRG_F16>(bytes));
		case Format::RG32F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRG_F32>>(memRes, Elv::Util::as_span<PixelRG_F32>(bytes));
		case Format::RG64F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRG_F64>>(memRes, Elv::Util::as_span<PixelRG_F64>(bytes));
		case Format::RGB8U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGB_U8>>(memRes, Elv::Util::as_span<PixelRGB_U8>(bytes));
		case Format::RGB16U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGB_U16>>(memRes, Elv::Util::as_span<PixelRGB_U16>(bytes));
		case Format::RGB32U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGB_U32>>(memRes, Elv::Util::as_span<PixelRGB_U32>(bytes));
		case Format::RGB8S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGB_S8>>(memRes, Elv::Util::as_span<PixelRGB_S8>(bytes));
		case Format::RGB16S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGB_S16>>(memRes, Elv::Util::as_span<PixelRGB_S16>(bytes));
		case Format::RGB32S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGB_S32>>(memRes, Elv::Util::as_span<PixelRGB_S32>(bytes));
		case Format::RGB16F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGB_F16>>(memRes, Elv::Util::as_span<PixelRGB_F16>(bytes));
		case Format::RGB32F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGB_F32>>(memRes, Elv::Util::as_span<PixelRGB_F32>(bytes));
		case Format::RGB64F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGB_F64>>(memRes, Elv::Util::as_span<PixelRGB_F64>(bytes));
		case Format::BGR8U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGR_U8>>(memRes, Elv::Util::as_span<PixelBGR_U8>(bytes));
		case Format::BGR16U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGR_U16>>(memRes, Elv::Util::as_span<PixelBGR_U16>(bytes));
		case Format::BGR32U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGR_U32>>(memRes, Elv::Util::as_span<PixelBGR_U32>(bytes));
		case Format::BGR8S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGR_S8>>(memRes, Elv::Util::as_span<PixelBGR_S8>(bytes));
		case Format::BGR16S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGR_S16>>(memRes, Elv::Util::as_span<PixelBGR_S16>(bytes));
		case Format::BGR32S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGR_S32>>(memRes, Elv::Util::as_span<PixelBGR_S32>(bytes));
		case Format::BGR16F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGR_F16>>(memRes, Elv::Util::as_span<PixelBGR_F16>(bytes));
		case Format::BGR32F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGR_F32>>(memRes, Elv::Util::as_span<PixelBGR_F32>(bytes));
		case Format::BGR64F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGR_F64>>(memRes, Elv::Util::as_span<PixelBGR_F64>(bytes));
		case Format::RGBA8U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGBA_U8>>(memRes, Elv::Util::as_span<PixelRGBA_U8>(bytes));
		case Format::RGBA16U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGBA_U16>>(memRes, Elv::Util::as_span<PixelRGBA_U16>(bytes));
		case Format::RGBA32U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGBA_U32>>(memRes, Elv::Util::as_span<PixelRGBA_U32>(bytes));
		case Format::RGBA8S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGBA_S8>>(memRes, Elv::Util::as_span<PixelRGBA_S8>(bytes));
		case Format::RGBA16S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGBA_S16>>(memRes, Elv::Util::as_span<PixelRGBA_S16>(bytes));
		case Format::RGBA32S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGBA_S32>>(memRes, Elv::Util::as_span<PixelRGBA_S32>(bytes));
		case Format::RGBA16F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGBA_F16>>(memRes, Elv::Util::as_span<PixelRGBA_F16>(bytes));
		case Format::RGBA32F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGBA_F32>>(memRes, Elv::Util::as_span<PixelRGBA_F32>(bytes));
		case Format::RGBA64F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGBA_F64>>(memRes, Elv::Util::as_span<PixelRGBA_F64>(bytes));
		case Format::BGRA8U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGRA_U8>>(memRes, Elv::Util::as_span<PixelBGRA_U8>(bytes));
		case Format::BGRA16U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGRA_U16>>(memRes, Elv::Util::as_span<PixelBGRA_U16>(bytes));
		case Format::BGRA32U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGRA_U32>>(memRes, Elv::Util::as_span<PixelBGRA_U32>(bytes));
		case Format::BGRA8S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGRA_S8>>(memRes, Elv::Util::as_span<PixelBGRA_S8>(bytes));
		case Format::BGRA16S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGRA_S16>>(memRes, Elv::Util::as_span<PixelBGRA_S16>(bytes));
		case Format::BGRA32S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGRA_S32>>(memRes, Elv::Util::as_span<PixelBGRA_S32>(bytes));
		case Format::BGRA16F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGRA_F16>>(memRes, Elv::Util::as_span<PixelBGRA_F16>(bytes));
		case Format::BGRA32F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGRA_F32>>(memRes, Elv::Util::as_span<PixelBGRA_F32>(bytes));
		case Format::BGRA64F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelBGRA_F64>>(memRes, Elv::Util::as_span<PixelBGRA_F64>(bytes));
		case Format::ARGB8U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelARGB_U8>>(memRes, Elv::Util::as_span<PixelARGB_U8>(bytes));
		case Format::ARGB16U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelARGB_U16>>(memRes, Elv::Util::as_span<PixelARGB_U16>(bytes));
		case Format::ARGB32U: return Elv::Util::pmr_make_shared<ReferencePalette<PixelARGB_U32>>(memRes, Elv::Util::as_span<PixelARGB_U32>(bytes));
		case Format::ARGB8S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelARGB_S8>>(memRes, Elv::Util::as_span<PixelARGB_S8>(bytes));
		case Format::ARGB16S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelARGB_S16>>(memRes, Elv::Util::as_span<PixelARGB_S16>(bytes));
		case Format::ARGB32S: return Elv::Util::pmr_make_shared<ReferencePalette<PixelARGB_S32>>(memRes, Elv::Util::as_span<PixelARGB_S32>(bytes));
		case Format::ARGB16F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelARGB_F16>>(memRes, Elv::Util::as_span<PixelARGB_F16>(bytes));
		case Format::ARGB32F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelARGB_F32>>(memRes, Elv::Util::as_span<PixelARGB_F32>(bytes));
		case Format::ARGB64F: return Elv::Util::pmr_make_shared<ReferencePalette<PixelARGB_F64>>(memRes, Elv::Util::as_span<PixelARGB_F64>(bytes));
		case Format::RGB444: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGB444>>(memRes, Elv::Util::as_span<PixelRGB444>(bytes));
		case Format::RGB555: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGB555>>(memRes, Elv::Util::as_span<PixelRGB555>(bytes));
		case Format::RGB565: return Elv::Util::pmr_make_shared<ReferencePalette<PixelRGB565>>(memRes, Elv::Util::as_span<PixelRGB565>(bytes));
		default:
			throw std::runtime_error("Invalid pixel type!");
	}
}

uIReadOnlyPalette createUptrReadOnlyReferencePalette(const std::span<const std::byte>& bytes, Format format, std::pmr::memory_resource* memRes)
{
	switch (format) {
		case Format::R8U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelGreyscale_U8>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U8>(bytes));
		case Format::R16U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelGreyscale_U16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U16>(bytes));
		case Format::R32U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelGreyscale_U32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U32>(bytes));
		case Format::R8S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelGreyscale_S8>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S8>(bytes));
		case Format::R16S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelGreyscale_S16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S16>(bytes));
		case Format::R32S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelGreyscale_S32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S32>(bytes));
		case Format::R16F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelGreyscale_F16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F16>(bytes));
		case Format::R32F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelGreyscale_F32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F32>(bytes));
		case Format::R64F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelGreyscale_F64>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F64>(bytes));
		case Format::RG8U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRG_U8>>(memRes, Elv::Util::as_const_span<PixelRG_U8>(bytes));
		case Format::RG16U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRG_U16>>(memRes, Elv::Util::as_const_span<PixelRG_U16>(bytes));
		case Format::RG32U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRG_U32>>(memRes, Elv::Util::as_const_span<PixelRG_U32>(bytes));
		case Format::RG8S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRG_S8>>(memRes, Elv::Util::as_const_span<PixelRG_S8>(bytes));
		case Format::RG16S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRG_S16>>(memRes, Elv::Util::as_const_span<PixelRG_S16>(bytes));
		case Format::RG32S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRG_S32>>(memRes, Elv::Util::as_const_span<PixelRG_S32>(bytes));
		case Format::RG16F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRG_F16>>(memRes, Elv::Util::as_const_span<PixelRG_F16>(bytes));
		case Format::RG32F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRG_F32>>(memRes, Elv::Util::as_const_span<PixelRG_F32>(bytes));
		case Format::RG64F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRG_F64>>(memRes, Elv::Util::as_const_span<PixelRG_F64>(bytes));
		case Format::RGB8U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGB_U8>>(memRes, Elv::Util::as_const_span<PixelRGB_U8>(bytes));
		case Format::RGB16U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGB_U16>>(memRes, Elv::Util::as_const_span<PixelRGB_U16>(bytes));
		case Format::RGB32U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGB_U32>>(memRes, Elv::Util::as_const_span<PixelRGB_U32>(bytes));
		case Format::RGB8S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGB_S8>>(memRes, Elv::Util::as_const_span<PixelRGB_S8>(bytes));
		case Format::RGB16S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGB_S16>>(memRes, Elv::Util::as_const_span<PixelRGB_S16>(bytes));
		case Format::RGB32S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGB_S32>>(memRes, Elv::Util::as_const_span<PixelRGB_S32>(bytes));
		case Format::RGB16F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGB_F16>>(memRes, Elv::Util::as_const_span<PixelRGB_F16>(bytes));
		case Format::RGB32F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGB_F32>>(memRes, Elv::Util::as_const_span<PixelRGB_F32>(bytes));
		case Format::RGB64F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGB_F64>>(memRes, Elv::Util::as_const_span<PixelRGB_F64>(bytes));
		case Format::BGR8U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGR_U8>>(memRes, Elv::Util::as_const_span<PixelBGR_U8>(bytes));
		case Format::BGR16U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGR_U16>>(memRes, Elv::Util::as_const_span<PixelBGR_U16>(bytes));
		case Format::BGR32U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGR_U32>>(memRes, Elv::Util::as_const_span<PixelBGR_U32>(bytes));
		case Format::BGR8S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGR_S8>>(memRes, Elv::Util::as_const_span<PixelBGR_S8>(bytes));
		case Format::BGR16S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGR_S16>>(memRes, Elv::Util::as_const_span<PixelBGR_S16>(bytes));
		case Format::BGR32S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGR_S32>>(memRes, Elv::Util::as_const_span<PixelBGR_S32>(bytes));
		case Format::BGR16F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGR_F16>>(memRes, Elv::Util::as_const_span<PixelBGR_F16>(bytes));
		case Format::BGR32F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGR_F32>>(memRes, Elv::Util::as_const_span<PixelBGR_F32>(bytes));
		case Format::BGR64F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGR_F64>>(memRes, Elv::Util::as_const_span<PixelBGR_F64>(bytes));
		case Format::RGBA8U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGBA_U8>>(memRes, Elv::Util::as_const_span<PixelRGBA_U8>(bytes));
		case Format::RGBA16U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGBA_U16>>(memRes, Elv::Util::as_const_span<PixelRGBA_U16>(bytes));
		case Format::RGBA32U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGBA_U32>>(memRes, Elv::Util::as_const_span<PixelRGBA_U32>(bytes));
		case Format::RGBA8S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGBA_S8>>(memRes, Elv::Util::as_const_span<PixelRGBA_S8>(bytes));
		case Format::RGBA16S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGBA_S16>>(memRes, Elv::Util::as_const_span<PixelRGBA_S16>(bytes));
		case Format::RGBA32S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGBA_S32>>(memRes, Elv::Util::as_const_span<PixelRGBA_S32>(bytes));
		case Format::RGBA16F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGBA_F16>>(memRes, Elv::Util::as_const_span<PixelRGBA_F16>(bytes));
		case Format::RGBA32F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGBA_F32>>(memRes, Elv::Util::as_const_span<PixelRGBA_F32>(bytes));
		case Format::RGBA64F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGBA_F64>>(memRes, Elv::Util::as_const_span<PixelRGBA_F64>(bytes));
		case Format::BGRA8U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGRA_U8>>(memRes, Elv::Util::as_const_span<PixelBGRA_U8>(bytes));
		case Format::BGRA16U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGRA_U16>>(memRes, Elv::Util::as_const_span<PixelBGRA_U16>(bytes));
		case Format::BGRA32U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGRA_U32>>(memRes, Elv::Util::as_const_span<PixelBGRA_U32>(bytes));
		case Format::BGRA8S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGRA_S8>>(memRes, Elv::Util::as_const_span<PixelBGRA_S8>(bytes));
		case Format::BGRA16S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGRA_S16>>(memRes, Elv::Util::as_const_span<PixelBGRA_S16>(bytes));
		case Format::BGRA32S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGRA_S32>>(memRes, Elv::Util::as_const_span<PixelBGRA_S32>(bytes));
		case Format::BGRA16F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGRA_F16>>(memRes, Elv::Util::as_const_span<PixelBGRA_F16>(bytes));
		case Format::BGRA32F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGRA_F32>>(memRes, Elv::Util::as_const_span<PixelBGRA_F32>(bytes));
		case Format::BGRA64F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelBGRA_F64>>(memRes, Elv::Util::as_const_span<PixelBGRA_F64>(bytes));
		case Format::ARGB8U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelARGB_U8>>(memRes, Elv::Util::as_const_span<PixelARGB_U8>(bytes));
		case Format::ARGB16U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelARGB_U16>>(memRes, Elv::Util::as_const_span<PixelARGB_U16>(bytes));
		case Format::ARGB32U: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelARGB_U32>>(memRes, Elv::Util::as_const_span<PixelARGB_U32>(bytes));
		case Format::ARGB8S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelARGB_S8>>(memRes, Elv::Util::as_const_span<PixelARGB_S8>(bytes));
		case Format::ARGB16S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelARGB_S16>>(memRes, Elv::Util::as_const_span<PixelARGB_S16>(bytes));
		case Format::ARGB32S: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelARGB_S32>>(memRes, Elv::Util::as_const_span<PixelARGB_S32>(bytes));
		case Format::ARGB16F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelARGB_F16>>(memRes, Elv::Util::as_const_span<PixelARGB_F16>(bytes));
		case Format::ARGB32F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelARGB_F32>>(memRes, Elv::Util::as_const_span<PixelARGB_F32>(bytes));
		case Format::ARGB64F: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelARGB_F64>>(memRes, Elv::Util::as_const_span<PixelARGB_F64>(bytes));
		case Format::RGB444: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGB444>>(memRes, Elv::Util::as_const_span<PixelRGB444>(bytes));
		case Format::RGB555: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGB555>>(memRes, Elv::Util::as_const_span<PixelRGB555>(bytes));
		case Format::RGB565: return Elv::Util::pmr_make_unique<ReadOnlyReferencePalette<PixelRGB565>>(memRes, Elv::Util::as_const_span<PixelRGB565>(bytes));
		default:
			throw std::runtime_error("Invalid pixel type!");
	}
}

sIReadOnlyPalette createSptrReadOnlyReferencePalette(const std::span<const std::byte>& bytes, Format format, std::pmr::memory_resource* memRes)
{
	switch (format) {
		case Format::R8U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelGreyscale_U8>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U8>(bytes));
		case Format::R16U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelGreyscale_U16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U16>(bytes));
		case Format::R32U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelGreyscale_U32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_U32>(bytes));
		case Format::R8S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelGreyscale_S8>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S8>(bytes));
		case Format::R16S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelGreyscale_S16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S16>(bytes));
		case Format::R32S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelGreyscale_S32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_S32>(bytes));
		case Format::R16F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelGreyscale_F16>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F16>(bytes));
		case Format::R32F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelGreyscale_F32>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F32>(bytes));
		case Format::R64F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelGreyscale_F64>>(memRes, Elv::Util::as_const_span<PixelGreyscale_F64>(bytes));
		case Format::RG8U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRG_U8>>(memRes, Elv::Util::as_const_span<PixelRG_U8>(bytes));
		case Format::RG16U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRG_U16>>(memRes, Elv::Util::as_const_span<PixelRG_U16>(bytes));
		case Format::RG32U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRG_U32>>(memRes, Elv::Util::as_const_span<PixelRG_U32>(bytes));
		case Format::RG8S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRG_S8>>(memRes, Elv::Util::as_const_span<PixelRG_S8>(bytes));
		case Format::RG16S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRG_S16>>(memRes, Elv::Util::as_const_span<PixelRG_S16>(bytes));
		case Format::RG32S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRG_S32>>(memRes, Elv::Util::as_const_span<PixelRG_S32>(bytes));
		case Format::RG16F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRG_F16>>(memRes, Elv::Util::as_const_span<PixelRG_F16>(bytes));
		case Format::RG32F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRG_F32>>(memRes, Elv::Util::as_const_span<PixelRG_F32>(bytes));
		case Format::RG64F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRG_F64>>(memRes, Elv::Util::as_const_span<PixelRG_F64>(bytes));
		case Format::RGB8U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGB_U8>>(memRes, Elv::Util::as_const_span<PixelRGB_U8>(bytes));
		case Format::RGB16U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGB_U16>>(memRes, Elv::Util::as_const_span<PixelRGB_U16>(bytes));
		case Format::RGB32U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGB_U32>>(memRes, Elv::Util::as_const_span<PixelRGB_U32>(bytes));
		case Format::RGB8S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGB_S8>>(memRes, Elv::Util::as_const_span<PixelRGB_S8>(bytes));
		case Format::RGB16S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGB_S16>>(memRes, Elv::Util::as_const_span<PixelRGB_S16>(bytes));
		case Format::RGB32S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGB_S32>>(memRes, Elv::Util::as_const_span<PixelRGB_S32>(bytes));
		case Format::RGB16F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGB_F16>>(memRes, Elv::Util::as_const_span<PixelRGB_F16>(bytes));
		case Format::RGB32F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGB_F32>>(memRes, Elv::Util::as_const_span<PixelRGB_F32>(bytes));
		case Format::RGB64F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGB_F64>>(memRes, Elv::Util::as_const_span<PixelRGB_F64>(bytes));
		case Format::BGR8U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGR_U8>>(memRes, Elv::Util::as_const_span<PixelBGR_U8>(bytes));
		case Format::BGR16U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGR_U16>>(memRes, Elv::Util::as_const_span<PixelBGR_U16>(bytes));
		case Format::BGR32U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGR_U32>>(memRes, Elv::Util::as_const_span<PixelBGR_U32>(bytes));
		case Format::BGR8S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGR_S8>>(memRes, Elv::Util::as_const_span<PixelBGR_S8>(bytes));
		case Format::BGR16S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGR_S16>>(memRes, Elv::Util::as_const_span<PixelBGR_S16>(bytes));
		case Format::BGR32S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGR_S32>>(memRes, Elv::Util::as_const_span<PixelBGR_S32>(bytes));
		case Format::BGR16F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGR_F16>>(memRes, Elv::Util::as_const_span<PixelBGR_F16>(bytes));
		case Format::BGR32F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGR_F32>>(memRes, Elv::Util::as_const_span<PixelBGR_F32>(bytes));
		case Format::BGR64F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGR_F64>>(memRes, Elv::Util::as_const_span<PixelBGR_F64>(bytes));
		case Format::RGBA8U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGBA_U8>>(memRes, Elv::Util::as_const_span<PixelRGBA_U8>(bytes));
		case Format::RGBA16U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGBA_U16>>(memRes, Elv::Util::as_const_span<PixelRGBA_U16>(bytes));
		case Format::RGBA32U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGBA_U32>>(memRes, Elv::Util::as_const_span<PixelRGBA_U32>(bytes));
		case Format::RGBA8S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGBA_S8>>(memRes, Elv::Util::as_const_span<PixelRGBA_S8>(bytes));
		case Format::RGBA16S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGBA_S16>>(memRes, Elv::Util::as_const_span<PixelRGBA_S16>(bytes));
		case Format::RGBA32S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGBA_S32>>(memRes, Elv::Util::as_const_span<PixelRGBA_S32>(bytes));
		case Format::RGBA16F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGBA_F16>>(memRes, Elv::Util::as_const_span<PixelRGBA_F16>(bytes));
		case Format::RGBA32F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGBA_F32>>(memRes, Elv::Util::as_const_span<PixelRGBA_F32>(bytes));
		case Format::RGBA64F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGBA_F64>>(memRes, Elv::Util::as_const_span<PixelRGBA_F64>(bytes));
		case Format::BGRA8U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGRA_U8>>(memRes, Elv::Util::as_const_span<PixelBGRA_U8>(bytes));
		case Format::BGRA16U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGRA_U16>>(memRes, Elv::Util::as_const_span<PixelBGRA_U16>(bytes));
		case Format::BGRA32U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGRA_U32>>(memRes, Elv::Util::as_const_span<PixelBGRA_U32>(bytes));
		case Format::BGRA8S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGRA_S8>>(memRes, Elv::Util::as_const_span<PixelBGRA_S8>(bytes));
		case Format::BGRA16S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGRA_S16>>(memRes, Elv::Util::as_const_span<PixelBGRA_S16>(bytes));
		case Format::BGRA32S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGRA_S32>>(memRes, Elv::Util::as_const_span<PixelBGRA_S32>(bytes));
		case Format::BGRA16F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGRA_F16>>(memRes, Elv::Util::as_const_span<PixelBGRA_F16>(bytes));
		case Format::BGRA32F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGRA_F32>>(memRes, Elv::Util::as_const_span<PixelBGRA_F32>(bytes));
		case Format::BGRA64F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelBGRA_F64>>(memRes, Elv::Util::as_const_span<PixelBGRA_F64>(bytes));
		case Format::ARGB8U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelARGB_U8>>(memRes, Elv::Util::as_const_span<PixelARGB_U8>(bytes));
		case Format::ARGB16U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelARGB_U16>>(memRes, Elv::Util::as_const_span<PixelARGB_U16>(bytes));
		case Format::ARGB32U: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelARGB_U32>>(memRes, Elv::Util::as_const_span<PixelARGB_U32>(bytes));
		case Format::ARGB8S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelARGB_S8>>(memRes, Elv::Util::as_const_span<PixelARGB_S8>(bytes));
		case Format::ARGB16S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelARGB_S16>>(memRes, Elv::Util::as_const_span<PixelARGB_S16>(bytes));
		case Format::ARGB32S: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelARGB_S32>>(memRes, Elv::Util::as_const_span<PixelARGB_S32>(bytes));
		case Format::ARGB16F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelARGB_F16>>(memRes, Elv::Util::as_const_span<PixelARGB_F16>(bytes));
		case Format::ARGB32F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelARGB_F32>>(memRes, Elv::Util::as_const_span<PixelARGB_F32>(bytes));
		case Format::ARGB64F: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelARGB_F64>>(memRes, Elv::Util::as_const_span<PixelARGB_F64>(bytes));
		case Format::RGB444: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGB444>>(memRes, Elv::Util::as_const_span<PixelRGB444>(bytes));
		case Format::RGB555: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGB555>>(memRes, Elv::Util::as_const_span<PixelRGB555>(bytes));
		case Format::RGB565: return Elv::Util::pmr_make_shared<ReadOnlyReferencePalette<PixelRGB565>>(memRes, Elv::Util::as_const_span<PixelRGB565>(bytes));
		default:
			throw std::runtime_error("Invalid pixel type!");
	}
}

uIMutablePalette createUptrFullPalette(const Palette& paletteSource, std::pmr::memory_resource* memRes)
{
	return createUptrFullPalette(paletteSource.data,paletteSource.format,memRes);
}

sIMutablePalette createSptrFullPalette(const Palette& paletteSource, std::pmr::memory_resource* memRes)
{
	return createSptrFullPalette(paletteSource.data,paletteSource.format,memRes);
}

uIMutablePalette createUptrMutableReferencePalette(Palette& paletteSource, std::pmr::memory_resource* memRes)
{
	return createUptrMutableReferencePalette(paletteSource.data,paletteSource.format,memRes);
}

sIMutablePalette createSptrMutableReferencePalette(Palette& paletteSource, std::pmr::memory_resource* memRes)
{
	return createSptrMutableReferencePalette(paletteSource.data,paletteSource.format,memRes);
}

uIReadOnlyPalette createUptrReadOnlyReferencePalette(const Palette& paletteSource, std::pmr::memory_resource* memRes)
{
	return createUptrReadOnlyReferencePalette(paletteSource.data,paletteSource.format,memRes);
}

sIReadOnlyPalette createSptrReadOnlyReferencePalette(const Palette& paletteSource, std::pmr::memory_resource* memRes)
{
	return createSptrReadOnlyReferencePalette(paletteSource.data,paletteSource.format,memRes);
}

}
}
}
