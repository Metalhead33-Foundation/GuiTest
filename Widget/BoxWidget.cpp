#include "BoxWidget.hpp"

namespace SYS {
BoxWidget::BoxWidget(const glm::fvec2& topLeft, const glm::fvec2& bottomRight)
	: SimpleWidget(topLeft,bottomRight)
{

}
void BoxWidget::render(MH33::GFX::GuiRenderingContext& renderer)
{
	const bool isClicked = this->getIsClicked() || getTimeSinceLastClick() <= 250;
	renderer.colouredIMesh->ensureVertexCount(4);
	renderer.colouredIMesh->accessVertices([this](void* dst, size_t s) {
		std::span<Renderer::ColouredWidgetVert> span(static_cast<Renderer::ColouredWidgetVert*>(dst),s);
		span[0].POS = glm::fvec2(std::min(topLeft.x,bottomRight.x),-1.0f * std::min(topLeft.y,bottomRight.y));
		span[1].POS = glm::fvec2(std::max(topLeft.x,bottomRight.x),-1.0f * std::min(topLeft.y,bottomRight.y));
		span[2].POS = glm::fvec2(std::min(topLeft.x,bottomRight.x),-1.0f * std::max(topLeft.y,bottomRight.y));
		span[3].POS = glm::fvec2(std::max(topLeft.x,bottomRight.x),-1.0f * std::max(topLeft.y,bottomRight.y));
	});
	if(isClicked) renderer.colouredPipeline.renderTriangles(*renderer.colouredIMesh,glm::fvec4(1.0f, 1.0f, 1.0f, 0.75f));
	else if(getIsActive()) renderer.colouredPipeline.renderTriangles(*renderer.colouredIMesh,glm::fvec4(1.0f, 0.0f, 0.0f, 0.75f));
	else renderer.colouredPipeline.renderTriangles(*renderer.colouredIMesh,glm::fvec4(0.0f, 0.5f, 0.5f, 0.75f));
}

}
