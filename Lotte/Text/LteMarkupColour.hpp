#ifndef LTEMARKUPCOLOUR_HPP
#define LTEMARKUPCOLOUR_HPP
/**
 * @file LteMarkupColour.hpp
 * @brief Parses MML colour specifications into normalized RGBA colours.
 *
 * Salvaged from `MhLib/Util/ColourHelper.{hpp,cpp}` and the `color=` tag
 * handler of `Text/MmlParser.cpp` on the `NewBeginning` branch. The three
 * accepted forms are the ones the MML specification documents:
 *
 * - a web colour name, `[color=blue]`;
 * - a hexadecimal code, `[color=#00FF00]` (with optional alpha, `#00FF0080`);
 * - normalized decimal components, `[color=1.0, 0.332, 0.75]` (alpha optional).
 *
 * Deviations from the original implementation:
 * - The legacy 300-entry `namedColours` table is gone: names are resolved
 *   through Euphemy's `Euph::Media::Image::getStandardColour`, which is the
 *   canonical table in this tree. Looking the name up in
 *   `getAllValidColorNames()` first is required, because `getStandardColour`
 *   silently falls back to black for unknown names.
 * - `sscanf`-based hex parsing is replaced with explicit digit parsing, so
 *   odd lengths and non-hex characters are rejected instead of partially
 *   parsed.
 * - Short hex forms (`#RGB`, `#RGBA`) are accepted in addition to the legacy
 *   `#RRGGBB` / `#RRGGBBAA` forms.
 * - Decimal components above `1.0` are interpreted as `0..255` byte values
 *   rather than being clamped into the normalized range.
 * - Names are matched case-insensitively, as MML requires tags and their
 *   arguments to be case-insensitive.
 */

#include <Lotte/LotteLib.hpp>
#include <Lotte/Text/LteTextAttributes.hpp>
#include <string_view>

namespace Lotte {
namespace Text {

/**
 * @brief Parses an MML colour specification.
 * @param specification Colour text as written after `color=` in a tag.
 * @param colour Receives the parsed colour; untouched on failure.
 * @return `true` when @a specification was understood.
 */
MH_LOTTE_API bool parseMarkupColour(std::string_view specification, Colour& colour);

/**
 * @brief Parses an MML colour specification.
 * @param specification Colour text as written after `color=` in a tag.
 * @param fallback Colour returned when parsing fails.
 * @return The parsed colour, or @a fallback.
 */
MH_LOTTE_API Colour parseMarkupColour(std::string_view specification, const Colour& fallback = Colour(0.0f, 0.0f, 0.0f, 1.0f));

}
}

#endif // LTEMARKUPCOLOUR_HPP
