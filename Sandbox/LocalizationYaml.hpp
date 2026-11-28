#ifndef SANDBOX_LOCALIZATIONYAML_HPP
#define SANDBOX_LOCALIZATIONYAML_HPP
/**
 * @file LocalizationYaml.hpp
 * @brief Reads a localization document into a @ref Euph::Loc::Localization.
 *
 * The engine has no parser (see `docs/PROCEDURAL.md` §1): `Euph::Loc::Localization`
 * is a plain key/value table, and this file is the adapter that fills it from YAML.
 * libyaml is a dependency of this directory only, exactly like `nlohmann_json` is for
 * the theme loader.
 *
 * @section sandbox_localization_shape The accepted document shape
 *
 * A document is one root mapping whose values are the per-language tables - the same
 * shape the reference format uses, one file per language:
 *
 * ```yaml
 * l_english:
 *   menu_new_game:0 "New Game"
 *   settings_volume_music:0 "Music"
 * ```
 *
 * The `:0 "text"` form is not strict YAML: a plain scalar may not contain `": "`, but
 * `key:0 "text"` has no space after the colon, so libyaml hands the whole thing over
 * as the *key* of a pair whose value is null. The parser recognises that shape and
 * splits it, which means both the reference dialect and a strict `key: "text"` pair
 * produce the same table. The trailing number (a version counter in the reference
 * format) is ignored.
 *
 * Nested mappings and sequences are skipped rather than rejected: a localization file
 * may grow structure the UI does not need, and failing the whole document over it
 * would be worse than dropping it. Every skipped or malformed entry becomes one line
 * in `LocalizationParseResult::problems`.
 */
#include <Euphemy/Localization/EuphLocalization.hpp>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace Sandbox {

/** @brief What one localization document produced. */
struct LocalizationParseResult {
	/** @brief The key/value pairs the document declared. */
	Euph::Loc::Localization table;
	/** @brief Human-readable problems, one line each; empty on a clean read. */
	std::vector<std::string> problems;
	/** @brief True when the document was at least structurally readable. */
	bool ok = false;

	/** @brief Number of entries the document contributed. */
	std::size_t entryCount() const noexcept { return table.size(); }
};

/**
 * @brief Parses one localization document.
 * @param text Whole file contents.
 * @return The parsed table, the problems met on the way and whether the document
 *         could be read at all.
 */
LocalizationParseResult parseLocalizationYaml(std::string_view text);

} // namespace Sandbox

#endif // SANDBOX_LOCALIZATIONYAML_HPP
