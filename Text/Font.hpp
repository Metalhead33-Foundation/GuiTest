#ifndef FONT_H
#define FONT_H
#include <string>
#include <memory>
#include <map>
#include <glm/glm.hpp>
#include <MhLib/Media/GFX/MhGFXResourceFactory.hpp>
#include <Renderer/Shared/GuiRenderer.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace TXT {

typedef std::shared_ptr<FT_LibraryRec_> sFreeTypeSystem;
typedef std::shared_ptr<FT_FaceRec> sFreeTypeFace;
class Font;

DEFINE_STRUCT(TextRenderState)
struct TextRenderState {
	glm::fvec2 reciprocalSize;
	glm::fvec2 currentOffset;
	glm::fvec2 originalOffset;
	float maxHeight;
	float scale;
	glm::fvec4 colour;
	int spacing;
	struct {
		bool isItalic : 1;
		bool isUnderline : 1;
		bool isStrikethrough : 1;
	} attributes;
};

DEFINE_STRUCT(TextRenderingContext)
struct TextRenderingContext {
	MH33::GFX::ColouredTexturedGuiTechnique textPipeline;
	MH33::GFX::ColouredGuiTechnique linePipeline;
	MH33::GFX::sIndexedMesh textMesh;
	MH33::GFX::sUnindexedMesh lineMesh;
	std::vector<Renderer::ColouredWidgetVert> lineCache;
	std::vector<Renderer::TexturedWidgetVert> textCache;
	std::vector<uint32_t> indexCache;
	TextRenderingContext(MH33::GFX::ResourceFactory& factory);
};

DEFINE_STRUCT(TextBlockUtf8)
struct TextBlockUtf8 {
	std::string text;
	Font* font;
	glm::vec4 colour;
	bool isItalic;
	bool isUnderline;
	bool isStrikethrough;
};
DEFINE_STRUCT(TextBlockUtf32)
struct TextBlockUtf32 {
	std::u32string text;
	Font* font;
	glm::vec4 colour;
	bool isItalic;
	bool isUnderline;
	bool isStrikethrough;
};

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
private:
	MH33::GFX::sWriteableTexture2D texture;
	glm::ivec2 textureOffset;
	glm::ivec2 maxCharSizeSoFar;
	std::map<char32_t,Character> characters;
	void insertCharacters(const std::pair<char32_t,char32_t>& range);
	void insertCharacters();
	sFreeTypeFace fontFace;
	sFreeTypeSystem sys;
	bool isBold;
	Font(const Font& cpy) = delete;
	Font& operator=(const Font& cpy) = delete;
	void addCharacterFromBlock(char32_t c);
public:
	explicit Font(MH33::GFX::ResourceFactory& factory, const sFreeTypeSystem& system, const sFreeTypeFace& fontface, bool bold = false);
	explicit Font(MH33::GFX::ResourceFactory& factory, const sFreeTypeSystem& system, sFreeTypeFace&& fontface, bool bold = false);
	explicit Font(MH33::GFX::ResourceFactory& factory, sFreeTypeSystem&& system, sFreeTypeFace&& fontface, bool bold = false);
	explicit Font(Font&& mov);
	Font& operator=(Font&& mov);
	const MH33::GFX::sWriteableTexture2D& getTexture() const;
	void renderText(TextRenderingContext& renderingContext, const std::string& text, TextRenderState& state);
	void renderText(TextRenderingContext& renderingContext, const std::u32string& text, TextRenderState& state);
	static void renderTextBlocks(TextRenderingContext& renderingContext, const std::span<const TextBlockUtf8> textBlocks, const glm::fvec2& offset, const glm::fvec2& reciprocalSize, float scale, int spacing = 8);
	static void renderTextBlocks(TextRenderingContext& renderingContext, const std::span<const TextBlockUtf32> textBlocks, const glm::fvec2& offset, const glm::fvec2& reciprocalSize, float scale, int spacing = 8);
	bool getIsBold() const;
	bool getIsItalic() const;
};
typedef std::shared_ptr<Font> sFont;

}

#endif // FONT_H
