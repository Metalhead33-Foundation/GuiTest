#ifndef RICHTEXTPROCESSOR_HPP
#define RICHTEXTPROCESSOR_HPP
#include "FontRepository.hpp"
#include <sstream>
#include <locale>
#include <codecvt>
#include <cuchar>

class RichTextProcessor
{
private:
	std::vector<TextBlockUtf32> blocks;
	TextBlockUtf32 currentBlock;
	std::vector<glm::fvec4> colours;
	// Enable stuff
	void enableItalic();
	void enableBold();
	void enableUnderline();
	// Disable stuff
	void disableItalic();
	void disableBold();
	void disableUnderline();
	// Set stuff
	void setFontSize(int siz);
	void setFontFace(int face);
	void setFontColour(int clr);
	int defaultSize;
	int currentSize;
	int currentColour;
	int currentFont;
#ifdef NATIVE_STR32
	std::basic_stringstream<char32_t> sstrm;
#else
	std::stringstream sstrm;
#endif
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
	sFontRepository fontRepo;
	bool isBold;
	bool isItalic;
public:
	void flush();
	explicit RichTextProcessor(const sFontRepository& repo);
	explicit RichTextProcessor(sFontRepository&& repo);
	const std::vector<TextBlockUtf32>& getBlocks() const;
	std::vector<TextBlockUtf32>& getBlocks();
};
typedef std::shared_ptr<RichTextProcessor> sRichTextProcessor;

#endif // RICHTEXTPROCESSOR_HPP
