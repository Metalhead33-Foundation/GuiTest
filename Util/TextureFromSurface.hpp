#ifndef TEXTUREFROMSURFACE_HPP
#define TEXTUREFROMSURFACE_HPP
#include <memory>
#include "../Texture/Texture.hpp"
#include <SDL2/SDL_surface.h>
namespace SoftwareRenderer {
sTexture textureFromSurface(SDL_Surface& surface);
sTexture textureFromSurfaceCopy(SDL_Surface& surface);
uTexture textureFromSurfaceU(SDL_Surface& surface);
uTexture textureFromSurfaceCopyU(SDL_Surface& surface);
std::unique_ptr<SYS::ITexture> textureFromSurfaceA(SDL_Surface& surface, bool accelerated);
std::unique_ptr<SYS::ITexture> textureFromSurfaceCopyA(SDL_Surface& surface, bool accelerated);
std::shared_ptr<SYS::ITexture> textureFromSurfaceSA(SDL_Surface& surface, bool accelerated);
std::shared_ptr<SYS::ITexture> textureFromSurfaceCopySA(SDL_Surface& surface, bool accelerated);
}
#endif // TEXTUREFROMSURFACE_HPP
