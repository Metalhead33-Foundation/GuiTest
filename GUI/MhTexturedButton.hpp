#ifndef MHTEXTUREDBUTTON_HPP
#define MHTEXTUREDBUTTON_HPP
#include <GUI/MhTexturedWidget.hpp>
namespace MH33 {
namespace GUI {
class TexturedButton : public TexturedWidget
{
public:
	TexturedButton();

	// Renderable interface
public:
	void iterateOverChildren(const ChildIterator& iterator) override;
	void iterateOverChildren(const ConstChildIterator& iterator) const override;

	// Widget interface
protected:
	void timedUpdate(float deltaTime) override;

public:
	bool onClick(const glm::fvec2& offset, uint8_t button, uint8_t mousestate, uint8_t clicks) override;
};
}
}

#endif // MHTEXTUREDBUTTON_HPP
