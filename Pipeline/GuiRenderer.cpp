#include "GuiRenderer.hpp"

void GuiRenderer::renderCLines(const std::span<glm::vec2>& points, const glm::vec4& colour)
{
	for(size_t i = 1; i < points.size(); ++i) {
		renderCLine(points[i-1],points[i],colour);
	}
}
