#ifndef SANDBOX_THEMEJSON_HPP
#define SANDBOX_THEMEJSON_HPP
/**
 * @file ThemeJson.hpp
 * @brief Turns a theme JSON document into a `Lotte::Gui::ThemeDefinition`.
 *
 * JSON lives here - in the Sandbox - and not in Lotte on purpose: the libraries
 * describe themes as C++ data (`LteThemeDefinition.hpp`), and only the applications
 * and the tests link a parser. That keeps a dependency out of every consumer of Lotte
 * and keeps the schema in one place instead of leaking into the renderer.
 *
 * The document shape is documented by `gfx/theme_000.json`; the loader accepts these
 * keys and rejects the rest:
 *
 * @code{.json}
 * {
 *   "name": "legacy_crpg",
 *   "texture_atlas": [
 *     { "alias": "GUI", "filename": "gfx/atlas_000.png" },
 *     { "alias": "PROC", "proc": "crpg_stone", "seed": 4242, "scale": 100 }
 *   ],
 *   "elements": {
 *     "button": {
 *       "skins": [
 *         { "states": ["hover"], "atlas": "PROC", "rect": [0, 0, 128, 48],
 *           "tint": "#ffffff", "slice": [8, 8, 8, 8] }
 *       ],
 *       "metrics": { "padding": [8, 4], "borderWidth": 2, "cornerRadius": 4 },
 *       "text": { "face": "Vinque", "weight": 700, "size": 20, "colour": "#ffd166" }
 *     }
 *   },
 *   "roles": { "body": { "face": "Vinque", "size": 24, "colour": "#ffffff" } }
 * }
 * @endcode
 *
 * Name-to-enum resolution is done by Lotte's tables
 * (`parseThemeElement`, `parseThemeTextRole`, `parseThemeAtlasRecipe`), so the
 * vocabulary lives next to the enums instead of in the loader.
 */
#include <Lotte/Gui/LteThemeDefinition.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace Sandbox {

/** @brief Outcome of one `parseThemeJson` call. */
struct ThemeParseResult {
	/** @brief The parsed definition; only meaningful when @ref ok is true. */
	Lotte::Gui::ThemeDefinition definition;
	/** @brief True when the document parsed and validated without problems. */
	bool ok = false;
	/** @brief Parsing and validation problems, in document order. */
	std::vector<std::string> problems;
};

/**
 * @brief Parses a theme document.
 * @param json Source text of the document.
 * @return The definition, and every problem found; `ok` is false when any exists.
 * @note A malformed document (invalid JSON, wrong types) is reported in `problems`
 *       rather than thrown, because a theme is user data and a bad file should not
 *       terminate the application.
 */
ThemeParseResult parseThemeJson(std::string_view json);

} // namespace Sandbox

#endif // SANDBOX_THEMEJSON_HPP
