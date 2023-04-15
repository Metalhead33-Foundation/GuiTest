#ifndef TICKBOXWIDGET_H
#define TICKBOXWIDGET_H

#include "SimpleWidget.hpp"

namespace SYS {
class TickboxWidget : public SimpleWidget
{
private:
	int thickness;
public:
	TickboxWidget(const glm::fvec2& topLeft, const glm::fvec2& bottomRight,int thickness = 1);
	static void assignPoints(MH33::GFX::GuiRenderingContext& renderer, const std::span<const glm::fvec2>& points);
	void render(MH33::GFX::GuiRenderingContext& renderer) override;
	int getThickness() const;
	void setThickness(int newThickness);
};
}

#endif // TICKBOXWIDGET_H
