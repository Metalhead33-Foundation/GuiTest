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
 * - A printable code point is looked up in the face and its **intended layout box**
 *   is used: the whole-pixel `bearingX`/`bearingY`/`width`/`height` fields
 *   (`FT_GlyphSlot.bitmap_left`/`bitmap_top`/dimensions, or the SVG viewport).
 *   Faces assembled by hand may only carry the float metric box
 *   (`horiBearingX`/`horiBearingY`/`metricWidth`/`metricHeight`); that is the
 *   fallback. See @ref lte_text_layout_padding for the padding expansion.
 * - Italic runs are sheared by shifting the bottom edge right by half the glyph
 *   height (`italicMagicNumber = 0.5` in the original). The shear is stored per
 *   glyph so the vertex stage can apply it.
 * - Bold is a weight request, not a separate atlas. The repository may resolve it
 *   to a real face registered with a matching @ref Lotte::Text::FontWeight; otherwise the
 *   regular face is kept and the text shader dilates the SDF contour, which keeps
 *   one atlas and one pipeline for every weight.
 * - Underline and strikethrough are emitted as one segment per styled run, spanning
 *   only that run's text (not the whole line). This keeps mixed styles from
 *   painting a decoration under unrelated words.
 * - The pen then advances by `horiAdvance` (float pixels) and falls back to
 *   `advanceX` when the float field is absent. Version 3 stores both in pixels;
 *   versions 1/2 are decoded from raw 26.6 while importing, so no division
 *   happens here. `TextLayoutOptions::wordSpacing` adds an extra gap after each
 *   space. Kerning of the *next* pair is applied when that glyph is placed.
 *
 * @section lte_text_layout_padding Padding runway and the drawing quad
 *
 * Every stored tile is the square payload, and its padding is baked in: the
 * glyph content occupies the inner `(L - 2P) x (L - 2P)` box of the generation
 * canvas. The runtime must therefore draw the **whole tile** on a quad that is
 * larger than the intended box:
 *
 * ```
 * padX = width  * P / (L - 2P)
 * padY = height * P / (L - 2P)
 *
 * quadTopLeft = (penX + bearingX - padX, penY - bearingY - padY)
 * quadSize    = (width + 2 * padX,        height + 2 * padY)
 * ```
 *
 * The atlas maps the full tile (including its padding) onto this quad, so the
 * silhouette lands exactly on `width x height` at `bearingX`/`bearingY` while
 * glows, shadows and bevels get the `P_final = P * S / L` texel runway around
 * it. `PlacedGlyph::position`/`size` are already the expanded quad;
 * `PlacedGlyph::contentSize` keeps the unpadded layout box.
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
 * - the padded quad above un-stretches the image again. Never derive the quad
 *   from the payload's dimensions - it is always square.
 *
 * The face itself is rasterized at `FT_Set_Pixel_Sizes(internalProcessSize - padding)`,
 * so one padding is subtracted for the metrics and two for the SDF content box.
 * The padding baked into the payload is `bitmapPadding * bitmapSize / bitmapLogicalSize`
 * after the downsample. `LteFontAtlas.hpp` lists what the atlas has to do with that.
 *
 * @section lte_text_layout_units Units and scaling
 *
 * For font input every metric is a pixel value at `em = bitmapLogicalSize - bitmapPadding`,
 * so that em is the reference size: a run whose `TextAttributes::size` equals
 * `L - P` gets a scale of 1. `faceHeight` is the baseline-to-baseline distance,
 * **not** the em, and must not be used as the reference. For an SVG-backed face
 * (`familyName == "SVG"`) the metrics are SVG user units and `unitsPerEm` is the
 * viewport height, which is the reference size to scale against.
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
 *   not a texture coordinate. `LteFontAtlas.hpp` supplies the texture coordinates
 *   and page number through `FontAtlas`, and `LteTextRenderer.hpp` turns that into
 *   vertex/index data and draw commands.
 * - **Advances are float pixels.** FontPacker version 3 stores `advance_x` as a
 *   float; versions 1 and 2 are decoded from raw 26.6 by the font-face importer
 *   (with the version 1 SVG plain-pixel exception). `horiAdvance` is a float pixel
 *   value in every version and is preferred. Note that `bearingX`, `bearingY`,
 *   `width` and `height` are whole pixels, not fixed point: they come from
 *   `bitmap_left`/`bitmap_top` and the bitmap dimensions.
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
#include <type_traits>
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
	/** @brief Whether the run is bold. */
	bool bold = false;
	/** @brief Effective numeric weight of the run. */
	FontWeight weight = DefaultFontWeight;
	/** @brief Whether the run is underlined. */
	bool underline = false;
	/** @brief Whether the run is struck through. */
	bool strikethrough = false;

	/** @brief Weight with the compatibility @ref bold flag applied. */
	FontWeight effectiveWeight() const noexcept
	{
		return bold && !fontWeightIsBold(weight) ? FontWeight::Bold : weight;
	}

	/** @brief Compares every field. */
	bool operator==(const TextStyle&) const = default;
};

