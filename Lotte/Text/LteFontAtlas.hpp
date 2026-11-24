#ifndef LTEFONTATLAS_HPP
#define LTEFONTATLAS_HPP
/**
 * @file LteFontAtlas.hpp
 * @brief Lazy glyph-atlas pages for a `PreprocessedFontFace`, plus glyph quads.
 *
 * Lotte's faces arrive as preprocessed SDF bitmaps (`LteFontFace.hpp`), so the
 * atlas has three jobs:
 *
 * 1. **Decode** a `StoredCharacter::sdf` payload with Euphemy's image decoders
 *    (`PreprocessedFontFace::imageFormat`, normally PNG).
 * 2. **Pack** the decoded `S x S` tiles into `WriteableTexture2D` pages with a
 *    shelf packer. A face may need several pages (CJK), so glyphs remember their
 *    page; 2D pages are used instead of array textures because
 *    `maxImageArrayLayers` is far too small for a full CJK set.
 * 3. **Map** a code point to a normalized rectangle on a page and expose the page
 *    handle to the renderer (`GlyphAtlasMapping` / `FontAtlas`).
 *
 * Loading is lazy: `ensureGlyph()` decodes and stamps a tile on first use, and
 * `recordUpdates()` turns the accumulated dirty rectangles into `UpdateTexture2D`
 * commands. On a backend with render passes those commands must be submitted
 * before the frame begins, which is why `TextRenderer::prepare()` groups them
 * into one pre-frame step.
 *
 * The atlas is CPU-authoritative. Pages are ordinary RGBA8 surfaces because the
 * shader reads `.r` (SDF), `.rgb` (MSDF) or `.rgba` (MSDFA) from one texture
 * layout; the fragments below explain why that does not lose data.
 *
 * @section lte_font_atlas_payload What the stored payload actually is
 *
 * Everything below was confirmed against FontPacker's generator
 * (`SdfGenerationContext::processOutlineGlyphEnd()` /
 * `FontOutlineDecompositionContext::translateToNewSize()`), and it constrains the
 * packer and the shader:
 *
 * - **Every payload is a square.** Each glyph's outline is scaled to fill
 *   `internalProcessSize - 2 * padding` and then downsampled, so the decoded image is
 *   `storedBitmapSize()` x `storedBitmapSize()` for *all* glyphs. Pack cells are
 *   therefore uniform squares. The quad that un-stretches the image is the
 *   **intended layout box plus the padding runway**, not the payload size and not
 *   the metric box alone.
 * - **The padding is already inside the payload.** It is added at
 *   `bitmapLogicalSize` and shrinks with the downsample, so the effective padding is
 *   `bitmapPadding * bitmapSize / bitmapLogicalSize`. Pack cells must not add another
 *   `bitmapPadding` border; `FontAtlasSettings::gutter` is only there to stop
 *   bilinear taps from reaching a neighbouring tile.
 * - **Draw the whole tile on the expanded quad.** `PlacedGlyph::position`/`size`
 *   already contain the expansion: `padX = width * P / (L - 2P)` and
 *   `padY = height * P / (L - 2P)` on each side, with the silhouette at
 *   `width x height`. `GlyphAtlasMapping` therefore returns the rectangle of the
 *   **whole tile, including its padding**: `appendGlyphQuads()` maps `uv0` to the
 *   top-left and `uv1` to the bottom-right of the padded quad.
 * - **SDF/MSDFA stay single/multi-channel; MSDF is stored as RGB.** FontPacker converts
 *   MSDF images to `Format_RGB888` before encoding, so an MSDF payload decodes to three
 *   channels and must not be sampled as a single-channel distance texture.
 * - **Distance ranges are stored in version 2+.** `distanceRangeX`/`Y` are the nominal
 *   search radius in level-0 texels. The renderer scales them by the on-screen
 *   pixels-per-texel ratio to obtain `screenPxRange`; v1 faces get a reconstructed
 *   value from the importer.
 *
 * @section lte_font_atlas_ready What is already usable
 *
 * `appendGlyphQuads()` converts a @ref Lotte::Text::TextLayout into textured quads,
 * applying the italic shear and the four-corner winding that `Font::renderText`
 * emitted directly. `FontAtlas` supplies the mapping (including the page number),
 * so a renderer only has to group runs by page and bind the page texture.
 */

