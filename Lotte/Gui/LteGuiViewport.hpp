#ifndef LTEGUIVIEWPORT_HPP
#define LTEGUIVIEWPORT_HPP
/**
 * @file LteGuiViewport.hpp
 * @brief Resolution-independent placement: a virtual canvas mapped onto the window.
 *
 * @section lte_viewport_why Why a canvas
 *
 * A GUI written in window pixels is written for exactly one window. The moment the
 * window is 640x480 instead of 800x600, the last button of a menu falls off the
 * bottom edge and the font is suddenly twice as large relative to the screen. The
 * fix is the one every desktop toolkit and every game UI has: the author places
 * elements on a *virtual canvas* of a fixed size (the "virtual resolution") and the
 * viewport maps that canvas onto whatever framebuffer the window happens to have.
 *
 * Three numbers come out of the mapping and they are the whole model:
 *
 * - **scale** - real pixels per canvas unit. A 34 px caption on an 800x600 canvas
 *   drawn into a 640x480 window is 27.2 real pixels tall.
 * - **visible** - how many canvas units the window shows. Under `fit`
 *   this is never smaller than the canvas, so the canvas always fits; on a wider
 *   screen it is *wider* than the canvas, which is the room a layout may grow into.
 * - **origin** - where canvas `(0, 0)` lands in visible space. With more room than
 *   the canvas needs, the canvas is centred and the origin is positive; when the
 *   scale is pushed above the fitted value (see `uiScale`) the
 *   canvas is cropped instead and the origin is negative.
 *
 * @section lte_viewport_modes Fitting modes
 *
 * `fit` keeps the whole canvas on screen and letterboxes the spare
 * space; `fill` covers the window and crops the overflow. Both stay
 * **uniform**: x and y always share one factor, because a GUI that is stretched in
 * one axis distorts SDF text and 9-slice borders, which never looks intentional.
 *
 * @section lte_viewport_authoring What the author writes
 *
 * A layout should be expressed in one of two coordinate systems, both of which this
 * class provides: **canvas units** (the design resolution, e.g. `y = 160`) or
 * **normalized** fractions of the canvas (`0.0`-`1.0`, via `normalizedToCanvas()`). The real resolution is only an input to
 * `setResolution()`; a layout never has to read it, and that is what
 * makes the same script work on every window size.
 *
 * @note Nothing here rounds. Widget rectangles stay fractional, which keeps a 0.8x
 *       scale smooth while moving; a renderer that wants crisp texels rounds when it
 *       converts to clip space, not here.
 *
 * @note The scale is deliberately the *only* place resolution enters the GUI. Text
 *       sizes, widget boxes and mouse coordinates all go through it, so "GUI scale"
 *       is one number rather than a per-element concern.
 */
#include <Lotte/LotteLib.hpp>
#include <glm/glm.hpp>
#include <cstdint>
#include <string_view>

namespace Lotte {
namespace Gui {

/**
 * @brief How the canvas is fitted into the window.
 *
 * Both modes scale uniformly; they differ only in which axis decides the factor.
 */
enum class ScaleMode : std::uint8_t {
	Fit,  /**< `min(x, y)`: the whole canvas is visible, spare space is letterboxed. */
	Fill  /**< `max(x, y)`: the window is covered, the overflow is cropped. */
};

/** @brief Name of a @ref ScaleMode, or `nullptr` past the last entry. */
MH_LOTTE_API const char* scaleModeName(ScaleMode mode) noexcept;

/**
 * @brief Parses a @ref ScaleMode name (`"fit"`, `"fill"`).
 * @param name Name to parse; case-sensitive.
 * @param mode Receives the mode when the name is known.
 * @return True when @p name was recognised and @p mode was written.
 */
MH_LOTTE_API bool parseScaleMode(std::string_view name, ScaleMode& mode) noexcept;

/**
 * @brief What the GUI author designs for, and how it reaches the screen.
 *
 * The defaults describe an 800x600 design fitted into the window at 1:1, which is
 * what a project that has not thought about scaling yet wants: on an 800x600 window
 * the scale is exactly 1 and canvas units *are* pixels, so an old pixel layout keeps
 * working unchanged.
 */
struct ViewportSettings {
	/** @brief The virtual resolution a layout is written in, in canvas units. */
	glm::fvec2 canvas = glm::fvec2(800.0f, 600.0f);
	/** @brief How the canvas is fitted into the window. */
	ScaleMode mode = ScaleMode::Fit;
	/**
	 * @brief Player/author multiplier on top of the fitted scale.
	 *
	 * `1` is "as fitted". Larger values enlarge every element (and, past the point
	 * where the canvas stops covering the window, crop it) - that is the knob for
	 * "the text is too small at 4K" or "too large at 640x480". A non-positive value
	 * is treated as `1`.
	 */
	float uiScale = 1.0f;
	/**
	 * @brief Lower clamp in real pixels per canvas unit; `0` disables it.
	 *
	 * The clamps are the seam where a future "big screens get more room, small screens
	 * get squeezed" policy plugs in: clamping the scale stops the canvas from fitting
	 * the window, and @ref GuiViewport::visibleSize() then reports the extra room a
	 * layout may use. They are off by default because that policy does not exist yet.
	 */
	float minimumScale = 0.0f;
	/** @brief Upper clamp in real pixels per canvas unit; `0` disables it. */
	float maximumScale = 0.0f;
};

/**
 * @brief Maps canvas coordinates onto the window, and back.
 *
 * The object is cheap and value-like: @ref setResolution() and @ref setSettings()
 * recompute the derived numbers, and a GUI re-applies its layout by asking for
 * @ref toPixels() again. It is safe to share one instance between the renderer, the
 * widget scene and the script, which is how a resize reaches all three at once.
 */
class MH_LOTTE_API GuiViewport
{
public:
	/**
	 * @brief Creates a viewport.
	 * @param settings Canvas, fitting mode, scale multiplier and clamps.
	 * @param resolution Real framebuffer size in pixels.
	 */
	explicit GuiViewport(ViewportSettings settings = {}, glm::fvec2 resolution = glm::fvec2(1.0f));

