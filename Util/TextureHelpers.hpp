#ifndef TEXTUREHELPERS_H
#define TEXTUREHELPERS_H
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

struct ButtonTextureCollection {
	std::vector<uint32_t> ENABLED_NORMAL, ENABLED_CLICKED, DISABLED_NORMAL, DISABLED_CLICEKD;
};

uint32_t argb32(const float alpha);
uint32_t argb32(const glm::vec3& rgbF);
uint32_t argb32(const glm::vec4& rgbaF);
void createButtons(ButtonTextureCollection& buttons, int width, int height, const glm::vec3& baseColour, const glm::vec3& borderColour);
void createCircleTexture(std::vector<uint32_t>& output, const glm::vec3& colour, int CIRCLE_W, int CIRCLE_H);
void createCircleTextures(std::vector<uint32_t>& redCircle, std::vector<uint32_t>& greenCircle, std::vector<uint32_t>& blueCircle, int CIRCLE_W, int CIRCLE_H);

#endif // TEXTUREHELPERS_H
