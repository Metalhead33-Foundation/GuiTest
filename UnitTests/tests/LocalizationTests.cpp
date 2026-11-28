/**
 * @file LocalizationTests.cpp
 * @brief Tests for the string table and the YAML document reader.
 *
 * Two layers are covered: `Euph::Loc::Localization` (the container and its formatting
 * rules, which live in Euphemy) and `Sandbox::parseLocalizationYaml()` (the adapter
 * that reads the reference dialect, which lives in the Sandbox by policy). The latter
 * is compiled straight into this binary, exactly like the theme loader.
 */
#include <catch2/catch_test_macros.hpp>
#include <Euphemy/Localization/EuphLocalization.hpp>
#include <LocalizationYaml.hpp>
#include <string>
#include <string_view>

namespace {

using Euph::Loc::Localization;

} // namespace

TEST_CASE("Localization stores, layers and formats", "[localization]")
{
	Localization table;
	REQUIRE(table.empty());
	// An empty key can never be looked up, so it is ignored rather than stored.
	table.insert("", "ignored");
	REQUIRE(table.empty());

	table.insert("menu_exit", "Exit");
	REQUIRE(table.size() == 1u);
	REQUIRE(table.contains("menu_exit"));
	REQUIRE(table.find("menu_exit") != nullptr);
	REQUIRE(*table.find("menu_exit") == "Exit");

	// The fallback of a missing key is the key itself: visible, never empty.
	REQUIRE(table.text("menu_missing") == "menu_missing");
	REQUIRE(table.find("menu_missing") == nullptr);

	// A later insert replaces the previous text instead of adding an entry.
	table.insert("menu_exit", "Quit");
	REQUIRE(table.size() == 1u);
	REQUIRE(table.text("menu_exit") == "Quit");
}

TEST_CASE("Localization substitutes placeholders and leaves unknown ones", "[localization]")
{
	Localization table;
	table.insert("greeting", "Hello $name$, you have $count$ message(s)");
	table.insert("dangling", "50$ off");
	table.insert("plain", "nothing to do");

	REQUIRE(table.format("greeting", { { "name", "Anne" }, { "count", "3" } }) ==
			"Hello Anne, you have 3 message(s)");
	// A parameter that was not supplied stays visible with its dollars.
	REQUIRE(table.format("greeting", { { "name", "Anne" } }) ==
			"Hello Anne, you have $count$ message(s)");
	// A dangling `$` is literal text, not a placeholder.
	REQUIRE(table.format("dangling", {}) == "50$ off");
	REQUIRE(table.format("plain", {}) == "nothing to do");
	// A missing key formats its own key, so the absence is visible in the UI.
	REQUIRE(table.format("absent", { { "name", "x" } }) == "absent");
	REQUIRE(Localization::formatText("$a$-$b$", { { "a", "1" }, { "b", "2" } }) == "1-2");
}

TEST_CASE("Localization merges by replacement and overlays by gap", "[localization]")
{
	Localization base;
	base.insert("menu_exit", "Exit");
	base.insert("menu_back", "Back");

	Localization language;
	language.insert("menu_exit", "Beenden");

	// `merge` is the tool for a set of files that belong to the same language: the
	// last file wins on a duplicate key.
	Localization merged = base;
	merged.merge(language);
	REQUIRE(merged.text("menu_exit") == "Beenden");
	REQUIRE(merged.text("menu_back") == "Back");

	// `overlay` assembles a language over a fallback: the specific table wins, the
	// fallback only fills the gaps.
	Localization layered = language;
	layered.overlay(base);
	REQUIRE(layered.text("menu_exit") == "Beenden");
	REQUIRE(layered.text("menu_back") == "Back");
	REQUIRE(layered.size() == 2u);
}

TEST_CASE("Localization YAML reads the reference dialect", "[localization][yaml]")
{
	// The shape a shipped file uses: a language root, then `key:0 "text"` entries.
	// That line is not valid YAML - the colon is glued to the version number - so the
	// loader normalises it before libyaml sees it.
	const std::string document =
		"# a comment\n"
		"l_english:\n"
		" menu_new_game:0 \"New Game\"\n"
		" menu_exit:0 \"Exit\"\n"
		" settings_volume_music:0 \"Music\"\n";

	const Sandbox::LocalizationParseResult parsed = Sandbox::parseLocalizationYaml(document);
	REQUIRE(parsed.ok);
	REQUIRE(parsed.problems.empty());
	REQUIRE(parsed.entryCount() == 3u);
	REQUIRE(parsed.table.text("menu_new_game") == "New Game");
	REQUIRE(parsed.table.text("menu_exit") == "Exit");
	REQUIRE(parsed.table.text("settings_volume_music") == "Music");
}

TEST_CASE("Localization YAML also reads strict YAML and quotes", "[localization][yaml]")
{
	const std::string document =
		"l_english:\n"
		"  menu_exit: \"Exit\"\n"
		"  menu_credits: \"Credits: the \\\"short\\\" list\"\n"
		"  menu_empty:\n";

	const Sandbox::LocalizationParseResult parsed = Sandbox::parseLocalizationYaml(document);
	REQUIRE(parsed.ok);
	REQUIRE(parsed.entryCount() == 3u);
	REQUIRE(parsed.table.text("menu_exit") == "Exit");
	// Escapes survive the rewriting the loader performs on dialect lines.
	REQUIRE(parsed.table.text("menu_credits") == "Credits: the \"short\" list");
	// A key with no value is an empty translation, not a dropped key.
	REQUIRE(parsed.table.find("menu_empty") != nullptr);
	REQUIRE(parsed.table.text("menu_empty").empty());
}

TEST_CASE("Localization YAML skips nested structure and reports it", "[localization][yaml]")
{
	const std::string document =
		"l_english:\n"
		" menu_exit:0 \"Exit\"\n"
		" nested:\n"
		"  inner: \"ignored\"\n"
		" menu_back:0 \"Back\"\n";

	const Sandbox::LocalizationParseResult parsed = Sandbox::parseLocalizationYaml(document);
	REQUIRE(parsed.ok);
	// The nested block is dropped, its siblings are not.
	REQUIRE(parsed.entryCount() == 2u);
	REQUIRE(parsed.table.text("menu_exit") == "Exit");
	REQUIRE(parsed.table.text("menu_back") == "Back");
	REQUIRE_FALSE(parsed.problems.empty());
}

TEST_CASE("Localization YAML rejects a document without a root mapping", "[localization][yaml]")
{
	const Sandbox::LocalizationParseResult parsed =
		Sandbox::parseLocalizationYaml("- just\n- a\n- list\n");
	REQUIRE_FALSE(parsed.ok);
	REQUIRE(parsed.entryCount() == 0u);
	REQUIRE_FALSE(parsed.problems.empty());
}
