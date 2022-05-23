#ifndef BOXWIDGET_H
#define BOXWIDGET_H

#include "SimpleWidget.hpp"

class BoxWidget : public SimpleWidget
{
public:
	BoxWidget(const glm::vec2& topLeft, const glm::vec2& bottomRight);
	void render(GuiRenderer& renderer) override;
};

#endif // BOXWIDGET_H