#include <Lotte/LotteLib.hpp>
#include <Lotte/Text/LteFontFace.hpp>
#include <Lotte/Text/LteTextLayout.hpp>
#include <Lotte/GFX/LteResourceFactory.hpp>
#include <Lotte/GFX/LteWriteableTexture2D.hpp>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <unordered_map>
#include <unordered_set>
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
	 * @brief Looks up the normalized texture rectangle of one glyph tile.
	 *
	 * The rectangle must cover the **whole stored tile**, including the padding
	 * baked into it - not the inner content box. `appendGlyphQuads()` maps the
	 * whole rectangle onto the padded drawing quad, so the padding runway stays
	 * available to the shader.
	 *
	 * @param codePoint Code point to look up.
	 * @param uv0 Receives the top-left texture coordinate of the tile.
	 * @param uv1 Receives the bottom-right texture coordinate of the tile.
	 * @return `true` when the glyph is present in the atlas.
	 */
	virtual bool mapGlyph(char32_t codePoint, glm::fvec2& uv0, glm::fvec2& uv1) const = 0;

	/**
	 * @brief Returns the page index that owns @p codePoint's tile.
	 *
	 * The default implementation returns 0 for single-page mappings. Multi-page
	 * atlases return the real page; @ref InvalidAtlasPage means "not mapped".
	 * Callers should only consult this after @ref mapGlyph succeeded.
	 * @param codePoint Code point to look up.
	 */
	virtual std::uint32_t pageForGlyph(char32_t codePoint) const
	{
		(void)codePoint; // the default mapping is single-page, so every glyph reports page 0
		return 0;
	}
};

/** @brief Page index that means "this glyph is not in the atlas". */
inline constexpr std::uint32_t InvalidAtlasPage = std::numeric_limits<std::uint32_t>::max();

/** @brief Mapping that contains no glyphs; every lookup fails. */
class EmptyGlyphAtlasMapping final : public GlyphAtlasMapping
{
public:
	/** @brief Always fails. */
	bool mapGlyph(char32_t, glm::fvec2&, glm::fvec2&) const override { return false; }
};

/** @brief Page layout of a @ref FontAtlas. */
struct FontAtlasSettings {
	/** @brief Page width in pixels; must fit one cell. */
	std::uint16_t pageWidth = 1024;
	/** @brief Page height in pixels; must fit one cell. */
	std::uint16_t pageHeight = 1024;
	/**
	 * @brief Transparent border around each tile, in pixels.
	 *
	 * The serialized SDF already carries its own padding, so this is only a
	 * bilinear-filter guard against reaching the neighbouring tile. One pixel is
	 * enough for the clamped, non-mipmapped sampler the text renderer uses.
	 */
	std::uint16_t gutter = 1;
	/** @brief Pages after which @ref FontAtlas::ensureGlyph throws; 0 means unlimited. */
	std::size_t maxPages = 128;
};

/** @brief Location of one glyph tile inside an atlas. */
struct FontAtlasGlyph {
	/** @brief Page index, or @ref InvalidAtlasPage when the glyph is not mapped. */
	std::uint32_t page = InvalidAtlasPage;
	/** @brief Left edge of the tile in page pixels (the gutter excluded). */
	std::uint16_t x = 0;
	/** @brief Top edge of the tile in page pixels (the gutter excluded). */
	std::uint16_t y = 0;
};

/**
 * @brief Lazy, multi-page glyph atlas for one preprocessed font face.
 *
 * The atlas decodes a glyph's payload on first use, stamps it into a shelf-packed
 * page and records the dirty rectangle as `UpdateTexture2D` commands. It is also
 * the @ref GlyphAtlasMapping a renderer consumes, so mapping and storage stay in
 * sync by construction.
 *
 * @section lte_font_atlas_usage Usage
 *
 * @code
 * FontAtlas atlas(factory, face);
 * atlas.ensureGlyphs(layout);   // decode everything this frame needs
 * atlas.recordUpdates();        // record uploads before the draws
 * appendGlyphQuads(layout, atlas, quads);
 * // bind atlas.pageTexture(quad.page) for each run
 * @endcode
 *
 * @warning Destroying the atlas records destroy commands for its pages. Do not
 *          destroy it between recording a draw and submitting that frame.
 */
