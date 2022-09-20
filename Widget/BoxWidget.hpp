#ifndef BOXWIDGET_H
#define BOXWIDGET_H

#include "SimpleWidget.hpp"
namespace SYS {
class BoxWidget : public SimpleWidget
{
public:
	BoxWidget(const glm::fvec2& topLeft, const glm::fvec2& bottomRight);
	void render(GuiRenderer& renderer) override;
};

}

#endif // BOXWIDGET_H
