#ifndef LTECURSOR_HPP
#define LTECURSOR_HPP
/**
 * @file LteCursor.hpp
 * @brief Anchored, alpha-crawlable cursor with 2D and 3D presentation.
 *
 * Salvaged from `GUI/MhCursor.{hpp,cpp}` on the `GuiTest-legacy2` tree. The old
 * class knew exactly one thing: "walk the diagonal of the alpha mask from the
 * upper-left corner until an opaque pixel shows up, call that the origin". That
 * covers an arrow cursor and nothing else - not a left-pointing arrow, not a
 * symmetrical hourglass, not a 3D model.
 *
 * This version separates the two concerns the old class conflated:
 *
 * - **Where the origin is** (`AlphaCoverage` + `CursorMetadata`): nine
 *   anchors (four corners, four edge midpoints, center) and five crawl modes that
 *   walk from the anchor towards the image interior until a pixel passes the
 *   alpha threshold. Corners crawl diagonally, edges perpendicularly, and the
 *   center crawls radially (nearest opaque pixel by Chebyshev ring), which is the
 *   case a "center intended origin" asset like `hourglass2.png` needs.
 * - **What is drawn** (`CursorImageVisual` or `CursorModelVisual`): a textured
 *   quad through @ref Lotte::Gui::GuiRenderer, or a model-space origin plus
 *   transform for a 3D cursor. A model cursor does not need an alpha mask at all,
 *   because its origin is authored in model space.
 *
 * @section lte_cursor_metadata Cursor metadata
 *
 * `CursorMetadata` is deliberately a plain aggregate: it is the object a JSON
 * cursor descriptor will deserialize into. Nothing here parses JSON yet, so the
 * struct is the contract, not a serialization format.
 *
 * @section lte_cursor_coordinates Coordinates
 *
 * Everything is in **image pixels**: the hotspot is a position inside the source
 * image, and `Cursor::render()` places the quad so that the hotspot lands on
 * the requested screen position. Continuous pixel coordinates are used (the
 * top-left corner of the image is `(0, 0)`, the bottom-right corner is
 * `(width, height)`), and a crawled origin is the *center* of the pixel it found
 * (`x + 0.5`, `y + 0.5`), which is what keeps an arrow tip visually stable.
 */
