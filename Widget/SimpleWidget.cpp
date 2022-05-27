#include "SimpleWidget.hpp"
#include <iostream>

const glm::fvec2& SimpleWidget::getTopLeft() const
{
	return topLeft;
}

void SimpleWidget::setTopLeft(const glm::fvec2& newTopLeft)
{
	topLeft = newTopLeft;
}

const glm::fvec2& SimpleWidget::getBottomRight() const
{
	return bottomRight;
}

void SimpleWidget::setBottomRight(const glm::fvec2& newBottomRight)
{
	bottomRight = newBottomRight;
}

void SimpleWidget::setPosition(const glm::fvec2& topLeft, const glm::fvec2& bottomRight)
{
	this->topLeft = topLeft;
	this->bottomRight = bottomRight;
}

void SimpleWidget::offsetPos(const glm::fvec2& posOffset)
{
	topLeft += posOffset;
	bottomRight += posOffset;
}

bool SimpleWidget::onClick(const glm::fvec2& offset, uint8_t button, uint8_t mousestate, uint8_t clicks)
{
	if(mousestate == SDL_PRESSED) {
	isClicked = true;
	lastClick = SDL_GetTicks();
	std::cout << "[" << static_cast<void*>(this) << "] - Click at offset [" << offset.x << ", " << offset.y << "]." << std::endl;
	return true;
	} else {
		isClicked = false;
		return false;
	}
}

bool SimpleWidget::onHover(const glm::fvec2& offset, const glm::fvec2& relativePosToLast)
{
	(void)offset;
	isActive = true;
	if(isClicked) {
		offsetPos(relativePosToLast);
	}
	return true;
}

void SimpleWidget::onOutOfFocus()
{
	isActive = false;
	isClicked = false;
}

uint32_t SimpleWidget::getTimeSinceLastClick() const
{
	return SDL_GetTicks() - lastClick;
}

/*
	uint32_t lastClick;
	bool isActive;
*/

bool SimpleWidget::getIsActive() const
{
	return isActive;
}

bool SimpleWidget::getIsClicked() const
{
	return isClicked;
}

SimpleWidget::SimpleWidget(const glm::fvec2& topLeft, const glm::fvec2& bottomRight)
	: topLeft(topLeft), bottomRight(bottomRight), lastClick(SDL_GetTicks()), isActive(false)
{

}
