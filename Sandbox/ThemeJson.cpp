#include "ThemeJson.hpp"
#include <Lotte/Text/LteMarkupColour.hpp>
#include <nlohmann/json.hpp>
#include <array>
#include <cstdint>
#include <string>
#include <utility>

namespace Sandbox {

namespace {

using nlohmann::json;

/// @brief Reads a colour written as an MML colour string.
Lotte::Text::Colour readColour(const json& value, const Lotte::Text::Colour& fallback)
{
	if (!value.is_string()) return fallback;
	return Lotte::Text::parseMarkupColour(value.get<std::string>(), fallback);
}

/// @brief Reads a two-component float vector stored as `[x, y]`.
glm::fvec2 readVector2(const json& value, glm::fvec2 fallback)
{
	if (!value.is_array() || value.size() < 2 || !value[0].is_number() || !value[1].is_number()) {
		return fallback;
	}
	return glm::fvec2(value[0].get<float>(), value[1].get<float>());
}

/// @brief Reads a pixel rectangle stored as `[x, y, width, height]`.
Lotte::Gui::ThemeRect readRect(const json& value)
{
	Lotte::Gui::ThemeRect rect;
	if (!value.is_array() || value.size() < 4) return rect;
	rect.x = value[0].get<std::int32_t>();
	rect.y = value[1].get<std::int32_t>();
	rect.width = value[2].get<std::uint32_t>();
	rect.height = value[3].get<std::uint32_t>();
	return rect;
}

/// @brief Reads nine-slice margins stored as `[left, top, right, bottom]`.
Lotte::Gui::ThemeSlice readSlice(const json& value)
{
	Lotte::Gui::ThemeSlice slice;
	if (!value.is_array() || value.size() < 4) return slice;
	slice.left = value[0].get<float>();
	slice.top = value[1].get<float>();
	slice.right = value[2].get<float>();
	slice.bottom = value[3].get<float>();
	return slice;
}

/// @brief Maps a state name to its widget-state bits.
bool stateBitsFromName(std::string_view name, Lotte::Gui::WidgetState& bits)
{
	if (name == "idle" || name == "any" || name == "all") {
		bits = 0;
		return true;
	}
	if (name == "enabled") {
		bits = static_cast<Lotte::Gui::WidgetState>(Lotte::Gui::WidgetStateFlag::Enabled);
		return true;
	}
	if (name == "clicked" || name == "pressed") {
		bits = static_cast<Lotte::Gui::WidgetState>(Lotte::Gui::WidgetStateFlag::Clicked);
		return true;
	}
	if (name == "hover" || name == "focused" || name == "focus") {
		bits = static_cast<Lotte::Gui::WidgetState>(Lotte::Gui::WidgetStateFlag::InMouseFocus);
		return true;
	}
	return false;
}

/// @brief Reads a `states` array into one mask.
Lotte::Gui::WidgetState readStateMask(const json& value, bool& ok)
{
	Lotte::Gui::WidgetState mask = 0;
	if (value.is_null()) return 0;
	if (!value.is_array()) {
		ok = false;
		return 0;
	}
	for (const json& entry : value) {
		if (!entry.is_string()) {
			ok = false;
			continue;
		}
		Lotte::Gui::WidgetState bits = 0;
		if (!stateBitsFromName(entry.get<std::string>(), bits)) {
			ok = false;
			continue;
		}
		mask |= bits;
	}
	return mask;
}

/// @brief Reads a text style object.
Lotte::Gui::ThemeTextStyleDefinition readTextStyle(const json& value)
{
	Lotte::Gui::ThemeTextStyleDefinition style;
	if (value.contains("face")) style.face = value["face"].get<std::string>();
	if (value.contains("weight")) {
		style.weight = Lotte::Text::fontWeightFromValue(value["weight"].get<std::uint16_t>());
	}
	if (value.contains("size")) style.size = value["size"].get<float>();
	if (value.contains("colour")) style.colour = readColour(value["colour"], style.colour);
	if (value.contains("italic")) style.italic = value["italic"].get<bool>();
	if (value.contains("underline")) style.underline = value["underline"].get<bool>();
	if (value.contains("strikethrough")) style.strikethrough = value["strikethrough"].get<bool>();
	return style;
}

/// @brief Reads a metrics object.
Lotte::Gui::ThemeMetrics readMetrics(const json& value)
{
	Lotte::Gui::ThemeMetrics metrics;
	if (value.contains("padding")) metrics.padding = readVector2(value["padding"], metrics.padding);
	if (value.contains("spacing")) metrics.spacing = readVector2(value["spacing"], metrics.spacing);
	if (value.contains("minimumSize")) {
		metrics.minimumSize = readVector2(value["minimumSize"], metrics.minimumSize);
	}
	if (value.contains("borderWidth")) metrics.borderWidth = value["borderWidth"].get<float>();
	if (value.contains("cornerRadius")) metrics.cornerRadius = value["cornerRadius"].get<float>();
	return metrics;
}

} // namespace
ThemeParseResult parseThemeJson(std::string_view source)
{
	ThemeParseResult result;
	json document;
	try {
		document = json::parse(source.begin(), source.end());
	} catch (const std::exception& error) {
		result.problems.push_back(std::string("theme: invalid JSON: ") + error.what());
		return result;
	}
	if (!document.is_object()) {
		result.problems.push_back("theme: the document is not an object");
		return result;
	}
	if (document.contains("name") && document["name"].is_string()) {
		result.definition.name = document["name"].get<std::string>();
	}

	if (document.contains("texture_atlas")) {
		if (!document["texture_atlas"].is_array()) {
			result.problems.push_back("theme: 'texture_atlas' is not an array");
		} else {
			for (const json& entry : document["texture_atlas"]) {
				if (!entry.is_object()) {
					result.problems.push_back("theme: a 'texture_atlas' entry is not an object");
					continue;
				}
				Lotte::Gui::ThemeAtlasDefinition atlas;
				if (entry.contains("alias") && entry["alias"].is_string()) {
					atlas.alias = entry["alias"].get<std::string>();
				}
				for (const char* key : { "filename", "path" }) {
					if (entry.contains(key) && entry[key].is_string()) {
						atlas.path = entry[key].get<std::string>();
					}
				}
				if (entry.contains("proc")) {
					atlas.source = Lotte::Gui::ThemeAtlasSource::Procedural;
					const std::string recipe = entry["proc"].is_string()
					? entry["proc"].get<std::string>()
					: std::string();
					if (!Lotte::Gui::parseThemeAtlasRecipe(recipe, atlas.recipe)) {
						result.problems.push_back("theme: unknown procedural recipe '" + recipe + "'");
					}
				}
				if (entry.contains("seed")) atlas.proceduralSeed = entry["seed"].get<std::uint32_t>();
				if (entry.contains("scale")) {
					atlas.proceduralScalePercent = entry["scale"].get<std::uint32_t>();
				}
				if (atlas.alias.empty()) {
					// The legacy file had no alias: the filename is a good enough name.
					const std::size_t slash = atlas.path.find_last_of("/\\");
					const std::size_t start = slash == std::string::npos ? 0 : slash + 1;
					const std::size_t dot = atlas.path.find_last_of('.');
					atlas.alias = atlas.path.substr(start, dot == std::string::npos || dot < start
					? std::string::npos
					: dot - start);
				}
				result.definition.atlases.push_back(std::move(atlas));
			}
		}
	}

	if (document.contains("elements") && document["elements"].is_object()) {
		for (const auto& [name, value] : document["elements"].items()) {
			Lotte::Gui::ThemeElement element = Lotte::Gui::ThemeElement::Panel;
			if (!Lotte::Gui::parseThemeElement(name, element)) {
				result.problems.push_back("theme: unknown element '" + name + "'");
				continue;
			}
			Lotte::Gui::ThemeElementDefinition definition;
			if (value.contains("skins") && value["skins"].is_array()) {
				for (const json& skinValue : value["skins"]) {
					Lotte::Gui::ThemeSkinVariant variant;
					bool statesOk = true;
					if (skinValue.contains("states")) {
						variant.stateMask = readStateMask(skinValue["states"], statesOk);
						if (!statesOk) {
							result.problems.push_back("theme: '" + name +
							"' has an unknown state name");
						}
					}
					if (skinValue.contains("atlas") && skinValue["atlas"].is_string()) {
						variant.skin.atlas = skinValue["atlas"].get<std::string>();
					}
					if (skinValue.contains("rect")) {
						variant.skin.rect = readRect(skinValue["rect"]);
					}
					if (skinValue.contains("tint")) {
						variant.skin.tint = readColour(skinValue["tint"], variant.skin.tint);
					}
					if (skinValue.contains("slice")) {
						variant.skin.slice = readSlice(skinValue["slice"]);
					}
					definition.skins.push_back(std::move(variant));
				}
			}
			if (value.contains("metrics") && value["metrics"].is_object()) {
				definition.metrics = readMetrics(value["metrics"]);
			}
			if (value.contains("text") && value["text"].is_object()) {
				definition.textStyle = readTextStyle(value["text"]);
			}
			result.definition.elements[static_cast<std::size_t>(element)] = std::move(definition);
		}
	}

	if (document.contains("roles") && document["roles"].is_object()) {
		for (const auto& [name, value] : document["roles"].items()) {
			Lotte::Gui::ThemeTextRole role = Lotte::Gui::ThemeTextRole::Body;
			if (!Lotte::Gui::parseThemeTextRole(name, role)) {
				result.problems.push_back("theme: unknown text role '" + name + "'");
				continue;
			}
			if (!value.is_object()) continue;
			result.definition.roles[static_cast<std::size_t>(role)] = readTextStyle(value);
		}
	}

	for (std::string& problem : Lotte::Gui::validate(result.definition)) {
		result.problems.push_back(std::move(problem));
	}
	result.ok = result.problems.empty();
	return result;
}

} // namespace Sandbox
