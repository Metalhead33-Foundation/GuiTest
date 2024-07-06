#include "MhTexturedButton.hpp"
namespace MH33 {
namespace GUI {

TexturedButton::TexturedButton()
{

}

void TexturedButton::iterateOverChildren(const ChildIterator& iterator)
{
	(void)iterator;
}

void TexturedButton::iterateOverChildren(const ConstChildIterator& iterator) const
{
	(void)iterator;
}

void TexturedButton::timedUpdate(float deltaTime)
{
	(void)deltaTime;
}

bool TexturedButton::onClick(const glm::fvec2& offset, uint8_t button, uint8_t mousestate, uint8_t clicks)
{
}

}
}
