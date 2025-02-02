#include "EuphStandardImage.hpp"
namespace Euph {
namespace Media {
namespace Image {

uResizeableImage2D createUptrResizeableImage2D(const std::span<const std::byte>& bytes, Format format, unsigned width, unsigned height, std::pmr::memory_resource* memRes)
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

uResizeableImage2D createUptrResizeableImage2D(const Frame& frame, Format format, std::pmr::memory_resource* memRes)
{
	return createUptrResizeableImage2D(frame.data,format,frame.width,frame.height,memRes);
}

uResizeableImage2D createUptrResizeableImage2D(Format format, unsigned width, unsigned height, std::pmr::memory_resource* memRes)
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

sResizeableImage2D createSptrResizeableImage2D(const std::span<const std::byte>& bytes, Format format, unsigned width, unsigned height, std::pmr::memory_resource* memRes)
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

sResizeableImage2D createSptrResizeableImage2D(const Frame& frame, Format format, std::pmr::memory_resource* memRes)
{
	return createSptrResizeableImage2D(frame.data,format,frame.width,frame.height,memRes);
}

sResizeableImage2D createSptrResizeableImage2D(Format format, unsigned width, unsigned height, std::pmr::memory_resource* memRes)
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

uImage2D createUptrReferenceImage2D(std::span<std::byte> bytes, Format format, unsigned width, unsigned height, std::pmr::memory_resource* memRes)
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

uImage2D createUptrReferenceImage2D(Frame& frame, Format format, std::pmr::memory_resource* memRes)
{
	return createUptrReferenceImage2D(frame.data, format, frame.width, frame.height, memRes);
}

sImage2D createSptrReferenceImage2D(std::span<std::byte> bytes, Format format, unsigned width, unsigned height, std::pmr::memory_resource* memRes)
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

sImage2D createSptrReferenceImage2D(Frame& frame, Format format, std::pmr::memory_resource* memRes)
{
	return createSptrReferenceImage2D(frame.data, format, frame.width, frame.height, memRes);
}

uReadOnlyImage2D createUptrReadOnlyReferenceImage2D(const std::span<const std::byte>& bytes, Format format, unsigned width, unsigned height, std::pmr::memory_resource* memRes)
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

uReadOnlyImage2D createUptrRReadOnlyeferenceImage2D(const Frame& frame, Format format, std::pmr::memory_resource* memRes)
{
	return createUptrReadOnlyReferenceImage2D(frame.data, format, frame.width, frame.height, memRes);
}

sReadOnlyImage2D createSptrReadOnlyReferenceImage2D(const std::span<const std::byte>& bytes, Format format, unsigned width, unsigned height, std::pmr::memory_resource* memRes)
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

sReadOnlyImage2D createSptrReadOnlyReferenceImage2D(const Frame& frame, Format format, std::pmr::memory_resource* memRes)
{
	return createSptrReadOnlyReferenceImage2D(frame.data, format, frame.width, frame.height, memRes);
}

}
}
}
