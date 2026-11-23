/**
 * @file GuiWidgetTests.cpp
 * @brief CPU tests for the render tree, widget state machine and textured widget.
 *
 * The ported widget layer has no GPU dependency: `GuiRenderer` is an interface, so
 * a recording implementation captures exactly what a widget submits. These tests
 * verify z-index accumulation/stable ordering, root ownership, signal semantics,
 * hit testing and state-selected atlas coordinates.
 */
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <Lotte/Gui/LteGuiRootNode.hpp>
#include <Lotte/Gui/LteTexturedWidget.hpp>
#include <Lotte/Gui/LteTheme.hpp>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace Lotte::Gui;

/** @brief Renderable that records a name and forwards iteration to its children. */
class TestRenderable : public Renderable {
public:
	TestRenderable(std::string name, std::int32_t zIndex = 0)
		: name_(std::move(name))
	{
		setZIndex(zIndex);
	}

	void render(GuiRenderer& renderer) override { (void)renderer; }
	void iterateOverChildren(const ChildIterator& iterator) override
	{
		for (const auto& child : children) iterator(*child);
	}
	void iterateOverChildren(const ConstChildIterator& iterator) const override
	{
		for (const auto& child : children) iterator(*child);
	}

	const std::string& name() const noexcept { return name_; }
	std::vector<std::shared_ptr<Renderable>> children;

private:
	std::string name_;
};

/** @brief Minimal concrete widget used to exercise the base state/signal logic. */
class TestWidget : public Widget {
public:
	bool clicked = false;

	bool onClick(const glm::fvec2& offset, std::uint8_t button,
				 std::uint8_t state, std::uint8_t clicks) override
	{
		(void)offset;
		(void)button;
		(void)state;
		(void)clicks;
		clicked = true;
		return true;
	}
	void render(GuiRenderer& renderer) override { (void)renderer; }
	void iterateOverChildren(const ChildIterator& iterator) override { (void)iterator; }
	void iterateOverChildren(const ConstChildIterator& iterator) const override { (void)iterator; }

protected:
	void timedUpdate(float deltaSeconds) override { (void)deltaSeconds; }
};

/** @brief GuiRenderer that records textured submissions and ignores the rest. */
class RecordingGuiRenderer : public GuiRenderer {
public:
	std::vector<TexturedQuad> texturedQuads;
	int flushCount = 0;

	void render(const ColouredQuad& target) override { (void)target; }
	void render(const GradientQuad& target) override { (void)target; }
	void render(const ColouredTriangle& target) override { (void)target; }
	void render(const GradientTriangle& target) override { (void)target; }
	void render(const ColouredLine& target) override { (void)target; }
	void render(const GradientLine& target) override { (void)target; }
	void render(const TexturedQuad& target) override { texturedQuads.push_back(target); }
	void render(const ColouredTexturedQuad& target) override { (void)target; }
	void render(const GradientTexturedQuad& target) override { (void)target; }
	void flush() override { ++flushCount; }
};

/** @brief Concrete theme used only to prove the interface is implementable. */
class TestTheme : public Theme {
public:
	std::string_view name() const override { return "TestTheme"; }

	std::optional<ThemeSkin> skin(ThemeElement element, WidgetState state) const override
	{
		if (element != ThemeElement::Button || state != 1u) return std::nullopt;
		ThemeSkin skin;
		skin.binding.texture = 7;
		skin.uv[0] = glm::fvec2(0.25f, 0.5f);
		skin.uv[1] = glm::fvec2(0.75f, 1.0f);
		skin.slice = ThemeSlice{ 2.0f, 3.0f, 4.0f, 5.0f };
		skin.sourceSize = glm::fvec2(32.0f, 24.0f);
		return skin;
	}

	Lotte::Text::TextAttributes textStyle(ThemeTextRole role, WidgetState state) const override
	{
		(void)role;
		Lotte::Text::TextAttributes attributes;
		attributes.size = float(state) + 10.0f;
		return attributes;
	}

	ThemeMetrics metrics(ThemeElement element) const override
	{
		ThemeMetrics metrics;
		metrics.padding = glm::fvec2(float(element), 4.0f);
		return metrics;
	}
};

} // namespace

