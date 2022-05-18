#include "IWidget.hpp"
SDL_Rect IWidget::posToSCreenspaceRect(const glm::vec2& topLeft, const glm::vec2 bottomRight, const glm::ivec4& viewport) {
	const auto topLeftI = glm::ivec2(
									static_cast<int>( (topLeft.x + 1.0f) * 0.5f * static_cast<float>(viewport.z)) + viewport.x,
									static_cast<int>( (topLeft.y + 1.0f) * 0.5f * static_cast<float>(viewport.w)) + viewport.y
									);
	const auto bottomRightI = glm::ivec2(
									static_cast<int>( (bottomRight.x + 1.0f) * 0.5f * static_cast<float>(viewport.z)) + viewport.x,
									static_cast<int>( (bottomRight.y + 1.0f) * 0.5f * static_cast<float>(viewport.w)) + viewport.y
									);
	return SDL_Rect { .x = topLeftI.x, .y = topLeftI.y, .w = bottomRightI.x - topLeftI.x, .h = bottomRightI.y - topLeftI.y };
}

glm::ivec4 IWidget::posToSCreenspaceRect2(const glm::vec2& topLeft, const glm::vec2 bottomRight, const glm::ivec4& viewport)
{
	return glm::ivec4(
				static_cast<int>( (topLeft.x + 1.0f) * 0.5f* static_cast<float>(viewport.z)) + viewport.x,
				static_cast<int>( (topLeft.y + 1.0f) * 0.5f* static_cast<float>(viewport.w)) + viewport.y,
				static_cast<int>( (bottomRight.x + 1.0f) * 0.5f * static_cast<float>(viewport.z)) + viewport.x,
				static_cast<int>( (bottomRight.y + 1.0f) * 0.5f * static_cast<float>(viewport.w)) + viewport.y
				);
}

glm::vec2 IWidget::getRelativePosFromAbs(const glm::vec2& topLeft, const glm::vec2 bottomRight, const glm::vec2 absPos)
{
	return ( absPos - topLeft) / (bottomRight - topLeft);
}

glm::ivec2 IWidget::getAbsolutePositionFromRel(const glm::vec2& pos, const glm::ivec4& viewport)
{
	return glm::ivec2(
				static_cast<int>((pos.x + 1.0f) * 0.5f * static_cast<float>(viewport.z)) + viewport.x,
				static_cast<int>((pos.y + 1.0f) * 0.5f * static_cast<float>(viewport.w)) + viewport.y
				);
}
