#ifndef FONTREPOSITORY_H
#define FONTREPOSITORY_H
#include "Font.hpp"
#include <array>

namespace TXT {
DEFINE_CLASS(FontRepository)
class FontRepository
{
public:
	typedef std::array<sFont,2> AttributedFontMap;
	typedef std::map<std::string,AttributedFontMap> NamedFontMap;
private:
	NamedFontMap fonts;
	sFreeTypeSystem sys;
	MH33::GFX::sResourceFactory factory;
public:
	FontRepository(const sFreeTypeSystem& bsys, const MH33::GFX::sResourceFactory& factory);
	FontRepository(sFreeTypeSystem&& bsys, const MH33::GFX::sResourceFactory& factory);
	void initializeFont(const std::string& fontName, const std::string& path);
	sFont getFont(const std::string& fontName, uint8_t flags) const;
	sFont getFont(const std::string& fontName, bool isBold = false) const;
	const NamedFontMap& getFonts() const;
	NamedFontMap& getFonts();
	const MH33::GFX::sResourceFactory& getFactory() const;
};
}
#endif // FONTREPOSITORY_H
