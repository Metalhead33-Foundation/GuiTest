#include "TickboxWidget.hpp"

TickboxWidget::TickboxWidget(const glm::vec2& topLeft, const glm::vec2& bottomRight)
	: SimpleWidget(topLeft,bottomRight)
{

}

void TickboxWidget::render(SDL_Renderer& renderer, glm::ivec4 viewport)
{
	const bool isClicked = this->getIsClicked() || getTimeSinceLastClick() <= 250;

	if(isClicked) SDL_SetRenderDrawColor(&renderer,255,255,255,255);
	else if(getIsActive()) SDL_SetRenderDrawColor(&renderer,255,0,0,200);
	else SDL_SetRenderDrawColor(&renderer,0,127,127,200);

	const auto topLeft = getAbsolutePositionFromRel(this->topLeft,viewport);
	const auto bottomRight = getAbsolutePositionFromRel(this->bottomRight,viewport);

	if(isClicked)
	{
		SDL_Point points[] = {
			{ .x = topLeft.x, .y = topLeft.y },
			{ .x = bottomRight.x, .y = bottomRight.y },
			{ .x = bottomRight.x, .y = topLeft.y },
			{ .x = topLeft.x, .y = bottomRight.y },
			{ .x = bottomRight.x, .y = bottomRight.y },
			{ .x = topLeft.x, .y = bottomRight.y },
			{ .x = topLeft.x, .y = topLeft.y },
			{ .x = bottomRight.x, .y = topLeft.y }
		};
		SDL_RenderDrawLines(&renderer,points,8);
	} else {
	SDL_Point points[] = {
		{ .x = topLeft.x, .y = topLeft.y },
		{ .x = bottomRight.x, .y = topLeft.y },
		{ .x = bottomRight.x, .y = bottomRight.y },
		{ .x = topLeft.x, .y = bottomRight.y },
		{ .x = topLeft.x, .y = topLeft.y },
	};
	SDL_RenderDrawLines(&renderer,points,5);
	}

}
