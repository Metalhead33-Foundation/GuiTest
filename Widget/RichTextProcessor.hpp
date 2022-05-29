#ifndef RICHTEXTPROCESSOR_HPP
#define RICHTEXTPROCESSOR_HPP
#include "FontRepository.hpp"
#include <sstream>
#include <locale>
#include <codecvt>
#include <cuchar>
#include <ostream>

class RichTextProcessor
{
public:
	typedef std::function<void(RichTextProcessor&)> RichTextManipulator;
private:
	std::vector<TextBlockUtf32> blocks;
	TextBlockUtf32 currentBlock;
	int defaultSize;
	int currentSize;
	PixelRGBA_U8 currentColour;
	std::string currentFontName;
	std::stringstream sstrm;
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
	sFontRepository fontRepo;
	bool isBold;
	bool isItalic;
	bool isUnderline;
	bool isStrikethrough;
public:
	void flush();
	explicit RichTextProcessor(const sFontRepository& repo);
	explicit RichTextProcessor(sFontRepository&& repo);
	const std::vector<TextBlockUtf32>& getBlocks() const;
	std::vector<TextBlockUtf32>& getBlocks();
	// Enable stuff
	void enableItalic();
	void enableBold();
	void enableUnderline();
	void enableStrikethrough();
	// Disable stuff
	void disableItalic();
	void disableBold();
	void disableUnderline();
	void disableStrikethrough();
	// Set stuff
	void setFontSize(int siz);
	void setFontFace(const std::string& newfont);
	void setFontColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF);
	// Manipulators
	static RichTextManipulator ChangeFont(const std::string& fontname);
	static RichTextManipulator ChangeColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF);
	static RichTextManipulator EnableItalic();
	static RichTextManipulator EnableBold();
	static RichTextManipulator EnableUnderline();
	static RichTextManipulator EnableStrikethrough();
	static RichTextManipulator DisableItalic();
	static RichTextManipulator DisableBold();
	static RichTextManipulator DisableUnderline();
	static RichTextManipulator DisableStrikethrough();
	// Operators
	RichTextProcessor& operator<<(bool val);
	RichTextProcessor& operator<<(short val);
	RichTextProcessor& operator<<(unsigned short val);
	RichTextProcessor& operator<<(int val);
	RichTextProcessor& operator<<(unsigned int val);
	RichTextProcessor& operator<<(long val);
	RichTextProcessor& operator<<(unsigned long val);
	RichTextProcessor& operator<<(float val);
	RichTextProcessor& operator<<(double val);
	RichTextProcessor& operator<<(long double val);
	RichTextProcessor& operator<<(void* val);
	RichTextProcessor& operator<<(signed char c);
	RichTextProcessor& operator<<(unsigned char c);
	RichTextProcessor& operator<<(const char* s);
	RichTextProcessor& operator<<(const std::string& s);
	RichTextProcessor& operator<<(std::ostream& (*pf)(std::ostream&));
	RichTextProcessor& operator<<(std::ios& (*pf)(std::ios&));
	RichTextProcessor& operator<<(std::ios_base& (*pf)(std::ios_base&));
	RichTextProcessor& operator<<(RichTextProcessor& (*pf)(RichTextProcessor&));
	RichTextProcessor& operator<<(const RichTextManipulator& manipulator);
};
typedef std::shared_ptr<RichTextProcessor> sRichTextProcessor;

#endif // RICHTEXTPROCESSOR_HPP
