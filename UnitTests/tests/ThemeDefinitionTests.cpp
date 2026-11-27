/**
 * @file ThemeDefinitionTests.cpp
 * @brief Checks the theme schema, its JSON loader and the resulting `Theme`.
 *
 * The schema (`LteThemeDefinition.hpp`) is the data form of a theme; the loader
 * (`Sandbox/ThemeJson.cpp`) is the only place that touches JSON. These checks cover
 * both halves and the seam between them: name tables, validation, state-variant
 * specificity, pixel-to-UV conversion and the "missing atlas means no skin" rule.
 */
#include <catch2/catch_test_macros.hpp>
#include <ThemeJson.hpp>
#include <Lotte/Gui/LteThemeDefinition.hpp>
#include <Lotte/Gui/LteThemeCss.hpp>
#include <glm/glm.hpp>
#include <array>
#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace Lotte::Gui;

/// @brief A small, valid document: one procedural atlas, one button, one role.
constexpr std::string_view MinimalTheme = R"JSON(
{
"name": "test_theme",
"texture_atlas": [
{ "alias": "PROC", "proc": "crpg_stone", "seed": 99, "scale": 50 }
],
"elements": {
"button": {
"skins": [
{ "atlas": "PROC", "rect": [4, 8, 40, 16] },
{ "states": ["hover"], "atlas": "PROC", "rect": [48, 8, 40, 16] }
],
"metrics": { "padding": [2, 3], "borderWidth": 1.5, "cornerRadius": 4 },
"text": { "face": "Vinque", "weight": 700, "size": 18, "colour": "#ffd166" }
}
},
"roles": {
"body": { "face": "Vinque", "size": 24, "italic": true }
}
}
)JSON";

/// @brief Resolver that pretends a 100x50 atlas exists under @p expectedAlias.
ThemeTextureResolver fixedResolver(std::string expectedAlias, glm::fvec2 size,
   Kld::HandleId texture = Kld::HandleId(7))
{
	return [expectedAlias = std::move(expectedAlias), size, texture](
   std::string_view alias) -> std::optional<ThemeAtlasBinding> {
		if (alias != expectedAlias) return std::nullopt;
		ThemeAtlasBinding binding;
		binding.binding.texture = texture;
		binding.pixelSize = size;
		return binding;
	};
}

} // namespace

TEST_CASE("Theme schema: every enum round-trips through its name", "[theme]")
{
	for (std::size_t index = 0; index < ThemeElementCount; ++index) {
		const auto element = static_cast<ThemeElement>(index);
		ThemeElement parsed = ThemeElement::Count;
		REQUIRE(parseThemeElement(themeElementName(element), parsed));
		REQUIRE(parsed == element);
	}
	for (std::size_t index = 0; index < ThemeTextRoleCount; ++index) {
		const auto role = static_cast<ThemeTextRole>(index);
		ThemeTextRole parsed = ThemeTextRole::Count;
		REQUIRE(parseThemeTextRole(themeTextRoleName(role), parsed));
		REQUIRE(parsed == role);
	}
	REQUIRE(parseThemeAtlasRecipe("crpg_stone", *new ThemeAtlasRecipe()));
	REQUIRE_FALSE(parseThemeElement("not_an_element", *new ThemeElement()));
	REQUIRE_FALSE(parseThemeAtlasRecipe("nope", *new ThemeAtlasRecipe()));
}

TEST_CASE("Theme JSON: a minimal document parses into the schema", "[theme][json]")
{
	const Sandbox::ThemeParseResult result = Sandbox::parseThemeJson(MinimalTheme);
	REQUIRE(result.problems.empty());
	REQUIRE(result.ok);
	REQUIRE(result.definition.name == "test_theme");
	REQUIRE(result.definition.atlases.size() == 1);
	const ThemeAtlasDefinition& atlas = result.definition.atlases.front();
	REQUIRE(atlas.alias == "PROC");
	REQUIRE(atlas.source == ThemeAtlasSource::Procedural);
	REQUIRE(atlas.recipe == ThemeAtlasRecipe::CrpgStone);
	REQUIRE(atlas.proceduralSeed == 99u);
	REQUIRE(atlas.proceduralScalePercent == 50u);

	const ThemeElementDefinition* button = result.definition.findElement(ThemeElement::Button);
	REQUIRE(button != nullptr);
	REQUIRE(button->skins.size() == 2);
	REQUIRE(button->skins[0].stateMask == 0u);
	REQUIRE(button->skins[1].stateMask ==
			static_cast<WidgetState>(WidgetStateFlag::InMouseFocus));
	REQUIRE(button->metrics.padding == glm::fvec2(2.0f, 3.0f));
	REQUIRE(button->metrics.borderWidth == 1.5f);
	REQUIRE(button->textStyle.has_value());
	REQUIRE(button->textStyle->face == "Vinque");
	REQUIRE(button->textStyle->weight == Lotte::Text::FontWeight::Bold);

	const ThemeTextStyleDefinition* body = result.definition.findRole(ThemeTextRole::Body);
	REQUIRE(body != nullptr);
	REQUIRE(body->size == 24.0f);
	REQUIRE(body->italic);
}

