/**
 * @file GuiSliderTests.cpp
 * @brief CPU tests for `Lotte::Gui::Slider`: value model, drag and skin selection.
 *
 * A slider needs no GPU: `GuiRenderer` is an interface, so a recording implementation
 * captures exactly what the widget submits - both the fallback quads and the theme
 * skins - while the value maths is exercised through the public API.
 */
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <Lotte/Gui/LteSlider.hpp>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace {

using namespace Lotte::Gui;

/** @brief Renderer that remembers which quads it was handed. */
class RecordingRenderer : public GuiRenderer {
public:
	std::vector<ColouredQuad> quads;
	std::vector<ColouredTexturedQuad> texturedQuads;

	void render(const ColouredQuad& target) override { quads.push_back(target); }
	void render(const GradientQuad& target) override { (void)target; }
	void render(const ColouredTriangle& target) override { (void)target; }
	void render(const GradientTriangle& target) override { (void)target; }
	void render(const ColouredLine& target) override { (void)target; }
	void render(const GradientLine& target) override { (void)target; }
	void render(const TexturedQuad& target) override { (void)target; }
	void render(const ColouredTexturedQuad& target) override { texturedQuads.push_back(target); }
	void render(const GradientTexturedQuad& target) override { (void)target; }
	void flush() override {}
};

/** @brief Theme that answers only for the two slider parts. */
class SliderTheme : public Theme {
public:
	std::string_view name() const override { return "SliderTheme"; }

	std::optional<ThemeSkin> skin(ThemeElement element, WidgetState state) const override
	{
		(void)state;
		if (element != ThemeElement::SliderTrack && element != ThemeElement::SliderThumb) {
			return std::nullopt;
		}
		ThemeSkin skin;
		skin.binding.texture = element == ThemeElement::SliderTrack ? 11u : 12u;
		skin.uv[0] = glm::fvec2(0.0f, 0.0f);
		skin.uv[1] = glm::fvec2(0.5f, 0.5f);
		return skin;
	}

	Lotte::Text::TextAttributes textStyle(ThemeTextRole role, WidgetState state) const override
	{
		(void)role;
		(void)state;
		return Lotte::Text::TextAttributes{};
	}

	ThemeMetrics metrics(ThemeElement element) const override
	{
		(void)element;
		return ThemeMetrics{};
	}
};

/**
 * @brief Configures a slider at a known rectangle.
 *
 * A widget is not movable (its signals are not), so the helper fills one in place
 * instead of returning it.
 */
void configureSlider(Slider& slider, SliderOrientation orientation = SliderOrientation::Horizontal)
{
	slider.setOrientation(orientation);
	slider.setRange(0.0f, 1.0f);
	slider.setPosition(glm::fvec2(0.0f, 0.0f), glm::fvec2(240.0f, 32.0f));
}

} // namespace

TEST_CASE("Slider snaps, clamps and only reports real changes", "[gui][slider]")
{
	Slider slider;
	slider.setRange(0.2f, 1.0f, 0.2f);

	std::vector<float> reported;
	slider.valueChanged.connect([&reported](float value) { reported.push_back(value); });

	slider.setValue(0.31f);
	REQUIRE(slider.value() == Catch::Approx(0.4f));
	REQUIRE(reported.size() == 1u);

	// The same value again must not emit: a drag produces many identical frames.
	slider.setValue(0.4f);
	REQUIRE(reported.size() == 1u);

	// Below the minimum clamps, above the maximum clamps, each is one emission.
	slider.setValue(-5.0f);
	REQUIRE(slider.value() == Catch::Approx(0.2f));
	slider.setValue(9.0f);
	REQUIRE(slider.value() == Catch::Approx(1.0f));
	REQUIRE(reported.size() == 3u);

	// A range change re-normalises and re-emits only when the value moves.
	slider.setRange(0.0f, 2.0f, 0.0f);
	REQUIRE(slider.value() == Catch::Approx(1.0f));
	REQUIRE(slider.fraction() == Catch::Approx(0.5f));
	REQUIRE(slider.valueAtFraction(0.25f) == Catch::Approx(0.5f));
}

