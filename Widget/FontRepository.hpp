#ifndef FONTREPOSITORY_HPP
#define FONTREPOSITORY_HPP
#include "Font.hpp"
#include <array>

class FontRepository
{
public:
	typedef std::array<sFont,4> AttributedFontMap;
	typedef std::map<std::string,AttributedFontMap> NamedFontMap;
private:
	NamedFontMap fonts;
	sFreeTypeSystem sys;
public:
	FontRepository(const sFreeTypeSystem& bsys);
	FontRepository(sFreeTypeSystem&& bsys);
	void initializeFont(const std::string& fontName, const std::string& path);
	sFont getFont(const std::string& fontName, uint8_t flags) const;
	sFont getFont(const std::string& fontName, bool isBold = false, bool isItalic = false) const;
	const NamedFontMap& getFonts() const;
	NamedFontMap& getFonts();
};
typedef std::shared_ptr<FontRepository> sFontRepository;

#endif // FONTREPOSITORY_HPP
