#ifndef MHFONT_H
#define MHFONT_H
#include <MhLib/Text/MhTextBlock.hpp>
#include <MhLib/Io/MhIoDevice.hpp>
#include <span>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace MH33 {
namespace TXT {
typedef std::shared_ptr<FT_LibraryRec_> sFreeTypeSystem;
typedef std::shared_ptr<FT_FaceRec> sFreeTypeFace;

/*
		const glm::ivec2 glyphSize(fontFace->glyph->bitmap.width,fontFace->glyph->bitmap.rows);
		const std::span<const std::byte> bytes(reinterpret_cast<std::byte*>(fontFace->glyph->bitmap.buffer),glyphSize.x * glyphSize.y);
		maxCharSizeSoFar.x = std::max(maxCharSizeSoFar.x,glyphSize.x);
		maxCharSizeSoFar.y = std::max(maxCharSizeSoFar.y,glyphSize.y);
		const glm::ivec2 glyphBearing(fontFace->glyph->bitmap_left,fontFace->glyph->bitmap_top);
		const unsigned int glyphAdvance = static_cast<unsigned int>(fontFace->glyph->advance.x);
		const glm::ivec2 glyphOffset = textureOffset + glm::ivec2(1, 0);
		const glm::ivec2 intendedCorner = glyphOffset + glyphSize;
*/

DEFINE_CLASS(Font)
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
	typedef std::map<char32_t,Character> CharacterMap;
private:
	FT_Open_Args_ openArgs;
	FT_StreamRec stream;
	MH33::Io::uDevice iodev;
	CharacterMap characters;
	sFreeTypeFace fontFace;
	sFreeTypeSystem sys;
	void addCharacterFromBlock(char32_t c);
protected:
	bool isBold;
	bool isSdf;
	glm::ivec2 textureOffset;
	glm::ivec2 maxCharSizeSoFar;
	virtual void insertCharacterIntoBackend(Character& character,
											const std::span<const std::byte>& bytes,
											const glm::ivec2& glyphSize,
											const glm::ivec2& glyphBearing,
											unsigned int glyphAdvance,
											const glm::ivec2& glyphOffset,
											const glm::ivec2& intendedCorner) = 0;
	virtual void queueLineForRendering(const glm::fvec2& endA, const glm::fvec2& endB) = 0;
	virtual void queueGlyphForRendering(const Character& character, const glm::fvec2& pos1, const glm::fvec2& pos2, float xdiff = 0.0f) = 0;
	virtual void flushQueue() = 0;
public:
	virtual ~Font() = default;
	Font(MH33::Io::uDevice&& iodev, const sFreeTypeSystem& system, unsigned fontSize = 48, bool bold = false, bool isSdf = false);
	void renderText(const std::string& text, TextRenderState& state);
	void renderText(const std::u32string& text, TextRenderState& state);
	static void renderTextBlocks(const std::span<const TextBlockUtf8> textBlocks, const glm::fvec2& offset, const glm::fvec2& reciprocalSize, float scale, int spacing = 8);
	static void renderTextBlocks(const std::span<const TextBlockUtf32> textBlocks, const glm::fvec2& offset, const glm::fvec2& reciprocalSize, float scale, int spacing = 8);
	const CharacterMap& getCharacterMap() const;
	void insertCharacter(char32_t c);
	void insertCharacters(const std::pair<char32_t,char32_t>& range);
};

}
}
#endif // MHFONT_H
