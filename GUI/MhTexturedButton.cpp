#include "MhTexturedButton.hpp"
#include <SDL2/SDL.h>
namespace MH33 {
namespace GUI {

TexturedButton::TexturedButton(void* textureAtlasId)
	: TexturedWidget(textureAtlasId)
{

}

TexturedButton::TexturedButton(void* textureAtlasId, const std::span<const TextureCoordDuo>& coordDuos)
	: TexturedWidget(textureAtlasId, coordDuos)
{

}

TexturedButton::TexturedButton(void* textureAtlasId, const std::initializer_list<const TextureCoordDuo>& coordDuos)
	: TexturedWidget(textureAtlasId, coordDuos)
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
	if(getFlag(WidgetStateFlags::ENABLED) && mousestate == SDL_PRESSED) {
		bool isAlreadyClicked = getFlag(WidgetStateFlags::CLICKED);
		setFlag(!isAlreadyClicked,WidgetStateFlags::CLICKED);
	}
	return true;
}

}
}
