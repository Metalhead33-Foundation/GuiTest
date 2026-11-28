/**
 * @file GuiViewportTests.cpp
 * @brief CPU tests for `Lotte::Gui::GuiViewport`: the canvas-to-window mapping.
 *
 * The viewport is pure arithmetic, so these tests state the resolution-agnostic
 * promise exactly: an 800x600 design must fit a 640x480 window without clipping,
 * must keep one uniform factor for both axes, and must report the extra room a wider
 * window offers instead of silently moving the layout.
 */
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <Lotte/Gui/LteGuiViewport.hpp>
#include <cmath>
#include <glm/glm.hpp>
#include <string_view>

namespace {

using namespace Lotte::Gui;

/** @brief The design resolution most of these cases use. */
ViewportSettings designSettings()
{
	ViewportSettings settings;
	settings.canvas = glm::fvec2(800.0f, 600.0f);
	settings.mode = ScaleMode::Fit;
	return settings;
}

} // namespace

TEST_CASE("A fitted canvas shrinks into a smaller window without cropping", "[gui][viewport]")
{
	GuiViewport viewport(designSettings(), glm::fvec2(640.0f, 480.0f));

	// 640/800 and 480/600 are both 0.8: the same aspect ratio keeps the scale exact
	// and leaves no letterbox at all.
	CHECK(viewport.scale() == Catch::Approx(0.8f));
	CHECK(viewport.visibleSize().x == Catch::Approx(800.0f));
	CHECK(viewport.visibleSize().y == Catch::Approx(600.0f));
	CHECK(viewport.canvasOrigin().x == Catch::Approx(0.0f));
	CHECK(viewport.canvasOrigin().y == Catch::Approx(0.0f));

	// The bottom-right corner of the canvas is the bottom-right corner of the window,
	// which is what "the exit button is no longer chopped off" means in numbers.
	const glm::fvec2 corner = viewport.toPixels(glm::fvec2(800.0f, 600.0f));
	CHECK(corner.x == Catch::Approx(640.0f));
	CHECK(corner.y == Catch::Approx(480.0f));

	// A caption authored at 34 canvas units is 27.2 real pixels tall.
	CHECK(viewport.lengthToPixels(34.0f) == Catch::Approx(27.2f));
}

TEST_CASE("A canvas at its own resolution maps one to one", "[gui][viewport]")
{
	GuiViewport viewport(designSettings(), glm::fvec2(800.0f, 600.0f));

	CHECK(viewport.scale() == Catch::Approx(1.0f));
	const glm::fvec2 point = viewport.toPixels(glm::fvec2(160.0f, 240.0f));
	CHECK(viewport.toCanvas(point).x == Catch::Approx(160.0f));
	CHECK(viewport.toCanvas(point).y == Catch::Approx(240.0f));
}
TEST_CASE("A wider window keeps the fit scale and reports the extra room", "[gui][viewport]")
{
	GuiViewport viewport(designSettings(), glm::fvec2(1920.0f, 1080.0f));

	// Height decides the fit (1080/600 = 1.8 < 1920/800 = 2.4), so the window shows
	// 1066.67 canvas units across: the canvas is centred and the spare room is what a
	// future layout policy may grow into.
	CHECK(viewport.scale() == Catch::Approx(1.8f));
	CHECK(viewport.visibleSize().y == Catch::Approx(600.0f));
	CHECK(viewport.visibleSize().x == Catch::Approx(1920.0f / 1.8f));
	CHECK(viewport.canvasOrigin().x == Catch::Approx((1920.0f / 1.8f - 800.0f) * 0.5f));
	CHECK(viewport.canvasOrigin().y == Catch::Approx(0.0f));
	CHECK(viewport.canvasOrigin().x > 0.0f);

	// The canvas is centred: its left edge is *not* at x = 0 of the window.
	const glm::fvec2 left = viewport.toPixels(glm::fvec2(0.0f, 0.0f));
	CHECK(left.x == Catch::Approx(viewport.canvasOrigin().x * 1.8f));
	CHECK(left.x > 0.0f);
}

TEST_CASE("A taller window than the canvas ratio letterboxes the canvas", "[gui][viewport]")
{
	GuiViewport viewport(designSettings(), glm::fvec2(800.0f, 900.0f));

	// Width decides the fit here, so the canvas is as wide as the window and centred
	// vertically.
	CHECK(viewport.scale() == Catch::Approx(1.0f));
	CHECK(viewport.visibleSize().y == Catch::Approx(900.0f));
	CHECK(viewport.canvasOrigin().y == Catch::Approx(150.0f));
	const glm::fvec2 top = viewport.toPixels(glm::fvec2(0.0f, 0.0f));
	CHECK(top.y == Catch::Approx(150.0f));
}

TEST_CASE("Fill covers the window instead of letterboxing it", "[gui][viewport]")
{
	ViewportSettings settings = designSettings();
	settings.mode = ScaleMode::Fill;
	GuiViewport viewport(settings, glm::fvec2(1920.0f, 1080.0f));

	CHECK(viewport.scale() == Catch::Approx(2.4f));
	CHECK(viewport.visibleSize().x == Catch::Approx(800.0f));
	CHECK(viewport.visibleSize().y == Catch::Approx(450.0f));
	// Cropped vertically by 75 units top and bottom, which the negative origin states.
	CHECK(viewport.canvasOrigin().y == Catch::Approx(-75.0f));
}

