#ifndef LTEHTMLENTITIES_HPP
#define LTEHTMLENTITIES_HPP
/**
 * @file LteHtmlEntities.hpp
 * @brief HTML character reference (entity) decoding for MML text.
 *
 * MML requires HTML escapes to be supported, and mandates them for the
 * characters that would otherwise be parsed as markup (`<`, `>`, `[`, `]`,
 * `&`). The named-entity table in `LteHtmlEntities.cpp` is the one lifted
 * verbatim from `Text/MmlParser.cpp` on the `NewBeginning` branch: it is
 * reference *data*, not logic, so it transferred without changes.
 *
 * The decoder resolves three forms:
 *
 * - `&name;` - a named reference such as `&amp;` or `&rarr;`;
 * - `&#123;` - a decimal code point reference;
 * - `&#x1F600;` - a hexadecimal code point reference.
 *
 * Deviations from the original implementation:
 * - `#` alone used to mean "hexadecimal", so `&#123;` was silently read as
 *   `0x123`. The `#x` prefix is now required for hexadecimal, per the HTML
 *   specification, and plain `#` means decimal.
 * - A bare decimal such as `&123;` is still accepted for compatibility with
 *   the old parser.
 * - Unknown references are reported instead of being silently dropped, so the
 *   caller can choose to keep the raw text visible to the user.
 */

#include <Lotte/LotteLib.hpp>
#include <cstddef>
#include <string>
#include <string_view>

namespace Lotte {
namespace Text {

/**
 * @brief Looks up a named HTML entity.
 * @param name Entity name without the leading `&` and trailing `;`.
 * @param utf8 Receives the UTF-8 encoding of the entity on success.
 * @return `true` when the name is known.
 */
MH_LOTTE_API bool decodeNamedHtmlEntity(std::string_view name, std::string& utf8);

/**
 * @brief Decodes one HTML character reference.
 * @param entity Text between `&` and `;` (excluding both).
 * @param utf8 Receives the UTF-8 encoding of the referenced code point.
 * @return `true` when @a entity was understood.
 */
MH_LOTTE_API bool decodeHtmlEntity(std::string_view entity, std::string& utf8);

/** @brief Returns the number of named entities in the compiled-in table. */
MH_LOTTE_API std::size_t getNamedHtmlEntityCount();

}
}

#endif // LTEHTMLENTITIES_HPP
