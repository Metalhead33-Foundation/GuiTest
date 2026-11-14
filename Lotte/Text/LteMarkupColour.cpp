#include "LteMarkupColour.hpp"

#include <Euphemy/Media/Image/EuphStandardColour.hpp>
#include <algorithm>
#include <charconv>
#include <cctype>
#include <string>
#include <vector>

namespace Lotte {
namespace Text {

namespace {

/** @brief Trims ASCII whitespace from both ends of a view. */
std::string_view trim(std::string_view text)
{
	while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) text.remove_prefix(1);
	while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.remove_suffix(1);
	return text;
}

/** @brief Lowercases ASCII letters into a new string. */
std::string toLowerAscii(std::string_view text)
{
	std::string lowered(text);
	for (char& c : lowered) c = char(std::tolower(static_cast<unsigned char>(c)));
	return lowered;
}

/** @brief Converts one hexadecimal digit, or returns -1. */
int hexDigit(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

/** @brief Parses a hex colour body (without the leading '#'). */
bool parseHexColour(std::string_view digits, Colour& colour)
{
	std::vector<unsigned> nibbles;
	nibbles.reserve(digits.size());
	for (char c : digits) {
		const int value = hexDigit(c);
		if (value < 0) return false;
		nibbles.push_back(unsigned(value));
	}
	if (nibbles.size() != 3 && nibbles.size() != 4 && nibbles.size() != 6 && nibbles.size() != 8) return false;

	unsigned components[4] = { 0, 0, 0, 255 };
	if (nibbles.size() == 3 || nibbles.size() == 4) {
		for (std::size_t i = 0; i < nibbles.size(); ++i) components[i] = nibbles[i] * 17u;
	} else {
		const std::size_t channelCount = nibbles.size() / 2;
		for (std::size_t i = 0; i < channelCount; ++i) {
			components[i] = nibbles[i * 2] * 16u + nibbles[i * 2 + 1];
		}
	}
	colour = Colour(float(components[0]) / 255.0f, float(components[1]) / 255.0f,
					float(components[2]) / 255.0f, float(components[3]) / 255.0f);
	return true;
}

/** @brief Parses a comma-separated component list. */
bool parseComponentColour(std::string_view specification, Colour& colour)
{
	float components[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	std::size_t count = 0;
	std::size_t start = 0;
	while (start <= specification.size() && count < 4) {
		const std::size_t comma = specification.find(',', start);
		const std::string_view token = trim(specification.substr(start, comma == std::string_view::npos ? std::string_view::npos : comma - start));
		if (token.empty()) break;
		float value = 0.0f;
		const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
		if (result.ec != std::errc() || result.ptr != token.data() + token.size()) return false;
		components[count++] = value;
		if (comma == std::string_view::npos) break;
		start = comma + 1;
	}
	if (count < 3) return false;

	// Values above 1.0 mean the author wrote 0..255 byte components.
	const bool byteScaled = components[0] > 1.0f || components[1] > 1.0f || components[2] > 1.0f;
	const float scale = byteScaled ? 1.0f / 255.0f : 1.0f;
	for (std::size_t i = 0; i < 4; ++i) components[i] = std::clamp(components[i] * scale, 0.0f, 1.0f);
	colour = Colour(components[0], components[1], components[2], components[3]);
	return true;
}

/** @brief Resolves a web colour name through Euphemy's standard palette. */
bool parseNamedColour(std::string_view specification, Colour& colour)
{
	const std::string lowered = toLowerAscii(specification);
	const auto& palette = Euph::Media::Image::getAllValidColorNames();
	const auto it = palette.find(lowered);
	if (it == palette.end()) return false;
	const Euph::Media::Image::PixelRGB_U8& rgb = Euph::Media::Image::getStandardColour(it->second);
	colour = Colour(float(rgb.r) / 255.0f, float(rgb.g) / 255.0f, float(rgb.b) / 255.0f, 1.0f);
	return true;
}

}

bool parseMarkupColour(std::string_view specification, Colour& colour)
{
	const std::string_view trimmed = trim(specification);
	if (trimmed.empty()) return false;
	if (trimmed.front() == '#') {
		return parseHexColour(std::string_view(trimmed.begin() + 1, trimmed.end()), colour);
	}
	if (trimmed.find(',') != std::string_view::npos) {
		return parseComponentColour(trimmed, colour);
	}
	return parseNamedColour(trimmed, colour);
}

Colour parseMarkupColour(std::string_view specification, const Colour& fallback)
{
	Colour parsed = fallback;
	if (!parseMarkupColour(specification, parsed)) return fallback;
	return parsed;
}

}
}
