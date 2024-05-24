#ifndef TEXTUREHELPERS_H
#define TEXTUREHELPERS_H
#include <cstdint>
#include <span>
#include <MhLib/Media/Image/MhDecodeTarget.hpp>
#include <glm/glm.hpp>

namespace MH33 {
namespace Util {
struct ButtonTextureCollection {
	MH33::Image::DecodeTarget ENABLED_NORMAL, ENABLED_CLICKED, DISABLED_NORMAL, DISABLED_CLICEKD;
};

uint32_t argb32(const float alpha);
uint32_t argb32(const glm::fvec3& rgbF);
uint32_t argb32(const glm::fvec4& rgbaF);
uint32_t rgba32(const float alpha);
uint32_t rgba32(const glm::fvec3& rgbF);
uint32_t rgba32(const glm::fvec4& rgbaF);
void createButtons(ButtonTextureCollection& buttons, int width, int height, const glm::fvec3& baseColour, const glm::fvec3& borderColour);
void createCircleTexture(MH33::Image::DecodeTarget& output, const glm::fvec3& colour, int CIRCLE_W, int CIRCLE_H, bool argb = true);
void createCircleTextures(MH33::Image::DecodeTarget& redCircle, MH33::Image::DecodeTarget& greenCircle, MH33::Image::DecodeTarget& blueCircle, int CIRCLE_W, int CIRCLE_H, bool argb = true);
}
}
#endif // TEXTUREHELPERS_H
