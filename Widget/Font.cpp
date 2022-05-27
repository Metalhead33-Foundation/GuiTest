#include "Font.hpp"
#include <locale>
#include <codecvt>
#include <cuchar>

Font::Font(FT_Face fontface)
{
	for (int c = 0; c < 255; c++) {
		if (FT_Load_Char(fontface, c, FT_LOAD_RENDER))
		{
			continue;
		}
		Character character = {
			.texture = TexGreyscale_U8(reinterpret_cast<PixelGreyscale_U8*>(fontface->glyph->bitmap.buffer),
			fontface->glyph->bitmap.width,fontface->glyph->bitmap.rows),
			.size = glm::ivec2(fontface->glyph->bitmap.width,fontface->glyph->bitmap.rows),
			.bearing = glm::ivec2(fontface->glyph->bitmap_left,fontface->glyph->bitmap_top),
			.advance = static_cast<unsigned int>(fontface->glyph->advance.x),
		};
		characters.insert(std::pair<char32_t, Character>(c, character));
	}
}

void Font::renderText(GuiRenderer& renderer, const std::string& text, const glm::fvec2& offset,const glm::fvec2& reciprocalSize, float scale, const glm::fvec4& color, int spacing)
{
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
	renderText(renderer,convert.from_bytes(text.c_str()),offset,reciprocalSize,scale,color,spacing);
}

void Font::renderText(GuiRenderer& renderer, const std::u32string& text, const glm::fvec2& offset, const glm::fvec2& reciprocalSize, float scale, const glm::fvec4& color, int spacing)
{
	float x = offset.x;
	float y = offset.y;
	float maxHeight = 0;
	for(const auto c : text) {
		if(c == '\n') {
			x = offset.x;
			y += maxHeight + (reciprocalSize.y * static_cast<float>(spacing) * scale);
			continue;
		}
		auto charIt = characters.find(c);
		if(charIt == std::end(characters)) continue;

		const glm::fvec2 pos = glm::fvec2(x + (charIt->second.bearing.x * reciprocalSize.x * scale),
										  y + ((charIt->second.size.y - charIt->second.bearing.y) * reciprocalSize.y * scale));
		const glm::fvec2 dim = glm::fvec2(charIt->second.size.x * scale,charIt->second.size.y * -scale) * reciprocalSize;
		maxHeight = std::max(maxHeight,dim.y * -1.0f);
		renderer.renderTex( pos, pos + dim, charIt->second.texture );
		x += (charIt->second.advance >> 6) * reciprocalSize.x * scale;
	}
}
