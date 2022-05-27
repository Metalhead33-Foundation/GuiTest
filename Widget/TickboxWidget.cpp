#include "TickboxWidget.hpp"

int TickboxWidget::getThickness() const
{
	return thickness;
}

void TickboxWidget::setThickness(int newThickness)
{
	thickness = newThickness;
}

TickboxWidget::TickboxWidget(const glm::fvec2& topLeft, const glm::fvec2& bottomRight, int thickness)
	: SimpleWidget(topLeft,bottomRight), thickness(thickness)
{

}

void TickboxWidget::render(GuiRenderer& renderer)
{
	const bool isClicked = this->getIsClicked() || getTimeSinceLastClick() <= 250;

	glm::fvec4 clr;

	if(isClicked) clr = glm::fvec4(1.0f, 1.0f, 1.0f, 1.0f);
	else if(getIsActive()) clr = glm::fvec4(1.0f, 0.0f, 0.0f, 0.75f);
	else clr = glm::fvec4(0.0f, 0.5f, 0.5f, 0.75f);


	if(isClicked)
	{
		glm::fvec2 points[] = {
			glm::fvec2( topLeft.x, topLeft.y ),
			glm::fvec2( bottomRight.x, bottomRight.y ),
			glm::fvec2( bottomRight.x, topLeft.y ),
			glm::fvec2( topLeft.x, bottomRight.y ),
			glm::fvec2( bottomRight.x, bottomRight.y ),
			glm::fvec2( topLeft.x, bottomRight.y ),
			glm::fvec2( topLeft.x, topLeft.y ),
			glm::fvec2( bottomRight.x, topLeft.y )
		};
		renderer.renderCLines(points,clr,thickness);
	} else {
	glm::fvec2 points[] = {
		glm::fvec2( topLeft.x, topLeft.y ),
		glm::fvec2( bottomRight.x, topLeft.y ),
		glm::fvec2( bottomRight.x, bottomRight.y ),
		glm::fvec2( topLeft.x, bottomRight.y ),
		glm::fvec2( topLeft.x, topLeft.y )
	};
	renderer.renderCLines(points,clr,thickness);
	}

}
