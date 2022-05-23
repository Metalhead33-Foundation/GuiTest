#include "TickboxWidget.hpp"

TickboxWidget::TickboxWidget(const glm::vec2& topLeft, const glm::vec2& bottomRight)
	: SimpleWidget(topLeft,bottomRight)
{

}

void TickboxWidget::render(GuiRenderer& renderer)
{
	const bool isClicked = this->getIsClicked() || getTimeSinceLastClick() <= 250;

	glm::vec4 clr;

	if(isClicked) clr = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	else if(getIsActive()) clr = glm::vec4(1.0f, 0.0f, 0.0f, 0.75f);
	else clr = glm::vec4(0.0f, 0.5f, 0.5f, 0.75f);


	if(isClicked)
	{
		glm::vec2 points[] = {
			glm::vec2( topLeft.x, topLeft.y ),
			glm::vec2( bottomRight.x, bottomRight.y ),
			glm::vec2( bottomRight.x, topLeft.y ),
			glm::vec2( topLeft.x, bottomRight.y ),
			glm::vec2( bottomRight.x, bottomRight.y ),
			glm::vec2( topLeft.x, bottomRight.y ),
			glm::vec2( topLeft.x, topLeft.y ),
			glm::vec2( bottomRight.x, topLeft.y )
		};
		renderer.renderCLines(points,clr);
	} else {
	glm::vec2 points[] = {
		glm::vec2( topLeft.x, topLeft.y ),
		glm::vec2( bottomRight.x, topLeft.y ),
		glm::vec2( bottomRight.x, bottomRight.y ),
		glm::vec2( topLeft.x, bottomRight.y ),
		glm::vec2( topLeft.x, topLeft.y )
	};
	renderer.renderCLines(points,clr);
	}

}
