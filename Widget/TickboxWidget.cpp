#include "TickboxWidget.hpp"

namespace SYS {
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

void TickboxWidget::assignPoints(MH33::GFX::GuiRenderingContext& renderer, const std::span<const glm::fvec2>& points)
{
	renderer.colouredUMesh->ensureSize((points.size() - 1) * 2);
	renderer.colouredUMesh->access( [&points](void* dst, size_t s) {
		std::span<Renderer::ColouredWidgetVert> span(static_cast<Renderer::ColouredWidgetVert*>(dst),s);
		uint32_t z = 0;
		for(size_t i = 1; i < points.size(); ++i) {
			span[z].POS = points[i-1];
			span[z].POS.y *= -1.0f;
			++z;
			span[z].POS = points[i];
			span[z].POS.y *= -1.0f;
			++z;
		}
	});
}

void TickboxWidget::render(MH33::GFX::GuiRenderingContext& renderer)
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
		assignPoints(renderer,points);

	} else {
		glm::fvec2 points[] = {
			glm::fvec2( topLeft.x, topLeft.y ),
			glm::fvec2( bottomRight.x, topLeft.y ),
			glm::fvec2( bottomRight.x, bottomRight.y ),
			glm::fvec2( topLeft.x, bottomRight.y ),
			glm::fvec2( topLeft.x, topLeft.y )
		};
		assignPoints(renderer,points);
	}
	renderer.colouredPipeline.renderLines(*renderer.colouredUMesh,clr,thickness);

}
}

