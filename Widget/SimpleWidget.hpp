#ifndef SIMPLEWIDGET_H
#define SIMPLEWIDGET_H

#include "IWidget.hpp"

namespace SYS {
class SimpleWidget : public IWidget
{
protected:
	glm::fvec2 topLeft, bottomRight;
private:
	uint32_t lastClick;
	bool isActive, isClicked;
public:
	SimpleWidget(const glm::fvec2& topLeft, const glm::fvec2& bottomRight);
	const glm::fvec2& getTopLeft() const override;
	void setTopLeft(const glm::fvec2& newTopLeft);
	const glm::fvec2& getBottomRight() const override;
	void setBottomRight(const glm::fvec2& newBottomRight);
	void setPosition(const glm::fvec2& topLeft, const glm::fvec2& bottomRight);
	void offsetPos(const glm::fvec2& posOffset);

	virtual bool onClick(const glm::fvec2& offset, uint8_t button, uint8_t mousestate, uint8_t clicks) override;
	virtual bool onHover(const glm::fvec2& offset, const glm::fvec2& relativePosToLast) override;
	void onOutOfFocus() override;
	uint32_t getTimeSinceLastClick() const;
	bool getIsActive() const;
	bool getIsClicked() const;
	const glm::ivec2& getSize() const;
	const glm::ivec2& getCenter() const;
};
}

#endif // SIMPLEWIDGET_H
