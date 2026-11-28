#ifndef LTESLIDER_HPP
#define LTESLIDER_HPP
/**
 * @file LteSlider.hpp
 * @brief A continuous-value widget: groove, thumb, drag and a value signal.
 *
 * `ThemeElement::SliderTrack` and `ThemeElement::SliderThumb` existed in the theme
 * contract from the start, and the procedural atlas already packs a groove and a gem
 * thumb, but no widget ever consumed them. This is that widget. It is the first
 * *continuous* control in Lotte: everything else so far was a press/release button.
 *
 * @section lte_slider_model The value model
 *
 * A slider owns three numbers - `minimum`, `maximum` and an optional `step` - and the
 * current value inside that range. `setValue()` is the single mutation point: it
 * clamps, snaps to `step` when one is set, and emits `valueChanged` only when the
 * resulting value differs from the previous one. A drag therefore produces one
 * emission per pixel of travel, not one per event, and a caller can bind the signal
 * without de-duplicating.
 *
 * The mapping between the pointer and the value is *thumb-centre* based: the travel
 * of the thumb centre spans the widget minus the thumb, so both extremes put the
 * thumb fully inside the widget. A press anywhere on the groove jumps the value
 * there and starts a drag, which is what makes a thin groove usable.
 *
 * @section lte_slider_orientation Orientation
 *
 * Horizontal sliders grow left-to-right; vertical sliders grow bottom-to-top, which
 * is the convention a level meter or a volume fader has (and the opposite of the
 * pixel `y` axis, so the fraction is inverted when converting to a value).
 *
 * @section lte_slider_theme Theme
 *
 * With a theme, the groove is drawn as the `SliderTrack` skin's rectangle and the
 * thumb as `SliderThumb`'s, both tinted by the skin. Without one - or when the theme
 * answers `std::nullopt` for either part - the widget paints its own quads from
 * @ref Lotte::Gui::SliderStyle, so an unthemed GUI still gets a usable control.
 */
#include <Lotte/LotteLib.hpp>
#include <Lotte/Gui/LteGuiRenderer.hpp>
#include <Lotte/Gui/LteTheme.hpp>
#include <Lotte/Gui/LteWidget.hpp>
#include <Elvavena/Util/ElvSignal.hpp>
#include <glm/glm.hpp>
#include <cstdint>

namespace Lotte {
namespace Gui {

/** @brief Direction a @ref Slider travels in. */
enum class SliderOrientation : std::uint8_t {
	Horizontal, /**< Minimum at the left edge. */
	Vertical    /**< Minimum at the bottom edge. */
};

/**
 * @brief The look a slider falls back to when no theme skin is available.
 *
 * `thumbExtent` is the thumb's size as a fraction of the widget's short side, and
 * `grooveThickness` the groove's size along the short axis as a fraction of the same.
 * The defaults give a thumb that is comfortably grabbable in a 48 px-tall row.
 */
struct SliderStyle {
	/** @brief Groove fill when unthemed. */
	glm::fvec4 groove = glm::fvec4(0.12f, 0.14f, 0.19f, 1.0f);
	/** @brief Thumb fill for an idle enabled slider. */
	glm::fvec4 thumb = glm::fvec4(0.72f, 0.62f, 0.30f, 1.0f);
	/** @brief Thumb fill while hovered. */
	glm::fvec4 thumbHover = glm::fvec4(0.90f, 0.78f, 0.38f, 1.0f);
	/** @brief Thumb fill while dragged. */
	glm::fvec4 thumbActive = glm::fvec4(1.00f, 0.90f, 0.55f, 1.0f);
	/** @brief Thumb size along the short axis, as a fraction of that axis. */
	float thumbExtent = 0.80f;
	/** @brief Groove size along the short axis, as a fraction of that axis. */
	float grooveThickness = 0.45f;
};

/**
 * @brief A horizontal or vertical slider with a themeable groove and thumb.
 *
 * Input arrives through @ref onClick() for the press and the release, and through
 * @ref dragTo() for the motion in between: a scene that owns the pointer forwards
 * motion there for as long as @ref pressed() is true. That keeps the widget free of
 * any assumption about who polls the backend.
 */
class MH_LOTTE_API Slider : public Widget
{
public:
	/**
	 * @brief Creates a slider in `[0, 1]` with the value at the minimum.
	 * @param orientation Direction the control travels in.
	 */
	explicit Slider(SliderOrientation orientation = SliderOrientation::Horizontal);

	/// @name Range and value
	///@{
	/**
	 * @brief Replaces the range and snaps the current value into it.
	 * @param newMinimum Value at fraction 0.
	 * @param newMaximum Value at fraction 1; a maximum below the minimum is swapped.
	 * @param newStep Snap granularity; `0` leaves the value continuous.
	 */
	void setRange(float newMinimum, float newMaximum, float newStep = 0.0f);
	/** @brief Value at fraction 0. */
	float minimum() const noexcept { return rangeMinimum; }
	/** @brief Value at fraction 1. */
	float maximum() const noexcept { return rangeMaximum; }
	/** @brief Snap granularity; `0` means continuous. */
	float step() const noexcept { return rangeStep; }
	/**
	 * @brief Sets the value, clamped and snapped to @ref step().
	 * @param newValue Requested value.
	 *
	 * Emits @ref valueChanged only when the stored value actually changes.
	 */
	void setValue(float newValue);
	/** @brief The current value. */
	float value() const noexcept { return storedValue; }
	/** @brief Position of the thumb in `[0, 1]`. */
	float fraction() const noexcept;
	/** @brief Value that corresponds to @p newFraction, snapped like @ref setValue(). */
	float valueAtFraction(float newFraction) const;
	///@}

