#ifndef IWIDGET_H
#define IWIDGET_H
#include <Renderer/Shared/GuiRenderer.hpp>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <memory>

namespace SYS {
class IWidget {
public:
	virtual ~IWidget() = default;
	virtual const glm::fvec2& getTopLeft() const = 0;
	virtual const glm::fvec2& getBottomRight() const = 0;
	virtual bool getIsActive() const = 0;
	virtual bool getIsClicked() const = 0;
	virtual void render(MH33::GFX::GuiRenderingContext& renderer) = 0;
	virtual bool onClick(const glm::fvec2& offset, uint8_t button, uint8_t mousestate, uint8_t clicks) = 0;
	virtual bool onHover(const glm::fvec2& offset, const glm::fvec2& relativePosToLast) = 0;
	virtual void onOutOfFocus() = 0;
	static SDL_Rect posToSCreenspaceRect(const glm::fvec2& topLeft, const glm::fvec2 bottomRight, const glm::ivec4& viewport);
	static glm::ivec4 posToSCreenspaceRect2(const glm::fvec2& topLeft, const glm::fvec2 bottomRight, const glm::ivec4& viewport);
	static glm::fvec2 getRelativePosFromAbs(const glm::fvec2& topLeft, const glm::fvec2 bottomRight, const glm::fvec2 absPos);
	static glm::ivec2 getAbsolutePositionFromRel(const glm::fvec2& pos, const glm::ivec4& viewport);
};
typedef std::shared_ptr<IWidget> sWidget;
}

#endif // IWIDGET_H
