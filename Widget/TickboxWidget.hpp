#ifndef TICKBOXWIDGET_H
#define TICKBOXWIDGET_H

#include "SimpleWidget.hpp"

class TickboxWidget : public SimpleWidget
{
public:
	TickboxWidget(const glm::vec2& topLeft, const glm::vec2& bottomRight);
	void render(GuiRenderer& renderer) override;
};

#endif // TICKBOXWIDGET_H
