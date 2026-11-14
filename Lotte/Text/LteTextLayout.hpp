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
 * Coordinates are pixels with `y` growing downwards. The pen sits on the
 * **baseline** of the current line (`state.cursor.y` is that baseline), and a glyph
 * box extends *upwards* from it by `bearingY` and *downwards* by
 * `height - bearingY`. That is the convention the original code used: it computed
 * `pos.y = y + (size.y - bearing.y)` and then negated `y`, which puts the top of the
 * quad at `y - bearing.y`.
 *
 * For every run, the pen starts at `state.cursor` and advances horizontally:
 *
 * - `\f` (form feed) is skipped.
 * - `\n` closes the current line: the decorations of that line are emitted, the pen
 *   moves down by the line advance (the face's `faceHeight`, raised to the tallest
 *   glyph measured on the line) plus `spacing`, and the pen returns to
 *   `state.origin.x`. The original used the tallest glyph alone ("maxHeight"), which
 *   collapses lines that only contain flat glyphs; `faceHeight` is now the floor.
 * - **Kerning.** Before a glyph is placed, the pair
 *   `(previousCodePoint, codePoint)` is looked up in
 *   `PreprocessedFontFace::kerning` and the x offset is added to the pen. See
 *   @ref lte_text_layout_kerning for the unit and its limits.
 * - A printable code point is looked up in the face, and the glyph box is placed at
 *   `(penX + horiBearingX, penY - horiBearingY)` with size
 *   `(metricWidth, metricHeight)`, all scaled by the run scale. Those are the
 *   *metric* fields: FontPacker generates the SDF from the metric box, so the raw
 *   bitmap fields (`width`/`height`/`bearingX`/`bearingY`) are empty for
 *   outline-generated faces and are only used as a fallback.
 * - Italic runs are sheared by shifting the bottom edge right by half the glyph
 *   height (`italicMagicNumber = 0.5` in the original). The shear is stored per
 *   glyph so the vertex stage can apply it.
 * - Underline and strikethrough are emitted once per line, spanning from the line
 *   start to the current pen, as signed offsets from the baseline.
 * - The pen then advances by `horiAdvance` (pixels), falling back to `advance_x / 64`
 *   when the float field is absent, plus the kerning of the *next* pair, which is
 *   applied when that glyph is placed.
 *
 * @section lte_text_layout_sdf Square SDF normalization
 *
 * FontPacker normalizes every glyph before storing it: its
 * `FontOutlineDecompositionContext::translateToNewSize()` takes the glyph's metric
 * box `(horiBearingX, horiBearingY - metricHeight)` to
 * `(+metricWidth, +metricHeight)`, translates the outline into that box and then
 * scales it so the box fills a **square** of `internalProcessSize - 2 * padding`
 * pixels, with the padding added around it (and optionally Y flipped). The result is
 * downsampled to `bitmapSize` and encoded, so:
 *
 * - every glyph payload is a `bitmapSize` x `bitmapSize` image, regardless of the
 *   glyph's shape;
 * - the scale is **per axis and per glyph** (`scaleX = w / metricWidth`,
 *   `scaleY = h / metricHeight`), so the stored image is aspect-warped;
 * - the quad built here is `metricWidth x metricHeight`, which un-stretches the image
 *   in the shader again. Never derive the quad from the payload's dimensions - it is
 *   always square.
 *
 * Two side effects worth knowing: the face itself is rasterized at
 * `FT_Set_Pixel_Sizes(internalProcessSize - padding)` (a single padding, while the
 * SDF square subtracts two), and the padding baked into the payload is
 * `bitmapPadding * bitmapSize / bitmapLogicalSize` after the downsample.
 * `LteFontAtlas.hpp` lists what the atlas has to do with that.
 *
 * @section lte_text_layout_kerning Kerning units and limits
 *
 * FontPacker (<https://github.com/Metalhead33-Foundation/FontPacker>, see
 * `SdfGenerationContext::processFont`) fills `kerning[first][second]` from FreeType's
 * `FT_Get_Kerning()` with `FT_KERNING_DEFAULT` after
 * `FT_Set_Pixel_Sizes(face, size, size)`, converting the returned 26.6 vector with
 * `convert26_6ToDouble()`. The stored offsets are therefore **pixels at the face's
 * reference pixel size** - the same unit as `bearingX`, `width` and `faceHeight` - so
 * they are multiplied by the run scale and by nothing else. Only `x` is applied:
 * FreeType's horizontal kerning has no meaningful `y`, and the stored `y` is reserved
 * for vertical layout.
 *
 * Not every font carries kerning (`FT_HAS_KERNING()` can be false, and a pair with a
 * zero offset is not stored), so a missing entry means "no adjustment" and an empty
 * map is normal. Kerning is applied within a run only: a pair that straddles a run
 * boundary would have to be resolved against the *previous* run's face, and
 * `layoutRun()` does not see it.
 *
 * @section lte_text_layout_changes Changes from the original
 *
 * - **Pixels, not normalized device coordinates.** The original stored
 *   `reciprocalSize` (1/viewport) in the cursor state and negated every `y` when
 *   writing vertices, because its renderer consumed NDC. Layout now works in pixels
 *   with `y` growing downwards, and the vertex stage applies the viewport transform.
 *   `TextRenderState::reciprocalSize` and its `colour` field are gone for that
 *   reason: colour is per run, and the viewport is not a layout concern.
 * - **No atlas, no draw calls.** `PlacedGlyph` carries the code point and the quad,
 *   not a texture coordinate. `LteFontAtlas.hpp` documents what the missing atlas
 *   has to supply to turn these into textured vertices.
 * - **`advance_x` stays 26.6.** FontPacker writes `glyphSlot->advance.x` verbatim
 *   into `StoredCharacter::advanceX`, so it is a 26.6 fixed-point value; it is
 *   divided by 64 here, exactly like the original's `advance >> 6`. Note that
 *   `bearingX`, `bearingY`, `width` and `height` are *not* fixed point: they come
 *   from `bitmap_left`/`bitmap_top` and the bitmap dimensions.
 * - **Kerning applied.** The original ignored `Face::kerning` entirely; it is now
 *   applied per pair (see above).
 * - **Decoration offsets relative to the baseline.** The original used
 *   `maxHeight * 0.4` below the baseline for strikethrough and
 *   `reciprocalSize.y * spacing * scale` for underline - the second is
 *   viewport-dependent, so the underline moved when the window was resized. Both are
 *   now signed fractions of the measured line height from the baseline.
 * - **Missing glyphs are skipped, not rasterized.** The original lazily asked
 *   FreeType for a glyph that was not in the atlas yet. Faces are preprocessed here,
 *   so a missing code point simply produces no quad (and does not become the
 *   "previous glyph" for kerning).
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