	/** @brief Adopts a new settings block and recomputes the mapping. */
	void setSettings(const ViewportSettings& settings);
	/** @brief The settings in force. */
	const ViewportSettings& settings() const noexcept { return viewportSettings; }
	/** @brief Adopts a new framebuffer size and recomputes the mapping. */
	void setResolution(glm::fvec2 resolution);

	/** @brief Framebuffer size the canvas is mapped onto, in real pixels. */
	const glm::fvec2& resolution() const noexcept { return pixelSize; }
	/** @brief The virtual resolution a layout is written in, in canvas units. */
	const glm::fvec2& canvasSize() const noexcept { return canvas; }
	/** @brief How much canvas the window shows, in canvas units (never smaller under Fit). */
	const glm::fvec2& visibleSize() const noexcept { return visible; }
	/** @brief Canvas `(0, 0)` in visible space; centred, so non-zero when there is spare room. */
	const glm::fvec2& canvasOrigin() const noexcept { return origin; }

	/** @brief Real pixels per canvas unit. */
	float scale() const noexcept { return canvasScale; }
	/** @brief Aspect ratio of the canvas (width / height). */
	float aspect() const noexcept { return canvas.x / canvas.y; }
	/** @brief Aspect ratio of the window (width / height). */
	float visibleAspect() const noexcept { return pixelSize.x / pixelSize.y; }

	/**
	 * @brief Maps a canvas-space point to real pixels.
	 * @param point Canvas coordinates, as a layout writes them.
	 */
	glm::fvec2 toPixels(glm::fvec2 point) const noexcept;
	/**
	 * @brief Maps a real-pixel point (a mouse position, an image size) back to canvas space.
	 * @param point Pixel coordinates.
	 */
	glm::fvec2 toCanvas(glm::fvec2 point) const noexcept;
	/** @brief Converts a canvas-space length (a size, a font size) to real pixels. */
	float lengthToPixels(float length) const noexcept;
	/** @brief Converts a real-pixel length to canvas units. */
	float lengthToCanvas(float length) const noexcept;

	/**
	 * @brief Turns a `0..1` fraction of the canvas into canvas units.
	 * @param fraction Normalized position or size; `1` is the far edge of the canvas.
	 */
	glm::fvec2 normalizedToCanvas(glm::fvec2 fraction) const noexcept;
	/**
	 * @brief Turns canvas units into `0..1` fractions of the canvas.
	 * @param point Canvas position or size.
	 */
	glm::fvec2 canvasToNormalized(glm::fvec2 point) const noexcept;

private:
	/** @brief Recomputes @ref canvasScale, @ref visible and @ref origin. */
	void recompute() noexcept;

	/** @brief Canvas, mode, multiplier and clamps. */
	ViewportSettings viewportSettings {};
	/** @brief Real framebuffer size. */
	glm::fvec2 pixelSize = glm::fvec2(1.0f);
	/** @brief Guarded copy of @ref ViewportSettings::canvas. */
	glm::fvec2 canvas = glm::fvec2(1.0f);
	/** @brief Real pixels per canvas unit. */
	float canvasScale = 1.0f;
	/** @brief Canvas units the window shows. */
	glm::fvec2 visible = glm::fvec2(1.0f);
	/** @brief Canvas `(0, 0)` in visible space. */
	glm::fvec2 origin = glm::fvec2(0.0f);
};

} // namespace Gui
} // namespace Lotte

#endif // LTEGUIVIEWPORT_HPP
