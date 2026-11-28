#include "EuphLocalization.hpp"
#include <cstddef>

namespace Euph {
namespace Loc {

namespace {

/**
 * @brief Expands one `$name$` placeholder occurrence.
 * @param text Source text.
 * @param parameters Pairs to substitute.
 * @return The text with every *known* placeholder replaced.
 *
 * An unknown placeholder is copied verbatim, closing `$` included, so a typo shows up
 * in the UI instead of disappearing.
 */
std::string substitute(std::string_view text,
					   std::initializer_list<std::pair<std::string_view, std::string_view>> parameters)
{
	std::string result;
	result.reserve(text.size());
	std::size_t position = 0;
	while (position < text.size()) {
		const std::size_t open = text.find('$', position);
		if (open == std::string_view::npos) {
			result.append(text.substr(position));
			break;
		}
		const std::size_t close = text.find('$', open + 1);
		if (close == std::string_view::npos) {
			// A dangling `$` is not a placeholder; keep it as literal text.
			result.append(text.substr(position));
			break;
		}
		result.append(text.substr(position, open - position));
		const std::string_view name = text.substr(open + 1, close - open - 1);
		bool replaced = false;
		for (const auto& [key, value] : parameters) {
			if (key == name) {
				result.append(value);
				replaced = true;
				break;
			}
		}
		if (!replaced) {
			result.append(text.substr(open, close - open + 1));
		}
		position = close + 1;
	}
	return result;
}

} // namespace

Localization::Localization() = default;

void Localization::insert(std::string key, std::string text)
{
	if (key.empty()) {
		return;
	}
	entries.insert_or_assign(std::move(key), std::move(text));
}

void Localization::merge(const Localization& other)
{
	for (const auto& [key, text] : other.entries) {
		entries.insert_or_assign(key, text);
	}
}

void Localization::overlay(const Localization& base)
{
	for (const auto& [key, text] : base.entries) {
		// Only the gaps are filled: this table is the more specific layer.
		entries.try_emplace(key, text);
	}
}

void Localization::clear()
{
	entries.clear();
}

const std::string* Localization::find(std::string_view key) const
{
	const auto found = entries.find(key);
	return found == entries.end() ? nullptr : &found->second;
}

std::string_view Localization::text(std::string_view key) const
{
	const std::string* found = find(key);
	return found != nullptr ? std::string_view(*found) : key;
}

bool Localization::contains(std::string_view key) const
{
	return entries.find(key) != entries.end();
}

std::string Localization::formatText(
	std::string_view text,
	std::initializer_list<std::pair<std::string_view, std::string_view>> parameters)
{
	return substitute(text, parameters);
}

std::string Localization::format(
	std::string_view key,
	std::initializer_list<std::pair<std::string_view, std::string_view>> parameters) const
{
	return substitute(text(key), parameters);
}

} // namespace Loc
} // namespace Euph