TEST_CASE("The scale multiplier and the clamps shape the final factor", "[gui][viewport]")
{
	ViewportSettings settings = designSettings();
	settings.uiScale = 1.25f;
	GuiViewport viewport(settings, glm::fvec2(640.0f, 480.0f));
	CHECK(viewport.scale() == Catch::Approx(1.0f)); // 0.8 * 1.25

	// A multiplier large enough pushes the canvas past the window: the origin turns
	// negative, which is the documented way of saying "cropped".
	settings.uiScale = 2.0f;
	viewport.setSettings(settings);
	CHECK(viewport.scale() == Catch::Approx(1.6f));
	CHECK(viewport.canvasOrigin().x == Catch::Approx((640.0f / 1.6f - 800.0f) * 0.5f));
	CHECK(viewport.canvasOrigin().x < 0.0f);

	// The clamps sit on top of the multiplier in both directions.
	settings.uiScale = 1.0f;
	settings.maximumScale = 0.5f;
	viewport.setSettings(settings);
	CHECK(viewport.scale() == Catch::Approx(0.5f));

	settings.maximumScale = 0.0f;
	settings.minimumScale = 2.0f;
	viewport.setSettings(settings);
	CHECK(viewport.scale() == Catch::Approx(2.0f));

	// A maximum below the minimum resolves to the minimum instead of oscillating.
	settings.minimumScale = 3.0f;
	settings.maximumScale = 1.0f;
	viewport.setSettings(settings);
	CHECK(viewport.scale() == Catch::Approx(3.0f));

	// A zero or negative multiplier is a mistake, not a request for an invisible GUI.
	settings.minimumScale = 0.0f;
	settings.maximumScale = 0.0f;
	settings.uiScale = 0.0f;
	viewport.setSettings(settings);
	CHECK(viewport.scale() == Catch::Approx(0.8f));
}


TEST_CASE("Canvas points and pixel points round trip", "[gui][viewport]")
{
	GuiViewport viewport(designSettings(), glm::fvec2(1024.0f, 768.0f));
	const glm::fvec2 points[] = {
		glm::fvec2(0.0f, 0.0f),
		glm::fvec2(400.0f, 300.0f),
		glm::fvec2(800.0f, 600.0f),
		glm::fvec2(-40.0f, 720.0f)
	};
	for (const glm::fvec2& point : points) {
		const glm::fvec2 back = viewport.toCanvas(viewport.toPixels(point));
		CHECK(back.x == Catch::Approx(point.x));
		CHECK(back.y == Catch::Approx(point.y));
	}
	CHECK(viewport.lengthToCanvas(viewport.lengthToPixels(37.0f)) == Catch::Approx(37.0f));
}

TEST_CASE("Normalized helpers are fractions of the canvas", "[gui][viewport]")
{
	GuiViewport viewport(designSettings(), glm::fvec2(1920.0f, 1080.0f));

	CHECK(viewport.normalizedToCanvas(glm::fvec2(0.5f, 0.5f)).x == Catch::Approx(400.0f));
	CHECK(viewport.normalizedToCanvas(glm::fvec2(0.5f, 0.5f)).y == Catch::Approx(300.0f));
	CHECK(viewport.normalizedToCanvas(glm::fvec2(1.0f, 1.0f)).y == Catch::Approx(600.0f));
	CHECK(viewport.canvasToNormalized(glm::fvec2(200.0f, 150.0f)).x == Catch::Approx(0.25f));
	CHECK(viewport.canvasToNormalized(glm::fvec2(200.0f, 150.0f)).y == Catch::Approx(0.25f));

	// The aspect ratio is the canvas's, not the window's: that is what a layout may
	// branch on without ever reading the real resolution.
	CHECK(viewport.aspect() == Catch::Approx(800.0f / 600.0f));
	CHECK(viewport.visibleAspect() == Catch::Approx(1920.0f / 1080.0f));
	CHECK(viewport.visibleAspect() != Catch::Approx(viewport.aspect()));
}

TEST_CASE("Degenerate input keeps the mapping finite", "[gui][viewport]")
{
	ViewportSettings settings;
	settings.canvas = glm::fvec2(0.0f, 0.0f);
	GuiViewport viewport(settings, glm::fvec2(0.0f, 0.0f));

	// A zero canvas and a minimised window are clamped rather than dividing by zero.
	CHECK(viewport.canvasSize().x >= 1.0f);
	CHECK(viewport.canvasSize().y >= 1.0f);
	CHECK(viewport.resolution().x >= 1.0f);
	CHECK(viewport.scale() > 0.0f);
	CHECK(std::isfinite(viewport.scale()));
	CHECK(std::isfinite(viewport.toPixels(glm::fvec2(1.0f, 1.0f)).x));
}

TEST_CASE("Scale mode names round trip", "[gui][viewport]")
{
	ScaleMode mode = ScaleMode::Fit;
	REQUIRE(parseScaleMode("fill", mode));
	CHECK(mode == ScaleMode::Fill);
	REQUIRE(parseScaleMode("fit", mode));
	CHECK(mode == ScaleMode::Fit);
	CHECK_FALSE(parseScaleMode("stretch", mode));
	CHECK(std::string_view(scaleModeName(ScaleMode::Fill)) == "fill");
	CHECK(scaleModeName(static_cast<ScaleMode>(2)) == nullptr);
}
