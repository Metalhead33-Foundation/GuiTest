#ifndef IWIDGET_H
#define IWIDGET_H
#include "../Pipeline/GuiRenderer.hpp"
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <memory>

class IWidget {
public:
	virtual ~IWidget() = default;
	virtual const glm::vec2& getTopLeft() const = 0;
	virtual const glm::vec2& getBottomRight() const = 0;
	//virtual bool getIsActive() const = 0;
	//virtual bool getIsClicked() const = 0;
	virtual void render(GuiRenderer& renderer) = 0;
	virtual bool onClick(const glm::vec2& offset, uint8_t button, uint8_t mousestate, uint8_t clicks) = 0;
	virtual bool onHover(const glm::vec2& offset, const glm::vec2& relativePosToLast) = 0;
	virtual void onOutOfFocus() = 0;
	static SDL_Rect posToSCreenspaceRect(const glm::vec2& topLeft, const glm::vec2 bottomRight, const glm::ivec4& viewport);
	static glm::ivec4 posToSCreenspaceRect2(const glm::vec2& topLeft, const glm::vec2 bottomRight, const glm::ivec4& viewport);
	static glm::vec2 getRelativePosFromAbs(const glm::vec2& topLeft, const glm::vec2 bottomRight, const glm::vec2 absPos);
	static glm::ivec2 getAbsolutePositionFromRel(const glm::vec2& pos, const glm::ivec4& viewport);
};
typedef std::shared_ptr<IWidget> sWidget;

#endif // IWIDGET_H
