#ifndef LTEUTF_HPP
#define LTEUTF_HPP
/**
 * @file LteUtf.hpp
 * @brief Minimal UTF-8 <-> UTF-32 conversion used by Lotte's text layer.
 *
 * The salvaged text code from the `NewBeginning` branch used
 * `std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>`, which is
 * deprecated in C++17 and removed in C++26. Lotte stores text as
 * `std::u32string` (one code point per element, no surrogate pairs) and reads
 * markup as UTF-8 bytes, so it only ever needs these two directions.
 *
 * Decoding is validating and total: malformed sequences yield U+FFFD
 * (`REPLACEMENT CHARACTER`) instead of throwing, because text arrives from
 * untrusted documents and a bad byte must not abort a layout pass.
 */

#include <Lotte/LotteLib.hpp>
#include <cstddef>
#include <string>
#include <string_view>

namespace Lotte {
namespace Text {

/** @brief Code point emitted for malformed UTF-8 input. */
inline constexpr char32_t ReplacementCharacter = char32_t(0xFFFD);

/**
 * @brief Decodes UTF-8 bytes and appends the code points to @a dst.
 * @param dst Destination UTF-32 string.
 * @param utf8 Source bytes; may contain malformed sequences.
 * @return Number of code points appended.
 */
inline std::size_t appendUtf8(std::u32string& dst, std::string_view utf8)
{
	const std::size_t start = dst.size();
	std::size_t i = 0;
	while (i < utf8.size()) {
		const auto byte = static_cast<unsigned char>(utf8[i]);
		char32_t codePoint = ReplacementCharacter;
		std::size_t length = 1;
		if (byte < 0x80) {
			codePoint = char32_t(byte);
		} else if ((byte & 0xE0) == 0xC0) {
			length = 2;
			codePoint = char32_t(byte & 0x1F);
		} else if ((byte & 0xF0) == 0xE0) {
			length = 3;
			codePoint = char32_t(byte & 0x0F);
		} else if ((byte & 0xF8) == 0xF0) {
			length = 4;
			codePoint = char32_t(byte & 0x07);
		} else {
			dst.push_back(ReplacementCharacter);
			++i;
			continue;
		}

		if (length > 1) {
			bool valid = (i + length) <= utf8.size();
			for (std::size_t k = 1; valid && k < length; ++k) {
				const auto continuation = static_cast<unsigned char>(utf8[i + k]);
				if ((continuation & 0xC0) != 0x80) {
					valid = false;
					break;
				}
				codePoint = (codePoint << 6) | char32_t(continuation & 0x3F);
			}
			if (!valid) {
				dst.push_back(ReplacementCharacter);
				++i;
				continue;
			}
		}

		dst.push_back(codePoint);
		i += length;
	}
	return dst.size() - start;
}

/**
 * @brief Decodes UTF-8 bytes into a UTF-32 string.
 * @param utf8 Source bytes.
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
 * @param codePoint Code point to append; out-of-range values emit U+FFFD.
 */
inline void appendCodePointUtf8(std::string& dst, char32_t codePoint)
{
	if (codePoint > 0x10FFFF || (codePoint >= 0xD800 && codePoint <= 0xDFFF)) {
		codePoint = ReplacementCharacter;
	}
	if (codePoint < 0x80) {
		dst.push_back(char(codePoint));
	} else if (codePoint < 0x800) {
		dst.push_back(char(0xC0 | (codePoint >> 6)));
		dst.push_back(char(0x80 | (codePoint & 0x3F)));
	} else if (codePoint < 0x10000) {
		dst.push_back(char(0xE0 | (codePoint >> 12)));
		dst.push_back(char(0x80 | ((codePoint >> 6) & 0x3F)));
		dst.push_back(char(0x80 | (codePoint & 0x3F)));
	} else {
		dst.push_back(char(0xF0 | (codePoint >> 18)));
		dst.push_back(char(0x80 | ((codePoint >> 12) & 0x3F)));
		dst.push_back(char(0x80 | ((codePoint >> 6) & 0x3F)));
		dst.push_back(char(0x80 | (codePoint & 0x3F)));
	}
}

/**
 * @brief Encodes UTF-32 text as UTF-8 bytes.
 * @param utf32 Source code points.
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
