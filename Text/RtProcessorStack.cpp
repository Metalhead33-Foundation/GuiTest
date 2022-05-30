#include "RtProcessorStack.hpp"

RichTextProcessor* RtProcessorStack::getRTP() const
{
	return RTP;
}

void RtProcessorStack::setRTP(RichTextProcessor* newRTP)
{
	RTP = newRTP;
}

void RtProcessorStack::pushColour(const glm::fvec4& rgba)
{
	colourStack.push(rgba);
	if(RTP) RTP->setCurrentColour(colourStack.top());
}

void RtProcessorStack::pushColour(const glm::fvec3& rgb)
{
	colourStack.push(glm::fvec4(rgb,1.0f));
	if(RTP) RTP->setCurrentColour(colourStack.top());
}

void RtProcessorStack::pushColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	glm::fvec4 kernel;
	PixelRGBA_U8{ .r = r, .g = g, .b = b, .a = a }.toKernel(kernel);
	colourStack.push(kernel);
	if(RTP) RTP->setCurrentColour(colourStack.top());
}

void RtProcessorStack::pushFont(const std::string& fontname)
{
	fontStack.push(fontname);
	if(RTP) RTP->setCurrentFontName(fontStack.top());
}

void RtProcessorStack::pushSize(int size)
{
	sizeStack.push(size);
	if(RTP) RTP->setCurrentSize(sizeStack.top());
}

void RtProcessorStack::pushUnderscore()
{
	if(!underscoreStack) RTP->enableUnderline();
	++underscoreStack;
}

void RtProcessorStack::pushBold()
{
	if(!boldStack) RTP->enableBold();
	++boldStack;
}

void RtProcessorStack::pushItalic()
{
	if(!italicStack) RTP->enableItalic();
	++italicStack;
}

void RtProcessorStack::pushStrikethrough()
{
	if(!strikethroughStack) RTP->enableStrikethrough();
	++strikethroughStack;
}

void RtProcessorStack::popColour()
{
	if(!colourStack.empty()) colourStack.pop();
	if(RTP) {
		if(colourStack.empty()) RTP->resetColour();
		else RTP->setCurrentColour(colourStack.top());
	}
}

void RtProcessorStack::popFont()
{
	if(!fontStack.empty()) fontStack.pop();
	if(RTP) {
		if(fontStack.empty()) RTP->resetFont();
		else RTP->setCurrentFontName(fontStack.top());
	}
}

void RtProcessorStack::popSize()
{
	if(!sizeStack.empty()) sizeStack.pop();
	if(RTP) {
		if(sizeStack.empty()) RTP->resetSize();
		else RTP->setCurrentSize(sizeStack.top());
	}
}

void RtProcessorStack::popUnderscore()
{
	if(underscoreStack > 0) RTP->disableUnderline();
	else --underscoreStack;
}

void RtProcessorStack::popBold()
{
	if(boldStack > 0) RTP->disableBold();
	else --boldStack;
}

void RtProcessorStack::popItalic()
{
	if(italicStack > 0) RTP->disableItalic();
	else --italicStack;
}

void RtProcessorStack::popStrikethrough()
{
	if(strikethroughStack > 0) RTP->disableStrikethrough();
	else --strikethroughStack;
}

RtProcessorStack::RtProcessorStack() : colourStack(),
	fontStack(), sizeStack(),
	underscoreStack(0),
	boldStack(0),
	italicStack(0),
	strikethroughStack(0),
	RTP(nullptr)
{
}

RtProcessorStack::RtProcessorStack(RichTextProcessor* nRTP) : colourStack(),
	fontStack(), sizeStack(),
	underscoreStack(0),
	boldStack(0),
	italicStack(0),
	strikethroughStack(0),
	RTP(nRTP)
{
	if(RTP) colourStack.push(RTP->getDefaultColour());
}
