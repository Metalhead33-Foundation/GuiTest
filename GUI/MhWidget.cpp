#include "MhWidget.hpp"
#include <iostream>
namespace MH33 {
namespace GUI {
bool Widget::getHidden() const
{
	return hidden;
}

void Widget::setHidden(bool newHidden)
{
	hidden = newHidden;
}

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
	std::cout << static_cast<uint32_t>(flag) << ' ' << state << std::endl;
	if(state) {
		changeState( this->state | static_cast<uint32_t>(flag));
	} else {
		changeState( this->state & (~static_cast<uint32_t>(flag)));
	}
}

bool Widget::getFlag(WidgetStateFlags flag) const
{
	return bool(state & static_cast<uint32_t>(flag));
}

Widget::Widget()
	: state(0)
{

}

}
}
