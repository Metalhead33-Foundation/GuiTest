#ifndef LTEFONTATLAS_HPP
#define LTEFONTATLAS_HPP
/**
 * @file LteFontAtlas.hpp
 * @brief Documents the glyph atlas Lotte does not have yet, and emits glyph quads.
 *
 * The `NewBeginning` branch had `GFX/Advanced/MhHardwareAcceleratedFont.{hpp,cpp}`:
 * a `Font` that owned a writable texture, rasterized glyphs with FreeType on
 * demand, packed them into that texture, remembered each glyph's pixel rectangle,
 * and turned rectangles into texture coordinates while drawing. Lotte keeps none
 * of that implementation - there is no FreeType, and glyphs arrive as
 * preprocessed SDF bitmaps (`LteFontFace.hpp`) - but the *shape* of the missing
 * piece is recorded here so it can be filled in without redesigning the text
 * pipeline around it.
 *
 * @section lte_font_atlas_missing What the atlas still has to provide
 *
 * 1. **A texture per face (or per face/size pair).** `StoredCharacter::sdf`
 *    holds an encoded image payload (`PreprocessedFontFace::imageFormat`, e.g.
 *    PNG). Decoding it reuses Euphemy's image decoders; upload goes through
 *    Kaldi (`Kld::OpCreateTexture2D` / `OpUpdateTexture2D`) or through
 *    `Lotte::Asset::GpuAssetStreamer`, which already tracks GPU byte budgets.
 * 2. **A packer.** Shelves or skyline packing, with `bitmapPadding` applied on
 *    every side, producing a pixel rectangle per code point. The old code grew
 *    the texture and re-packed when it ran out of room
 *    (`MhHardwareAcceleratedFont::flushTexture`).
 * 3. **A mapping from code point to normalized UV rectangle**, which is exactly
 *    the @ref Lotte::Text::GlyphAtlasMapping interface below.
 * 4. **Mip/derivative handling.** The faces are SDF/MSDF/MSDFA, so the shader
 *    reconstructs coverage from the distance field; the atlas must not apply
 *    smoothing that would destroy the distance values.
 *
 * @section lte_font_atlas_ready What is already usable
 *
 * `appendGlyphQuads()` below converts a @ref Lotte::Text::TextLayout into
 * textured quads, applying the italic shear and the four-corner winding that
 * `Font::renderText` emitted directly. It only needs a mapper, so the atlas can
 * be developed (and the quads unit-tested) independently of the renderer.
 */

#include <Lotte/LotteLib.hpp>
#include <Lotte/Text/LteTextLayout.hpp>
#include <cstdint>
#include <vector>

namespace Lotte {
namespace Text {

/** @brief Maps code points to a rectangle in a glyph atlas. */
class GlyphAtlasMapping
{
public:
	/** @brief Releases a mapping through the base interface. */
	virtual ~GlyphAtlasMapping() = default;

	/**
	 * @brief Looks up the normalized texture rectangle of one glyph.
	 * @param codePoint Code point to look up.
	 * @param uv0 Receives the top-left texture coordinate.
	 * @param uv1 Receives the bottom-right texture coordinate.
	 * @return `true` when the glyph is present in the atlas.
	 */
	virtual bool mapGlyph(char32_t codePoint, glm::fvec2& uv0, glm::fvec2& uv1) const = 0;
};

/** @brief Mapping that contains no glyphs; every lookup fails. */
class EmptyGlyphAtlasMapping final : public GlyphAtlasMapping
{
public:
	/** @brief Always fails. */
	bool mapGlyph(char32_t, glm::fvec2&, glm::fvec2&) const override { return false; }
};

/**
 * @brief One textured glyph quad.
 *
 * The corner order matches the original renderer: top-left, top-right,
 * bottom-left, bottom-right. Indices for two triangles are `0,1,2,1,2,3`.
 */
struct GlyphQuad {
	/** @brief Corner positions in layout space. */
	glm::fvec2 position[4];
	/** @brief Corner texture coordinates. */
	glm::fvec2 uv[4];
	/** @brief Index into @ref TextLayout::styles, for colour lookup. */
	std::uint32_t styleIndex = 0;
};

/**
 * @brief Converts a layout into textured glyph quads.
 * @tparam Mapping Concrete @ref GlyphAtlasMapping type (kept a template so the
 *         common case inlines and no virtual call happens per glyph).
 * @param layout Source layout.
 * @param mapping Atlas mapping used to resolve texture coordinates.
 * @param quads Destination quad list; appended to.
 * @param indices Optional index list; when non-null, two triangles per quad are
 *        appended, offset by the current number of quads.
 * @return Number of emitted quads.
 */
template <typename Mapping>
std::size_t appendGlyphQuads(const TextLayout& layout, const Mapping& mapping, std::vector<GlyphQuad>& quads,
							 std::vector<std::uint32_t>* indices = nullptr)
{
	std::size_t emitted = 0;
	for (const PlacedGlyph& glyph : layout.glyphs) {
		glm::fvec2 uv0(0.0f);
		glm::fvec2 uv1(0.0f);
		if (!mapping.mapGlyph(glyph.codePoint, uv0, uv1)) continue;

		const glm::fvec2 topLeft = glyph.position;
		const glm::fvec2 bottomRight = glyph.position + glyph.size;
		const std::uint32_t base = std::uint32_t(quads.size());

		GlyphQuad quad;
		quad.position[0] = topLeft;
		quad.position[1] = glm::fvec2(bottomRight.x, topLeft.y);
		quad.position[2] = glm::fvec2(topLeft.x, bottomRight.y);
		quad.position[3] = bottomRight;
		if (glyph.shear != 0.0f) {
			// Italic shear shifts the bottom edge only; the original applied
			// `xdiff` to the two vertices with the larger `y`.
			quad.position[2].x += glyph.shear;
			quad.position[3].x += glyph.shear;
		}
		quad.uv[0] = uv0;
		quad.uv[1] = glm::fvec2(uv1.x, uv0.y);
		quad.uv[2] = glm::fvec2(uv0.x, uv1.y);
		quad.uv[3] = uv1;
		quad.styleIndex = glyph.styleIndex;
		quads.push_back(quad);

		if (indices) {
			indices->push_back(base + 0);
			indices->push_back(base + 1);
			indices->push_back(base + 2);
			indices->push_back(base + 1);
			indices->push_back(base + 2);
			indices->push_back(base + 3);
		}
		++emitted;
	}
	return emitted;
}

}
}

#endif // LTEFONTATLAS_HPP
