#include "LteSlider.hpp"
#include <algorithm>
#include <cmath>
#include <optional>

namespace Lotte {
namespace Gui {

namespace {

/** @brief Clamps one component into `[0, 1]`. */
float clamp01(float value) noexcept
{
	return std::clamp(value, 0.0f, 1.0f);
}

} // namespace

Slider::Slider(SliderOrientation orientation) : sliderOrientation(orientation)
{
	// Like every other widget, a slider is usable the moment it exists: a caller that
	// wants a disabled control clears the bit itself.
	setFlag<WidgetStateFlag::Enabled>(true);
}

void Slider::setRange(float newMinimum, float newMaximum, float newStep)
{
	if (newMaximum < newMinimum) {
		std::swap(newMinimum, newMaximum);
	}
	rangeMinimum = newMinimum;
	rangeMaximum = newMaximum;
	rangeStep = newStep > 0.0f ? newStep : 0.0f;
	// Re-normalising also re-emits when clamping moved the value, which is what a
	// caller needs to keep its model in sync after a range change.
	setValue(storedValue);
}

float Slider::normalise(float candidate) const
{
	if (rangeMaximum <= rangeMinimum) {
		return rangeMinimum;
	}
	float result = std::clamp(candidate, rangeMinimum, rangeMaximum);
	if (rangeStep > 0.0f) {
		// Snapping is relative to the minimum, so a `[0.2, 1]` range with a 0.2 step
		// lands on 0.2 / 0.4 / ... rather than on multiples of 0.2 from zero.
		const float steps = std::round((result - rangeMinimum) / rangeStep);
		result = rangeMinimum + steps * rangeStep;
		result = std::clamp(result, rangeMinimum, rangeMaximum);
	}
	return result;
}

void Slider::setValue(float newValue)
{
	const float normalised = normalise(newValue);
	if (normalised == storedValue) {
		return;
	}
	storedValue = normalised;
	valueChanged(storedValue);
}

float Slider::fraction() const noexcept
{
	if (rangeMaximum <= rangeMinimum) {
		return 0.0f;
	}
	return clamp01((storedValue - rangeMinimum) / (rangeMaximum - rangeMinimum));
}

float Slider::valueAtFraction(float newFraction) const
{
	const float t = clamp01(newFraction);
	return normalise(rangeMinimum + t * (rangeMaximum - rangeMinimum));
}

void Slider::setOrientation(SliderOrientation newOrientation) noexcept
{
	sliderOrientation = newOrientation;
}

glm::fvec2 Slider::thumbSize() const noexcept
{
	const glm::fvec2 extent = glm::max(size(), glm::fvec2(0.0f));
	const float shortSide = std::min(extent.x, extent.y);
	const float thumb = shortSide * std::max(style.thumbExtent, 0.0f);
	// The thumb never grows past the widget it sits in, however lopsided the row is.
	const float side = std::min(shortSide, thumb);
	return glm::fvec2(side);
}

void Slider::grooveRect(glm::fvec2& topLeft, glm::fvec2& bottomRight) const
{
	const glm::fvec2 origin = this->topLeft();
	const glm::fvec2 extent = glm::max(size(), glm::fvec2(0.0f));
	const float thickness = std::max(style.grooveThickness, 0.0f);
	if (sliderOrientation == SliderOrientation::Horizontal) {
		const float half = extent.y * thickness * 0.5f;
		const float mid = origin.y + extent.y * 0.5f;
		topLeft = glm::fvec2(origin.x, mid - half);
		bottomRight = glm::fvec2(origin.x + extent.x, mid + half);
		return;
	}
	const float half = extent.x * thickness * 0.5f;
	const float mid = origin.x + extent.x * 0.5f;
	topLeft = glm::fvec2(mid - half, origin.y);
	bottomRight = glm::fvec2(mid + half, origin.y + extent.y);
}

void Slider::thumbRect(glm::fvec2& topLeft, glm::fvec2& bottomRight) const
{
	const glm::fvec2 origin = this->topLeft();
	const glm::fvec2 extent = glm::max(size(), glm::fvec2(0.0f));
	const glm::fvec2 thumb = thumbSize();
	const float t = fraction();
	if (sliderOrientation == SliderOrientation::Horizontal) {
		const float travel = std::max(extent.x - thumb.x, 0.0f);
		const float x = origin.x + travel * t;
		const float y = origin.y + (extent.y - thumb.y) * 0.5f;
		topLeft = glm::fvec2(x, y);
		bottomRight = glm::fvec2(x + thumb.x, y + thumb.y);
		return;
	}
	const float travel = std::max(extent.y - thumb.y, 0.0f);
	// Vertical sliders grow upwards, so the maximum sits at the top edge.
	const float y = origin.y + travel * (1.0f - t);
	const float x = origin.x + (extent.x - thumb.x) * 0.5f;
	topLeft = glm::fvec2(x, y);
	bottomRight = glm::fvec2(x + thumb.x, y + thumb.y);
}

float Slider::valueFromOffset(const glm::fvec2& offset) const
{
	const glm::fvec2 extent = glm::max(size(), glm::fvec2(0.0f));
	const glm::fvec2 thumb = thumbSize();
	if (sliderOrientation == SliderOrientation::Horizontal) {
		const float travel = std::max(extent.x - thumb.x, 0.0f);
		if (travel <= 0.0f) {
			return rangeMinimum;
		}
		// The pointer grabs the thumb's centre, so the first half thumb of travel is
		// already fraction zero instead of clamping early.
		const float centre = offset.x - thumb.x * 0.5f;
		return valueAtFraction(centre / travel);
	}
	const float travel = std::max(extent.y - thumb.y, 0.0f);
	if (travel <= 0.0f) {
		return rangeMinimum;
	}
	const float centre = offset.y - thumb.y * 0.5f;
	return valueAtFraction(1.0f - centre / travel);
}

void Slider::setHovered(bool hovered)
{
	setFlag<WidgetStateFlag::InMouseFocus>(hovered);
}

void Slider::dragTo(const glm::fvec2& offset)
{
	if (!pressed() || !getFlag(WidgetStateFlag::Enabled)) {
		return;
	}
	setValue(valueFromOffset(offset));
}

bool Slider::onClick(const glm::fvec2& offset, std::uint8_t button,
					 std::uint8_t state, std::uint8_t clicks)
{
	(void)button;
	(void)clicks;
	if (!getFlag(WidgetStateFlag::Enabled)) {
		return true;
	}
	const glm::fvec2 extent = size();
	const bool inside = offset.x >= 0.0f && offset.y >= 0.0f &&
						offset.x <= extent.x && offset.y <= extent.y;
	if (state != 0) {
		if (!inside) {
			return true;
		}
		setFlag<WidgetStateFlag::Clicked>(true);
		// Pressing the groove is a jump-and-drag, the same gesture a scrollbar has.
		setValue(valueFromOffset(offset));
		return true;
	}
	// A release ends the drag whether or not it landed inside: the thumb follows the
	// pointer, so it may well have been dragged past the edge and released there.
	if (pressed()) {
		setValue(valueFromOffset(offset));
	}
	setFlag<WidgetStateFlag::Clicked>(false);
	return true;
}

void Slider::onPointerMove(const glm::fvec2& offset)
{
	dragTo(offset);
}

glm::fvec4 Slider::thumbTint() const noexcept
{
	if (!getFlag(WidgetStateFlag::Enabled)) {
		return style.thumb * glm::fvec4(0.55f, 0.55f, 0.55f, 0.70f);
	}
	if (pressed()) {
		return style.thumbActive;
	}
	if (getFlag(WidgetStateFlag::InMouseFocus)) {
		return style.thumbHover;
	}
	return style.thumb;
}

void Slider::render(GuiRenderer& renderer)
{
	if (hidden()) {
		return;
	}
	const bool enabled = getFlag(WidgetStateFlag::Enabled);
	glm::fvec2 grooveTopLeft;
	glm::fvec2 grooveBottomRight;
	grooveRect(grooveTopLeft, grooveBottomRight);
	glm::fvec2 thumbTopLeft;
	glm::fvec2 thumbBottomRight;
	thumbRect(thumbTopLeft, thumbBottomRight);

	const std::optional<ThemeSkin> trackSkin =
		themedTheme != nullptr ? themedTheme->skin(ThemeElement::SliderTrack, state()) : std::nullopt;
	const std::optional<ThemeSkin> thumbSkin =
		themedTheme != nullptr ? themedTheme->skin(ThemeElement::SliderThumb, state()) : std::nullopt;

	if (trackSkin.has_value() && trackSkin->binding.texture != 0) {
		ColouredTexturedQuad quad;
		quad.topLeft = grooveTopLeft;
		quad.bottomRight = grooveBottomRight;
		quad.uv[0] = trackSkin->uv[0];
		quad.uv[1] = trackSkin->uv[1];
		quad.binding = trackSkin->binding;
		quad.colour = trackSkin->tint;
		if (!enabled) {
			quad.colour *= glm::fvec4(0.55f, 0.55f, 0.55f, 0.75f);
		}
		renderer.render(quad);
	} else {
		ColouredQuad quad;
		quad.topLeft = grooveTopLeft;
		quad.bottomRight = grooveBottomRight;
		quad.colour = enabled ? style.groove
							 : style.groove * glm::fvec4(0.55f, 0.55f, 0.55f, 0.75f);
		renderer.render(quad);
	}

	if (thumbSkin.has_value() && thumbSkin->binding.texture != 0) {
		ColouredTexturedQuad quad;
		quad.topLeft = thumbTopLeft;
		quad.bottomRight = thumbBottomRight;
		quad.uv[0] = thumbSkin->uv[0];
		quad.uv[1] = thumbSkin->uv[1];
		quad.binding = thumbSkin->binding;
		quad.colour = thumbSkin->tint;
		if (!enabled) {
			quad.colour *= glm::fvec4(0.55f, 0.55f, 0.55f, 0.75f);
		}
		renderer.render(quad);
	} else {
		ColouredQuad quad;
		quad.topLeft = thumbTopLeft;
		quad.bottomRight = thumbBottomRight;
		quad.colour = thumbTint();
		renderer.render(quad);
	}
}

void Slider::iterateOverChildren(const ChildIterator& iterator)
{
	(void)iterator;
}

void Slider::iterateOverChildren(const ConstChildIterator& iterator) const
{
	(void)iterator;
}

void Slider::timedUpdate(float deltaSeconds)
{
	(void)deltaSeconds;
}

} // namespace Gui
} // namespace Lotte