/** @brief Strikethrough offset from the baseline, as a fraction of the line height (negative = above). */
inline constexpr float StrikethroughOffset = -0.4f;

/** @brief Underline offset from the baseline, as a fraction of the line height (positive = below). */
inline constexpr float UnderlineOffset = 0.1f;

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

/** @brief One fixed-point scale denominator: FontPacker stores `advance_x` as 26.6. */
inline constexpr float FixedPoint26_6 = 64.0f;

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
	/** @brief Pen position after the last character: (advance end, last baseline). */
	glm::fvec2 endOffset = glm::fvec2(0.0f);
	/** @brief Layout-space `y` of the bottom of the last line (baseline + descender). */
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
	/** @brief Current pen position; `y` is the baseline of the current line. */
	glm::fvec2 cursor = glm::fvec2(0.0f);
	/** @brief Tallest glyph bitmap measured on the current line, in layout units. */
	float lineHeight = 0.0f;
	/** @brief Extra gap inserted between lines, in layout units. */
	float spacing = 0.0f;
	/** @brief Global scale applied on top of each run's size. */
	float scale = 1.0f;
};

/** @brief Parameters for a full run-list layout pass. */
struct TextLayoutOptions {
	/** @brief Top-left corner of the text block; its `y` is the first baseline. */
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
 * @note This is the divisor that converts a run's requested pixel size into a scale
 *       factor, so a run at this size maps 1:1 onto the stored metrics. FontPacker
 *       measured `faceHeight` at `FT_Set_Pixel_Sizes(internalProcessSize - padding)`,
 *       i.e. one padding below `bitmapLogicalSize` - do not use the SDF square's edge
 *       as the reference size.
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
