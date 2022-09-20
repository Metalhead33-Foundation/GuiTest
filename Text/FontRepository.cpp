#include "FontRepository.hpp"

namespace TXT {
const FontRepository::NamedFontMap& FontRepository::getFonts() const
{
	return fonts;
}

FontRepository::NamedFontMap& FontRepository::getFonts()
{
	return fonts;
}

bool FontRepository::getIsAccelerated() const
{
	return isAccelerated;
}

FontRepository::FontRepository(const sFreeTypeSystem& bsys, bool accelerated) : sys(bsys), isAccelerated(accelerated)
{

}

FontRepository::FontRepository(sFreeTypeSystem&& bsys, bool accelerated) : sys(std::move(bsys)), isAccelerated(accelerated)
{

}

void FontRepository::initializeFont(const std::string& fontName, const std::string& path)
{
	AttributedFontMap fmap;
	for(uint8_t flags = 0; flags < 2; ++flags) {
		FT_Face face;
		if (FT_New_Face(sys.get(), path.c_str(), 0, &face)) {
			return;
		}
		FT_Set_Pixel_Sizes(face, 0, 48);
		sFreeTypeFace fac(face,FT_Done_Face);
		fmap[flags] = std::make_shared<Font>(sys,fac, flags & 0x01, isAccelerated);
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
}