/** @brief One positioned glyph in layout space (pixels, `y` growing downwards). */
struct PlacedGlyph {
	/** @brief Code point to draw. */
	char32_t codePoint = U' ';
	/** @brief Top-left corner of the padded drawing quad (content box minus the runway). */
	glm::fvec2 position = glm::fvec2(0.0f);
	/** @brief Size of the padded drawing quad in layout units. */
	glm::fvec2 size = glm::fvec2(0.0f);
	/** @brief Size of the unpadded intended glyph box (the silhouette). */
	glm::fvec2 contentSize = glm::fvec2(0.0f);
	/** @brief Horizontal shear applied to the bottom edge; `0` for upright text. */
	float shear = 0.0f;
	/** @brief Effective em size of the run in layout pixels. */
	float emSize = 0.0f;
	/** @brief Index into @ref TextLayout::styles. */
	std::uint32_t styleIndex = 0;
	/** @brief Index into @ref TextLayout::faces. */
	std::uint32_t faceIndex = 0;
};

/** @brief A single underline or strikethrough segment. */
struct DecorationLine {
	/** @brief Start point. */
	glm::fvec2 from = glm::fvec2(0.0f);
	/** @brief End point. */
	glm::fvec2 to = glm::fvec2(0.0f);
	/** @brief Line thickness in layout pixels. */
	float thickness = 1.0f;
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
	/**
	 * @brief Non-owning faces referenced by @ref PlacedGlyph::faceIndex.
	 *
	 * The owner (a `FontRepository`, or the caller that supplied the lookup) must
	 * keep the faces alive for as long as the layout is used. Keeping the face in
	 * the layout is what lets a renderer pick the right atlas for each glyph when
	 * MML mixes families in one text block.
	 */
	std::vector<const PreprocessedFontFace*> faces;
	/** @brief Pen position after the last character: (advance end, last baseline). */
	glm::fvec2 endOffset = glm::fvec2(0.0f);
	/** @brief Layout-space `y` of the bottom of the last line (baseline + descender). */
	float height = 0.0f;

