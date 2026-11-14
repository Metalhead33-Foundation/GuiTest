#ifndef LTETEXTATTRIBUTES_HPP
#define LTETEXTATTRIBUTES_HPP
/**
 * @file LteTextAttributes.hpp
 * @brief Styling attributes and text runs shared by Lotte's rich-text pipeline.
 *
 * Salvaged from `Text/Font.hpp` (`TextRenderState`, `TextBlockUtf8`,
 * `TextBlockUtf32`) and `Text/RichTextProcessor.hpp` on the `NewBeginning`
 * branch. Two things changed, both forced by the font decision:
 *
 * - The blocks used to carry a `Font*` - an object that owned a FreeType face
 *   and a glyph texture atlas. Lotte deliberately has no such object; fonts are
 *   preprocessed `WODF` faces (`LteFontFace.hpp`) and no atlas exists yet. A run
 *   therefore names a font *family* and a *size*, and font resolution happens
 *   at layout time (@ref Lotte::Text::TextLayout in `LteTextLayout.hpp`).
 * - The two block types are merged into one run type over `std::u32string`.
 *   The UTF-8 variant only existed because `RichTextProcessor` buffered UTF-8
 *   in a `std::stringstream`; the conversion now happens at the input boundary
 *   (`LteUtf.hpp`), so everything downstream is UTF-32.
 *
 * `TextRenderState` from the same header is preserved as
 * @ref Lotte::Text::TextLayoutState in `LteTextLayout.hpp`, because it is the
 * cursor state that the salvaged rendering algorithm mutates.
 */

#include <Lotte/LotteLib.hpp>
#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace Lotte {
namespace Text {

/** @brief RGBA colour with normalized `[0,1]` components. */
using Colour = glm::fvec4;

/** @brief Attributes that can change between two runs of text. */
struct TextAttributes {
	/** @brief Text colour. */
	Colour colour = Colour(0.0f, 0.0f, 0.0f, 1.0f);
	/** @brief Lowercase font family name, as stored in a `WODF` face. */
	std::string familyName;
	/** @brief Requested pixel size of the face. */
	float size = 16.0f;
	/** @brief Bold weight request. */
	bool bold = false;
	/** @brief Italic (oblique shear) request. */
	bool italic = false;
	/** @brief Draw an underline below the run. */
	bool underline = false;
	/** @brief Strikethrough: draw a line through the run. */
	bool strikethrough = false;

	/** @brief Compares every styling field. */
	bool operator==(const TextAttributes&) const = default;
};

/**
 * @brief A contiguous piece of text that shares one attribute set.
 *
 * Attributes only change at run boundaries, which is what makes the run list a
 * cheap alternative to per-glyph state during layout.
 */
struct TextRun {
	/** @brief Text content. */
	std::u32string text;
	/** @brief Styling applied to the whole run. */
	TextAttributes attributes;
};

/** @brief Ordered list of styled runs produced by the rich-text pipeline. */
using TextRunList = std::vector<TextRun>;

}
}

#endif // LTETEXTATTRIBUTES_HPP
