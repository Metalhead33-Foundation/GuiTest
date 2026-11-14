#include "LteTextLayout.hpp"

#include <algorithm>

namespace Lotte {
namespace Text {

namespace {

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
 * @brief Emits the decoration segments of the line that just ended.
 * @param lineStartX Left edge the line started at.
 * @param penX Current pen position; the segment spans `lineStartX..penX`.
 * @param lineTopY Top of the line in layout space.
 * @param lineHeight Measured height of the line.
 * @param styleIndex Style referenced by the emitted segments.
 * @param attributes Styling of the run that ended the line.
 * @param layout Destination layout.
 */
void emitDecorations(float lineStartX, float penX, float lineTopY, float lineHeight,
					 std::uint32_t styleIndex, const TextAttributes& attributes, TextLayout& layout)
{
	if (penX <= lineStartX) return;
	if (attributes.strikethrough) {
		const float y = lineTopY + lineHeight * StrikethroughHeight;
		layout.lines.push_back(DecorationLine{ glm::fvec2(lineStartX, y), glm::fvec2(penX, y), styleIndex });
	}
	if (attributes.underline) {
		const float y = lineTopY + lineHeight * UnderlineHeight;
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

	for (char32_t codePoint : text) {
		if (codePoint == U'\f') continue;
		if (codePoint == U'\n') {
			emitDecorations(lineStartX, penX, penY, state.lineHeight, styleIndex, attributes, layout);
			penY += state.lineHeight + state.spacing;
			penX = lineStartX;
			state.lineHeight = 0.0f;
			continue;
		}

		const auto glyphIt = face.glyphs.find(std::uint32_t(codePoint));
		if (glyphIt == face.glyphs.end() || !glyphIt->second.valid) continue;
		const StoredCharacter& glyph = glyphIt->second;

		const float glyphWidth = float(glyph.width) * runScale;
		const float glyphHeight = float(glyph.height) * runScale;
		if (glyphWidth > 0.0f && glyphHeight > 0.0f) {
			PlacedGlyph placed;
			placed.codePoint = codePoint;
			placed.position = glm::fvec2(penX + float(glyph.bearingX) * runScale,
										penY + (float(glyph.height) - float(glyph.bearingY)) * runScale);
			placed.size = glm::fvec2(glyphWidth, glyphHeight);
			placed.shear = attributes.italic ? glyphHeight * ItalicShear : 0.0f;
			placed.styleIndex = styleIndex;
			layout.glyphs.push_back(placed);
			state.lineHeight = std::max(state.lineHeight, glyphHeight);
		}

		// Whitespace and other bitmap-less glyphs still advance the pen; the
		// original required `valid` before advancing and therefore dropped spaces.
		// TODO: apply face.kerning here once its offset unit is confirmed.
		penX += float(glyph.advanceX) * runScale;
	}

	emitDecorations(lineStartX, penX, penY, state.lineHeight, styleIndex, attributes, layout);

	state.cursor = glm::fvec2(penX, penY);
	layout.height = std::max(layout.height, penY + state.lineHeight);
}

}
}
