#ifndef LTETEXTLAYOUT_HPP
#define LTETEXTLAYOUT_HPP
/**
 * @file LteTextLayout.hpp
 * @brief Turns styled text runs into positioned glyphs and decoration lines.
 *
 * This is the salvage of `Font::renderText` and `Font::renderTextBlocks` from
 * `Text/Font.cpp` on the `NewBeginning` branch. Those methods mixed three
 * responsibilities: walking the string, measuring and positioning glyphs, and
 * issuing draw calls against a texture atlas. Only the middle one is portable,
 * so it is what lives here.
 *
 * @section lte_text_layout_algorithm The algorithm
 *
 * For every run, the pen starts at `state.cursor` and advances horizontally:
 *
 * - `\f` (form feed) is skipped.
 * - `\n` closes the current line: the decorations of that line are emitted,
 *   the pen moves down by the measured line height plus `spacing`, and the pen
 *   returns to `state.origin.x`. This is the original "maxHeight" behaviour: a
 *   line's height is the tallest glyph on it, not the font's nominal height, so
 *   mixed sizes do not overlap.
 * - A printable code point is looked up in the font face. `position` is
 *   `(penX + bearingX, penY + glyphHeight - bearingY)`, i.e. the bearing is
 *   measured the way the original FreeType code did, and the quad extends
 *   `size = (width, height)` pixels from there.
 * - Italic runs are sheared by shifting the bottom edge right by half the glyph
 *   height (`italicMagicNumber = 0.5` in the original). The shear is stored per
 *   glyph so the vertex stage can apply it.
 * - Underline and strikethrough are emitted once per line, spanning from the
 *   line start to the current pen, using fractions of the measured line height.
 *
 * @section lte_text_layout_changes Changes from the original
 *
 * - **Pixels, not normalized device coordinates.** The original stored
 *   `reciprocalSize` (1/viewport) in the cursor state and negated every `y` when
 *   writing vertices, because its renderer consumed NDC. Layout now works in
 *   pixels with `y` growing downwards, and the vertex stage applies the viewport
 *   transform. `TextRenderState::reciprocalSize` and its `colour` field are gone
 *   for that reason: colour is per run, and the viewport is not a layout
 *   concern.
 * - **No atlas, no draw calls.** `PlacedGlyph` carries the code point and the
 *   quad, not a texture coordinate. `LteFontAtlas.hpp` documents what the
 *   missing atlas has to supply to turn these into textured vertices.
 * - **`advance >> 6` is gone.** The original shifted the advance by six bits
 *   because FreeType reports advances in 26.6 fixed point. `StoredCharacter`
 *   stores `advanceX` in whole pixels, so shifting it would have divided the
 *   advance by 64.
 * - **Magic decoration offsets replaced.** The original used
 *   `maxHeight * 0.4` for strikethrough and `reciprocalSize.y * spacing * scale`
 *   for underline - the second is viewport-dependent, so the underline moved
 *   when the window was resized. Both are now fractions of the measured line
 *   height.
 * - **Missing glyphs are skipped, not rasterized.** The original lazily asked
 *   FreeType for a glyph that was not in the atlas yet. Faces are preprocessed
 *   here, so a missing code point simply produces no quad.
 * - **Kerning is loaded but not applied yet.** `PreprocessedFontFace::kerning`
 *   is parsed and available, but the unit of its offsets has to be confirmed
 *   against FontPacker before it can be applied, so this pass ignores it.
 */

#include <Lotte/LotteLib.hpp>
#include <Lotte/Text/LteFontFace.hpp>
#include <Lotte/Text/LteTextAttributes.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Lotte {
namespace Text {

/** @brief Horizontal shear applied to the bottom edge of an italic glyph. */
inline constexpr float ItalicShear = 0.5f;

/** @brief Strikethrough height as a fraction of the measured line height. */
inline constexpr float StrikethroughHeight = 0.5f;

/** @brief Underline height as a fraction of the measured line height. */
inline constexpr float UnderlineHeight = 1.0f;

/** @brief Layout-resolved styling referenced by glyphs and decoration lines. */
struct TextStyle {
	/** @brief Colour of the run. */
	Colour colour = Colour(0.0f, 0.0f, 0.0f, 1.0f);
	/** @brief Whether the run is italic. */
	bool italic = false;
	/** @brief Whether the run is underlined. */
	bool underline = false;
	/** @brief Whether the run is struck through. */
	bool strikethrough = false;

