#include "BoxWidget.hpp"

BoxWidget::BoxWidget(const glm::vec2& topLeft, const glm::vec2& bottomRight)
	: SimpleWidget(topLeft,bottomRight)
{

}

void BoxWidget::render(SDL_Renderer& renderer, glm::ivec4 viewport)
{
	const bool isClicked = this->getIsClicked() || getTimeSinceLastClick() <= 250;
	if(isClicked) SDL_SetRenderDrawColor(&renderer,255,255,255,255);
	else if(getIsActive()) SDL_SetRenderDrawColor(&renderer,255,0,0,200);
	else SDL_SetRenderDrawColor(&renderer,0,127,127,200);
	SDL_Rect sdlRect = posToSCreenspaceRect(topLeft,bottomRight,viewport);
	SDL_RenderFillRect(&renderer,&sdlRect);
}
