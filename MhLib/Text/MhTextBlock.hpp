#ifndef MHTEXTBLOCK_H
#define MHTEXTBLOCK_H
#include <MhLib/Util/MhGlobals.hpp>
#include <string>
#include <glm/glm.hpp>

namespace MH33 {
namespace TXT {
DEFINE_CLASS(Font)

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
DEFINE_STRUCT(TextBlockUtf8)
struct TextBlockUtf8 {
	std::string text;
	pFont font;
	glm::vec4 colour;
	bool isItalic;
	bool isUnderline;
	bool isStrikethrough;
};
DEFINE_STRUCT(TextBlockUtf32)
struct TextBlockUtf32 {
	std::u32string text;
	pFont font;
	glm::vec4 colour;
	bool isItalic;
	bool isUnderline;
	bool isStrikethrough;
};

}
}

#endif // MHTEXTBLOCK_H
