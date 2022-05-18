#ifndef SIMPLEWIDGET_H
#define SIMPLEWIDGET_H

#include "IWidget.hpp"

class SimpleWidget : public IWidget
{
protected:
	glm::vec2 topLeft, bottomRight;
private:
	uint32_t lastClick;
	bool isActive, isClicked;
public:
	SimpleWidget(const glm::vec2& topLeft, const glm::vec2& bottomRight);
	const glm::vec2& getTopLeft() const override;
	void setTopLeft(const glm::vec2& newTopLeft);
	const glm::vec2& getBottomRight() const override;
	void setBottomRight(const glm::vec2& newBottomRight);
	void setPosition(const glm::vec2& topLeft, const glm::vec2& bottomRight);
	void offsetPos(const glm::vec2& posOffset);

	virtual bool onClick(const glm::vec2& offset, uint8_t button, uint8_t mousestate, uint8_t clicks) override;
	virtual bool onHover(const glm::vec2& offset, const glm::vec2& relativePosToLast) override;
	void onOutOfFocus() override;
	uint32_t getTimeSinceLastClick() const;
	bool getIsActive() const;
	bool getIsClicked() const;
	const glm::ivec2& getSize() const;
	const glm::ivec2& getCenter() const;
};

#endif // SIMPLEWIDGET_H