class MH_LOTTE_API FontAtlas final : public GlyphAtlasMapping
{
public:
	/**
	 * @brief Creates an empty atlas for @p face.
	 * @param factory Factory that records texture creation and uploads.
	 * @param face Face whose glyph payloads are decoded on demand.
	 * @param settings Page layout.
	 * @throws std::invalid_argument When the face has no tile size or the page
	 *         and gutter geometry cannot hold a tile.
	 */
	FontAtlas(Gfx::ResourceFactory& factory, std::shared_ptr<const PreprocessedFontFace> face,
			  FontAtlasSettings settings = {});
	~FontAtlas() override;

	FontAtlas(const FontAtlas&) = delete;
	FontAtlas& operator=(const FontAtlas&) = delete;
	FontAtlas(FontAtlas&&) = delete;
	FontAtlas& operator=(FontAtlas&&) = delete;

	/**
	 * @brief Ensures one code point is decoded and packed.
	 * @return `true` when mapped; `false` when the face has no such glyph or the
	 *         glyph has no drawable payload.
	 * @throws std::runtime_error When the payload cannot be decoded, does not
	 *         match the tile size, or the page budget is exhausted.
	 */
	bool ensureGlyph(char32_t codePoint);

	/**
	 * @brief Ensures every glyph of @p layout that belongs to this atlas's face.
	 * @return Number of glyphs that were newly decoded and packed.
	 */
	std::size_t ensureGlyphs(const TextLayout& layout);

	/**
	 * @brief Records `UpdateTexture2D` commands for every dirty page.
	 *
	 * Call after the frame's `ensure*` calls and before the draw commands. On a
	 * backend with render passes this has to run *outside* the frame, so
	 * @ref Lotte::Text::TextRenderer::prepare() groups it with `ensureGlyphs()`.
	 * @return `true` when at least one command was recorded.
	 */
	bool recordUpdates();

	/** @brief Looks up a code point without decoding it. */
	const FontAtlasGlyph* findGlyph(char32_t codePoint) const;

	/** @brief Maps a code point to normalized page coordinates (whole tile). */
	bool mapGlyph(char32_t codePoint, glm::fvec2& uv0, glm::fvec2& uv1) const override;
	/** @brief Page that owns @p codePoint, or @ref InvalidAtlasPage. */
	std::uint32_t pageForGlyph(char32_t codePoint) const override;

	/** @brief Kaldi texture handle of a page, or 0 when out of range. */
	Kld::HandleId pageTexture(std::uint32_t page) const;
	/**
	 * @brief CPU surface of a page for inspection, or null when out of range.
	 * @note The pointer is invalidated when another page is allocated.
	 */
	const Gfx::WriteableTexture2D* pageSurface(std::uint32_t page) const;
	/** @brief Number of allocated pages. */
	std::size_t pageCount() const noexcept { return pages_.size(); }
	/** @brief Number of mapped glyphs. */
	std::size_t glyphCount() const noexcept { return glyphs_.size(); }
	/** @brief Edge length of one stored tile, in texels. */
	std::uint32_t tileSize() const noexcept { return tileSize_; }
	/** @brief Face the atlas belongs to. */
	const PreprocessedFontFace& face() const noexcept { return *face_; }
	/** @brief Page layout in use. */
	const FontAtlasSettings& settings() const noexcept { return settings_; }
	/** @brief Factory that records the page commands. */
	Gfx::ResourceFactory& factory() const noexcept { return *factory_; }

private:
	/** @brief One writeable texture page plus its shelf cursor. */
	struct Page {
		/** @brief CPU-authoritative RGBA8 page and its Kaldi mirror. */
		Gfx::WriteableTexture2D texture;
		/** @brief X of the next free cell. */
		std::uint32_t nextX = 0;
		/** @brief Y of the current shelf. */
		std::uint32_t nextY = 0;
	};

