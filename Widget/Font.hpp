#ifndef FONT_HPP
#define FONT_HPP
#include <string>
#include <memory>
#include <map>
#include <glm/glm.hpp>
#include "../Texture/StandardTexture.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include "../Pipeline/GuiRenderer.hpp"

typedef std::shared_ptr<FT_LibraryRec_> sFreeTypeSystem;
typedef std::shared_ptr<FT_FaceRec> sFreeTypeFace;

class Font
{
public:
	struct Character {
		glm::ivec2 offset; // Top-left pixel coordinate of the glyph.
		glm::ivec2 size;       // Size of glyph
		glm::ivec2 bearing;    // Offset from baseline to left/top of glyph
		unsigned int advance;    // Offset to advance to next glyph
	};
private:
	TexGreyscale_U8 texture;
	glm::ivec2 textureOffset;
	glm::ivec2 maxCharSizeSoFar;
	std::map<char32_t,Character> characters;
	void insertCharacters(FT_Face fontface, const std::pair<char32_t,char32_t>& range);
	void insertCharacters(FT_Face fontface);
	sFreeTypeSystem fontSys;
	sFreeTypeFace fontFace;
public:
	Font(const sFreeTypeSystem& fontSys, const sFreeTypeFace& fontface);
	Font(sFreeTypeSystem&& fontSys, sFreeTypeFace&& fontface);
	void renderText(GuiRenderer& renderer, const std::string& text, const glm::fvec2& offset, const glm::fvec2& reciprocalSize, float scale, const glm::fvec4& colour, int spacing = 8);
	void renderText(GuiRenderer& renderer, const std::u32string& text, const glm::fvec2& offset, const glm::fvec2& reciprocalSize, float scale, const glm::fvec4& colour, int spacing = 8);
	const TexGreyscale_U8& getTexture() const;
};
typedef std::shared_ptr<Font> sFont;

#endif // FONT_HPP
