#include "TexturedWidget.hpp"
#include "../Texture/StandardTexture.hpp"
#include <SDL2/SDL.h>

TexturedWidget::TexturedWidget(const glm::vec2& topLeft, const glm::vec2& bottomRight, int w, int h, const uint32_t* texdis,
							   const uint32_t* textact, const uint32_t* textclick)
	: topLeft(topLeft), bottomRight(bottomRight), w(w),h(h), wf(static_cast<float>(w-1)), hf(static_cast<float>(h-1)),alpha(w*h),
	  textureDistact(std::make_shared<TexARGB8888>(reinterpret_cast<const PixelARGB8888*>(texdis),w,h)),
	  textureActive(std::make_shared<TexARGB8888>(reinterpret_cast<const PixelARGB8888*>(textact),w,h)),
	  textureClicked(std::make_shared<TexARGB8888>(reinterpret_cast<const PixelARGB8888*>(textclick),w,h)),
	  state(OUT_OF_FOCUS)
{
	const int totalsize = w * h;
	for(int i = 0; i < totalsize; ++i) {
		alpha[i] = ((texdis[i] & 0xFF000000) != 0);
	}
}

const glm::vec2& TexturedWidget::getTopLeft() const
{
	return topLeft;
}

void TexturedWidget::setTopLeft(const glm::vec2& newTopLeft)
{
	topLeft = newTopLeft;
}

const glm::vec2& TexturedWidget::getBottomRight() const
{
	return bottomRight;
}

void TexturedWidget::offsetPos(const glm::vec2& posOffset)
{
	topLeft += posOffset;
	bottomRight += posOffset;
}

glm::ivec2 TexturedWidget::translateOffset(const glm::vec2& offset) const
{
	return glm::ivec2(static_cast<int>(std::round(offset.x * wf)),static_cast<int>(std::round(offset.y * hf)));
}

void TexturedWidget::setBottomRight(const glm::vec2& newBottomRight)
{
	bottomRight = newBottomRight;
}

void TexturedWidget::render(GuiRenderer& renderer)
{
	switch (state) {
		case OUT_OF_FOCUS: renderer.renderTex(topLeft,bottomRight,textureDistact); break;
		case IN_FOCUS: renderer.renderTex(topLeft,bottomRight,textureActive); break;
		case CLICKED: renderer.renderTex(topLeft,bottomRight,textureClicked); break;
	}
}

bool TexturedWidget::onClick(const glm::vec2& offset, uint8_t button, uint8_t mousestate, uint8_t clicks)
{
	const auto noffset = translateOffset(offset);
	if(alpha[(noffset.y*w)+noffset.x]) {
		if(mousestate == SDL_PRESSED) {
		state = CLICKED;
		return true;
		} else {
			state = IN_FOCUS;
			return false;
		}
	} else {
		state = OUT_OF_FOCUS;
		return false;
	}
}

bool TexturedWidget::onHover(const glm::vec2& offset, const glm::vec2& relativePosToLast)
{
	const auto noffset = translateOffset(offset);
	if( !alpha[(noffset.y*w)+noffset.x] ) {
		state = OUT_OF_FOCUS;
		return false;
	}
	if(state == CLICKED) {
		offsetPos(relativePosToLast);
	} else state = IN_FOCUS;
	return true;
}

void TexturedWidget::onOutOfFocus()
{
	state = OUT_OF_FOCUS;
}