TEST_CASE("Renderable accumulates parent z indices and sorts stably", "[gui][widget]")
{
	auto root = std::make_shared<RootNode>();
	auto parent = std::make_shared<TestRenderable>("parent", 5);
	auto childA = std::make_shared<TestRenderable>("A", 1);
	auto childB = std::make_shared<TestRenderable>("B", -2);
	auto sibling = std::make_shared<TestRenderable>("sibling", 5);

	parent->children.push_back(childA);
	parent->children.push_back(childB);
	root->addChild(parent);
	root->addChild(sibling);

	RenderList renderList;
	root->updateRenderList(renderList, 0, true, false);
	REQUIRE(renderList.size() == 5);
	CHECK(renderList[0].second == root.get());
	CHECK(renderList[1].second == childB.get());
	CHECK(renderList[2].second == parent.get());
	CHECK(renderList[3].second == sibling.get());
	CHECK(renderList[4].second == childA.get());
	CHECK(renderList[0].first == 0);
	CHECK(renderList[1].first == 3);
	CHECK(renderList[2].first == 5);
	CHECK(renderList[3].first == 5);
	CHECK(renderList[4].first == 6);

	RenderList descendingList;
	root->updateRenderList(descendingList, 0, true, true);
	REQUIRE(descendingList.size() == 5);
	CHECK(descendingList[0].second == childA.get());
	CHECK(descendingList[1].second == parent.get());
	CHECK(descendingList[2].second == sibling.get());
	CHECK(descendingList[3].second == childB.get());
	CHECK(descendingList[4].second == root.get());

	std::int32_t minimum = std::numeric_limits<std::int32_t>::max();
	std::int32_t maximum = std::numeric_limits<std::int32_t>::min();
	RenderList rangedList;
	root->updateRenderList(rangedList, minimum, maximum, 0, true, false);
	CHECK(minimum == 0);
	CHECK(maximum == 6);
	CHECK(rangedList.size() == 5);
}

TEST_CASE("RootNode owns unique children and iterates in insertion order", "[gui][widget]")
{
	RootNode root;
	auto first = std::make_shared<TestRenderable>("first");
	auto second = std::make_shared<TestRenderable>("second");

	root.addChild(nullptr);
	root.addChild(first);
	root.addChild(first);
	root.addChild(second);
	CHECK(root.childCount() == 2);
	REQUIRE(root.children().size() == 2);
	CHECK(root.children()[0] == first);
	CHECK(root.children()[1] == second);

	std::vector<std::string> names;
	root.iterateOverChildren([&names](Renderable& child) {
		names.push_back(static_cast<TestRenderable&>(child).name());
	});
	CHECK(names == std::vector<std::string>{ "first", "second" });

	const RootNode& constRoot = root;
	std::size_t constCount = 0;
	constRoot.iterateOverChildren([&constCount](const Renderable&) { ++constCount; });
	CHECK(constCount == 2);

	CHECK(root.removeChild(first.get()));
	CHECK_FALSE(root.removeChild(first.get()));
	CHECK(root.removeChild(second));
	CHECK(root.childCount() == 0);
	root.clearChildren();
}