TEST_CASE("Theme JSON: problems are reported instead of thrown", "[theme][json]")
{
	const auto unknownElement = Sandbox::parseThemeJson(R"JSON({
"texture_atlas": [ { "alias": "A", "filename": "a.png" } ],
"elements": { "not_an_element": { } }
})JSON");
	REQUIRE_FALSE(unknownElement.ok);
	REQUIRE_FALSE(unknownElement.problems.empty());

	const auto unknownAtlas = Sandbox::parseThemeJson(R"JSON({
"texture_atlas": [ { "alias": "A", "filename": "a.png" } ],
"elements": { "button": { "skins": [ { "atlas": "MISSING", "rect": [0, 0, 4, 4] } ] } }
})JSON");
	REQUIRE_FALSE(unknownAtlas.ok);

	const auto emptyRect = Sandbox::parseThemeJson(R"JSON({
"texture_atlas": [ { "alias": "A", "filename": "a.png" } ],
"elements": { "panel": { "skins": [ { "atlas": "A", "rect": [0, 0, 0, 8] } ] } }
})JSON");
	REQUIRE_FALSE(emptyRect.ok);

	const auto duplicateAlias = Sandbox::parseThemeJson(R"JSON({
"texture_atlas": [ { "alias": "A", "filename": "a.png" },
                   { "alias": "A", "filename": "b.png" } ]
})JSON");
	REQUIRE_FALSE(duplicateAlias.ok);

	const auto broken = Sandbox::parseThemeJson("{ not json");
	REQUIRE_FALSE(broken.ok);
	REQUIRE_FALSE(broken.problems.empty());
}

TEST_CASE("Theme definition: the most specific state variant wins", "[theme]")
{
	const Sandbox::ThemeParseResult result = Sandbox::parseThemeJson(MinimalTheme);
	REQUIRE(result.ok);
	const DefinitionTheme theme(result.definition, fixedResolver("PROC", glm::fvec2(100.0f, 50.0f)));

	const std::optional<ThemeSkin> idle = theme.skin(ThemeElement::Button, 0u);
	REQUIRE(idle.has_value());
	REQUIRE(idle->binding.texture == Kld::HandleId(7));

	const std::optional<ThemeSkin> hover = theme.skin(
ThemeElement::Button, static_cast<WidgetState>(WidgetStateFlag::InMouseFocus));
	REQUIRE(hover.has_value());
	REQUIRE(hover->uv[0].x > idle->uv[0].x);

	// The pixel rect becomes a normalized UV pair over the atlas size.
	REQUIRE(std::abs(idle->uv[0].x - 0.04f) < 1.0e-5f);
	REQUIRE(std::abs(idle->uv[0].y - 0.16f) < 1.0e-5f);
	REQUIRE(std::abs(idle->uv[1].x - 0.44f) < 1.0e-5f);
	REQUIRE(std::abs(idle->uv[1].y - 0.48f) < 1.0e-5f);
	REQUIRE(idle->sourceSize == glm::fvec2(40.0f, 16.0f));

	// No definition, no skin.
	REQUIRE_FALSE(theme.skin(ThemeElement::Panel, 0u).has_value());
	// No resolver answer, no skin either.
	const DefinitionTheme orphan(result.definition, {});
	REQUIRE_FALSE(orphan.skin(ThemeElement::Button, 0u).has_value());
}

TEST_CASE("Theme definition: metrics and text styles fall back quietly", "[theme]")
{
	const Sandbox::ThemeParseResult result = Sandbox::parseThemeJson(MinimalTheme);
	REQUIRE(result.ok);
	const DefinitionTheme theme(result.definition, fixedResolver("PROC", glm::fvec2(100.0f, 50.0f)));

	REQUIRE(theme.metrics(ThemeElement::Button).borderWidth == 1.5f);
	REQUIRE(theme.metrics(ThemeElement::Panel).borderWidth == 0.0f);

	const Lotte::Text::TextAttributes body = theme.textStyle(ThemeTextRole::Body, 0u);
	REQUIRE(body.familyName == "Vinque");
	REQUIRE(body.size == 24.0f);
	REQUIRE(body.italic);
	REQUIRE(theme.textStyle(ThemeTextRole::Heading, 0u).size == 16.0f);

	const Lotte::Text::TextAttributes label =
		Lotte::Gui::toTextAttributes(result.definition.findElement(ThemeElement::Button)->textStyle.value());
	REQUIRE(label.isBold());
	REQUIRE(label.size == 18.0f);
	REQUIRE(std::abs(label.colour.r - 1.0f) < 1.0e-5f);
}

