#ifndef TICKBOXWIDGET_H
#define TICKBOXWIDGET_H

#include "SimpleWidget.hpp"

class TickboxWidget : public SimpleWidget
{
private:
	int thickness;
public:
	TickboxWidget(const glm::fvec2& topLeft, const glm::fvec2& bottomRight,int thickness = 1);
	void render(GuiRenderer& renderer) override;
	int getThickness() const;
	void setThickness(int newThickness);
};

#endif // TICKBOXWIDGET_H
