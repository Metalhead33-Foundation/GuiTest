#include "TexturedWidget.hpp"
#include "../Texture/StandardTexture.hpp"
#include <SDL2/SDL.h>

namespace SYS {
TexturedWidget::TexturedWidget(const glm::fvec2& topLeft, const glm::fvec2& bottomRight, const sTexture& texdis,
							   const sTexture& textact, const sTexture& textclick)
	: topLeft(topLeft), bottomRight(bottomRight), w(textact->getWidth()),h(textact->getHeight()), wf(textact->getWidthF()), hf(textact->getHeightF()),
	  alpha(textact->getHeight() * textact->getWidth()),
	  textureDistact(texdis),
	  textureActive(textact),
	  textureClicked(textclick),
	  state(OUT_OF_FOCUS)
{
	textureDistact->iterateOverPixels(ITexture::ColourIterator([this](const glm::ivec2& pos, const glm::fvec4& kernel) {
		alpha[(pos.y*this->w)+pos.x] = kernel.w >= 0.007843137254902f;
	}));
}

TexturedWidget::TexturedWidget(const glm::fvec2& topLeft, const glm::fvec2& bottomRight, sTexture&& texdis, sTexture&& textact, sTexture&& textclick)
	: topLeft(topLeft), bottomRight(bottomRight), w(textact->getWidth()),h(textact->getHeight()), wf(textact->getWidthF()), hf(textact->getHeightF()),
	  alpha(textact->getHeight() * textact->getWidth()),
	  textureDistact(std::move(texdis)),
	  textureActive(std::move(textact)),
	  textureClicked(std::move(textclick)),
	  state(OUT_OF_FOCUS)
{
	textureDistact->iterateOverPixels(ITexture::ColourIterator([this](const glm::ivec2& pos, const glm::fvec4& kernel) {
		alpha[(pos.y*this->w)+pos.x] = kernel.w >= 0.007843137254902f;
	}));
}

const glm::fvec2& TexturedWidget::getTopLeft() const
{
	return topLeft;
}

void TexturedWidget::setTopLeft(const glm::fvec2& newTopLeft)
{
	topLeft = newTopLeft;
}

const glm::fvec2& TexturedWidget::getBottomRight() const
{
	return bottomRight;
}

void TexturedWidget::offsetPos(const glm::fvec2& posOffset)
{
	topLeft += posOffset;
	bottomRight += posOffset;
}

glm::ivec2 TexturedWidget::translateOffset(const glm::fvec2& offset) const
{
	return glm::ivec2(static_cast<int>(std::round(offset.x * wf)),static_cast<int>(std::round(offset.y * hf)));
}

void TexturedWidget::setBottomRight(const glm::fvec2& newBottomRight)
{
	bottomRight = newBottomRight;
}

void TexturedWidget::render(GuiRenderer& renderer)
{
	switch (state) {
		case OUT_OF_FOCUS: renderer.renderTex(topLeft,bottomRight,*textureDistact); break;
		case IN_FOCUS: renderer.renderTex(topLeft,bottomRight,*textureActive); break;
		case CLICKED: renderer.renderTex(topLeft,bottomRight,*textureClicked); break;
	}
}

bool TexturedWidget::onClick(const glm::fvec2& offset, uint8_t button, uint8_t mousestate, uint8_t clicks)
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

bool TexturedWidget::onHover(const glm::fvec2& offset, const glm::fvec2& relativePosToLast)
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
}
