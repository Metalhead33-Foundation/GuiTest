#include "TextureFromSurface.hpp"
#include "../../Texture/StandardTexture.hpp"
#include <stdexcept>
#include <AGL/GlTexture2D.hpp>

namespace SoftwareRenderer {
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

std::unique_ptr<SYS::ITexture> textureFromSurfaceA(SDL_Surface& surface, bool accelerated)
{
	auto sur = textureFromSurfaceU(surface);
	if(accelerated)
	{
		GLint internalFormat;
		GLenum format;
		GLenum type;
		switch (surface.format->format) {
		//case SDL_PIXELFORMAT_ARGB4444: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_4_4_4_4; break;
		case SDL_PIXELFORMAT_RGBA4444: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_4_4_4_4; break;
		/*case SDL_PIXELFORMAT_ABGR4444: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_4_4_4_4; break;
		case SDL_PIXELFORMAT_BGRA4444: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_4_4_4_4; break;
		case SDL_PIXELFORMAT_ARGB1555: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_5_5_5_1; break;*/
		case SDL_PIXELFORMAT_RGBA5551: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_5_5_5_1; break;
		//case SDL_PIXELFORMAT_ABGR1555: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_5_5_5_1; break;
		//case SDL_PIXELFORMAT_BGRA5551: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_5_5_5_1; break;
		case SDL_PIXELFORMAT_RGB565: internalFormat = GL_RGBA; format= GL_RGB; type = GL_UNSIGNED_SHORT_5_6_5; break;
		case SDL_PIXELFORMAT_BGR565: internalFormat = GL_RGBA; format= GL_RGB; type = GL_UNSIGNED_SHORT_5_6_5; break;
		case SDL_PIXELFORMAT_RGB888: internalFormat = GL_RGB; format= GL_RGB; type = GL_UNSIGNED_BYTE; break;
		/*case SDL_PIXELFORMAT_RGBX8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;
		case SDL_PIXELFORMAT_BGR888: internalFormat = GL_RGB; format= GL_RGB; type = GL_UNSIGNED_BYTE; break;
		case SDL_PIXELFORMAT_BGRX8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;
		case SDL_PIXELFORMAT_ARGB8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;*/
		case SDL_PIXELFORMAT_RGBA8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;
		/*case SDL_PIXELFORMAT_ABGR8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;
		case SDL_PIXELFORMAT_BGRA8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;*/
		default: throw std::runtime_error("Invalid texture format!");
		}
		return std::make_unique<GL::AcceleratedTexture>(std::move(sur),internalFormat,format,type);
	} else return std::move(sur);
}

std::unique_ptr<SYS::ITexture> textureFromSurfaceCopyA(SDL_Surface& surface, bool accelerated)
{
	auto sur = textureFromSurfaceCopyU(surface);
	if(accelerated)
	{
		GLint internalFormat;
		GLenum format;
		GLenum type;
		switch (surface.format->format) {
		//case SDL_PIXELFORMAT_ARGB4444: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_4_4_4_4; break;
		case SDL_PIXELFORMAT_RGBA4444: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_4_4_4_4; break;
		/*case SDL_PIXELFORMAT_ABGR4444: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_4_4_4_4; break;
		case SDL_PIXELFORMAT_BGRA4444: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_4_4_4_4; break;
		case SDL_PIXELFORMAT_ARGB1555: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_5_5_5_1; break;*/
		case SDL_PIXELFORMAT_RGBA5551: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_5_5_5_1; break;
		//case SDL_PIXELFORMAT_ABGR1555: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_5_5_5_1; break;
		//case SDL_PIXELFORMAT_BGRA5551: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_5_5_5_1; break;
		case SDL_PIXELFORMAT_RGB565: internalFormat = GL_RGBA; format= GL_RGB; type = GL_UNSIGNED_SHORT_5_6_5; break;
		case SDL_PIXELFORMAT_BGR565: internalFormat = GL_RGBA; format= GL_RGB; type = GL_UNSIGNED_SHORT_5_6_5; break;
		case SDL_PIXELFORMAT_RGB888: internalFormat = GL_RGB; format= GL_RGB; type = GL_UNSIGNED_BYTE; break;
		/*case SDL_PIXELFORMAT_RGBX8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;
		case SDL_PIXELFORMAT_BGR888: internalFormat = GL_RGB; format= GL_RGB; type = GL_UNSIGNED_BYTE; break;
		case SDL_PIXELFORMAT_BGRX8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;
		case SDL_PIXELFORMAT_ARGB8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;*/
		case SDL_PIXELFORMAT_RGBA8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;
		/*case SDL_PIXELFORMAT_ABGR8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;
		case SDL_PIXELFORMAT_BGRA8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;*/
		default: throw std::runtime_error("Invalid texture format!");
		}
		return std::make_unique<GL::AcceleratedTexture>(std::move(sur),internalFormat,format,type);
	} else return std::move(sur);
}

std::shared_ptr<SYS::ITexture> textureFromSurfaceSA(SDL_Surface& surface, bool accelerated)
{
	if(accelerated)
	{
		auto sur = textureFromSurfaceU(surface);
		GLint internalFormat;
		GLenum format;
		GLenum type;
		switch (surface.format->format) {
		//case SDL_PIXELFORMAT_ARGB4444: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_4_4_4_4; break;
		case SDL_PIXELFORMAT_RGBA4444: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_4_4_4_4; break;
		/*case SDL_PIXELFORMAT_ABGR4444: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_4_4_4_4; break;
		case SDL_PIXELFORMAT_BGRA4444: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_4_4_4_4; break;
		case SDL_PIXELFORMAT_ARGB1555: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_5_5_5_1; break;*/
		case SDL_PIXELFORMAT_RGBA5551: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_5_5_5_1; break;
		//case SDL_PIXELFORMAT_ABGR1555: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_5_5_5_1; break;
		//case SDL_PIXELFORMAT_BGRA5551: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_SHORT_5_5_5_1; break;
		case SDL_PIXELFORMAT_RGB565: internalFormat = GL_RGBA; format= GL_RGB; type = GL_UNSIGNED_SHORT_5_6_5; break;
		case SDL_PIXELFORMAT_BGR565: internalFormat = GL_RGBA; format= GL_RGB; type = GL_UNSIGNED_SHORT_5_6_5; break;
		case SDL_PIXELFORMAT_RGB888: internalFormat = GL_RGB; format= GL_RGB; type = GL_UNSIGNED_BYTE; break;
		/*case SDL_PIXELFORMAT_RGBX8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;
		case SDL_PIXELFORMAT_BGR888: internalFormat = GL_RGB; format= GL_RGB; type = GL_UNSIGNED_BYTE; break;
		case SDL_PIXELFORMAT_BGRX8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;
		case SDL_PIXELFORMAT_ARGB8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;*/
		case SDL_PIXELFORMAT_RGBA8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;
		/*case SDL_PIXELFORMAT_ABGR8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;
		case SDL_PIXELFORMAT_BGRA8888: internalFormat = GL_RGBA; format= GL_RGBA; type = GL_UNSIGNED_BYTE; break;*/
		default: throw std::runtime_error("Invalid texture format!");
		}
		return std::make_unique<GL::AcceleratedTexture>(std::move(sur),internalFormat,format,type);
	} else return textureFromSurface(surface);
}

std::shared_ptr<SYS::ITexture> textureFromSurfaceCopySA(SDL_Surface& surface, bool accelerated)
{
	if(accelerated)
	{
		bool hasAlpha = false;
		switch (surface.format->format) {
		case SDL_PIXELFORMAT_INDEX8: // Break through
		case SDL_PIXELFORMAT_RGB332: hasAlpha = false; break;
		case SDL_PIXELFORMAT_RGB444: hasAlpha = false; break;
		case SDL_PIXELFORMAT_RGB555: hasAlpha = false; break;
		case SDL_PIXELFORMAT_BGR555: hasAlpha = false; break;
		case SDL_PIXELFORMAT_ARGB4444: hasAlpha = true; break;
		case SDL_PIXELFORMAT_RGBA4444: hasAlpha = true; break;
		case SDL_PIXELFORMAT_ABGR4444: hasAlpha = true; break;
		case SDL_PIXELFORMAT_BGRA4444: hasAlpha = true; break;
		case SDL_PIXELFORMAT_ARGB1555: hasAlpha = true; break;
		case SDL_PIXELFORMAT_RGBA5551: hasAlpha = true; break;
		case SDL_PIXELFORMAT_ABGR1555: hasAlpha = true; break;
		case SDL_PIXELFORMAT_BGRA5551: hasAlpha = true; break;
		case SDL_PIXELFORMAT_RGB565: hasAlpha = false; break;
		case SDL_PIXELFORMAT_BGR565: hasAlpha = false; break;
		case SDL_PIXELFORMAT_RGB24: hasAlpha = false; break;
		case SDL_PIXELFORMAT_BGR24: hasAlpha = false; break;
		case SDL_PIXELFORMAT_RGB888: hasAlpha = false; break;
		case SDL_PIXELFORMAT_RGBX8888: hasAlpha = false; break;
		case SDL_PIXELFORMAT_BGR888: hasAlpha = false; break;
		case SDL_PIXELFORMAT_BGRX8888: hasAlpha = false; break;
		case SDL_PIXELFORMAT_ARGB8888: hasAlpha = true; break;
		case SDL_PIXELFORMAT_RGBA8888: hasAlpha = true; break;
		case SDL_PIXELFORMAT_ABGR8888: hasAlpha = true; break;
		case SDL_PIXELFORMAT_BGRA8888: hasAlpha = true; break;
		}
		if(hasAlpha)
		{
			std::unique_ptr<SDL_PixelFormat,decltype(&SDL_FreeFormat)> frm(SDL_AllocFormat(SDL_PIXELFORMAT_ABGR8888),SDL_FreeFormat);
			std::unique_ptr<SDL_Surface,decltype (&SDL_FreeSurface)> surorg(SDL_ConvertSurface(&surface,frm.get(),0),SDL_FreeSurface);
			auto sur = textureFromSurfaceCopyU(*surorg);
			return std::make_unique<GL::AcceleratedTexture>(std::move(sur),GL_RGBA,GL_RGBA,GL_UNSIGNED_BYTE);
		}
		else {
			std::unique_ptr<SDL_PixelFormat,decltype(&SDL_FreeFormat)> frm(SDL_AllocFormat(SDL_PIXELFORMAT_RGB565),SDL_FreeFormat);
			std::unique_ptr<SDL_Surface,decltype (&SDL_FreeSurface)> surorg(SDL_ConvertSurface(&surface,frm.get(),0),SDL_FreeSurface);
			auto sur = textureFromSurfaceCopyU(*surorg);
			return std::make_unique<GL::AcceleratedTexture>(std::move(sur),GL_RGB,GL_RGB,GL_UNSIGNED_SHORT_5_6_5);
		}
	} else return textureFromSurfaceCopy(surface);
}

}
