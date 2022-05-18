#include "TexturedWidget.hpp"

TexturedWidget::TexturedWidget(const glm::vec2& topLeft, const glm::vec2& bottomRight, SDL_Renderer& renderer, int w, int h, const uint32_t* texdis,
							   const uint32_t* textact, const uint32_t* textclick)
	: topLeft(topLeft), bottomRight(bottomRight), w(w),h(h), wf(static_cast<float>(w-1)), hf(static_cast<float>(h-1)),alpha(w*h),
	  textureDistact(SDL_CreateTexture(&renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STATIC,w,h),SDL_DestroyTexture),
	  textureActive(SDL_CreateTexture(&renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STATIC,w,h),SDL_DestroyTexture),
	  textureClicked(SDL_CreateTexture(&renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STATIC,w,h),SDL_DestroyTexture),
	  state(OUT_OF_FOCUS)
{
	SDL_UpdateTexture(textureDistact.get(),nullptr,texdis,w*sizeof(uint32_t));
	SDL_SetTextureBlendMode(textureDistact.get(),SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(textureActive.get(),nullptr,textact,w*sizeof(uint32_t));
	SDL_SetTextureBlendMode(textureActive.get(),SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(textureClicked.get(),nullptr,textclick,w*sizeof(uint32_t));
	SDL_SetTextureBlendMode(textureClicked.get(),SDL_BLENDMODE_BLEND);
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

void TexturedWidget::render(SDL_Renderer& renderer, glm::ivec4 viewport)
{
	SDL_Rect sdlRect = posToSCreenspaceRect(topLeft,bottomRight,viewport);
	switch (state) {
		case OUT_OF_FOCUS: SDL_RenderCopy(&renderer,textureDistact.get(),nullptr,&sdlRect); break;
		case IN_FOCUS: SDL_RenderCopy(&renderer,textureActive.get(),nullptr,&sdlRect); break;
		case CLICKED: SDL_RenderCopy(&renderer,textureClicked.get(),nullptr,&sdlRect); break;
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
