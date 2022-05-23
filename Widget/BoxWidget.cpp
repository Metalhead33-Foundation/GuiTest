#include "BoxWidget.hpp"

BoxWidget::BoxWidget(const glm::vec2& topLeft, const glm::vec2& bottomRight)
	: SimpleWidget(topLeft,bottomRight)
{

}

void BoxWidget::render(GuiRenderer& renderer)
{
	const bool isClicked = this->getIsClicked() || getTimeSinceLastClick() <= 250;
	if(isClicked) renderer.renderCRect(topLeft,bottomRight,glm::vec4(1.0f, 1.0f, 1.0f, 0.75f));
	else if(getIsActive()) renderer.renderCRect(topLeft,bottomRight,glm::vec4(1.0f, 0.0f, 0.0f, 0.75f));
	else renderer.renderCRect(topLeft,bottomRight,glm::vec4(0.0f, 0.5f, 0.5f, 0.75f));
}
