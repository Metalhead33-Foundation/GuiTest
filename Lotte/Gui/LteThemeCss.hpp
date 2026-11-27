#ifndef LTETHEMECSS_HPP
#define LTETHEMECSS_HPP
/**
 * @file LteThemeCss.hpp
 * @brief Resolves a theme's CSS declaration block into text style, metrics and atlas hints.
 *
 * A theme document may carry real CSS next to its JSON: a font stack, a size, a colour, a
 * padding and a border width are exactly what a widget skin needs, and libcss already
 * cascades and unit-resolves them (`em`, `pt`, `%`, keywords). Instead of inventing a
 * second vocabulary for the same concepts, the theme hands the block to libcss and reads
 * the computed values back:
 *
 * | CSS | Becomes |
 * |---|---|
 * | `font-family`, `font-size`, `font-weight`, `font-style` | `ThemeTextStyleDefinition` |
 * | `color`, `text-decoration` | text colour, underline, strikethrough |
 * | `padding` | `ThemeMetrics::padding` (the average of the horizontal/vertical pair) |
 * | `border-width` | `ThemeMetrics::borderWidth` and the atlas bevel |
 * | `background-color` | the procedural atlas face tint |
 *
 * The block is wrapped in a universal rule and matched against a single `label` element,
 * so a plain `font-size: 18px` is enough - no selectors, no markup, no document.
 *
 * Without `LTE_WITH_LIBCSS` the call still succeeds structurally but reports a problem and
 * leaves the base style untouched, which is what keeps a theme document usable in a build
 * that dropped the dependency.
 */
#include <Lotte/LotteLib.hpp>
#include <Lotte/Gui/LteTheme.hpp>
#include <Lotte/Gui/LteThemeDefinition.hpp>
#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace Lotte {
namespace Gui {

/** @brief Atlas-level hints a theme's CSS can override. */
struct ThemeCssAtlasHints {
	/** @brief True when `border-width` set a stroke width. */
	bool hasBorderWidth = false;
	/** @brief Stroke width in pixels, which the atlas uses as its bevel. */
	float borderWidth = 0.0f;
	/** @brief True when `background-color` set a face tint. */
	bool hasBackground = false;
	/** @brief Face tint for a generated atlas, in `[0, 1]` RGB. */
	glm::fvec3 background = glm::fvec3(1.0f);
};

/** @brief Everything one declaration block resolves to. */
struct ThemeCssStyle {
	/** @brief False when the block could not be applied; see @ref problems. */
	bool valid = false;
	/** @brief Text style the element should use. */
	ThemeTextStyleDefinition text;
	/** @brief Padding, border and spacing hints for the element's box. */
ThemeMetrics metrics;
/** @brief Hints for a procedural atlas generated from the same block. */
ThemeCssAtlasHints atlas;
/** @brief Human-readable problems; empty when the block parsed cleanly. */
std::vector<std::string> problems;
};

/**
 * @brief Resolves a CSS declaration block for one theme element.
 * @param css Declarations only, e.g. `"font-size: 18px; color: #ffd27f; padding: 6px"`.
 * @param base Text style the block starts from, i.e. the theme's explicit values.
 * @return The resolved style. @ref ThemeCssStyle::valid is false when the block is
 *         empty, when the build has no libcss, or when libcss rejected the block.
 */
MH_LOTTE_API ThemeCssStyle resolveThemeCss(std::string_view css,
   const ThemeTextStyleDefinition& base = {});

/**
 * @brief Resolves every CSS block a definition carries, in place.
 * @param definition Definition whose elements and text roles are resolved.
 * @param problems Receives one line per block that could not be applied; may be null.
 * @return Number of blocks that were applied.
 *
 * Called by `DefinitionTheme` when it is constructed, so a widget never pays for a libcss
 * parse while drawing: it sees the already resolved `ThemeTextStyleDefinition`.
 */
MH_LOTTE_API std::size_t resolveThemeCssInPlace(ThemeDefinition& definition,
												std::vector<std::string>* problems = nullptr);

} // namespace Gui
} // namespace Lotte

#endif // LTETHEMECSS_HPP
