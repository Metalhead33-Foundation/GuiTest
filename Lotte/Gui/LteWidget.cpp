#include "LteWidget.hpp"

namespace Lotte {
namespace Gui {

Widget::Widget() = default;

bool Widget::getFlag(WidgetStateFlag flag) const noexcept
{
	return (state_ & static_cast<WidgetState>(flag)) != 0u;
}

void Widget::setFlag(bool enabled, WidgetStateFlag flag)
{
	const WidgetState bit = static_cast<WidgetState>(flag);
	changeState(enabled ? (state_ | bit) : (state_ & ~bit));
}

void Widget::setTopLeft(const glm::fvec2& newTopLeft)
{
	topLeft_ = newTopLeft;
	positionChanged(this, topLeft_, bottomRight_);
}

void Widget::setBottomRight(const glm::fvec2& newBottomRight)
{
	bottomRight_ = newBottomRight;
	positionChanged(this, topLeft_, bottomRight_);
}

void Widget::setPosition(const glm::fvec2& newTopLeft, const glm::fvec2& newBottomRight)
{
	topLeft_ = newTopLeft;
	bottomRight_ = newBottomRight;
	positionChanged(this, topLeft_, bottomRight_);
}

bool Widget::contains(const glm::fvec2& point) const noexcept
{
	const glm::fvec2 minimum = glm::min(topLeft_, bottomRight_);
	const glm::fvec2 maximum = glm::max(topLeft_, bottomRight_);
	return point.x >= minimum.x && point.x <= maximum.x &&
		   point.y >= minimum.y && point.y <= maximum.y;
}

void Widget::onPointerMove(const glm::fvec2& offset)
{
	(void)offset;
}

void Widget::changeState(WidgetState newState)
{
	if (state_ == newState) return;
	const WidgetState oldState = state_;
	state_ = newState;
	stateChanged(this, oldState, newState);
}

} // namespace Gui
} // namespace Lotte