TEST_CASE("Slider maps the pointer to a value and keeps the thumb inside",
		  "[gui][slider]")
{
	Slider slider;
	configureSlider(slider);
	// The thumb is 80% of the 32 px short side, so it travels over 240 - 25.6 px and is
	// grabbed by its centre.
	slider.setValue(0.5f);
	REQUIRE(slider.fraction() == Catch::Approx(0.5f));

	RecordingRenderer renderer;
	slider.render(renderer);
	REQUIRE(renderer.quads.size() == 2u);
	// Groove first, thumb second.
	const ColouredQuad& groove = renderer.quads[0];
	const ColouredQuad& thumb = renderer.quads[1];
	REQUIRE(groove.topLeft.x == Catch::Approx(0.0f));
	REQUIRE(groove.bottomRight.x == Catch::Approx(240.0f));
	REQUIRE(thumb.bottomRight.x - thumb.topLeft.x > 0.0f);
	REQUIRE(thumb.topLeft.y >= 0.0f);
	REQUIRE(thumb.bottomRight.y <= 32.0f);

	slider.setValue(0.0f);
	renderer.quads.clear();
	slider.render(renderer);
	REQUIRE(renderer.quads[1].topLeft.x == Catch::Approx(0.0f));
	slider.setValue(1.0f);
	renderer.quads.clear();
	slider.render(renderer);
	REQUIRE(renderer.quads[1].bottomRight.x == Catch::Approx(240.0f));
}

TEST_CASE("Slider drags between press and release and ignores a disabled state",
		  "[gui][slider]")
{
	Slider slider;
	configureSlider(slider);
	// A press on the groove jumps the value and starts the drag.
	REQUIRE(slider.onClick(glm::fvec2(0.0f, 16.0f), 1u, 1u, 1u));
	REQUIRE(slider.pressed());
	REQUIRE(slider.value() == Catch::Approx(0.0f));

	// Motion continues the drag, including outside the rectangle.
	slider.onPointerMove(glm::fvec2(120.0f, 16.0f));
	REQUIRE(slider.value() == Catch::Approx(0.5f));
	slider.onPointerMove(glm::fvec2(400.0f, -80.0f));
	REQUIRE(slider.value() == Catch::Approx(1.0f));

	// The release ends the drag; further motion is ignored.
	slider.onClick(glm::fvec2(400.0f, -80.0f), 1u, 0u, 1u);
	REQUIRE_FALSE(slider.pressed());
	slider.onPointerMove(glm::fvec2(0.0f, 16.0f));
	REQUIRE(slider.value() == Catch::Approx(1.0f));

	// A disabled slider consumes the event but changes nothing.
	slider.setFlag<WidgetStateFlag::Enabled>(false);
	REQUIRE(slider.onClick(glm::fvec2(0.0f, 16.0f), 1u, 1u, 1u));
	REQUIRE_FALSE(slider.pressed());
	REQUIRE(slider.value() == Catch::Approx(1.0f));
}

TEST_CASE("Slider draws theme skins when a theme has them", "[gui][slider]")
{
	Slider slider;
	configureSlider(slider);
	SliderTheme theme;
	slider.setTheme(&theme);

	RecordingRenderer renderer;
	slider.render(renderer);
	REQUIRE(renderer.quads.empty());
	REQUIRE(renderer.texturedQuads.size() == 2u);
	REQUIRE(renderer.texturedQuads[0].binding.texture == 11u);
	REQUIRE(renderer.texturedQuads[1].binding.texture == 12u);

	// Clearing the theme restores the self-contained look.
	slider.setTheme(nullptr);
	renderer.texturedQuads.clear();
	slider.render(renderer);
	REQUIRE(renderer.texturedQuads.empty());
	REQUIRE(renderer.quads.size() == 2u);
}

TEST_CASE("Slider vertical orientation puts the maximum at the top", "[gui][slider]")
{
	Slider slider;
	configureSlider(slider, SliderOrientation::Vertical);
	slider.setPosition(glm::fvec2(0.0f, 0.0f), glm::fvec2(32.0f, 240.0f));
	REQUIRE(slider.orientation() == SliderOrientation::Vertical);

	slider.setValue(1.0f);
	RecordingRenderer renderer;
	slider.render(renderer);
	REQUIRE(renderer.quads.size() == 2u);
	REQUIRE(renderer.quads[1].topLeft.y == Catch::Approx(0.0f));

	slider.setValue(0.0f);
	renderer.quads.clear();
	slider.render(renderer);
	REQUIRE(renderer.quads[1].bottomRight.y == Catch::Approx(240.0f));
}
