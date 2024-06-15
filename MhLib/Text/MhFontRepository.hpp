#ifndef MHFONTREPOSITORY_H
#define MHFONTREPOSITORY_H
#include <MhLib/Text/MhFont.hpp>
#include <map>
#include <MhLib/IoSys/MhIoSystem.hpp>

namespace MH33 {
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
	MH33::Io::sSystem iosys;
protected:
	virtual sFont createFont(const MH33::Io::sDevice& iodev, const sFreeTypeSystem& system, unsigned fontSize, bool bold) const = 0;
public:
	FontRepository(const MH33::Io::sSystem& iosys);
	FontRepository(MH33::Io::sSystem&& iosys);
	void initializeFont(const std::string& fontName, const std::string& path);
	sFont getFont(const std::string& fontName, uint8_t flags) const;
	sFont getFont(const std::string& fontName, bool isBold = false) const;
	const NamedFontMap& getFonts() const;
	NamedFontMap& getFonts();
};

}
}

#endif // MHFONTREPOSITORY_H
