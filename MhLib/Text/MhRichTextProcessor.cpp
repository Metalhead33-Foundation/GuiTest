#include "MhRichTextProcessor.hpp"
#include <MhLib/Util/MhNormDenorm.hpp>
#include <MhLib/Util/MhPixelFormat.hpp>

namespace MH33 {
namespace TXT {
int RichTextProcessor::getDefaultSize() const
{
	return defaultSize;
}

void RichTextProcessor::setDefaultSize(int newDefaultSize)
{
	defaultSize = newDefaultSize;
}

const std::string& RichTextProcessor::getDefaultFontName() const
{
	return defaultFontName;
}

void RichTextProcessor::setDefaultFontName(const std::string& newDefaultFontName)
{
	defaultFontName = newDefaultFontName;
}

const glm::fvec4& RichTextProcessor::getDefaultColour() const
{
	return defaultColour;
}

void RichTextProcessor::setDefaultColour(const glm::fvec4& newDefaultColour)
{
	defaultColour = newDefaultColour;
}

const glm::fvec4& RichTextProcessor::getCurrentColour() const
{
	return currentColour;
}

const std::string& RichTextProcessor::getCurrentFontName() const
{
	return currentFontName;
}

bool RichTextProcessor::getIsBold() const
{
	return isBold;
}

bool RichTextProcessor::getIsItalic() const
{
	return isItalic;
}

bool RichTextProcessor::getIsUnderline() const
{
	return isUnderline;
}

bool RichTextProcessor::getIsStrikethrough() const
{
	return isStrikethrough;
}

int RichTextProcessor::getCurrentSize() const
{
	return currentSize;
}

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
		currentBlock.colour = currentColour;
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
		rtp.setCurrentFontName(fontname);
	};
}

RichTextProcessor::RichTextManipulator RichTextProcessor::ChangeColour(const glm::fvec4& rgba)
{
	return [rgba](RichTextProcessor& rtp) {
		rtp.setCurrentColour(rgba);
	};
}

RichTextProcessor::RichTextManipulator RichTextProcessor::ChangeColour(const glm::fvec3& rgb)
{
	return [rgb](RichTextProcessor& rtp) {
		rtp.setCurrentColour(rgb);
	};
}

RichTextProcessor::RichTextManipulator RichTextProcessor::ChangeColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return [r,g,b,a](RichTextProcessor& rtp) {
		rtp.setCurrentColour(r,g,b,a);
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

void RichTextProcessor::resetColour()
{
	flush();
	currentColour = defaultColour;
}

void RichTextProcessor::resetFont()
{
	flush();
	currentFontName = defaultFontName;
}

void RichTextProcessor::resetSize()
{
	flush();
	currentSize = defaultSize;
}

void RichTextProcessor::setCurrentSize(int siz)
{
	currentSize = siz;
}

void RichTextProcessor::setCurrentFontName(const std::string& newfont)
{
	flush();
	currentFontName = newfont;
}

void RichTextProcessor::setCurrentColour(const glm::fvec4& rgba)
{
	flush();
	currentColour = rgba;
}

void RichTextProcessor::setCurrentColour(const glm::fvec3& rgb)
{
	flush();
	currentColour = glm::fvec4(rgb,1.0f);
}

void RichTextProcessor::setCurrentColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	Util::PixelRGBA_U8 rgba8 = { .r = r, .g = g, .b = b, .a = a };
	flush();
	rgba8.toKernel(currentColour);
}

RichTextProcessor::RichTextProcessor(const sFontRepository& repo)
	: fontRepo(repo), defaultSize(10), currentSize(10), currentColour(0.0f, 0.0f, 0.0f, 1.0f),
	  currentFontName("Noto"), defaultFontName("Noto"), isBold(false), isItalic(false), isUnderline(false), isStrikethrough(false)
{
	currentBlock.font = nullptr;
	/*int num = 0;
	for(auto it = std::begin(repo->getFonts()); it != std::end(repo->getFonts());++it) {
		++num;
	}*/
}

RichTextProcessor::RichTextProcessor(sFontRepository&& repo)
	: fontRepo(std::move(repo)), defaultSize(10), currentSize(10), currentColour(0.0f, 0.0f, 0.0f, 1.0f),
	  currentFontName("Noto"), defaultFontName("Noto"), isBold(false), isItalic(false), isUnderline(false), isStrikethrough(false)
{
	currentBlock.font = nullptr;
	/*int num = 0;
	for(auto it = std::begin(fontRepo->getFonts()); it != std::end(fontRepo->getFonts());++it) {
		++num;
	}*/
}
RichTextProcessor& RichTextProcessor::operator<<(char c) {
	sstrm << c;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(const char* s) {
	sstrm << s;
	return *this;
}
RichTextProcessor& RichTextProcessor::operator<<(char32_t c) {
	sstrm << convert.to_bytes(c);
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
RichTextProcessor& RichTextProcessor::operator<<(const RichTextManipulator& manipulator) {
	manipulator(*this);
	return *this;
}
}
}
