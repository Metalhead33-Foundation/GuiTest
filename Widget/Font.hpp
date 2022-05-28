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
class Font;
struct TextBlockUtf8 {
	std::string text;
	Font* font;
	glm::vec4 colour;
};
struct TextBlockUtf32 {
	std::u32string text;
	Font* font;
	glm::vec4 colour;
};

class Font
{
public:
	struct Character {
		bool valid;
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
	void insertCharacters(const std::pair<char32_t,char32_t>& range);
	void insertCharacters();
	sFreeTypeFace fontFace;
	sFreeTypeSystem sys;
	bool isBold;
	bool isItalic;
	Font(const Font& cpy) = delete;
	Font& operator=(const Font& cpy) = delete;
	void addCharacterFromBlock(char32_t c);
public:
	explicit Font(const sFreeTypeSystem& system, const sFreeTypeFace& fontface, bool bold = false, bool italic = false);
	explicit Font(const sFreeTypeSystem& system, sFreeTypeFace&& fontface, bool bold = false, bool italic = false);
	explicit Font(sFreeTypeSystem&& system, sFreeTypeFace&& fontface, bool bold = false, bool italic = false);
	explicit Font(Font&& mov);
	Font& operator=(Font&& mov);
	void renderText(GuiRenderer& renderer, const std::string& text, const glm::fvec2& offset, const glm::fvec2& reciprocalSize, float scale, const glm::fvec4& colour, int spacing = 8);
	void renderText(GuiRenderer& renderer, const std::u32string& text, const glm::fvec2& offset, const glm::fvec2& reciprocalSize, float scale, const glm::fvec4& colour, int spacing = 8);
	static void renderTextBlocks(GuiRenderer& renderer, const std::span<const TextBlockUtf8> textBlocks, const glm::fvec2& offset, const glm::fvec2& reciprocalSize, float scale, int spacing = 8);
	static void renderTextBlocks(GuiRenderer& renderer, const std::span<const TextBlockUtf32> textBlocks, const glm::fvec2& offset, const glm::fvec2& reciprocalSize, float scale, int spacing = 8);
	const TexGreyscale_U8& getTexture() const;
	bool getIsBold() const;
	bool getIsItalic() const;
};
typedef std::shared_ptr<Font> sFont;

#endif // FONT_HPP