TEST_CASE("Widget state flags emit only when the bitmask changes", "[gui][widget]")
{
	TestWidget widget;
	std::vector<std::pair<WidgetState, WidgetState>> transitions;
	std::vector<std::pair<glm::fvec2, glm::fvec2>> positionEvents;

	auto stateConnection = widget.stateChanged.connect(
		[&transitions](Widget* source, WidgetState oldState, WidgetState newState) {
			CHECK(source != nullptr);
			transitions.emplace_back(oldState, newState);
		});
	auto positionConnection = widget.positionChanged.connect(
		[&positionEvents](Widget* source, glm::fvec2 topLeft, glm::fvec2 bottomRight) {
			CHECK(source != nullptr);
			positionEvents.emplace_back(topLeft, bottomRight);
		});

	CHECK(widget.state() == 0u);
	CHECK(widget.stateIndex() == 0u);
	widget.setFlag(true, WidgetStateFlag::Enabled);
	widget.setFlag(true, WidgetStateFlag::Enabled);
	REQUIRE(transitions.size() == 1);
	CHECK(transitions[0].first == 0u);
	CHECK(transitions[0].second == static_cast<WidgetState>(WidgetStateFlag::Enabled));
	CHECK(widget.getFlag<WidgetStateFlag::Enabled>());

	widget.setFlag<WidgetStateFlag::Clicked>(true);
	widget.setFlag(true, WidgetStateFlag::InMouseFocus);
	REQUIRE(transitions.size() == 3);
	CHECK(widget.stateIndex() == 7u);
	CHECK(widget.state() == 7u);

	widget.setFlag<WidgetStateFlag::Clicked>(false);
	REQUIRE(transitions.size() == 4);
	CHECK(widget.stateIndex() == 5u);
	CHECK_FALSE(widget.getFlag(WidgetStateFlag::Clicked));
	CHECK(widget.getFlag(WidgetStateFlag::InMouseFocus));

	widget.setPosition(glm::fvec2(10.0f, 20.0f), glm::fvec2(30.0f, 40.0f));
	REQUIRE(positionEvents.size() == 1);
	CHECK(positionEvents[0].first == glm::fvec2(10.0f, 20.0f));
	CHECK(positionEvents[0].second == glm::fvec2(30.0f, 40.0f));
	CHECK(widget.size() == glm::fvec2(20.0f, 20.0f));
	CHECK(widget.contains(glm::fvec2(15.0f, 25.0f)));
	CHECK_FALSE(widget.contains(glm::fvec2(9.0f, 25.0f)));

	widget.setPosition(glm::fvec2(30.0f, 40.0f), glm::fvec2(10.0f, 20.0f));
	CHECK(widget.contains(glm::fvec2(15.0f, 25.0f)));
	CHECK(widget.contains(glm::fvec2(10.0f, 20.0f)));

	CHECK_FALSE(widget.hidden());
	widget.setHidden(true);
	CHECK(widget.hidden());
	widget.setHidden(false);
	CHECK_FALSE(widget.hidden());

	CHECK(widget.onClick(glm::fvec2(1.0f, 2.0f), 1, 1, 1));
	CHECK(widget.clicked);
	CHECK(stateConnection.isConnected());
	CHECK(positionConnection.isConnected());
}


