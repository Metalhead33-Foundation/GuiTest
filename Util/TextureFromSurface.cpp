#include "TextureFromSurface.hpp"
#include "../Texture/StandardTexture.hpp"
#include <stdexcept>

std::shared_ptr<Texture> textureFromSurface(SDL_Surface& surface)
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
