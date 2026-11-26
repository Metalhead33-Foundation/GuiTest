/**
 * @file ThemedWidgetTests.cpp
 * @brief Checks that a theme actually dresses a widget the scene draws.
 *
 * The theme work has three seams that can each fail silently: the scene's atlas table
 * (alias to texture/size/manifest), `DefinitionTheme` (definition to skin), and the
 * widget (`ScriptButton` preferring the theme's skin over its own colours). This file
 * drives all three through a stub renderer, so the assertion is about the quad the
 * renderer received rather than about any pixel on a screen.
 */
#include <catch2/catch_test_macros.hpp>
#include <GuiScene.hpp>
#include <Lotte/Gui/LteThemeDefinition.hpp>
#include <Lotte/Procedural/LteGuiAtlasParts.hpp>
#include <glm/glm.hpp>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace {

using namespace Lotte::Gui;

/// @brief Renderer that remembers the last textured quad it was given.
struct QuadSpy final : Lotte::Gui::GuiRenderer {
	int texturedQuads = 0;
	int colouredQuads = 0;
	ColouredTexturedQuad lastTextured;

	void render(const ColouredQuad&) override { ++colouredQuads; }
	void render(const GradientQuad&) override {}
	void render(const ColouredTriangle&) override {}
	void render(const GradientTriangle&) override {}
	void render(const ColouredLine&) override {}
	void render(const GradientLine&) override {}
	void render(const TexturedQuad& quad) override { ++texturedQuads; lastTextured.uv[0] = quad.uv[0]; lastTextured.uv[1] = quad.uv[1]; lastTextured.binding = quad.binding; }
	void render(const ColouredTexturedQuad& quad) override { ++texturedQuads; lastTextured = quad; }
	void render(const GradientTexturedQuad&) override {}
	void flush() override {}
};

/// @brief A uniform RGBA8 image of the given size.
Lotte::Gfx::Rgba8Image makeImage(std::uint16_t width, std::uint16_t height)
{
	Lotte::Gfx::Rgba8Image image;
	image.width = width;
	image.height = height;
	image.pixels.assign(std::size_t(width) * height * 4u, 0x80u);
	for (std::size_t index = 3; index < image.pixels.size(); index += 4) {
		image.pixels[index] = 0xFFu;
	}
	return image;
}

} // namespace