TEST_CASE("TexturedWidget selects atlas coordinates from its state", "[gui][widget]")
{
	GuiTextureBinding binding;
	binding.texture = 42;
	binding.sampler = 7;

	std::array<TextureCoordDuo, WidgetStateCount> coordinates;
	for (std::size_t index = 0; index < coordinates.size(); ++index) {
		coordinates[index].first = glm::fvec2(0.0f, float(index) / float(WidgetStateCount));
		coordinates[index].second = glm::fvec2(1.0f, float(index + 1) / float(WidgetStateCount));
	}

	TexturedWidget widget(binding, std::span<const TextureCoordDuo>(coordinates));
	widget.setPosition(glm::fvec2(10.0f, 20.0f), glm::fvec2(30.0f, 40.0f));
	CHECK(widget.binding().texture == binding.texture);
	CHECK(widget.binding().sampler == binding.sampler);
	CHECK(widget.stateIndex() == 0u);
	CHECK(widget.currentAtlasCoordinate().first.y == Catch::Approx(0.0f));

	RecordingGuiRenderer renderer;
	widget.setFlag(true, WidgetStateFlag::Enabled);
	widget.render(renderer);
	REQUIRE(renderer.texturedQuads.size() == 1);
	CHECK(renderer.texturedQuads[0].topLeft == glm::fvec2(10.0f, 20.0f));
	CHECK(renderer.texturedQuads[0].bottomRight == glm::fvec2(30.0f, 40.0f));
	CHECK(renderer.texturedQuads[0].binding.texture == binding.texture);
	CHECK(renderer.texturedQuads[0].binding.sampler == binding.sampler);
	CHECK(renderer.texturedQuads[0].uv[0].y == Catch::Approx(1.0f / 8.0f));
	CHECK(renderer.texturedQuads[0].uv[1].y == Catch::Approx(2.0f / 8.0f));

	widget.setFlag(true, WidgetStateFlag::Clicked);
	widget.render(renderer);
	REQUIRE(renderer.texturedQuads.size() == 2);
	CHECK(renderer.texturedQuads[1].uv[0].y == Catch::Approx(3.0f / 8.0f));
	CHECK(renderer.texturedQuads[1].uv[1].y == Catch::Approx(4.0f / 8.0f));

	widget.setFlag(true, WidgetStateFlag::InMouseFocus);
	widget.render(renderer);
	REQUIRE(renderer.texturedQuads.size() == 3);
	CHECK(renderer.texturedQuads[2].uv[0].y == Catch::Approx(7.0f / 8.0f));
	CHECK(renderer.texturedQuads[2].uv[1].y == Catch::Approx(1.0f));

	widget.setHidden(true);
	widget.render(renderer);
	CHECK(renderer.texturedQuads.size() == 3);
	widget.setHidden(false);

	binding.texture = 0;
	widget.setBinding(binding);
	widget.render(renderer);
	CHECK(renderer.texturedQuads.size() == 3);

	binding.texture = 43;
	widget.setBinding(binding);
	widget.setAtlasCoordinate(0, TextureCoordDuo{ glm::fvec2(0.1f, 0.2f),
												  glm::fvec2(0.3f, 0.4f) });
	CHECK(widget.atlasCoordinate(0).first == glm::fvec2(0.1f, 0.2f));
	CHECK(widget.atlasCoordinate(0).second == glm::fvec2(0.3f, 0.4f));
	widget.setAtlasCoordinates({ TextureCoordDuo{ glm::fvec2(0.5f, 0.6f),
												 glm::fvec2(0.7f, 0.8f) } });
	CHECK(widget.atlasCoordinate(0).first == glm::fvec2(0.5f, 0.6f));
	CHECK(widget.atlasCoordinate(1).first.y == Catch::Approx(1.0f / 8.0f));
	CHECK_THROWS_AS(widget.atlasCoordinate(WidgetStateCount), std::out_of_range);

	CHECK_FALSE(widget.onClick(glm::fvec2(0.0f), 1, 1, 1));
	CHECK(renderer.flushCount == 0);
}

TEST_CASE("Theme exposes optional skins, text styles and metrics", "[gui][theme]")
{
	static_assert(std::is_abstract_v<Renderable>);
	static_assert(std::is_abstract_v<Widget>);
	static_assert(std::is_abstract_v<Theme>);

	TestTheme theme;
	CHECK(theme.name() == "TestTheme");
	CHECK_FALSE(theme.skin(ThemeElement::Button, 0).has_value());
	const std::optional<ThemeSkin> skin = theme.skin(ThemeElement::Button, 1);
	REQUIRE(skin.has_value());
	CHECK(skin->binding.texture == 7);
	CHECK(skin->uv[0] == glm::fvec2(0.25f, 0.5f));
	CHECK(skin->uv[1] == glm::fvec2(0.75f, 1.0f));
	CHECK(skin->slice.left == 2.0f);
	CHECK(skin->slice.top == 3.0f);
	CHECK(skin->slice.right == 4.0f);
	CHECK(skin->slice.bottom == 5.0f);
	CHECK(skin->sourceSize == glm::fvec2(32.0f, 24.0f));

	CHECK(theme.textStyle(ThemeTextRole::Heading, 5).size == Catch::Approx(15.0f));
	CHECK(theme.metrics(ThemeElement::Button).padding.x ==
		  Catch::Approx(float(ThemeElement::Button)));

	const ThemeSkin defaultSkin;
	CHECK(defaultSkin.uv[0] == glm::fvec2(0.0f));
	CHECK(defaultSkin.uv[1] == glm::fvec2(1.0f));
	CHECK(defaultSkin.tint == glm::fvec4(1.0f));
	CHECK(defaultSkin.sourceSize == glm::fvec2(0.0f));
	const ThemeMetrics defaultMetrics;
	CHECK(defaultMetrics.borderWidth == 0.0f);
	CHECK(defaultMetrics.cornerRadius == 0.0f);
}