	/** @brief Creates a transparent page and appends it to @ref pages_. */
	void createPage();
	/** @brief Finds room for one tile, adding pages as needed. */
	bool allocateTile(FontAtlasGlyph& glyph);
	/** @brief Decodes @p glyph's payload and stamps it at @p target. */
	void stampGlyph(char32_t codePoint, const StoredCharacter& glyph, const FontAtlasGlyph& target);

	/** @brief Non-owning factory; outlives the atlas by contract. */
	Gfx::ResourceFactory* factory_ = nullptr;
	/** @brief Face whose payloads are decoded. */
	std::shared_ptr<const PreprocessedFontFace> face_;
	/** @brief Page layout. */
	FontAtlasSettings settings_;
	/** @brief Edge length of one tile. */
	std::uint32_t tileSize_ = 0;
	/** @brief `tileSize_ + 2 * gutter`. */
	std::uint32_t cellSize_ = 0;
	/** @brief Allocated pages. */
	std::vector<Page> pages_;
	/** @brief Mapped glyphs. */
	std::unordered_map<char32_t, FontAtlasGlyph> glyphs_;
	/** @brief Code points that cannot be drawn (missing, invalid or empty). */
	std::unordered_set<char32_t> unavailable_;
	/** @brief True when page creation or a stamp recorded commands not yet submitted. */
	bool pendingUploads_ = false;
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
	/** @brief Atlas page that holds the tile; see @ref GlyphAtlasMapping::pageForGlyph. */
	std::uint32_t page = 0;
	/** @brief Index into @ref TextLayout::styles, for colour lookup. */
	std::uint32_t styleIndex = 0;
};

/**
 * @brief Builds the textured quad of one placed glyph.
 *
 * Shared by @ref appendGlyphQuads() and the text renderer, so the winding, shear
 * and page lookup stay in one place.
 * @tparam Mapping Concrete @ref GlyphAtlasMapping type.
 * @param glyph Positioned glyph from a layout.
 * @param mapping Atlas mapping that supplies the tile and page.
 * @param quad Receives the quad on success.
 * @return `false` when the mapping has no tile for the code point.
 */
template <typename Mapping>
bool buildGlyphQuad(const PlacedGlyph& glyph, const Mapping& mapping, GlyphQuad& quad)
{
	glm::fvec2 uv0(0.0f);
	glm::fvec2 uv1(0.0f);
	if (!mapping.mapGlyph(glyph.codePoint, uv0, uv1)) {
		return false;
	}
	const glm::fvec2 topLeft = glyph.position;
	const glm::fvec2 bottomRight = glyph.position + glyph.size;
	quad = GlyphQuad{};
	quad.position[0] = topLeft;
	quad.position[1] = glm::fvec2(bottomRight.x, topLeft.y);
	quad.position[2] = glm::fvec2(topLeft.x, bottomRight.y);
	quad.position[3] = bottomRight;
	if (glyph.shear != 0.0f) {
		// Italic shear shifts the bottom edge only; the original applied `xdiff`
		// to the two vertices with the larger `y`.
		quad.position[2].x += glyph.shear;
		quad.position[3].x += glyph.shear;
	}
	quad.uv[0] = uv0;
	quad.uv[1] = glm::fvec2(uv1.x, uv0.y);
	quad.uv[2] = glm::fvec2(uv0.x, uv1.y);
	quad.uv[3] = uv1;
	quad.page = mapping.pageForGlyph(glyph.codePoint);
	quad.styleIndex = glyph.styleIndex;
	return true;
}

/**
 * @brief Converts a layout into textured glyph quads.
 *
 * Each emitted quad is `PlacedGlyph::position`/`size`, i.e. the intended box
 * expanded by the SDF padding runway. The atlas rectangle returned by @a mapping
 * is mapped onto that whole quad, so it must be the whole tile including padding.
 *
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
		GlyphQuad quad;
		if (!buildGlyphQuad(glyph, mapping, quad)) continue;
		const std::uint32_t base = std::uint32_t(quads.size());
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
