#include "Font.hpp"

Font::Font(FT_Face fontface)
{
	for (unsigned char c = 0; c < 128; c++) {
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
		characters.insert(std::pair<char, Character>(c, character));
	}
}

void Font::renderText(GuiRenderer& renderer, const std::string& text, const glm::fvec2& offset, float scale, const glm::fvec4& color)
{
	float x = offset.x;
	float y = offset.y;
	for(const auto c : text) {
		auto charIt = characters.find(c);
		if(charIt == std::end(characters)) continue;
		const glm::fvec2 pos = glm::fvec2(x + (charIt->second.bearing.x * scale),
										  y + ((charIt->second.size.y - charIt->second.bearing.y) * scale));
		const glm::fvec2 dim = glm::fvec2(charIt->second.size.x * scale,charIt->second.size.y * -scale);
		renderer.renderTex( pos, pos + dim, charIt->second.texture );
		x += (charIt->second.advance >> 6) * scale;
	}
}
