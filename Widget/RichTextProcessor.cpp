#include "RichTextProcessor.hpp"
#include "../Util/NormDenorm.hpp"

void RichTextProcessor::flush()
{
	auto tmpStr = sstrm.str();
	if(!tmpStr.empty()) {
		sstrm.str(std::string());
#ifdef NATIVE_STR32
		currentBlock.text = std::move(tmpStr);
#else
		currentBlock.text = convert.from_bytes(tmpStr);
#endif
		//currentBlock.colour = getColour(currentColour);
		//currentBlock.font = getFont(currentFont).get();
		blocks.push_back(currentBlock);
	}
}

const std::vector<TextBlockUtf32>& RichTextProcessor::getBlocks() const
{
	return blocks;
}

std::vector<TextBlockUtf32>& RichTextProcessor::getBlocks()
{
	return blocks;
}

void RichTextProcessor::enableItalic()
{
	if(!isItalic) {
		isItalic = true;
		flush();
	}
}

void RichTextProcessor::enableBold()
{
	if(!isBold) {
		isBold = true;
		flush();
	}
}

void RichTextProcessor::enableUnderline()
{

}

void RichTextProcessor::disableItalic()
{
	if(isItalic) {
		isItalic = false;
		flush();
	}
}

void RichTextProcessor::disableBold()
{
	if(isBold) {
		isBold = false;
		flush();
	}
}

void RichTextProcessor::disableUnderline()
{

}

void RichTextProcessor::setFontSize(int siz)
{

}

void RichTextProcessor::setFontFace(int face)
{
	if(currentFont != face) {
		currentFont = face;
		flush();
	}
}

void RichTextProcessor::setFontColour(int clr)
{
	if(currentColour != clr) {
		currentColour = clr;
		flush();
	}
}
/*
	int defaultSize;
	int currentSize;
	int currentColour;
	int currentFont;
	bool isBold;
	bool isItalic;
*/

RichTextProcessor::RichTextProcessor(const sFontRepository& repo)
	: fontRepo(repo), defaultSize(10), currentSize(10), currentColour(0), currentFont(0), isBold(false), isItalic(false)
{
	colours.emplace_back(1.0f,1.0f,1.0f,1.0f);
	currentBlock.colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	currentBlock.font = nullptr;
	/*int num = 0;
	for(auto it = std::begin(repo->getFonts()); it != std::end(repo->getFonts());++it) {
		++num;
	}*/
}

RichTextProcessor::RichTextProcessor(sFontRepository&& repo)
	: fontRepo(std::move(repo)), defaultSize(10), currentSize(10), currentColour(0), currentFont(0), isBold(false), isItalic(false)
{
	//register_color(255,255,255);
	colours.emplace_back(1.0f,1.0f,1.0f,1.0f);
	currentBlock.colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	currentBlock.font = nullptr;
	/*int num = 0;
	for(auto it = std::begin(fontRepo->getFonts()); it != std::end(fontRepo->getFonts());++it) {
		++num;
	}*/
}
