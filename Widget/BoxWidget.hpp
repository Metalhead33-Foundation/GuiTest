#ifndef BOXWIDGET_H
#define BOXWIDGET_H

#include "SimpleWidget.hpp"

class BoxWidget : public SimpleWidget
{
public:
	BoxWidget(const glm::vec2& topLeft, const glm::vec2& bottomRight);
	void render(SDL_Renderer& renderer, glm::ivec4 viewport) override;
};

#endif // BOXWIDGET_H
