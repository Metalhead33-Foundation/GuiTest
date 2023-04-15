#ifndef RICHTEXTPROCESSOR_H
#define RICHTEXTPROCESSOR_H
#include "FontRepository.hpp"
#include <sstream>
#include <locale>
#include <codecvt>
#include <cuchar>
#include <ostream>

namespace TXT {
DEFINE_CLASS(RichTextProcessor)
class RichTextProcessor
{
public:
	typedef std::function<void(RichTextProcessor&)> RichTextManipulator;
private:
	std::vector<TextBlockUtf32> blocks;
	TextBlockUtf32 currentBlock;
	int defaultSize;
	int currentSize;
	glm::fvec4 defaultColour;
	glm::fvec4 currentColour;
	std::string defaultFontName;
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
	// Set stuff and get
	void resetColour();
	void resetFont();
	void resetSize();
	void setCurrentSize(int siz);
	void setCurrentFontName(const std::string& newfont);
	void setCurrentColour(const glm::fvec4& rgba);
	void setCurrentColour(const glm::fvec3& rgb);
	void setCurrentColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF);
	int getDefaultSize() const;
	void setDefaultSize(int newDefaultSize);
	const std::string& getDefaultFontName() const;
	void setDefaultFontName(const std::string& newDefaultFontName);
	const glm::fvec4& getDefaultColour() const;
	void setDefaultColour(const glm::fvec4& newDefaultColour);
	const glm::fvec4& getCurrentColour() const;
	const std::string& getCurrentFontName() const;
	bool getIsBold() const;
	bool getIsItalic() const;
	bool getIsUnderline() const;
	bool getIsStrikethrough() const;
	int getCurrentSize() const;
	// Manipulators
	static RichTextManipulator ChangeFont(const std::string& fontname);
	static RichTextManipulator ChangeColour(const glm::fvec4& rgba);
	static RichTextManipulator ChangeColour(const glm::fvec3& rgb);
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
	RichTextProcessor& operator<<(char c);
	RichTextProcessor& operator<<(char32_t c);
	RichTextProcessor& operator<<(const char* s);
	RichTextProcessor& operator<<(const std::string& s);
	RichTextProcessor& operator<<(std::ostream& (*pf)(std::ostream&));
	RichTextProcessor& operator<<(std::ios& (*pf)(std::ios&));
	RichTextProcessor& operator<<(std::ios_base& (*pf)(std::ios_base&));
	RichTextProcessor& operator<<(const RichTextManipulator& manipulator);
};
}
#endif // RICHTEXTPROCESSOR_H
