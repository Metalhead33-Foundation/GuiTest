#include "LteThemeDefinition.hpp"
#include "LteThemeCss.hpp"
#include <algorithm>
#include <array>

namespace Lotte {
namespace Gui {

namespace {

/// @brief Names in the same order as the @ref ThemeElement enumerators.
constexpr std::array<const char*, ThemeElementCount> ElementNames = {
	"window_background", "window_border", "window_corner_top_left",
	"window_corner_top_right", "window_corner_bottom_left", "window_corner_bottom_right",
	"panel", "button", "push_button", "tool_button", "check_box", "tick_box",
	"radio_button", "text_box", "spin_box", "combo_box", "slider_track", "slider_thumb",
	"dial", "progress_track", "progress_fill", "scroll_track", "scroll_thumb", "scroll_area",
	"tab_bar", "tab_page", "tab_widget", "table_view", "table_header", "window_title_bar",
	"tool_tip"
};

/// @brief Names in the same order as the @ref ThemeTextRole enumerators.
constexpr std::array<const char*, ThemeTextRoleCount> RoleNames = {
	"body", "label", "heading", "window_title", "placeholder", "tool_tip"
};

/// @brief Names in the same order as the @ref ThemeAtlasSource enumerators.
constexpr std::array<const char*, 2> AtlasSourceNames = {"file", "procedural"};

/// @brief Names in the same order as the @ref ThemeAtlasRecipe enumerators.
constexpr std::array<const char*, static_cast<std::size_t>(ThemeAtlasRecipe::Count)> RecipeNames = {
	"crpg_stone"
};

/// @brief Number of set bits in a state mask, used to rank variants.
int stateBitCount(WidgetState state) noexcept
{
	int count = 0;
	while (state != 0) {
		count += static_cast<int>(state & 1u);
		state >>= 1u;
	}
	return count;
}

/// @brief Builds the `ThemeSkin` of a definition once its atlas is known.
std::optional<ThemeSkin> makeSkin(const ThemeSkinDefinition& source, const ThemeAtlasBinding& atlas)
{
	if (source.rect.isEmpty() || atlas.pixelSize.x <= 0.0f || atlas.pixelSize.y <= 0.0f) {
		return std::nullopt;
	}
	// The definition stores pixels because that is what an artist edits; the renderer
	// wants normalized UVs, so the conversion happens here, once per query.
	ThemeSkin skin;
	skin.binding = atlas.binding;
	const glm::fvec2 origin(static_cast<float>(source.rect.x), static_cast<float>(source.rect.y));
	const glm::fvec2 extent(static_cast<float>(source.rect.width),
							static_cast<float>(source.rect.height));
	skin.uv[0] = origin / atlas.pixelSize;
	skin.uv[1] = (origin + extent) / atlas.pixelSize;
	skin.tint = source.tint;
	skin.slice = source.slice;
	skin.sourceSize = extent;
	return skin;
}

/// @brief Appends one validation message.
void addProblem(std::vector<std::string>& problems, std::string message)
{
	problems.push_back(std::move(message));
}

} // namespace

const char* themeElementName(ThemeElement element) noexcept
{
	const auto index = static_cast<std::size_t>(element);
	return index < ElementNames.size() ? ElementNames[index] : "";
}

bool parseThemeElement(std::string_view name, ThemeElement& element) noexcept
{
	for (std::size_t index = 0; index < ElementNames.size(); ++index) {
		if (name == ElementNames[index]) {
			element = static_cast<ThemeElement>(index);
			return true;
		}
	}
	return false;
}

const char* themeTextRoleName(ThemeTextRole role) noexcept
{
	const auto index = static_cast<std::size_t>(role);
	return index < RoleNames.size() ? RoleNames[index] : "";
}

bool parseThemeTextRole(std::string_view name, ThemeTextRole& role) noexcept
{
	for (std::size_t index = 0; index < RoleNames.size(); ++index) {
		if (name == RoleNames[index]) {
			role = static_cast<ThemeTextRole>(index);
			return true;
		}
	}
	return false;
}

const char* themeAtlasSourceName(ThemeAtlasSource source) noexcept
{
	const auto index = static_cast<std::size_t>(source);
	return index < AtlasSourceNames.size() ? AtlasSourceNames[index] : "";
}

bool parseThemeAtlasSource(std::string_view name, ThemeAtlasSource& source) noexcept
{
	for (std::size_t index = 0; index < AtlasSourceNames.size(); ++index) {
		if (name == AtlasSourceNames[index]) {
			source = static_cast<ThemeAtlasSource>(index);
			return true;
		}
	}
	return false;
}

const char* themeAtlasRecipeName(ThemeAtlasRecipe recipe) noexcept
{
	const auto index = static_cast<std::size_t>(recipe);
	return index < RecipeNames.size() ? RecipeNames[index] : "";
}

bool parseThemeAtlasRecipe(std::string_view name, ThemeAtlasRecipe& recipe) noexcept
{
	for (std::size_t index = 0; index < RecipeNames.size(); ++index) {
		if (name == RecipeNames[index]) {
			recipe = static_cast<ThemeAtlasRecipe>(index);
			return true;
		}
	}
	return false;
}

Text::TextAttributes toTextAttributes(const ThemeTextStyleDefinition& style)
{
	Text::TextAttributes attributes;
	attributes.familyName = style.face;
	attributes.weight = style.weight;
	attributes.bold = Text::fontWeightIsBold(style.weight);
	attributes.size = style.size;
	attributes.colour = style.colour;
	attributes.italic = style.italic;
	attributes.underline = style.underline;
	attributes.strikethrough = style.strikethrough;
	return attributes;
}

const ThemeAtlasDefinition* ThemeDefinition::findAtlas(std::string_view alias) const noexcept
{
	for (const ThemeAtlasDefinition& atlas : atlases) {
		if (atlas.alias == alias) return &atlas;
	}
	return nullptr;
}

const ThemeElementDefinition* ThemeDefinition::findElement(ThemeElement element) const noexcept
{
	const auto index = static_cast<std::size_t>(element);
	if (index >= elements.size()) return nullptr;
	return elements[index].has_value() ? &*elements[index] : nullptr;
}

const ThemeTextStyleDefinition* ThemeDefinition::findRole(ThemeTextRole role) const noexcept
{
	const auto index = static_cast<std::size_t>(role);
	if (index >= roles.size()) return nullptr;
	return roles[index].has_value() ? &*roles[index] : nullptr;
}

DefinitionTheme::DefinitionTheme(ThemeDefinition definition, ThemeTextureResolver resolver)
	: data(std::move(definition)), textureResolver(std::move(resolver))
{
	// CSS is resolved once, here: a widget asks for its text style every frame, and a
	// libcss parse per frame would be absurd. A block that does not parse is reported by
	// `validate()` and leaves the explicit fields in place.
	resolveThemeCssInPlace(data, nullptr);
}

std::string_view DefinitionTheme::name() const
{
	return data.name;
}

std::optional<ThemeSkin> DefinitionTheme::skin(ThemeElement element, WidgetState state) const
{
	const ThemeElementDefinition* definition = data.findElement(element);
	if (definition == nullptr || definition->skins.empty() || !textureResolver) {
		return std::nullopt;
	}
	// Most specific mask that is fully contained in the requested state wins, so a
	// `stateMask == 0` entry is the fallback for every state the theme ignores.
	const ThemeSkinVariant* best = nullptr;
	int bestBits = -1;
	for (const ThemeSkinVariant& variant : definition->skins) {
		if ((state & variant.stateMask) != variant.stateMask) continue;
		const int bits = stateBitCount(variant.stateMask);
		if (bits > bestBits) {
			best = &variant;
			bestBits = bits;
		}
	}
	if (best == nullptr) return std::nullopt;
	const std::optional<ThemeAtlasBinding> atlas = textureResolver(best->skin.atlas);
	if (!atlas.has_value()) return std::nullopt;
	// A named part only exists in a generated atlas, and only the atlas knows where the
	// packer put it; an explicit rectangle keeps working for hand-authored atlases.
	ThemeSkinDefinition resolved = best->skin;
	if (resolved.part.has_value()) {
		const std::optional<ThemeRect> rect =
			atlas->partRect ? atlas->partRect(*resolved.part) : std::nullopt;
		if (!rect.has_value()) return std::nullopt;
		resolved.rect = *rect;
	}
	return makeSkin(resolved, *atlas);
}

Text::TextAttributes DefinitionTheme::textStyle(ThemeTextRole role, WidgetState state) const
{
	// The state is part of the Theme contract for themes that distinguish a disabled
	// label; the data form keeps one style per role, so it is accepted and ignored.
	(void)state;
	const ThemeTextStyleDefinition* style = data.findRole(role);
	if (style == nullptr) return Text::TextAttributes{};
	return toTextAttributes(*style);
}

ThemeMetrics DefinitionTheme::metrics(ThemeElement element) const
{
	const ThemeElementDefinition* definition = data.findElement(element);
	return definition != nullptr ? definition->metrics : ThemeMetrics{};
}

std::optional<Text::TextAttributes> DefinitionTheme::elementTextStyle(ThemeElement element,
																	  WidgetState state) const
{
	(void)state;
	const ThemeElementDefinition* definition = data.findElement(element);
	if (definition == nullptr || !definition->textStyle.has_value()) {
		return std::nullopt;
	}
	return toTextAttributes(*definition->textStyle);
}

std::vector<std::string> validate(const ThemeDefinition& definition)
{
	std::vector<std::string> problems;
	std::vector<std::string_view> aliases;
	for (const ThemeAtlasDefinition& atlas : definition.atlases) {
		if (atlas.alias.empty()) {
			addProblem(problems, "theme: an atlas has no alias");
			continue;
		}
		if (std::find(aliases.begin(), aliases.end(), atlas.alias) != aliases.end()) {
			addProblem(problems, "theme: duplicate atlas alias '" + atlas.alias + "'");
		}
		aliases.push_back(atlas.alias);
		if (atlas.source == ThemeAtlasSource::File && atlas.path.empty()) {
			addProblem(problems, "theme: file atlas '" + atlas.alias + "' has no path");
		}
		if (atlas.source == ThemeAtlasSource::Procedural && atlas.proceduralScalePercent == 0) {
			addProblem(problems,
					"theme: procedural atlas '" + atlas.alias + "' has a zero scale");
		}
	}

	for (std::size_t index = 0; index < definition.elements.size(); ++index) {
		const std::optional<ThemeElementDefinition>& element = definition.elements[index];
		if (!element.has_value()) continue;
		const std::string elementName = themeElementName(static_cast<ThemeElement>(index));
		for (const ThemeSkinVariant& variant : element->skins) {
			if (variant.skin.atlas.empty()) {
				addProblem(problems, "theme: '" + elementName + "' has a skin without an atlas");
			} else if (definition.findAtlas(variant.skin.atlas) == nullptr) {
				addProblem(problems, "theme: '" + elementName + "' references unknown atlas '" +
										variant.skin.atlas + "'");
			}
			if (variant.skin.rect.isEmpty() && !variant.skin.part.has_value()) {
				addProblem(problems, "theme: '" + elementName +
										 "' has a skin with neither a rectangle nor a part");
			}
		}
		if (element->textStyle.has_value() && element->textStyle->size <= 0.0f) {
			addProblem(problems, "theme: '" + elementName + "' has a non-positive text size");
		}
	}

	// A block libcss rejects is a problem in the document, not in a widget, so it belongs
	// in the same list as a dangling atlas alias.
	{
		ThemeDefinition probe = definition;
		std::vector<std::string> cssProblems;
		resolveThemeCssInPlace(probe, &cssProblems);
		problems.insert(problems.end(), cssProblems.begin(), cssProblems.end());
	}

	for (std::size_t index = 0; index < definition.roles.size(); ++index) {
		const std::optional<ThemeTextStyleDefinition>& role = definition.roles[index];
		if (role.has_value() && role->size <= 0.0f) {
			addProblem(problems, "theme: text role '" +
									std::string(themeTextRoleName(static_cast<ThemeTextRole>(index))) +
									"' has a non-positive size");
		}
	}
	return problems;
}

} // namespace Gui
} // namespace Lotte