	/** @brief Drops all glyphs, lines, styles and face references while keeping the capacity. */
	void clear()
	{
		glyphs.clear();
		lines.clear();
		styles.clear();
		faces.clear();
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
	/** @brief Tallest unpadded glyph height measured on the current line, in layout units. */
	float lineHeight = 0.0f;
	/** @brief Extra gap inserted between lines, in layout units. */
	float spacing = 0.0f;
	/** @brief Extra gap inserted after every space character, in layout units. */
	float wordSpacing = 0.0f;
	/** @brief Global scale applied on top of each run's size. */
	float scale = 1.0f;
};

/** @brief Parameters for a full run-list layout pass. */
struct TextLayoutOptions {
	/** @brief Top-left corner of the text block; its `y` is the first baseline. */
	glm::fvec2 origin = glm::fvec2(0.0f);
	/** @brief Extra gap inserted between lines, in layout units. */
	float spacing = 0.0f;
	/** @brief Extra gap inserted after every space character, in layout units. */
	float wordSpacing = 0.0f;
	/** @brief Global scale applied on top of each run's size. */
	float scale = 1.0f;
};

/**
 * @brief Returns the pixel size a font face's metrics are expressed in.
 * @param face Preprocessed font face.
 * @return For an SVG-backed face, the viewport height (`unitsPerEm`); for a font
 *         face, `em = bitmapLogicalSize - bitmapPadding`; otherwise `faceHeight`,
 *         `bitmapLogicalSize`, or 1.
 * @note This is the divisor that converts a run's requested pixel size into a scale
 *       factor, so a run at this size maps 1:1 onto the stored metrics. It is the
 *       FreeType em size `L - P` (one padding below `bitmapLogicalSize`), not the
 *       baseline-to-baseline `faceHeight` and not the SDF square's edge.
 */
inline float getReferencePixelSize(const PreprocessedFontFace& face)
{
	if (face.isSvgBacked() && face.unitsPerEm > 0)
		return float(face.unitsPerEm);
	if (const std::uint32_t em = face.referenceEmSize(); em > 0)
		return float(em);
	if (face.faceHeight > 0.0f) return face.faceHeight;
	if (face.bitmapLogicalSize > 0) return float(face.bitmapLogicalSize);
	return 1.0f;
}

/**
 * @brief Returns the per-axis padding ratio `P / (L - 2P)` used to expand glyph quads.
 * @param face Preprocessed font face.
 * @return The ratio, or 0 when the geometry is missing or invalid (`2P >= L`).
 * @note Multiplying the intended box size by this ratio gives the extra runway the
 *       stored tile needs on each side. See @ref lte_text_layout_padding.
 */
inline float getGlyphPaddingRatio(const PreprocessedFontFace& face)
{
	const std::uint32_t logical = face.bitmapLogicalSize;
	const std::uint32_t padding = face.bitmapPadding;
	if (logical == 0 || padding == 0) return 0.0f;
	if (static_cast<std::uint64_t>(padding) * 2u >= logical) return 0.0f;
	return float(padding) / float(logical - 2u * padding);
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
 * @tparam FaceLookup Callable `(const std::string&, FontWeight) -> const PreprocessedFontFace*`
 *         or the legacy `(const std::string&, bool)` form.
 * @param runs Styled runs in draw order.
 * @param faceLookup Resolves a family name and weight to a face; may return null.
 *        Legacy bool lookups receive `attributes.bold`; weight-aware lookups receive
 *        `attributes.effectiveWeight()`.
 * @param options Layout parameters.
 * @return The composed layout. Runs whose face cannot be resolved are skipped.
 */
template <typename FaceLookup>
TextLayout layoutRuns(const TextRunList& runs, FaceLookup&& faceLookup,
					const TextLayoutOptions& options = TextLayoutOptions{})
{
	TextLayout layout;
	TextLayoutState state;
	state.origin = options.origin;
	state.cursor = options.origin;
	state.spacing = options.spacing;
	state.wordSpacing = options.wordSpacing;
	state.scale = options.scale;
	for (const TextRun& run : runs) {
		const PreprocessedFontFace* face = nullptr;
		if constexpr (std::is_invocable_v<FaceLookup&, const std::string&, FontWeight>) {
			face = faceLookup(run.attributes.familyName, run.attributes.effectiveWeight());
		} else {
			face = faceLookup(run.attributes.familyName, run.attributes.bold);
		}
		if (!face) continue;
		layoutRun(*face, run.text, run.attributes, state, layout);
	}
	layout.endOffset = state.cursor;
	return layout;
}

}
}

#endif // LTETEXTLAYOUT_HPP
