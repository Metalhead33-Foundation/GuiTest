#include "RichTextProcessor.hpp"
#include "../Util/NormDenorm.hpp"

void RichTextProcessor::flush()
{
	auto tmpStr = sstrm.str();
	if(!tmpStr.empty()) {
		sstrm.str(std::string());
		currentBlock.text = convert.from_bytes(tmpStr);
		currentBlock.font = fontRepo->getFont(currentFontName,isBold).get();
		currentBlock.isItalic = isItalic;
		currentBlock.isUnderline = isUnderline;
		currentBlock.isStrikethrough = isStrikethrough;
		currentColour.toKernel(currentBlock.colour);
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

RichTextProcessor::RichTextManipulator RichTextProcessor::ChangeFont(const std::string& fontname)
{
	return [&fontname](RichTextProcessor& rtp) {
		rtp.setFontFace(fontname);
	};
}

RichTextProcessor::RichTextManipulator RichTextProcessor::ChangeColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return [r,g,b,a](RichTextProcessor& rtp) {
		rtp.setFontColour(r,g,b,a);
	};
}

RichTextProcessor::RichTextManipulator RichTextProcessor::EnableItalic()
{
	return [](RichTextProcessor& rtp) {
		rtp.enableItalic();
	};
}
RichTextProcessor::RichTextManipulator RichTextProcessor::EnableBold()
{
	return [](RichTextProcessor& rtp) {
		rtp.enableBold();
	};
}
RichTextProcessor::RichTextManipulator RichTextProcessor::EnableUnderline()
{
	return [](RichTextProcessor& rtp) {
		rtp.enableUnderline();
	};
}

RichTextProcessor::RichTextManipulator RichTextProcessor::EnableStrikethrough()
{
	return [](RichTextProcessor& rtp) {
		rtp.enableStrikethrough();
	};
}
RichTextProcessor::RichTextManipulator RichTextProcessor::DisableItalic()
{
	return [](RichTextProcessor& rtp) {
		rtp.disableItalic();
	};
}
RichTextProcessor::RichTextManipulator RichTextProcessor::DisableBold()
{
	return [](RichTextProcessor& rtp) {
		rtp.disableBold();
	};
}
RichTextProcessor::RichTextManipulator RichTextProcessor::DisableUnderline()
{
	return [](RichTextProcessor& rtp) {
		rtp.disableUnderline();
	};
}

RichTextProcessor::RichTextManipulator RichTextProcessor::DisableStrikethrough()
{
	return [](RichTextProcessor& rtp) {
		rtp.disableStrikethrough();
	};
}

void RichTextProcessor::enableItalic()
{
	if(!isItalic) {
		flush();
		isItalic = true;
	}
}

void RichTextProcessor::enableBold()
{
	if(!isBold) {
		flush();
		isBold = true;
	}
}

void RichTextProcessor::enableUnderline()
{
	if(!isUnderline) {
		flush();
		isUnderline = true;
	}
}

void RichTextProcessor::enableStrikethrough()
{
	if(!isStrikethrough) {
		flush();
		isStrikethrough = true;
	}
}

void RichTextProcessor::disableItalic()
{
	if(isItalic) {
		flush();
		isItalic = false;
	}
}

void RichTextProcessor::disableBold()
{
	if(isBold) {
		flush();
		isBold = false;
	}
}

void RichTextProcessor::disableUnderline()
{
	if(isUnderline) {
		flush();
		isUnderline = false;
	}
}

void RichTextProcessor::disableStrikethrough()
{
	if(isStrikethrough) {
		flush();
		isStrikethrough = false;
	}
}

void RichTextProcessor::setFontSize(int siz)
{

}

void RichTextProcessor::setFontFace(const std::string& newfont)
{
	if(currentFontName != newfont) {
		flush();
		currentFontName = newfont;
	}
}

void RichTextProcessor::setFontColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	if(currentColour.r != r || currentColour.g != g || currentColour.b != b || currentColour.a != a) {
		flush();
		currentColour.r = r;
		currentColour.g = g;
		currentColour.b = b;
		currentColour.a = a;
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
	: fontRepo(repo), defaultSize(10), currentSize(10), currentColour{ .r = 255, .g = 255, .b = 255, .a = 255 },
	  currentFontName("Noto"), isBold(false), isItalic(false), isUnderline(false), isStrikethrough(false)
{
	currentBlock.font = nullptr;
	/*int num = 0;
	for(auto it = std::begin(repo->getFonts()); it != std::end(repo->getFonts());++it) {
		++num;
	}*/
}

RichTextProcessor::RichTextProcessor(sFontRepository&& repo)
	: fontRepo(std::move(repo)), defaultSize(10), currentSize(10), currentColour{ .r = 255, .g = 255, .b = 255, .a = 255 },
	  currentFontName("Noto"), isBold(false), isItalic(false), isUnderline(false), isStrikethrough(false)
{
	currentBlock.font = nullptr;
	/*int num = 0;
	for(auto it = std::begin(fontRepo->getFonts()); it != std::end(fontRepo->getFonts());++it) {
		++num;
	}*/
}
RichTextProcessor& RichTextProcessor::operator<<(bool val) {
	sstrm << val;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(short val) {
	sstrm << val;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(unsigned short val) {
	sstrm << val;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(int val) {
	sstrm << val;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(unsigned int val) {
	sstrm << val;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(long val) {
	sstrm << val;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(unsigned long val) {
	sstrm << val;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(float val) {
	sstrm << val;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(double val) {
	sstrm << val;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(long double val) {
	sstrm << val;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(void* val) {
	sstrm << val;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(signed char c) {
	sstrm << c;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(unsigned char c) {
	sstrm << c;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(const char* s) {
	sstrm << s;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(const std::string& s) {
	sstrm << s;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(std::ostream& (*pf)(std::ostream&)) {
	sstrm << pf;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(std::ios& (*pf)(std::ios&)) {
	sstrm << pf;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(std::ios_base& (*pf)(std::ios_base&)) {
	sstrm << pf;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(RichTextProcessor& (*pf)(RichTextProcessor&)) {
	return pf(*this);
}
RichTextProcessor& RichTextProcessor::operator<<(const RichTextManipulator& manipulator) {
	manipulator(*this);
	return *this;
}