	/// @name Orientation and styling
	///@{
	/** @brief Direction the control travels in. */
	SliderOrientation orientation() const noexcept { return sliderOrientation; }
	/** @brief Replaces the direction without changing the value. */
	void setOrientation(SliderOrientation newOrientation) noexcept;
	/** @brief The colours used when no theme skin covers this control. */
	const SliderStyle& sliderStyle() const noexcept { return style; }
	/**
	 * @brief Replaces the fallback colours and proportions.
	 * @param newStyle Style to adopt.
	 */
	void setSliderStyle(const SliderStyle& newStyle) noexcept { style = newStyle; }
	/**
	 * @brief Draws through a theme's groove and thumb skins.
	 * @param newTheme Theme to query; null restores the self-contained look.
	 */
	void setTheme(const Theme* newTheme) noexcept { themedTheme = newTheme; }
	/** @brief The theme this slider draws through, or null. */
	const Theme* themedBy() const noexcept { return themedTheme; }
	///@}

	/// @name Pointer state
	///@{
	/**
	 * @brief Sets or clears the hover bit.
	 * @param hovered True when the pointer is over the control.
	 */
	void setHovered(bool hovered);
	/** @brief True while a press that landed inside is still held. */
	bool pressed() const noexcept { return getFlag(WidgetStateFlag::Clicked); }
	/**
	 * @brief Continues a drag that @ref onClick() started.
	 * @param offset Pointer position relative to @ref topLeft().
	 *
	 * Does nothing unless @ref pressed() is true, so a scene can forward every motion
	 * event unconditionally.
	 */
	void dragTo(const glm::fvec2& offset);
	///@}

	/// @name Widget
	///@{
	/**
	 * @brief Applies a press, a drag inside the widget or a release.
	 * @param offset Pointer position relative to @ref topLeft().
	 * @param button Backend mouse-button index; unused.
	 * @param state Zero for a release, non-zero for a press.
	 * @param clicks Click count of the event; unused.
	 * @return Always true: a slider consumes the event.
	 */
	bool onClick(const glm::fvec2& offset, std::uint8_t button,
				 std::uint8_t state, std::uint8_t clicks) override;
	/**
	 * @brief Follows the pointer while the thumb is held.
	 * @param offset Pointer position relative to @ref topLeft().
	 */
	void onPointerMove(const glm::fvec2& offset) override;
	/** @brief Draws the groove and the thumb. */
	void render(GuiRenderer& renderer) override;
	/** @brief Leaves: no children. */
	void iterateOverChildren(const ChildIterator& iterator) override;
	/** @brief Leaves: no children. */
	void iterateOverChildren(const ConstChildIterator& iterator) const override;
	///@}

	/** @brief Emitted with the new value whenever it changes. */
	Elv::Util::Signal<float> valueChanged;

protected:
	/** @brief No animation. */
	void timedUpdate(float deltaSeconds) override;

private:
	/** @brief Size of the thumb in pixels for the current geometry. */
	glm::fvec2 thumbSize() const noexcept;
	/**
	 * @brief Rectangle the thumb occupies in window pixels.
	 * @param topLeft Receives the top-left corner.
	 * @param bottomRight Receives the bottom-right corner.
	 */
	void thumbRect(glm::fvec2& topLeft, glm::fvec2& bottomRight) const;
	/**
	 * @brief Rectangle the groove occupies in window pixels.
	 * @param topLeft Receives the top-left corner.
	 * @param bottomRight Receives the bottom-right corner.
	 */
	void grooveRect(glm::fvec2& topLeft, glm::fvec2& bottomRight) const;
	/** @brief Value that a pointer at @p offset selects. */
	float valueFromOffset(const glm::fvec2& offset) const;
	/** @brief Colour the thumb is filled with in the current state. */
	glm::fvec4 thumbTint() const noexcept;
	/** @brief Clamps and snaps a candidate value into the range. */
	float normalise(float candidate) const;

	/** @brief Value at fraction 0. */
	float rangeMinimum = 0.0f;
	/** @brief Value at fraction 1. */
	float rangeMaximum = 1.0f;
	/** @brief Snap granularity; `0` means continuous. */
	float rangeStep = 0.0f;
	/** @brief The stored, already normalised value. */
	float storedValue = 0.0f;
	/** @brief Direction the control travels in. */
	SliderOrientation sliderOrientation = SliderOrientation::Horizontal;
	/** @brief Fallback colours and proportions. */
	SliderStyle style;
	/** @brief Theme this slider draws through; null keeps @ref style. */
	const Theme* themedTheme = nullptr;
};

} // namespace Gui
} // namespace Lotte

#endif // LTESLIDER_HPP
