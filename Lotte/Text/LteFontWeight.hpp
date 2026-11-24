#ifndef LTEFONTWEIGHT_HPP
#define LTEFONTWEIGHT_HPP
/**
 * @file LteFontWeight.hpp
 * @brief Numeric font weights shared by text attributes, CSS, layout and rendering.
 *
 * WODF has no weight metadata, so a face's weight is a property of its repository
 * registration, not of the container. The default is CSS `normal` (400). Font
 * families conventionally provide a bold face at 700: of 245 regular/bold pairs
 * installed on the development host, 243 used exactly 400 and 700, and the mean
 * was 299.18, so 400 -> 700 is the best portable assumption.
 *
 * `FontWeight` remains deliberately coarse (the standard CSS 100-step ladder). A
 * renderer only needs a relative weight delta, not an exact continuous value.
 */
#include <cstdint>

namespace Lotte {
namespace Text {

/** @brief CSS-style font weight, 100 (thin) through 900 (black). */
enum class FontWeight : std::uint16_t {
	Thin = 100,       /**< Thin. */
	ExtraLight = 200, /**< Extra light. */
	Light = 300,      /**< Light. */
	Normal = 400,     /**< Normal / regular; the default for faces without metadata. */
	Medium = 500,     /**< Medium. */
	SemiBold = 600,   /**< Semibold. */
	Bold = 700,       /**< Bold; the conventional alias for `[b]`/`strong`. */
	ExtraBold = 800,  /**< Extra bold. */
	Black = 900       /**< Black / heavy. */
};

/** @brief Numeric value of a font weight. */
inline constexpr std::uint16_t fontWeightValue(FontWeight weight) noexcept
{
	return static_cast<std::uint16_t>(weight);
}

/**
 * @brief Clamps and rounds a numeric value to the nearest supported weight.
 *
 * Values below 100 become @ref FontWeight::Thin and values above 900 become
 * @ref FontWeight::Black. Intermediate values round to the nearest 100-step.
 */
inline constexpr FontWeight fontWeightFromValue(std::uint16_t value) noexcept
{
	if (value <= 100u) return FontWeight::Thin;
	if (value >= 900u) return FontWeight::Black;
	return static_cast<FontWeight>(((value + 50u) / 100u) * 100u);
}

/** @brief True when a weight is conventionally bold (600 or heavier). */
inline constexpr bool fontWeightIsBold(FontWeight weight) noexcept
{
	return fontWeightValue(weight) >= 600u;
}

/** @brief The next heavier CSS weight, following the CSS `bolder` table. */
inline constexpr FontWeight fontWeightBolder(FontWeight weight) noexcept
{
	const std::uint16_t value = fontWeightValue(weight);
	if (value < 400u) return FontWeight::Normal;
	if (value < 600u) return FontWeight::Bold;
	return FontWeight::Black;
}

/** @brief The next lighter CSS weight, following the CSS `lighter` table. */
inline constexpr FontWeight fontWeightLighter(FontWeight weight) noexcept
{
	const std::uint16_t value = fontWeightValue(weight);
	if (value < 600u) return FontWeight::Thin;
	if (value < 800u) return FontWeight::Normal;
	return FontWeight::Bold;
}

/** @brief Weight used for ordinary text when no face metadata exists. */
inline constexpr FontWeight DefaultFontWeight = FontWeight::Normal;
/** @brief Weight assigned to the `[b]`/`<strong>`/`font-weight: bold` alias. */
inline constexpr FontWeight BoldFontWeight = FontWeight::Bold;

} // namespace Text
} // namespace Lotte

#endif // LTEFONTWEIGHT_HPP
