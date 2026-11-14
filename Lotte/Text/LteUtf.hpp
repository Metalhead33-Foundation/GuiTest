#ifndef LTEUTF_HPP
#define LTEUTF_HPP
/**
 * @file LteUtf.hpp
 * @brief UTF-8 <-> UTF-32 conversion for Lotte's text layer, over utfcpp.
 *
 * Lotte stores text as `std::u32string` (one code point per element, so glyph
 * lookup never has to deal with surrogate pairs) and reads markup as UTF-8 bytes.
 * The conversion itself is delegated to [utfcpp](https://github.com/nemtrif/utfcpp),
 * which is a header-only system dependency:
 *
 *     #include <utf8cpp/utf8.h>   // Gentoo, Debian/Ubuntu (libutfcpp-dev)
 *
 * The `checked` API throws on malformed input, which is not acceptable in the text
 * pipeline: documents are untrusted, and a bad byte must not abort a layout pass.
 * Decoding therefore validates first and rewrites invalid sequences to U+FFFD before
 * the conversion runs; `unchecked` is used for the hot path once the input is known
 * to be valid.
 *
 * The salvaged text code on the `NewBeginning` branch used
 * `std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>`, which is deprecated
 * in C++17 and removed in C++26; these helpers are the replacement.
 */

#include <Lotte/LotteLib.hpp>
#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>

#if __has_include(<utf8cpp/utf8.h>)
#include <utf8cpp/utf8.h>
#elif __has_include(<utf8.h>)
#include <utf8.h>
#else
#error "Lotte/Text/LteUtf.hpp requires utfcpp (<https://github.com/nemtrif/utfcpp>)"
#endif

namespace Lotte {
namespace Text {

/** @brief Code point emitted for malformed UTF-8 input and for invalid code points. */
inline constexpr char32_t ReplacementCharacter = char32_t(0xFFFD);

/**
 * @brief Decodes UTF-8 bytes and appends the code points to @a dst.
 * @param dst Destination UTF-32 string.
 * @param utf8 Source bytes; malformed sequences become @ref ReplacementCharacter.
 * @return Number of code points appended.
 */
inline std::size_t appendUtf8(std::u32string& dst, std::string_view utf8)
{
	if (utf8.empty()) return 0;
	const std::size_t start = dst.size();
	if (utf8::is_valid(utf8.begin(), utf8.end())) {
		utf8::unchecked::utf8to32(utf8.begin(), utf8.end(), std::back_inserter(dst));
		return dst.size() - start;
	}
	// Invalid input: rewrite the offending sequences as U+FFFD, then decode. The
	// sanitized copy is the price of never throwing from this layer.
	std::string sanitized;
	sanitized.reserve(utf8.size());
	utf8::replace_invalid(utf8.begin(), utf8.end(), std::back_inserter(sanitized), ReplacementCharacter);
	utf8::unchecked::utf8to32(sanitized.begin(), sanitized.end(), std::back_inserter(dst));
	return dst.size() - start;
}

/**
 * @brief Decodes UTF-8 bytes into a UTF-32 string.
 * @param utf8 Source bytes; malformed sequences become @ref ReplacementCharacter.
 * @return The decoded string.
 */
inline std::u32string decodeUtf8(std::string_view utf8)
{
	std::u32string decoded;
	decoded.reserve(utf8.size());
	appendUtf8(decoded, utf8);
	return decoded;
}

/**
 * @brief Encodes one code point as UTF-8 and appends it to @a dst.
 * @param dst Destination byte string.
 * @param codePoint Code point to append; out-of-range values and surrogates emit U+FFFD.
 */
inline void appendCodePointUtf8(std::string& dst, char32_t codePoint)
{
	// utfcpp's unchecked encoder assumes a valid code point, so the range check stays
	// here: text can carry `&#x110000;` or a lone surrogate from a numeric reference.
	if (codePoint > 0x10FFFF || (codePoint >= 0xD800 && codePoint <= 0xDFFF)) {
		codePoint = ReplacementCharacter;
	}
	utf8::unchecked::append(codePoint, std::back_inserter(dst));
}

/**
 * @brief Encodes UTF-32 text as UTF-8 bytes.
 * @param utf32 Source code points; invalid values become U+FFFD.
 * @return The encoded byte string.
 */
inline std::string encodeUtf8(std::u32string_view utf32)
{
	std::string encoded;
	encoded.reserve(utf32.size());
	for (char32_t codePoint : utf32) appendCodePointUtf8(encoded, codePoint);
	return encoded;
}

}
}

#endif // LTEUTF_HPP