#include <Lotte/LotteLib.hpp>
#include <Lotte/LteKld.hpp>
#include <Lotte/Gui/LteGuiRenderer.hpp>
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <glm/glm.hpp>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace Lotte {
namespace Gui {

/** @brief Where on the cursor image the origin point sits. */
enum class CursorAnchor : std::uint8_t {
	TopLeft,     /**< Upper-left corner; the classic arrow cursor. */
	Top,         /**< Middle of the top edge. */
	TopRight,    /**< Upper-right corner. */
	Left,        /**< Middle of the left edge. */
	Center,      /**< Image center; the hourglass/throbber case. */
	Right,       /**< Middle of the right edge. */
	BottomLeft,  /**< Lower-left corner. */
	Bottom,      /**< Middle of the bottom edge. */
	BottomRight  /**< Lower-right corner. */
};

/**
 * @brief How an alpha crawl walks away from the anchor to find the real origin.
 *
 * `NoCrawl` uses the anchor point itself and ignores the alpha mask.
 *
 * @note The "no crawl" enumerator is called `NoCrawl` rather than `None` because
 *       `<X11/Xlib.h>` - pulled in by `SDL_syswm.h`, and therefore by every SDL
 *       host - defines a `None` macro. The same restriction is why Kaldi spells
 *       the always-passing depth comparison `AlwaysPass`.
 */
enum class CursorCrawlMode : std::uint8_t {
	NoCrawl,    /**< No crawl; the anchor point is the origin. */
	Diagonal,   /**< Corner anchors: corner towards the opposite corner. */
	Vertical,   /**< Top/bottom anchors: middle column, inwards. */
	Horizontal, /**< Left/right anchors: middle row, inwards. */
	Radial      /**< Center anchors: nearest opaque pixel by Chebyshev ring. */
};

/** @brief Stable name of @p anchor, for logs and future descriptor text. */
const char* cursorAnchorName(CursorAnchor anchor) noexcept;
/** @brief Stable name of @p mode, for logs and future descriptor text. */
const char* cursorCrawlModeName(CursorCrawlMode mode) noexcept;

/** @brief The anchor position as a fraction of the image extent, in `[0, 1]`. */
glm::fvec2 cursorAnchorFraction(CursorAnchor anchor) noexcept;
/** @brief The crawl mode that fits @p anchor (the default when none is configured). */
CursorCrawlMode defaultCrawlFor(CursorAnchor anchor) noexcept;

/**
 * @brief Alpha-only coverage mask of a cursor image.
 *
 * The legacy `Cursor` built a `std::vector<bool>` through
 * `Image2D::produceAlphaMap()` on every construction and threw it away. Keeping
 * one byte per pixel instead is cheaper to search, survives on the cursor object,
 * and makes the origin search testable without any image decoder.
 *
 * The mask is row-major, top row first, exactly like the image it came from.
 */
class MH_LOTTE_API AlphaCoverage {
public:
	/** @brief Creates an empty mask. */
	AlphaCoverage() = default;
	/**
	 * @brief Creates a mask from explicit alpha bytes.
	 * @param width Width in pixels.
	 * @param height Height in pixels.
	 * @param alpha `width * height` alpha bytes, row-major; truncated input is
	 *        rejected and leaves the mask empty.
	 */
	AlphaCoverage(std::uint16_t width, std::uint16_t height, std::vector<std::uint8_t> alpha);

	/** @brief Extracts the alpha channel of a tightly packed RGBA8 image. */
	static AlphaCoverage fromRgba8(const std::uint8_t* pixels, std::uint16_t width,
				       std::uint16_t height);
	/**
	 * @brief Extracts a mask from a decoded image.
	 *
	 * The image is converted to RGBA8 first, so formats without an alpha channel
	 * produce a fully opaque mask. Throws `std::invalid_argument` for images the
	 * converter rejects.
	 */
	static AlphaCoverage fromDecodeTarget(const Euph::Media::Image::DecodeTarget& source);

	/** @brief Width in pixels. */
	std::uint16_t width() const noexcept { return width_; }
	/** @brief Height in pixels. */
	std::uint16_t height() const noexcept { return height_; }
	/** @brief True when the mask has no storage. */
	bool empty() const noexcept { return width_ == 0 || height_ == 0 || alpha_.empty(); }
	/** @brief The alpha bytes, row-major. */
	const std::vector<std::uint8_t>& alpha() const noexcept { return alpha_; }

	/** @brief True when the pixel at @p x , @p y passes @p threshold (0..1). */
	bool isOpaque(std::uint16_t x, std::uint16_t y, float threshold) const noexcept;

	/** @brief The anchor position in continuous image pixel coordinates. */
	glm::fvec2 anchorPixel(CursorAnchor anchor) const noexcept;
	/**
	 * @brief Walks from @p anchor according to @p mode until an opaque pixel is found.
	 * @param mode Crawl direction; `NoCrawl` returns @ref anchorPixel().
	 * @param anchor Anchor the crawl starts from.
	 * @param threshold Alpha threshold in `[0, 1]`.
	 * @return The center of the first opaque pixel, or the anchor position when the
	 *         whole path is transparent (and the same for an empty mask).
	 */
	glm::fvec2 crawlOrigin(CursorCrawlMode mode, CursorAnchor anchor,
			       float threshold) const noexcept;

	/**
	 * @brief Resolves the origin in one call.
	 * @param anchor Anchor position.
	 * @param alphaCrawl When false, returns @ref anchorPixel() without searching.
	 * @param crawlMode Crawl mode used when @p alphaCrawl is true.
	 * @param threshold Alpha threshold in `[0, 1]`.
	 */
	glm::fvec2 origin(CursorAnchor anchor, bool alphaCrawl, CursorCrawlMode crawlMode,
			  float threshold) const noexcept;
private:
	/** @brief Nearest opaque pixel by Chebyshev ring around the anchor. */
	glm::fvec2 nearestOpaque(glm::fvec2 anchor, float threshold) const noexcept;

	std::uint16_t width_ = 0;
	std::uint16_t height_ = 0;
	std::vector<std::uint8_t> alpha_;
};

/**
 * @brief Descriptor of a cursor's origin behaviour; the future JSON target.
 *
 * Field names are spelled out instead of using a constructor so a serializer can
 * fill it member by member, and every field has the documented default a missing
 * key should produce.
 */
struct CursorMetadata {
	/** @brief Anchor used when no explicit hotspot is given. */
	CursorAnchor anchor = CursorAnchor::TopLeft;
	/** @brief Crawl mode override; empty means `defaultCrawlFor(anchor)`. */
	std::optional<CursorCrawlMode> crawl;
	/** @brief Enables the alpha crawl; when false the anchor point wins. */
	bool alphaCrawl = false;
	/** @brief Alpha threshold in `[0, 1]`; pixels at or above it are opaque. */
	float alphaThreshold = 0.5f;
	/** @brief Explicit hotspot in image pixels; overrides anchor and crawl. */
	std::optional<glm::fvec2> hotspotPixels;
	/** @brief Uniform display scale applied by @ref Cursor::render(). */
	float scale = 1.0f;
	/** @brief When false the cursor is skipped by @ref Cursor::render(). */
	bool visible = true;
};

/** @brief A 2D cursor image, optionally a sub-rectangle of an atlas page. */
struct CursorImageVisual {
	/** @brief Sampled texture handle; 0 means "nothing to draw". */
	Kld::HandleId texture = 0;
	/** @brief Full image size in pixels, used for the quad extent. */
	glm::uvec2 pixelSize = glm::uvec2(0);
	/** @brief Atlas UV rectangle: top-left and bottom-right texture coordinates. */
	glm::fvec2 uv[2] = { glm::fvec2(0.0f), glm::fvec2(1.0f) };
};

/** @brief A 3D model cursor: an origin in model space plus a placement. */
struct CursorModelVisual {
	/** @brief Mesh handle; 0 means "nothing to draw". */
	Kld::HandleId mesh = 0;
	/** @brief Pipeline handle the renderer should bind for the model. */
	Kld::HandleId pipeline = 0;
	/** @brief Model-space point that lands on the screen position. */
	glm::fvec3 originInModel = glm::fvec3(0.0f);
	/** @brief Non-uniform scale applied around the model origin. */
	glm::fvec3 scale = glm::fvec3(1.0f);
	/** @brief Euler rotation in radians, applied X then Y then Z. */
	glm::fvec3 rotationRadians = glm::fvec3(0.0f);
};

/** @brief Where and how a 3D cursor model belongs in the frame. */
struct CursorModelPlacement {
	/** @brief Screen position the model origin should cover, in pixels. */
	glm::fvec2 screenOrigin = glm::fvec2(0.0f);
	/** @brief Model matrix: `T(screenOrigin) * R * S * T(-originInModel)`. */
	glm::mat4 transform = glm::mat4(1.0f);
};

/**
 * @brief An anchored cursor with a 2D or 3D presentation.
 *
 * The object is cheap to copy (one alpha mask plus a small variant) and does not
 * own the texture or mesh: it stores handles, so a cursor can be created,
 * destroyed and recreated every frame without touching the resource factory.
 */
class MH_LOTTE_API Cursor {
public:
	/** @brief The active presentation. */
	using Visual = std::variant<CursorImageVisual, CursorModelVisual>;

	/** @brief Creates a cursor with no visual; renders nothing. */
	Cursor() = default;
	/**
	 * @brief Creates a 2D image cursor.
	 * @param visual Texture handle, size and atlas UVs.
	 * @param coverage Optional alpha mask used by the crawl; may be null.
	 * @param metadata Origin behaviour and display options.
	 *
	 * The mask is copied. When it is null or its size does not match
	 * `visual.pixelSize`, the anchor fraction is used without crawling.
	 */
	Cursor(CursorImageVisual visual, const AlphaCoverage* coverage,
	       CursorMetadata metadata = {});
	/**
	 * @brief Creates a 3D model cursor.
	 * @param visual Mesh/pipeline handles and the model-space origin.
	 * @param metadata Origin behaviour and display options.
	 *
	 * The model origin comes from the visual, so no alpha mask is involved; the
	 * anchor and crawl fields still describe where the *screen* origin is.
	 */
	Cursor(CursorModelVisual visual, CursorMetadata metadata = {});

	/** @brief The current metadata. */
	const CursorMetadata& metadata() const noexcept { return metadata_; }
	/**
	 * @brief Replaces the metadata and re-resolves the hotspot.
	 * @param metadata New origin behaviour and display options.
	 */
	void setMetadata(CursorMetadata metadata);

	/** @brief True when the visual is a 2D image. */
	bool isImage() const noexcept { return std::holds_alternative<CursorImageVisual>(visual_); }
	/** @brief True when the visual is a 3D model. */
	bool isModel() const noexcept { return std::holds_alternative<CursorModelVisual>(visual_); }
	/** @brief The image visual, or @c nullptr for a model cursor. */
	const CursorImageVisual* image() const noexcept { return std::get_if<CursorImageVisual>(&visual_); }
	/** @brief The model visual, or @c nullptr for an image cursor. */
	const CursorModelVisual* model() const noexcept { return std::get_if<CursorModelVisual>(&visual_); }

	/** @brief The copied alpha mask (empty for a model cursor). */
	const AlphaCoverage& coverage() const noexcept { return coverage_; }
	/** @brief Replaces the alpha mask and re-resolves the hotspot. */
	void setCoverage(const AlphaCoverage& coverage);

	/** @brief The resolved origin, in continuous image pixel coordinates. */
	const glm::fvec2& hotspotPixels() const noexcept { return hotspot_; }
	/** @brief Replaces the visual and re-resolves the hotspot. */
	void setImageVisual(CursorImageVisual visual);
	/** @brief Replaces the visual and re-resolves the hotspot. */
	void setModelVisual(CursorModelVisual visual);

	/** @brief True when @ref render() draws anything. */
	bool visible() const noexcept { return metadata_.visible; }
	/** @brief Enables or disables drawing. */
	void setVisible(bool visible) noexcept { metadata_.visible = visible; }
	/** @brief Uniform display scale. */
	float scale() const noexcept { return metadata_.scale; }
	/** @brief Replaces the display scale. */
	void setScale(float scale) noexcept { metadata_.scale = scale; }

	/**
	 * @brief Draws a 2D image cursor so its hotspot lands on @p position.
	 * @param renderer Renderer to submit the quad to.
	 * @param position Screen position of the cursor origin, in pixels.
	 */
	void render(GuiRenderer& renderer, glm::fvec2 position) const;
	/**
	 * @brief Draws a 2D image cursor with a tint.
	 * @param renderer Renderer to submit the quad to.
	 * @param position Screen position of the cursor origin, in pixels.
	 * @param tint Colour multiplied into the sampled texels.
	 */
	void render(GuiRenderer& renderer, glm::fvec2 position, glm::fvec4 tint) const;

	/**
	 * @brief Computes the model matrix of a 3D cursor.
	 * @param position Screen position of the cursor origin, in pixels.
	 * @param depth Depth to place the model origin at.
	 * @return The placement; `transform` is identity for a 2D cursor.
	 */
	CursorModelPlacement modelPlacement(glm::fvec2 position, float depth = 0.0f) const;

private:
	/** @brief Recomputes @ref hotspot_ from the metadata and coverage. */
	void refreshHotspot();

	Visual visual_ {};
	CursorMetadata metadata_ {};
	AlphaCoverage coverage_ {};
	glm::fvec2 hotspot_ = glm::fvec2(0.0f);
};

} // namespace Gui
} // namespace Lotte

#endif // LTECURSOR_HPP
