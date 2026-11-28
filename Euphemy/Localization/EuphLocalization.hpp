#ifndef EUPHLOCALIZATION_HPP
#define EUPHLOCALIZATION_HPP
/**
 * @file EuphLocalization.hpp
 * @brief A language-neutral string table: keys in, translated text out.
 *
 * The engine never contains a user-visible string. Every caption, label and piece of
 * prose is looked up by key in a @ref Euph::Loc::Localization, and the *file format*
 * that fills one is deliberately not part of this library: a loader parses whatever
 * the project uses (the sandbox reads the YAML dialect from
 * `Sandbox/LocalizationYaml.hpp`) and hands the key/value pairs over through
 * `Localization::insert()`. That is the same split the theme loader uses, and for the same
 * reason - no parser is ever a dependency of an engine library.
 *
 * @section euph_localization_keys Keys
 *
 * A key is a lowercase identifier; the recommended shape is
 * `<area>_<subject>[_<detail>]`, which is what the reference format's
 * `menu_new_game` / `settings_volume_music` scheme produces and what keeps a
 * grep for a caption land on exactly one place. Keys are never translated, so they
 * are also what a caller may compare or switch on.
 *
 * @section euph_localization_format Values
 *
 * A value is UTF-8 text and may contain `$name$` placeholders. `Localization::format()`
 * substitutes them; an unknown placeholder is left in place verbatim, which makes a
 * missing parameter visible in the UI instead of silently eating the word. Rich-text
 * markup is *not* interpreted here: a value goes into the text pipeline as-is, and
 * whatever markup the caller's text layer understands travels through untouched.
 *
 * @section euph_localization_fallback Fallback and layering
 *
 * One @ref Euph::Loc::Localization holds one language. `Localization::overlay()` merges another table
 * underneath - the classic "load the fallback language first, then the requested
 * one" - and `Localization::text()` answers with the key itself when nothing matched, so a
 * missing translation renders as `menu_new_game` rather than as an empty button.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Util/ElvAllocatorBasic.hpp>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Euph {
namespace Loc {

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(Localization)

/**
 * @brief A lookup table of translated strings, keyed by a stable identifier.
 *
 * The class is a thin container on purpose: no file access, no caching of parsed
 * documents, no language negotiation. Those belong to the loader and to the
 * application, which keeps this header usable from a test without a filesystem.
 */
class MH_EUPH_API Localization
{
public:
	/** @brief Maps a key to its translated text, ordered for deterministic iteration. */
	using EntryMap = std::map<std::string, std::string, std::less<>>;

	/** @brief Creates an empty table. */
	Localization();
	/** @brief Copies a table. */
	Localization(const Localization&) = default;
	/** @brief Moves a table. */
	Localization(Localization&&) = default;
	/** @brief Copies a table. */
	Localization& operator=(const Localization&) = default;
	/** @brief Moves a table. */
	Localization& operator=(Localization&&) = default;
	/** @brief Releases the table. */
	~Localization() = default;

	/// @name Insertion
	///@{
	/**
	 * @brief Adds or replaces one entry.
	 * @param key Identifier the text is looked up by.
	 * @param text UTF-8 value; may contain `$name$` placeholders.
	 *
	 * An empty key is ignored: it could never be looked up, and letting it in would
	 * only make @ref size() lie about how many useful strings the table holds.
	 */
	void insert(std::string key, std::string text);
	/**
	 * @brief Merges a table into this one, replacing on conflict.
	 * @param other Table to copy from; it is not modified.
	 */
	void merge(const Localization& other);
	/**
	 * @brief Layers a table underneath this one.
	 * @param base Table whose entries are only taken where this table has none.
	 *
	 * This is how a language is assembled: overlay the fallback language first, then
	 * the requested one, so a partially translated language still has every key.
	 */
	void overlay(const Localization& base);
	/** @brief Removes every entry. */
	void clear();
	///@}

	/// @name Lookup
	///@{
	/**
	 * @brief Finds the text of one key.
	 * @param key Identifier to look up.
	 * @return The stored text, or null when the key is unknown.
	 */
	const std::string* find(std::string_view key) const;
	/**
	 * @brief Finds the text of one key, falling back to the key itself.
	 * @param key Identifier to look up.
	 * @return The stored text, or a view of @p key when it is unknown.
	 */
	std::string_view text(std::string_view key) const;
	/**
	 * @brief Looks up a key and substitutes `$name$` placeholders.
	 * @param key Identifier to look up.
	 * @param parameters Placeholder/value pairs, in any order.
	 * @return The formatted text; an unknown key formats its own key, and a
	 *         placeholder without a parameter is left untouched.
	 */
	std::string format(std::string_view key,
					   std::initializer_list<std::pair<std::string_view, std::string_view>> parameters) const;
	/**
	 * @brief Substitutes `$name$` placeholders in arbitrary text.
	 * @param text Source text.
	 * @param parameters Placeholder/value pairs, in any order.
	 * @return The formatted text.
	 */
	static std::string formatText(std::string_view text,
								  std::initializer_list<std::pair<std::string_view, std::string_view>> parameters);
	/** @brief Number of stored entries. */
	std::size_t size() const noexcept { return entries.size(); }
	/** @brief True when no entry is stored. */
	bool empty() const noexcept { return entries.empty(); }
	/** @brief True when @p key has an entry. */
	bool contains(std::string_view key) const;
	/** @brief Every entry, ordered by key. */
	const EntryMap& all() const noexcept { return entries; }
	///@}

private:
	/** @brief Key to text. */
	EntryMap entries;
};

} // namespace Loc
} // namespace Euph

#endif // EUPHLOCALIZATION_HPP
