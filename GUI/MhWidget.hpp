#ifndef MHWIDGET_HPP
#define MHWIDGET_HPP
#include <GUI/MhGuiRenderable.hpp>
#include <GUI/sigslot.h>
#include <glm/glm.hpp>

namespace MH33 {
namespace GUI {

DEFINE_CLASS(Renderer)
DEFINE_CLASS(Renderable)

enum class WidgetStateFlags : uint32_t {
	ENABLED = 1,
	CLICKED = 1 << 1,
	IN_MOUSE_FOCUS = 1 << 2,
	FULL_STATE = ENABLED | CLICKED | IN_MOUSE_FOCUS
};

DEFINE_CLASS(Widget)
class Widget : public Renderable
{
protected:
	bool hidden;
	uint32_t state;
	glm::vec2 topLeft, bottomRight;
	void changeState(uint32_t newState);
	virtual void timedUpdate(float deltaTime) = 0;
public:
	Widget();
	bool getFlag(WidgetStateFlags flag) const;
	void setFlag(bool state, WidgetStateFlags flag);
	const glm::vec2& getTopLeft() const;
	void setTopLeft(const glm::vec2& newTopLeft);
	const glm::vec2& getBottomRight() const;
	void setBottomRight(const glm::vec2& newBottomRight);
	void setNewPosition(const glm::vec2& newTopleft, const glm::vec2& newBottomRight);
	// Interface
	virtual bool onClick(const glm::fvec2& offset, uint8_t button, uint8_t mousestate, uint8_t clicks) = 0;
	virtual void render(Renderer& renderer) = 0;
	virtual void iterateOverChildren(const ChildIterator& iterator) = 0;
	virtual void iterateOverChildren(const ConstChildIterator& iterator) const = 0;
	// Signals
	sigslot::signal<pWidget,uint32_t,uint32_t> signal_onStateChanged;
	sigslot::signal<pWidget,const glm::vec2*, const glm::vec2*> signal_onPositionChanged;
	bool getHidden() const;
	void setHidden(bool newHidden);
};

}
}

#endif // MHWIDGET_HPP
