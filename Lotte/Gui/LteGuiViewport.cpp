#include <Lotte/Gui/LteGuiViewport.hpp>
#include <algorithm>
#include <array>

namespace Lotte {
namespace Gui {

namespace {

/** @brief Smallest scale accepted, so a zero-size window cannot divide by zero. */
constexpr float MinimumUsefulScale = 1.0e-4f;

/** @brief Smallest canvas extent accepted; a canvas is a design size, never zero. */
constexpr float MinimumCanvasExtent = 1.0f;

} // namespace

const char* scaleModeName(ScaleMode mode) noexcept
{
	static constexpr std::array<const char*, 2> names = { "fit", "fill" };
	const auto index = static_cast<std::size_t>(mode);
	return index < names.size() ? names[index] : nullptr;
}

bool parseScaleMode(std::string_view name, ScaleMode& mode) noexcept
{
	static constexpr std::array<ScaleMode, 2> modes = { ScaleMode::Fit, ScaleMode::Fill };
	for (std::size_t index = 0; index < modes.size(); ++index) {
		const char* candidate = scaleModeName(modes[index]);
		if (candidate != nullptr && name == candidate) {
			mode = modes[index];
			return true;
		}
	}
	return false;
}

GuiViewport::GuiViewport(ViewportSettings settings, glm::fvec2 resolution)
{
	// Both inputs are sanitised by the setters, so a zero canvas or a minimised window
	// cannot reach the mapping through the constructor either.
	setSettings(settings);
	setResolution(resolution);
}

void GuiViewport::setSettings(const ViewportSettings& settings)
{
	viewportSettings = settings;
	canvas = glm::fvec2(std::max(settings.canvas.x, MinimumCanvasExtent),
						std::max(settings.canvas.y, MinimumCanvasExtent));
	recompute();
}

void GuiViewport::setResolution(glm::fvec2 resolution)
{
	// A window can report zero while it is being minimised; clamping here keeps every
	// derived number finite instead of pushing infinities into a layout.
	pixelSize = glm::fvec2(std::max(resolution.x, 1.0f), std::max(resolution.y, 1.0f));
	recompute();
}

void GuiViewport::recompute() noexcept
{
	const float fitX = pixelSize.x / canvas.x;
	const float fitY = pixelSize.y / canvas.y;
	// One factor for both axes: `Fit` never crops the canvas, `Fill` never leaves a bar.
	const float fitted = viewportSettings.mode == ScaleMode::Fill ? std::max(fitX, fitY)
																: std::min(fitX, fitY);
	const float multiplier = viewportSettings.uiScale > 0.0f ? viewportSettings.uiScale : 1.0f;
	float scale = fitted * multiplier;
	if (viewportSettings.minimumScale > 0.0f) {
		scale = std::max(scale, viewportSettings.minimumScale);
	}
	if (viewportSettings.maximumScale > 0.0f) {
		// A maximum below the minimum is a configuration mistake; the minimum wins so the
		// result stays deterministic rather than depending on the clamp order.
		scale = std::min(scale, std::max(viewportSettings.maximumScale,
										viewportSettings.minimumScale));
	}
	canvasScale = std::max(scale, MinimumUsefulScale);
	visible = pixelSize / canvasScale;
	// Centring is what makes a letterbox look deliberate and what keeps a cropped canvas
	// centred on the thing the author cared about.
	origin = (visible - canvas) * 0.5f;
}

glm::fvec2 GuiViewport::toPixels(glm::fvec2 point) const noexcept
{
	return (point + origin) * canvasScale;
}

glm::fvec2 GuiViewport::toCanvas(glm::fvec2 point) const noexcept
{
	return point / canvasScale - origin;
}

float GuiViewport::lengthToPixels(float length) const noexcept
{
	return length * canvasScale;
}

float GuiViewport::lengthToCanvas(float length) const noexcept
{
	return length / canvasScale;
}

glm::fvec2 GuiViewport::normalizedToCanvas(glm::fvec2 fraction) const noexcept
{
	return fraction * canvas;
}

glm::fvec2 GuiViewport::canvasToNormalized(glm::fvec2 point) const noexcept
{
	return point / canvas;
}

} // namespace Gui
} // namespace Lotte
