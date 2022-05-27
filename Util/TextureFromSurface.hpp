#ifndef TEXTUREFROMSURFACE_HPP
#define TEXTUREFROMSURFACE_HPP
#include <memory>
#include "../Texture/Texture.hpp"
#include <SDL2/SDL_surface.h>

sTexture textureFromSurface(SDL_Surface& surface);
sTexture textureFromSurfaceCopy(SDL_Surface& surface);

#endif // TEXTUREFROMSURFACE_HPP
