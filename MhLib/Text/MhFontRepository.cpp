#include "MhFontRepository.hpp"
#include <MhLib/Io/MhIoDevice.hpp>

namespace MH33 {
namespace TXT {

FontRepository::NamedFontMap& FontRepository::getFonts()
{
	return fonts;
}

FontRepository::FontRepository(const Io::sSystem& iosys)
	: iosys(iosys)
{
	FT_Library ft;
	FT_Init_FreeType(&ft);
	sys = sFreeTypeSystem(ft,FT_Done_FreeType);
}

FontRepository::FontRepository(Io::sSystem&& iosys)
	: iosys(std::move(iosys))
{
	FT_Library ft;
	FT_Init_FreeType(&ft);
	sys = sFreeTypeSystem(ft,FT_Done_FreeType);
}

void FontRepository::initializeFont(const std::string& fontName, const std::string& path)
{
	AttributedFontMap fmap;
	for(uint8_t flags = 0; flags < 2; ++flags) {
		MH33::Io::uDevice iodev(iosys->open(path,MH33::Io::Mode::READ));
		fmap[flags] = createFont(std::move(iodev), sys, 48, flags & 0x01);
	}
	fonts.insert_or_assign(fontName,fmap);
}

sFont FontRepository::getFont(const std::string& fontName, uint8_t flags) const
{
	const auto iter = fonts.find(fontName);
	if(iter == std::end(fonts)) return nullptr;
	return iter->second[flags & 1];
}

sFont FontRepository::getFont(const std::string& fontName, bool isBold) const
{
	return getFont(fontName,static_cast<uint8_t>( uint8_t(isBold)  ) );
}

const FontRepository::NamedFontMap& FontRepository::getFonts() const
{
	return fonts;
}

}
}
