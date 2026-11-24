#include "LteTextLayout.hpp"

#include <algorithm>

namespace Lotte {
namespace Text {

namespace {

/** @brief Geometry of one glyph, expressed in face pixels (the reference em size). */
struct GlyphBox {
	/** @brief Intended layout-box width. */
	float width = 0.0f;
	/** @brief Intended layout-box height. */
	float height = 0.0f;
	/** @brief Horizontal offset of the box from the pen position. */
	float bearingX = 0.0f;
	/** @brief Vertical offset of the box from the baseline (positive = above). */
	float bearingY = 0.0f;
	/** @brief Horizontal advance, including side bearings. */
	float advance = 0.0f;
	/** @brief Whether the glyph has a drawable box. */
	bool drawable = false;
};

/**
 * @brief Extracts the intended layout box and advance of one stored glyph.
 * @param glyph Stored glyph data.
 * @return Geometry in face pixels.
 *
 * FontPacker stores the **intended layout box** in `width`/`height`/`bearingX`/
 * `bearingY` (FreeType's bitmap dimensions, `bitmap_left` and `bitmap_top`), and
 * the box the SDF was stretched from in `metricWidth`/`metricHeight`/
 * `horiBearingX`/`horiBearingY`. The runtime draws the glyph at the intended box
 * (plus the padding runway), so those whole-pixel fields are preferred. Faces
 * assembled by hand may leave them zero and carry only the float metric box;
 * that is the fallback.
 *
 * The advance prefers `horiAdvance`, a float pixel value in every format version.
 * `advanceX` is float pixels in version 3 and is decoded from raw 26.6 on import
 * for versions 1/2, so both are comparable here.
 */
GlyphBox glyphBox(const StoredCharacter& glyph)
{
	GlyphBox box;
	if (glyph.width > 0 && glyph.height > 0) {
		box.width = float(glyph.width);
		box.height = float(glyph.height);
		box.bearingX = float(glyph.bearingX);
		box.bearingY = float(glyph.bearingY);
	} else {
		box.width = glyph.metricWidth;
		box.height = glyph.metricHeight;
		box.bearingX = glyph.horiBearingX;
		box.bearingY = glyph.horiBearingY;
	}
	box.advance = glyph.horiAdvance > 0.0f ? glyph.horiAdvance : glyph.advanceX;
	box.drawable = box.width > 0.0f && box.height > 0.0f;
	return box;
}

/** @brief Returns the style index for an attribute set, adding it when new. */
std::uint32_t findOrAddStyle(TextLayout& layout, const TextAttributes& attributes)
{
	TextStyle style;
	style.colour = attributes.colour;
	style.italic = attributes.italic;
	style.bold = attributes.isBold();
	style.weight = attributes.effectiveWeight();
	style.underline = attributes.underline;
	style.strikethrough = attributes.strikethrough;
	for (std::size_t i = 0; i < layout.styles.size(); ++i) {
		if (layout.styles[i] == style) return std::uint32_t(i);
	}
	layout.styles.push_back(style);
	return std::uint32_t(layout.styles.size() - 1);
}

/** @brief Returns the face index for a face, adding it when new. */
std::uint32_t findOrAddFace(TextLayout& layout, const PreprocessedFontFace& face)
{
	for (std::size_t i = 0; i < layout.faces.size(); ++i) {
		if (layout.faces[i] == &face) return std::uint32_t(i);
	}
	layout.faces.push_back(&face);
	return std::uint32_t(layout.faces.size() - 1);
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
 * @brief Emits the decoration segments of one run segment.
 * @param segmentStartX Left edge of the segment that just ended.
 * @param penX Current pen position; the segment spans `segmentStartX..penX`.
 * @param baselineY Baseline of the line, in layout space.
 * @param lineHeight Measured height of the line.
 * @param styleIndex Style referenced by the emitted segments.
 * @param attributes Styling of the run that ended the segment.
 * @param layout Destination layout.
 */
void emitDecorations(float segmentStartX, float penX, float baselineY, float lineHeight,
					 std::uint32_t styleIndex, const TextAttributes& attributes, TextLayout& layout)
{
	if (penX <= segmentStartX) return;
	const float thickness = std::max(1.0f, lineHeight * 0.05f);
	if (attributes.strikethrough) {
		const float y = baselineY + lineHeight * StrikethroughOffset;
		layout.lines.push_back(DecorationLine{ glm::fvec2(segmentStartX, y), glm::fvec2(penX, y),
											   thickness, styleIndex });
	}
	if (attributes.underline) {
		const float y = baselineY + lineHeight * UnderlineOffset;
		layout.lines.push_back(DecorationLine{ glm::fvec2(segmentStartX, y), glm::fvec2(penX, y),
											   thickness, styleIndex });
	}
}

} // namespace

void layoutRun(const PreprocessedFontFace& face, std::u32string_view text, const TextAttributes& attributes,
			   TextLayoutState& state, TextLayout& layout)
{
	const float referenceSize = getReferencePixelSize(face);
	const float requestedSize = attributes.size > 0.0f ? attributes.size : referenceSize;
	const float runScale = state.scale * (requestedSize / referenceSize);
	const float paddingRatio = getGlyphPaddingRatio(face);
	const std::uint32_t styleIndex = findOrAddStyle(layout, attributes);
	const std::uint32_t faceIndex = findOrAddFace(layout, face);
	const float emSize = requestedSize * state.scale;

	const float lineStartX = state.origin.x;
	float segmentStartX = state.cursor.x;
	float penX = state.cursor.x;
	float penY = state.cursor.y;
	char32_t previousCodePoint = 0;

	for (char32_t codePoint : text) {
		if (codePoint == U'\f') continue;
		if (codePoint == U'\n') {
			emitDecorations(segmentStartX, penX, penY, state.lineHeight, styleIndex, attributes, layout);
			// The face's nominal line height is a better baseline-to-baseline distance
			// than the tallest bitmap alone: a line of flat glyphs would otherwise
			// collapse onto the next one.
			const float nominalHeight = face.faceHeight > 0.0f ? face.faceHeight * runScale : 0.0f;
			penY += std::max(state.lineHeight, nominalHeight) + state.spacing;
			penX = lineStartX;
			segmentStartX = lineStartX;
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
			const float glyphWidth = box.width * runScale;
			const float glyphHeight = box.height * runScale;
			// The stored tile contains the padding runway, so the quad must be larger
			// than the intended box. See lte_text_layout_padding.
			const float padX = glyphWidth * paddingRatio;
			const float padY = glyphHeight * paddingRatio;
			PlacedGlyph placed;
			placed.codePoint = codePoint;
			// `bearingY` is the height of the box above the baseline, and `y` grows
			// downwards, so the padded quad starts above and left of the pen.
			placed.position = glm::fvec2(penX + box.bearingX * runScale - padX,
										 penY - box.bearingY * runScale - padY);
			placed.size = glm::fvec2(glyphWidth + 2.0f * padX, glyphHeight + 2.0f * padY);
			placed.contentSize = glm::fvec2(glyphWidth, glyphHeight);
			placed.shear = attributes.italic ? glyphHeight * ItalicShear : 0.0f;
			placed.emSize = emSize;
			placed.styleIndex = styleIndex;
			placed.faceIndex = faceIndex;
			layout.glyphs.push_back(placed);
			// Line metrics use the unpadded silhouette, not the glow runway.
			state.lineHeight = std::max(state.lineHeight, glyphHeight);
		}

		// Bitmap-less glyphs (spaces, combining marks) still advance the pen.
		penX += box.advance * runScale;
		if (codePoint == U' ') penX += state.wordSpacing;
	}

	emitDecorations(segmentStartX, penX, penY, state.lineHeight, styleIndex, attributes, layout);

	state.cursor = glm::fvec2(penX, penY);
	const float descent = face.descender < 0.0f ? -face.descender * runScale : 0.0f;
	layout.height = std::max(layout.height, penY + descent);
}

}
}
