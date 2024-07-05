#include "MhWidget.hpp"
namespace MH33 {
namespace GUI {
void Widget::changeState(uint32_t newState)
{
	if(state != newState) {
		uint32_t oldState = state;
		state = newState;
		signal_onStateChanged(this,oldState,newState);
	}
}

const glm::vec2& Widget::getBottomRight() const
{
	return bottomRight;
}

void Widget::setBottomRight(const glm::vec2& newBottomRight)
{
	bottomRight = newBottomRight;
	signal_onPositionChanged(this,&bottomRight,&topLeft);
}

void Widget::setNewPosition(const glm::vec2& newTopleft, const glm::vec2& newBottomRight)
{
	topLeft = newTopleft;
	bottomRight = newBottomRight;
	signal_onPositionChanged(this,&bottomRight,&topLeft);
}

const glm::vec2& Widget::getTopLeft() const
{
	return topLeft;
}

void Widget::setTopLeft(const glm::vec2& newTopLeft)
{
	topLeft = newTopLeft;
	signal_onPositionChanged(this,&bottomRight,&topLeft);
}

void Widget::setFlag(bool state, WidgetStateFlags flag)
{
	if(state) {
		changeState( state | static_cast<uint32_t>(flag));
	} else {
		changeState( state & ~static_cast<uint32_t>(flag));
	}
}

bool Widget::getFlag(WidgetStateFlags flag) const
{
	return (state & static_cast<uint32_t>(flag)) != 0;
}

Widget::Widget()
{

}

}
}
