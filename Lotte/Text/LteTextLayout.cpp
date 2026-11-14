#include "LteTextLayout.hpp"

#include <algorithm>

namespace Lotte {
namespace Text {

namespace {

/** @brief Geometry of one glyph, expressed in face pixels (the reference size). */
struct GlyphBox {
	/** @brief Ink width. */
	float width = 0.0f;
	/** @brief Ink height. */
	float height = 0.0f;
	/** @brief Horizontal offset of the ink box from the pen position. */
	float bearingX = 0.0f;
	/** @brief Vertical offset of the ink box from the baseline (positive = above). */
	float bearingY = 0.0f;
	/** @brief Horizontal advance, including side bearings. */
	float advance = 0.0f;
	/** @brief Whether the glyph has a drawable ink box. */
	bool drawable = false;
};

/**
 * @brief Extracts the geometry of one glyph from a stored character.
 * @param glyph Stored glyph data.
 * @return Geometry in face pixels.
 *
 * FontPacker normalizes every glyph by its **metric box** - `metricWidth`,
 * `metricHeight`, `horiBearingX`, `horiBearingY` - stretching that box onto a square
 * SDF, so those are the fields that describe the quad. `width`, `height`, `bearingX`
 * and `bearingY` are the *raw FreeType bitmap* fields (and `bitmap_left`/`bitmap_top`
 * are only filled once FreeType has rendered a bitmap), which means they are zero for
 * fonts produced from outlines. They are still honoured as a fallback so that faces
 * assembled from pre-rendered bitmaps keep working.
 *
 * The advance uses `horiAdvance` when present because it is already in pixels, while
 * `advanceX` is 26.6 fixed point - except on the vector-image glyph path, where
 * FontPacker stores a pixel value there too. Preferring the float field avoids that
 * inconsistency entirely.
 */
GlyphBox glyphBox(const StoredCharacter& glyph)
{
	GlyphBox box;
	if (glyph.metricWidth > 0.0f && glyph.metricHeight > 0.0f) {
		box.width = glyph.metricWidth;
		box.height = glyph.metricHeight;
		box.bearingX = glyph.horiBearingX;
		box.bearingY = glyph.horiBearingY;
	} else {
		box.width = float(glyph.width);
		box.height = float(glyph.height);
		box.bearingX = float(glyph.bearingX);
		box.bearingY = float(glyph.bearingY);
	}
	box.advance = glyph.horiAdvance > 0.0f ? glyph.horiAdvance : float(glyph.advanceX) / FixedPoint26_6;
	box.drawable = box.width > 0.0f && box.height > 0.0f;
	return box;
}

/** @brief Returns the style index for an attribute set, adding it when new. */
std::uint32_t findOrAddStyle(TextLayout& layout, const TextAttributes& attributes)
{
	TextStyle style;
	style.colour = attributes.colour;
	style.italic = attributes.italic;
	style.underline = attributes.underline;
	style.strikethrough = attributes.strikethrough;
	for (std::size_t i = 0; i < layout.styles.size(); ++i) {
		if (layout.styles[i] == style) return std::uint32_t(i);
	}
	layout.styles.push_back(style);
	return std::uint32_t(layout.styles.size() - 1);
}

/**
 * @brief Looks up the horizontal kerning adjustment between two code points.
 * @param face Face that owns the kerning table.
 * @param first Code point of the preceding glyph.
 * @param second Code point of the following glyph.
 * @return Kerning offset in face pixels; `0` when the pair is not kerned.
 *
 * FontPacker stores these offsets in pixels (see @ref lte_text_layout_kerning), so
 * the caller scales them like any other face metric. Most fonts have no kerning at
 * all, in which case the outer map is empty and the lookup stops immediately.
 */
float kerningOffset(const PreprocessedFontFace& face, char32_t first, char32_t second)
{
	const auto firstIt = face.kerning.find(std::uint32_t(first));
	if (firstIt == face.kerning.end()) return 0.0f;
	const auto secondIt = firstIt->second.find(std::uint32_t(second));
	if (secondIt == firstIt->second.end()) return 0.0f;
	return secondIt->second.x;
}

/**
 * @brief Emits the decoration segments of the line that just ended.
 * @param lineStartX Left edge the line started at.
 * @param penX Current pen position; the segment spans `lineStartX..penX`.
 * @param baselineY Baseline of the line, in layout space.
 * @param lineHeight Measured height of the line.
 * @param styleIndex Style referenced by the emitted segments.
 * @param attributes Styling of the run that ended the line.
 * @param layout Destination layout.
 */
void emitDecorations(float lineStartX, float penX, float baselineY, float lineHeight,
					 std::uint32_t styleIndex, const TextAttributes& attributes, TextLayout& layout)
{
	if (penX <= lineStartX) return;
	if (attributes.strikethrough) {
		const float y = baselineY + lineHeight * StrikethroughOffset;
		layout.lines.push_back(DecorationLine{ glm::fvec2(lineStartX, y), glm::fvec2(penX, y), styleIndex });
	}
	if (attributes.underline) {
		const float y = baselineY + lineHeight * UnderlineOffset;
		layout.lines.push_back(DecorationLine{ glm::fvec2(lineStartX, y), glm::fvec2(penX, y), styleIndex });
	}
}

} // namespace

void layoutRun(const PreprocessedFontFace& face, std::u32string_view text, const TextAttributes& attributes,
			   TextLayoutState& state, TextLayout& layout)
{
	const float referenceSize = getReferencePixelSize(face);
	const float requestedSize = attributes.size > 0.0f ? attributes.size : referenceSize;
	const float runScale = state.scale * (requestedSize / referenceSize);
	const std::uint32_t styleIndex = findOrAddStyle(layout, attributes);

	const float lineStartX = state.origin.x;
	float penX = state.cursor.x;
	float penY = state.cursor.y;
	char32_t previousCodePoint = 0;

	for (char32_t codePoint : text) {
		if (codePoint == U'\f') continue;
		if (codePoint == U'\n') {
			emitDecorations(lineStartX, penX, penY, state.lineHeight, styleIndex, attributes, layout);
			// The face's nominal line height is a better baseline-to-baseline distance
			// than the tallest bitmap alone: a line of flat glyphs would otherwise
			// collapse onto the next one.
			const float nominalHeight = face.faceHeight > 0.0f ? face.faceHeight * runScale : 0.0f;
			penY += std::max(state.lineHeight, nominalHeight) + state.spacing;
			penX = lineStartX;
			state.lineHeight = 0.0f;
			previousCodePoint = 0;
			continue;
		}

		const auto glyphIt = face.glyphs.find(std::uint32_t(codePoint));
		if (glyphIt == face.glyphs.end() || !glyphIt->second.valid) continue;
		const StoredCharacter& glyph = glyphIt->second;

		// Kerning belongs to the pair, so it is applied when the second glyph is placed.
		if (previousCodePoint != 0) penX += kerningOffset(face, previousCodePoint, codePoint) * runScale;
		previousCodePoint = codePoint;

		const GlyphBox box = glyphBox(glyph);
		if (box.drawable) {
			PlacedGlyph placed;
			placed.codePoint = codePoint;
			// `bearingY` is the height of the ink box above the baseline, and `y` grows
			// downwards, so the box starts above the pen.
			placed.position = glm::fvec2(penX + box.bearingX * runScale, penY - box.bearingY * runScale);
			placed.size = glm::fvec2(box.width * runScale, box.height * runScale);
			placed.shear = attributes.italic ? placed.size.y * ItalicShear : 0.0f;
			placed.styleIndex = styleIndex;
			layout.glyphs.push_back(placed);
			state.lineHeight = std::max(state.lineHeight, placed.size.y);
		}

		// Bitmap-less glyphs (spaces, combining marks) still advance the pen.
		penX += box.advance * runScale;
	}

	emitDecorations(lineStartX, penX, penY, state.lineHeight, styleIndex, attributes, layout);

	state.cursor = glm::fvec2(penX, penY);
	const float descent = face.descender < 0.0f ? -face.descender * runScale : 0.0f;
	layout.height = std::max(layout.height, penY + descent);
}

}
}
