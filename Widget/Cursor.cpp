#include "Cursor.hpp"
#include <limits>

static const float magicAlpha = 0.995f;

float Cursor::getMouseScale() const
{
	return mouseScale;
}

void Cursor::setMouseScale(float newMouseScale)
{
	mouseScale = newMouseScale;
}

void Cursor::init()
{
	dimensions = glm::ivec2(texture->getWidth(),texture->getHeight());
	const int maxCrawl = std::min(dimensions.x,dimensions.y);
	glm::vec4 kernel;
	for(int dist = 0; dist < maxCrawl; ++dist) {
		if(!dist) {
			texture->getPixel(glm::ivec2(0,0),kernel);
			if(kernel.w > magicAlpha) {
				topLeft = glm::ivec2(0,0);
				break;
			}
		} else if(topLeft.x >= 0 && topLeft.y >= 0) {
			break;
		}
		if(topLeft.y < 0) for(int x = 0; x < dist; ++x) {
			texture->getPixel(glm::ivec2(x,dist),kernel);
			if(kernel.w >= magicAlpha) {
				topLeft.y = dist;
			}
		}
		if(topLeft.x < 0) for(int y = 0; y < dist; ++y) {
			texture->getPixel(glm::ivec2(dist,y),kernel);
			if(kernel.w >= magicAlpha) {
				topLeft.x = dist;
			}
		}
	}
}

Cursor::Cursor(const sTexture& tex, float mouseScale)
	: texture(tex), topLeft(-1,-1), mouseScale(mouseScale)
{
	init();
}

Cursor::Cursor(sTexture&& tex, float mouseScale)
	: texture(std::move(tex)), topLeft(-1,-1), mouseScale(mouseScale)
{
	init();
}

void Cursor::render(GuiRenderer& renderer, const glm::fvec2& mousePos, const glm::fvec2& screenReciprocal)
{
	const glm::fvec2 cursorTopLeft = mousePos - (glm::fvec2(static_cast<float>(topLeft.x) * screenReciprocal.x,static_cast<float>(topLeft.y) * screenReciprocal.y) * mouseScale);
	const glm::fvec2 cursorBottomRight = cursorTopLeft + (glm::fvec2(static_cast<float>(dimensions.x)*screenReciprocal.x,static_cast<float>(dimensions.y) * screenReciprocal.y) * mouseScale);
	renderer.renderTex(cursorTopLeft,cursorBottomRight,texture);
}
