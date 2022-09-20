#ifndef FONTREPOSITORY_H
#define FONTREPOSITORY_H
#include "Font.hpp"
#include <array>

namespace TXT {
class FontRepository
{
public:
	typedef std::array<sFont,2> AttributedFontMap;
	typedef std::map<std::string,AttributedFontMap> NamedFontMap;
private:
	NamedFontMap fonts;
	sFreeTypeSystem sys;
public:
	FontRepository(const sFreeTypeSystem& bsys);
	FontRepository(sFreeTypeSystem&& bsys);
	void initializeFont(const std::string& fontName, const std::string& path);
	sFont getFont(const std::string& fontName, uint8_t flags) const;
	sFont getFont(const std::string& fontName, bool isBold = false) const;
	const NamedFontMap& getFonts() const;
	NamedFontMap& getFonts();
};
typedef std::shared_ptr<FontRepository> sFontRepository;
}
#endif // FONTREPOSITORY_H