TEST_CASE("Theme definition: validation is order-stable and descriptive", "[theme]")
{
	ThemeDefinition definition;
	definition.name = "broken";
	definition.atlases.push_back(ThemeAtlasDefinition{});
	ThemeAtlasDefinition duplicate;
	duplicate.alias = "A";
	duplicate.path = "a.png";
	definition.atlases.push_back(duplicate);
	definition.atlases.push_back(duplicate);
	ThemeElementDefinition panel;
	panel.skins.push_back(ThemeSkinVariant{});
	definition.elements[static_cast<std::size_t>(ThemeElement::Panel)] = panel;
	ThemeTextStyleDefinition role;
	role.size = 0.0f;
	definition.roles[static_cast<std::size_t>(ThemeTextRole::Body)] = role;

	const std::vector<std::string> problems = validate(definition);
	REQUIRE(problems.size() >= 4);
	REQUIRE(problems.front().find("no alias") != std::string::npos);
}

TEST_CASE("Theme CSS: a declaration block styles text, metrics and the atlas", "[theme][css]")
{
	const Lotte::Gui::ThemeCssStyle style = Lotte::Gui::resolveThemeCss(
		"font-size: 18px; color: #ffd27f; font-weight: bold; font-style: italic; "
		"text-decoration: underline; padding: 6px; border-width: 3px; "
		"background-color: #102030");
	CAPTURE(style.problems);
	REQUIRE(style.valid);
	REQUIRE(style.text.size == 18.0f);
	REQUIRE(style.text.colour.r > 0.99f);
	REQUIRE(std::fabs(style.text.colour.g - 0.8235f) < 0.01f);
	REQUIRE(std::fabs(style.text.colour.b - 0.4980f) < 0.01f);
	REQUIRE(style.text.weight == Lotte::Text::FontWeight::Bold);
	REQUIRE(style.text.italic);
	REQUIRE(style.text.underline);
	REQUIRE(style.metrics.padding.x == 6.0f);
	REQUIRE(style.metrics.padding.y == 6.0f);
	REQUIRE(style.metrics.borderWidth == 3.0f);
	REQUIRE(style.atlas.hasBorderWidth);
	REQUIRE(style.atlas.borderWidth == 3.0f);
	REQUIRE(style.atlas.hasBackground);
	REQUIRE(std::fabs(style.atlas.background.b - 0.1882f) < 0.01f);
}

TEST_CASE("Theme CSS: what the document does not mention keeps its JSON value", "[theme][css]")
{
	// This used to be the trap: libcss's initial values (16 px, weight 400) flattened the
	// document's own numbers whenever a block mentioned only part of the style.
	Lotte::Gui::ThemeTextStyleDefinition base;
	base.size = 32.0f;
	base.weight = Lotte::Text::FontWeight::Bold;
	base.colour = Lotte::Text::Colour(0.0f, 1.0f, 0.0f, 1.0f);
	const Lotte::Gui::ThemeCssStyle style = Lotte::Gui::resolveThemeCss("color: #0000ff", base);
	REQUIRE(style.valid);
	REQUIRE(style.text.size == 32.0f);
	REQUIRE(style.text.weight == Lotte::Text::FontWeight::Bold);
	REQUIRE(style.text.colour.b > 0.99f);
	REQUIRE(style.text.colour.r < 0.01f);
}

TEST_CASE("Theme CSS: an empty block is reported, not applied", "[theme][css]")
{
	const Lotte::Gui::ThemeCssStyle style = Lotte::Gui::resolveThemeCss("");
	REQUIRE_FALSE(style.valid);
	REQUIRE_FALSE(style.problems.empty());
}

TEST_CASE("Theme definition: CSS roles are resolved when the theme is built", "[theme][css]")
{
	Sandbox::ThemeParseResult parsed = Sandbox::parseThemeJson(R"JSON({
		"name": "css_theme",
		"texture_atlas": [ { "alias": "A", "filename": "a.png" } ],
		"elements": {},
		"roles": { "heading": { "face": "Vinque", "size": 32, "weight": 700,
			"colour": "#ffffff", "css": "font-size: 40px; text-decoration: underline" } }
	})JSON");
	REQUIRE(parsed.ok);
	REQUIRE(parsed.definition.roles[static_cast<std::size_t>(Lotte::Gui::ThemeTextRole::Heading)]
				.has_value());
	const Lotte::Gui::DefinitionTheme theme(parsed.definition,
			[](std::string_view) -> std::optional<Lotte::Gui::ThemeAtlasBinding> {
				return std::nullopt;
			});
	const Lotte::Text::TextAttributes heading =
		theme.textStyle(Lotte::Gui::ThemeTextRole::Heading, 0u);
	REQUIRE(heading.size == 40.0f);
	REQUIRE(heading.underline);
	REQUIRE(heading.effectiveWeight() == Lotte::Text::FontWeight::Bold);
}