TEST_CASE("Themed widget: a skin from the theme decides the quad", "[theme][widget]")
{
	Lotte::CommandBuffer commands;
	Lotte::Gfx::ResourceFactory factory(commands);
	Sandbox::GuiScene scene(factory);
	REQUIRE(scene.addAtlas("T", makeImage(32, 16)));
	const std::optional<ThemeAtlasBinding> binding = scene.resolveAtlas("T");
	REQUIRE(binding.has_value());
	REQUIRE(binding->pixelSize == glm::fvec2(32.0f, 16.0f));
	REQUIRE_FALSE(static_cast<bool>(binding->partRect));
	REQUIRE_FALSE(scene.resolveAtlas("missing").has_value());

	ThemeDefinition definition;
	definition.name = "themed";
	ThemeAtlasDefinition atlas;
	atlas.alias = "T";
	atlas.path = "unused.png";
	definition.atlases.push_back(atlas);
	ThemeElementDefinition button;
	ThemeSkinVariant idle;
	idle.stateMask = 0;
	idle.skin.atlas = "T";
	idle.skin.rect = ThemeRect{0, 0, 16, 8};
	ThemeSkinVariant hover;
	hover.stateMask = static_cast<WidgetState>(WidgetStateFlag::InMouseFocus);
	hover.skin.atlas = "T";
	hover.skin.rect = ThemeRect{16, 0, 16, 8};
	button.skins = { idle, hover };
	definition.elements[static_cast<std::size_t>(ThemeElement::Button)] = button;

	DefinitionTheme theme(definition, [&scene](std::string_view alias) {
		return scene.resolveAtlas(alias);
	});

	Sandbox::ButtonStyle style;
	auto widget = std::make_shared<Sandbox::ScriptButton>(Lotte::Gui::GuiTextureBinding{}, style);
	widget->setPosition(glm::fvec2(0.0f, 0.0f), glm::fvec2(64.0f, 32.0f));
	scene.addButton(widget);
	// Installing the theme after the widget exists is what the sandbox does (the script
// runs after `startTheme`), so it has to dress the already-created widget too.
	scene.setTheme(&theme);
	REQUIRE(widget->themedBy() == &theme);

	QuadSpy spy;
	scene.render(spy);
	REQUIRE(spy.texturedQuads == 1);
	REQUIRE(spy.colouredQuads == 0);
	REQUIRE(spy.lastTextured.binding.texture == binding->binding.texture);
	REQUIRE(spy.lastTextured.uv[0] == glm::fvec2(0.0f, 0.0f));
	REQUIRE(spy.lastTextured.uv[1] == glm::fvec2(0.5f, 0.5f));

	// Hover switches to the theme's hover variant.
scene.setMousePosition(glm::fvec2(10.0f, 10.0f));
QuadSpy hoverSpy;
scene.render(hoverSpy);
REQUIRE(hoverSpy.texturedQuads == 1);
REQUIRE(hoverSpy.lastTextured.uv[0] == glm::fvec2(0.5f, 0.0f));
REQUIRE(hoverSpy.lastTextured.uv[1] == glm::fvec2(1.0f, 0.5f));

// A disabled widget keeps the idle skin but dims it, because the state mask cannot
// express "not enabled".
scene.setMousePosition(glm::fvec2(500.0f, 500.0f));
widget->setFlag<WidgetStateFlag::Enabled>(false);
QuadSpy disabledSpy;
scene.render(disabledSpy);
REQUIRE(disabledSpy.texturedQuads == 1);
REQUIRE(disabledSpy.lastTextured.colour.r < 1.0f);

// Clearing the theme puts the widget back on its own style (a coloured quad here).
scene.setTheme(nullptr);
QuadSpy plainSpy;
scene.render(plainSpy);
REQUIRE(plainSpy.colouredQuads == 1);
}

TEST_CASE("Themed widget: a part name resolves through the atlas manifest", "[theme][widget]")
{
Lotte::CommandBuffer commands;
Lotte::Gfx::ResourceFactory factory(commands);
Sandbox::GuiScene scene(factory);

Lotte::Procedural::GuiAtlas manifest;
manifest.parts.push_back(Lotte::Procedural::GuiAtlasRect{
Lotte::Procedural::GuiAtlasPart::ButtonIdle, 4, 4, 8, 8 });
REQUIRE(scene.addAtlas("P", makeImage(32, 16), &manifest));

ThemeDefinition definition;
definition.name = "parts";
ThemeAtlasDefinition atlas;
atlas.alias = "P";
atlas.path = "generated";
atlas.source = ThemeAtlasSource::Procedural;
definition.atlases.push_back(atlas);
ThemeElementDefinition button;
ThemeSkinVariant idle;
idle.skin.atlas = "P";
idle.skin.part = Lotte::Procedural::GuiAtlasPart::ButtonIdle;
button.skins = { idle };
definition.elements[static_cast<std::size_t>(ThemeElement::Button)] = button;
REQUIRE(validate(definition).empty());

DefinitionTheme theme(definition, [&scene](std::string_view alias) {
return scene.resolveAtlas(alias);
});
const std::optional<ThemeSkin> skin = theme.skin(ThemeElement::Button, 0u);
REQUIRE(skin.has_value());
// 4..12 x 4..12 inside a 32x16 atlas.
REQUIRE(skin->uv[0] == glm::fvec2(0.125f, 0.25f));
REQUIRE(skin->uv[1] == glm::fvec2(0.375f, 0.75f));

// A part the manifest does not know is not a skin.
idle.skin.part = Lotte::Procedural::GuiAtlasPart::Parchment;
button.skins = { idle };
definition.elements[static_cast<std::size_t>(ThemeElement::Button)] = button;
DefinitionTheme partial(definition, [&scene](std::string_view alias) {
return scene.resolveAtlas(alias);
});
REQUIRE_FALSE(partial.skin(ThemeElement::Button, 0u).has_value());
}
