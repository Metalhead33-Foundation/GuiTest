#include "GuiRenderer.hpp"

void GuiRenderer::renderCLines(const std::span<glm::fvec2>& points, const glm::fvec4& colour, int thickness)
{
	for(size_t i = 1; i < points.size(); ++i) {
		renderCLine(points[i-1],points[i],colour,thickness);
	}
}
