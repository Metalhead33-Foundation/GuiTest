#include <Renderer/Software/SoftwareRenderer.hpp>
#include <Renderer/Software/SoftStandardTexture.hpp>
#include <Renderer/Software/SoftMesh.hpp>
#include <Renderer/Software/SoftUniformBuffer.hpp>
#include <Renderer/Software/SoftGuiPipeline.hpp>

namespace Renderer {
namespace Software {

RenderingContext CONTEXT;


sTexture textureFromSurface(SDL_Surface& surface)
{
	switch (surface.format->format) {
	case SDL_PIXELFORMAT_INDEX8: // Break through
	case SDL_PIXELFORMAT_RGB332: return std::make_shared<RefTexRGB332>(static_cast<PixelRGB332*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB444: return std::make_shared<RefTexRGB444>(static_cast<PixelRGB444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB555: return std::make_shared<RefTexRGB555>(static_cast<PixelRGB555*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGR555: return std::make_shared<RefTexBGR555>(static_cast<PixelBGR555*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ARGB4444: return std::make_shared<RefTexARGB4444>(static_cast<PixelARGB4444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGBA4444: return std::make_shared<RefTexRGBA4444>(static_cast<PixelRGBA4444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ABGR4444: return std::make_shared<RefTexABGR4444>(static_cast<PixelABGR4444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGRA4444: return std::make_shared<RefTexBGRA4444>(static_cast<PixelBGRA4444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ARGB1555: return std::make_shared<RefTexARGB1555>(static_cast<PixelARGB1555*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGBA5551: return std::make_shared<RefTexRGBA5551>(static_cast<PixelRGBA5551*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ABGR1555: return std::make_shared<RefTexABGR1555>(static_cast<PixelABGR1555*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGRA5551: return std::make_shared<RefTexBGRA5551>(static_cast<PixelBGRA5551*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB565: return std::make_shared<RefTexRGB565>(static_cast<PixelRGB565*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGR565: return std::make_shared<RefTexBGR565>(static_cast<PixelBGR565*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB24: return std::make_shared<RefTexRGB_U8>(static_cast<PixelRGB_U8*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGR24: return std::make_shared<RefTexBGR_U8>(static_cast<PixelBGR_U8*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB888: return std::make_shared<RefTexRGB888>(static_cast<PixelRGB888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGBX8888: return std::make_shared<RefTexRGBX8888>(static_cast<PixelRGBX8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGR888: return std::make_shared<RefTexBGR888>(static_cast<PixelBGR888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGRX8888: return std::make_shared<RefTexBGRX8888>(static_cast<PixelBGRX8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ARGB8888: return std::make_shared<RefTexARGB8888>(static_cast<PixelARGB8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGBA8888: return std::make_shared<RefTexRGBA8888>(static_cast<PixelRGBA8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ABGR8888: return std::make_shared<RefTexABGR8888>(static_cast<PixelABGR8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGRA8888: return std::make_shared<RefTexBGRA8888>(static_cast<PixelBGRA8888*>(surface.pixels),surface.w,surface.h);
	default: throw std::runtime_error("Invalid texture format!");
	}
	return nullptr;
}

sTexture textureFromSurfaceCopy(SDL_Surface& surface)
{
	switch (surface.format->format) {
	case SDL_PIXELFORMAT_INDEX8: // Break through
	case SDL_PIXELFORMAT_RGB332: return std::make_shared<TexRGB332>(static_cast<PixelRGB332*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB444: return std::make_shared<TexRGB444>(static_cast<PixelRGB444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB555: return std::make_shared<TexRGB555>(static_cast<PixelRGB555*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGR555: return std::make_shared<TexBGR555>(static_cast<PixelBGR555*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ARGB4444: return std::make_shared<TexARGB4444>(static_cast<PixelARGB4444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGBA4444: return std::make_shared<TexRGBA4444>(static_cast<PixelRGBA4444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ABGR4444: return std::make_shared<TexABGR4444>(static_cast<PixelABGR4444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGRA4444: return std::make_shared<TexBGRA4444>(static_cast<PixelBGRA4444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ARGB1555: return std::make_shared<TexARGB1555>(static_cast<PixelARGB1555*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGBA5551: return std::make_shared<TexRGBA5551>(static_cast<PixelRGBA5551*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ABGR1555: return std::make_shared<TexABGR1555>(static_cast<PixelABGR1555*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGRA5551: return std::make_shared<TexBGRA5551>(static_cast<PixelBGRA5551*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB565: return std::make_shared<TexRGB565>(static_cast<PixelRGB565*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGR565: return std::make_shared<TexBGR565>(static_cast<PixelBGR565*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB24: return std::make_shared<TexRGB_U8>(static_cast<PixelRGB_U8*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGR24: return std::make_shared<TexBGR_U8>(static_cast<PixelBGR_U8*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB888: return std::make_shared<TexRGB888>(static_cast<PixelRGB888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGBX8888: return std::make_shared<TexRGBX8888>(static_cast<PixelRGBX8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGR888: return std::make_shared<TexBGR888>(static_cast<PixelBGR888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGRX8888: return std::make_shared<TexBGRX8888>(static_cast<PixelBGRX8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ARGB8888: return std::make_shared<TexARGB8888>(static_cast<PixelARGB8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGBA8888: return std::make_shared<TexRGBA8888>(static_cast<PixelRGBA8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ABGR8888: return std::make_shared<TexABGR8888>(static_cast<PixelABGR8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGRA8888: return std::make_shared<TexBGRA8888>(static_cast<PixelBGRA8888*>(surface.pixels),surface.w,surface.h);
	default: throw std::runtime_error("Invalid texture format!");
	}
	return nullptr;
}

uTexture textureFromSurfaceU(SDL_Surface& surface)
{
	switch (surface.format->format) {
	case SDL_PIXELFORMAT_INDEX8: // Break through
	case SDL_PIXELFORMAT_RGB332: return std::make_unique<RefTexRGB332>(static_cast<PixelRGB332*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB444: return std::make_unique<RefTexRGB444>(static_cast<PixelRGB444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB555: return std::make_unique<RefTexRGB555>(static_cast<PixelRGB555*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGR555: return std::make_unique<RefTexBGR555>(static_cast<PixelBGR555*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ARGB4444: return std::make_unique<RefTexARGB4444>(static_cast<PixelARGB4444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGBA4444: return std::make_unique<RefTexRGBA4444>(static_cast<PixelRGBA4444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ABGR4444: return std::make_unique<RefTexABGR4444>(static_cast<PixelABGR4444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGRA4444: return std::make_unique<RefTexBGRA4444>(static_cast<PixelBGRA4444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ARGB1555: return std::make_unique<RefTexARGB1555>(static_cast<PixelARGB1555*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGBA5551: return std::make_unique<RefTexRGBA5551>(static_cast<PixelRGBA5551*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ABGR1555: return std::make_unique<RefTexABGR1555>(static_cast<PixelABGR1555*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGRA5551: return std::make_unique<RefTexBGRA5551>(static_cast<PixelBGRA5551*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB565: return std::make_unique<RefTexRGB565>(static_cast<PixelRGB565*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGR565: return std::make_unique<RefTexBGR565>(static_cast<PixelBGR565*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB24: return std::make_unique<RefTexRGB_U8>(static_cast<PixelRGB_U8*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGR24: return std::make_unique<RefTexBGR_U8>(static_cast<PixelBGR_U8*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB888: return std::make_unique<RefTexRGB888>(static_cast<PixelRGB888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGBX8888: return std::make_unique<RefTexRGBX8888>(static_cast<PixelRGBX8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGR888: return std::make_unique<RefTexBGR888>(static_cast<PixelBGR888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGRX8888: return std::make_unique<RefTexBGRX8888>(static_cast<PixelBGRX8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ARGB8888: return std::make_unique<RefTexARGB8888>(static_cast<PixelARGB8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGBA8888: return std::make_unique<RefTexRGBA8888>(static_cast<PixelRGBA8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ABGR8888: return std::make_unique<RefTexABGR8888>(static_cast<PixelABGR8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGRA8888: return std::make_unique<RefTexBGRA8888>(static_cast<PixelBGRA8888*>(surface.pixels),surface.w,surface.h);
	default: throw std::runtime_error("Invalid texture format!");
	}
	return nullptr;
}

uTexture textureFromSurfaceCopyU(SDL_Surface& surface)
{
	switch (surface.format->format) {
	case SDL_PIXELFORMAT_INDEX8: // Break through
	case SDL_PIXELFORMAT_RGB332: return std::make_unique<TexRGB332>(static_cast<PixelRGB332*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB444: return std::make_unique<TexRGB444>(static_cast<PixelRGB444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB555: return std::make_unique<TexRGB555>(static_cast<PixelRGB555*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGR555: return std::make_unique<TexBGR555>(static_cast<PixelBGR555*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ARGB4444: return std::make_unique<TexARGB4444>(static_cast<PixelARGB4444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGBA4444: return std::make_unique<TexRGBA4444>(static_cast<PixelRGBA4444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ABGR4444: return std::make_unique<TexABGR4444>(static_cast<PixelABGR4444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGRA4444: return std::make_unique<TexBGRA4444>(static_cast<PixelBGRA4444*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ARGB1555: return std::make_unique<TexARGB1555>(static_cast<PixelARGB1555*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGBA5551: return std::make_unique<TexRGBA5551>(static_cast<PixelRGBA5551*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ABGR1555: return std::make_unique<TexABGR1555>(static_cast<PixelABGR1555*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGRA5551: return std::make_unique<TexBGRA5551>(static_cast<PixelBGRA5551*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB565: return std::make_unique<TexRGB565>(static_cast<PixelRGB565*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGR565: return std::make_unique<TexBGR565>(static_cast<PixelBGR565*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB24: return std::make_unique<TexRGB_U8>(static_cast<PixelRGB_U8*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGR24: return std::make_unique<TexBGR_U8>(static_cast<PixelBGR_U8*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGB888: return std::make_unique<TexRGB888>(static_cast<PixelRGB888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGBX8888: return std::make_unique<TexRGBX8888>(static_cast<PixelRGBX8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGR888: return std::make_unique<TexBGR888>(static_cast<PixelBGR888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGRX8888: return std::make_unique<TexBGRX8888>(static_cast<PixelBGRX8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ARGB8888: return std::make_unique<TexARGB8888>(static_cast<PixelARGB8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_RGBA8888: return std::make_unique<TexRGBA8888>(static_cast<PixelRGBA8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_ABGR8888: return std::make_unique<TexABGR8888>(static_cast<PixelABGR8888*>(surface.pixels),surface.w,surface.h);
	case SDL_PIXELFORMAT_BGRA8888: return std::make_unique<TexBGRA8888>(static_cast<PixelBGRA8888*>(surface.pixels),surface.w,surface.h);
	default: throw std::runtime_error("Invalid texture format!");
	}
	return nullptr;
}

SoftwareRenderer::SoftwareRenderer(SDL_Window* window, SDL_SysWMinfo* winfo)
	: window(window), winfo(winfo)
{
	SDL_Surface* window_surface = SDL_GetWindowSurface(window);
	CONTEXT.framebuffer = textureFromSurface(*window_surface);
	CONTEXT.viewport = glm::ivec4(0,0,CONTEXT.framebuffer->getWidth(),CONTEXT.framebuffer->getHeight());
	CONTEXT.blending = AlphaBlending::ALPHA_BLENDING;
	CONTEXT.filtering = TextureFiltering::NEAREST_NEIGHBOUR;
	CONTEXT.wrapping = Wrap::REPEAT;
}

static sTexture createSoftwareTexture(const MH33::Image::DecodeTarget& source)
{
	if(source.frames.empty()) return nullptr;
	switch (source.format) {
	case MH33::Image::Format::R8U:
		return sTexture(
					new TexGreyscale_U8(reinterpret_cast<const PixelGreyscale_U8*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::R16U:
		return sTexture(
					new TexGreyscale_U16(reinterpret_cast<const PixelGreyscale_U16*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::R32U:
		return sTexture(
					new TexGreyscale_U32(reinterpret_cast<const PixelGreyscale_U32*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::RG8U:
		return sTexture(
					new TexRG_U8(reinterpret_cast<const PixelRG_U8*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::RG16U:
		return sTexture(
					new TexRG_U16(reinterpret_cast<const PixelRG_U16*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::RG32U:
		return sTexture(
					new TexRG_U32(reinterpret_cast<const PixelRG_U32*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::RGB8U:
		return sTexture(
					new TexRGB_U8(reinterpret_cast<const PixelRGB_U8*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::RGB16U:
		return sTexture(
					new TexRGB_U16(reinterpret_cast<const PixelRGB_U16*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::RGB32U:
		return sTexture(
					new TexRGB_U32(reinterpret_cast<const PixelRGB_U32*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::BGR8U:
		return sTexture(
					new TexBGR_U8(reinterpret_cast<const PixelBGR_U8*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::BGR16U:
		return sTexture(
					new TexBGR_U16(reinterpret_cast<const PixelBGR_U16*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::BGR32U:
		return sTexture(
					new TexBGR_U32(reinterpret_cast<const PixelBGR_U32*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::RGBA8U:
		return sTexture(
					new TexRGBA_U8(reinterpret_cast<const PixelRGBA_U8*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::RGBA16U:
		return sTexture(
					new TexRGBA_U16(reinterpret_cast<const PixelRGBA_U16*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::RGBA32U:
		return sTexture(
					new TexRGBA_U32(reinterpret_cast<const PixelRGBA_U32*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::BGRA8U:
		return sTexture(
					new TexBGRA_U8(reinterpret_cast<const PixelBGRA_U8*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::BGRA16U:
		return sTexture(
					new TexBGRA_U16(reinterpret_cast<const PixelBGRA_U16*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::BGRA32U:
		return sTexture(
					new TexBGRA_U32(reinterpret_cast<const PixelBGRA_U32*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::ARGB8U:
		return sTexture(
					new TexARGB_U8(reinterpret_cast<const PixelARGB_U8*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::ARGB16U:
		return sTexture(
					new TexARGB_U16(reinterpret_cast<const PixelARGB_U16*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::ARGB32U:
		return sTexture(
					new TexARGB_U32(reinterpret_cast<const PixelARGB_U32*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::RGB565:
		return sTexture(
					new TexRGB565(reinterpret_cast<const PixelRGB565*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::RGB555:
		return sTexture(
					new TexRGB555(reinterpret_cast<const PixelRGB555*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::RGB444:
		return sTexture(
					new TexRGB444(reinterpret_cast<const PixelRGB444*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	case MH33::Image::Format::RGBA444:
		return sTexture(
					new TexRGBA4444(reinterpret_cast<const PixelRGBA4444*>(source.frames[0].imageData.data()),
					source.frames[0].width,
					source.frames[0].height));
	default: return nullptr;
	}
}

MH33::GFX::sWriteableTexture2D SoftwareRenderer::createWriteableTexture2D(const MH33::Image::DecodeTarget& source)
{
	return createSoftwareTexture(source);
}

MH33::GFX::sTexture2D SoftwareRenderer::createTexture2D(const MH33::Image::DecodeTarget& source)
{
	return createSoftwareTexture(source);
}

MH33::GFX::sTexture2D SoftwareRenderer::createTexture2D(const MH33::Image::DecodeTarget& source, std::vector<bool>& bitmap, glm::ivec2& dimensions)
{
	auto tex = createSoftwareTexture(source);
	if(tex) {
		const int width = tex->getWidth();
		const int height = tex->getHeight();
		dimensions.x = width;
		dimensions.y = height;
		bitmap.resize(width * height);
		tex->iterateOverPixels(Texture::ColourIterator([&bitmap,&width](const glm::ivec2& pos, const glm::fvec4& clr) {
			bitmap[(pos.y * width) + pos.x] = clr.a >= 0.007843137254902f;
		}));
	}
	return std::move(tex);
}

MH33::GFX::sWriteableTexture2D SoftwareRenderer::createWriteableTexture2D(MH33::GFX::TextureFormat format, const glm::ivec2& dimensions)
{
	switch (format) {
	case MH33::Image::Format::R8U:
		return sTexture(new TexGreyscale_U8(dimensions.x, dimensions.y));
	case MH33::Image::Format::R16U:
		return sTexture(new TexGreyscale_U16(dimensions.x, dimensions.y));
	case MH33::Image::Format::R32U:
		return sTexture(new TexGreyscale_U32(dimensions.x, dimensions.y));
	case MH33::Image::Format::RG8U:
		return sTexture(new TexRG_U8(dimensions.x, dimensions.y));
	case MH33::Image::Format::RG16U:
		return sTexture(new TexRG_U16(dimensions.x, dimensions.y));
	case MH33::Image::Format::RG32U:
		return sTexture(new TexRG_U32(dimensions.x, dimensions.y));
	case MH33::Image::Format::RGB8U:
		return sTexture(new TexRGB_U8(dimensions.x, dimensions.y));
	case MH33::Image::Format::RGB16U:
		return sTexture(new TexRGB_U16(dimensions.x, dimensions.y));
	case MH33::Image::Format::RGB32U:
		return sTexture(new TexRGB_U32(dimensions.x, dimensions.y));
	case MH33::Image::Format::BGR8U:
		return sTexture(new TexBGR_U8(dimensions.x, dimensions.y));
	case MH33::Image::Format::BGR16U:
		return sTexture(new TexBGR_U16(dimensions.x, dimensions.y));
	case MH33::Image::Format::BGR32U:
		return sTexture(new TexBGR_U32(dimensions.x, dimensions.y));
	case MH33::Image::Format::RGBA8U:
		return sTexture(new TexRGBA_U8(dimensions.x, dimensions.y));
	case MH33::Image::Format::RGBA16U:
		return sTexture(new TexRGBA_U16(dimensions.x, dimensions.y));
	case MH33::Image::Format::RGBA32U:
		return sTexture(new TexRGBA_U32(dimensions.x, dimensions.y));
	case MH33::Image::Format::BGRA8U:
		return sTexture(new TexBGRA_U8(dimensions.x, dimensions.y));
	case MH33::Image::Format::BGRA16U:
		return sTexture(new TexBGRA_U16(dimensions.x, dimensions.y));
	case MH33::Image::Format::BGRA32U:
		return sTexture(new TexBGRA_U32(dimensions.x, dimensions.y));
	case MH33::Image::Format::ARGB8U:
		return sTexture(new TexARGB_U8(dimensions.x, dimensions.y));
	case MH33::Image::Format::ARGB16U:
		return sTexture(new TexARGB_U16(dimensions.x, dimensions.y));
	case MH33::Image::Format::ARGB32U:
		return sTexture(new TexARGB_U32(dimensions.x, dimensions.y));
	case MH33::Image::Format::RGB565:
		return sTexture(new TexRGB565(dimensions.x, dimensions.y));
	case MH33::Image::Format::RGB555:
		return sTexture(new TexRGB555(dimensions.x, dimensions.y));
	case MH33::Image::Format::RGB444:
		return sTexture(new TexRGB444(dimensions.x, dimensions.y));
	case MH33::Image::Format::RGBA444:
		return sTexture(new TexRGBA4444(dimensions.x, dimensions.y));
	default: return nullptr;
	}
}

MH33::GFX::sUnindexedMesh SoftwareRenderer::createUnindexedMesh(const MH33::GFX::VertexDescriptor* vertexDescriptor, bool streaming)
{
	(void)streaming;
	return sUnindexedMesh(new UnindexedMesh(vertexDescriptor));
}

MH33::GFX::sUnindexedMesh SoftwareRenderer::createUnindexedMesh(const MH33::GFX::VertexDescriptor* vertexDescriptor, const std::span<const std::byte>& vertexData, bool streaming)
{
	(void)streaming;
	return sUnindexedMesh(new UnindexedMesh(vertexDescriptor,vertexData,vertexData.size() / vertexDescriptor->stride));
}

MH33::GFX::sIndexedMesh SoftwareRenderer::createIndexedMesh(const MH33::GFX::VertexDescriptor* vertexDescriptor, bool streaming)
{
	(void)streaming;
	return sIndexedMesh(new IndexedMesh(vertexDescriptor));
}

MH33::GFX::sIndexedMesh SoftwareRenderer::createIndexedMesh(const MH33::GFX::VertexDescriptor* vertexDescriptor, const std::span<const std::byte>& vertexData, const std::span<const uint32_t>& indices, bool streaming)
{
	(void)streaming;
	return sIndexedMesh(new IndexedMesh(vertexDescriptor,vertexData,vertexData.size() / vertexDescriptor->stride,indices));
}

MH33::GFX::sUniformBuffer SoftwareRenderer::createUniformBuffer(size_t size)
{
	return sUniformBuffer(new UniformBuffer(size));
}

bool SoftwareRenderer::supportsCustomShaders() const
{
	return false;
}

MH33::GFX::sPipeline SoftwareRenderer::createPipeline(const std::string& name)
{
	if(!name.compare("single_colour_widget")) return MH33::GFX::sPipeline(new ColouredWidgetPipeline());
	else if(!name.compare("textured_widget")) return MH33::GFX::sPipeline(new TexturedWidgetPipeline());
	else if(!name.compare("coloured_textured_widget")) return MH33::GFX::sPipeline(new ColouredTexturedWidgetPipeline(false));
	else if(!name.compare("coloured_text")) return MH33::GFX::sPipeline(new ColouredTexturedWidgetPipeline(true));
	return nullptr;
}

MH33::GFX::sPipeline SoftwareRenderer::createPipeline(const MH33::GFX::ShaderBinary& binaries, MH33::GFX::VertexDescriptor* vertexInput)
{
	return nullptr;
}

void SoftwareRenderer::beginFrame()
{
	CONTEXT.framebuffer->clearToColour(glm::fvec4(0.0,0.0,0.0,1.0));
}

void SoftwareRenderer::endFrame()
{
	SDL_UpdateWindowSurface(window);
}

}
}

