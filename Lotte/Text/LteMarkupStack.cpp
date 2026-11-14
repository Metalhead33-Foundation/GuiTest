#include "LteMarkupStack.hpp"

#include <utility>

namespace Lotte {
namespace Text {

MarkupStack::MarkupStack(RichTextProcessor* processor)
{
	setProcessor(processor);
}

void MarkupStack::setProcessor(RichTextProcessor* newProcessor)
{
	processor = newProcessor;
	if (processor && colourStack.empty()) {
		colourStack.push_back(processor->getDefaultAttributes().colour);
	}
}

void MarkupStack::pushColour(const Colour& rgba)
{
	colourStack.push_back(rgba);
	if (processor) processor->setCurrentColour(colourStack.back());
}

void MarkupStack::pushColour(const glm::fvec3& rgb)
{
	pushColour(Colour(rgb, 1.0f));
}

void MarkupStack::pushColour(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
{
	pushColour(Colour(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f, float(a) / 255.0f));
}

void MarkupStack::pushFont(std::string familyName)
{
	fontStack.push_back(std::move(familyName));
	if (processor) processor->setCurrentFontName(fontStack.back());
}

void MarkupStack::pushSize(float size)
{
	sizeStack.push_back(size);
	if (processor) processor->setCurrentSize(sizeStack.back());
}

void MarkupStack::pushUnderline()
{
	if (!underlineDepth && processor) processor->enableUnderline();
	++underlineDepth;
}

void MarkupStack::pushBold()
{
	if (!boldDepth && processor) processor->enableBold();
	++boldDepth;
}

void MarkupStack::pushItalic()
{
	if (!italicDepth && processor) processor->enableItalic();
	++italicDepth;
}

void MarkupStack::pushStrikethrough()
{
	if (!strikethroughDepth && processor) processor->enableStrikethrough();
	++strikethroughDepth;
}

void MarkupStack::popColour()
{
	if (!colourStack.empty()) colourStack.pop_back();
	if (!processor) return;
	if (colourStack.empty()) processor->resetColour();
	else processor->setCurrentColour(colourStack.back());
}

void MarkupStack::popFont()
{
	if (!fontStack.empty()) fontStack.pop_back();
	if (!processor) return;
	if (fontStack.empty()) processor->resetFont();
	else processor->setCurrentFontName(fontStack.back());
}

void MarkupStack::popSize()
{
	if (!sizeStack.empty()) sizeStack.pop_back();
	if (!processor) return;
	if (sizeStack.empty()) processor->resetSize();
	else processor->setCurrentSize(sizeStack.back());
}

void MarkupStack::popUnderline()
{
	if (!underlineDepth) return;
	--underlineDepth;
	if (!underlineDepth && processor) processor->disableUnderline();
}

void MarkupStack::popBold()
{
	if (!boldDepth) return;
	--boldDepth;
	if (!boldDepth && processor) processor->disableBold();
}

void MarkupStack::popItalic()
{
	if (!italicDepth) return;
	--italicDepth;
	if (!italicDepth && processor) processor->disableItalic();
}

void MarkupStack::popStrikethrough()
{
	if (!strikethroughDepth) return;
	--strikethroughDepth;
	if (!strikethroughDepth && processor) processor->disableStrikethrough();
}

}
}