	/** @brief Compares every field. */
	bool operator==(const TextStyle&) const = default;
};

/** @brief One positioned glyph in layout space (pixels, `y` growing downwards). */
struct PlacedGlyph {
	/** @brief Code point to draw. */
	char32_t codePoint = U' ';
	/** @brief Top-left corner of the glyph quad. */
	glm::fvec2 position = glm::fvec2(0.0f);
	/** @brief Size of the glyph quad in layout units. */
	glm::fvec2 size = glm::fvec2(0.0f);
	/** @brief Horizontal shear applied to the bottom edge; `0` for upright text. */
	float shear = 0.0f;
	/** @brief Index into @ref TextLayout::styles. */
	std::uint32_t styleIndex = 0;
};

/** @brief A single underline or strikethrough segment. */
struct DecorationLine {
	/** @brief Start point. */
	glm::fvec2 from = glm::fvec2(0.0f);
	/** @brief End point. */
	glm::fvec2 to = glm::fvec2(0.0f);
	/** @brief Index into @ref TextLayout::styles. */
	std::uint32_t styleIndex = 0;
};

/** @brief Result of a layout pass, renderer-agnostic and atlas-agnostic. */
struct TextLayout {
	/** @brief Positioned glyphs in draw order. */
	std::vector<PlacedGlyph> glyphs;
	/** @brief Decoration segments in draw order. */
	std::vector<DecorationLine> lines;
	/** @brief Deduplicated styles referenced by glyphs and lines. */
	std::vector<TextStyle> styles;
	/** @brief Pen position after the last character, useful for caret placement. */
	glm::fvec2 endOffset = glm::fvec2(0.0f);
	/** @brief Layout-space `y` of the bottom edge of the last laid-out line. */
	float height = 0.0f;

	/** @brief Drops all glyphs, lines and styles while keeping the capacity. */
	void clear()
	{
		glyphs.clear();
		lines.clear();
		styles.clear();
		endOffset = glm::fvec2(0.0f);
		height = 0.0f;
	}
};

/** @brief Cursor carried between runs; the salvage of `TextRenderState`. */
struct TextLayoutState {
	/** @brief Left edge of the current line. */
	glm::fvec2 origin = glm::fvec2(0.0f);
	/** @brief Current pen position. */
	glm::fvec2 cursor = glm::fvec2(0.0f);
	/** @brief Tallest glyph measured on the current line, in layout units. */
	float lineHeight = 0.0f;
	/** @brief Extra gap inserted between lines, in layout units. */
	float spacing = 0.0f;
	/** @brief Global scale applied on top of each run's size. */
	float scale = 1.0f;
};

/** @brief Parameters for a full run-list layout pass. */
struct TextLayoutOptions {
	/** @brief Top-left corner of the text block. */
	glm::fvec2 origin = glm::fvec2(0.0f);
	/** @brief Extra gap inserted between lines, in layout units. */
	float spacing = 0.0f;
	/** @brief Global scale applied on top of each run's size. */
	float scale = 1.0f;
};

/**
 * @brief Returns the pixel size a font face's metrics are expressed in.
 * @param face Preprocessed font face.
 * @return `faceHeight` when present, otherwise the logical bitmap size, otherwise 1.
 * @note This is the divisor that converts a run's requested pixel size into a
 *       scale factor. Refine it once the atlas knows the true em size.
 */
inline float getReferencePixelSize(const PreprocessedFontFace& face)
{
	if (face.faceHeight > 0.0f) return face.faceHeight;
	if (face.bitmapLogicalSize > 0) return float(face.bitmapLogicalSize);
	return 1.0f;
}

/**
 * @brief Lays out one run into a shared layout and updates the cursor state.
 * @param face Preprocessed font face that supplies metrics for the run.
 * @param text Code points of the run.
 * @param attributes Styling of the run.
 * @param state Cursor state; mutated in place so runs chain correctly.
 * @param layout Destination layout; glyphs, lines and styles are appended.
 */
MH_LOTTE_API void layoutRun(const PreprocessedFontFace& face, std::u32string_view text,
							const TextAttributes& attributes, TextLayoutState& state, TextLayout& layout);

/**
 * @brief Lays out a whole run list.
 * @tparam FaceLookup Callable `(const std::string&, bool) -> const PreprocessedFontFace*`.
 * @param runs Styled runs in draw order.
 * @param faceLookup Resolves a family name and bold flag to a face; may return null.
 * @param options Layout parameters.
 * @return The composed layout. Runs whose face cannot be resolved are skipped.
 */
template <typename FaceLookup>
TextLayout layoutRuns(const TextRunList& runs, FaceLookup&& faceLookup, const TextLayoutOptions& options = TextLayoutOptions{})
{
	TextLayout layout;
	TextLayoutState state;
	state.origin = options.origin;
	state.cursor = options.origin;
	state.spacing = options.spacing;
	state.scale = options.scale;
	for (const TextRun& run : runs) {
		const PreprocessedFontFace* face = faceLookup(run.attributes.familyName, run.attributes.bold);
		if (!face) continue;
		layoutRun(*face, run.text, run.attributes, state, layout);
	}
	layout.endOffset = state.cursor;
	return layout;
}

}
}

#endif // LTETEXTLAYOUT_HPP
