#ifndef FONT_HPP
#define FONT_HPP
#include <memory>
#include <map>
#include <glm/glm.hpp>
#include "../Texture/StandardTexture.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include "../Pipeline/GuiRenderer.hpp"

class Font
{
public:
	struct Character {
		TexGreyscale_U8 texture; // The texture
		glm::ivec2 size;       // Size of glyph
		glm::ivec2 bearing;    // Offset from baseline to left/top of glyph
		unsigned int advance;    // Offset to advance to next glyph
	};
	std::map<char,Character> characters;
public:
	Font(FT_Face fontface);
	void renderText(GuiRenderer& renderer, const std::string& text, const glm::fvec2& offset, float scale, const glm::fvec4& color);
};
typedef std::shared_ptr<Font> sFont;

#endif // FONT_HPP