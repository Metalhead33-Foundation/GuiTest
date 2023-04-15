#ifndef BOXWIDGET_H
#define BOXWIDGET_H
#include "SimpleWidget.hpp"
namespace SYS {
class BoxWidget : public SimpleWidget
{
public:
	BoxWidget(const glm::fvec2& topLeft, const glm::fvec2& bottomRight);
	void render(MH33::GFX::GuiRenderingContext& renderer) override;
};

}

#endif // BOXWIDGET_H
