/**
 * @file DocumentationTests.cpp
 * @brief Checks that the Markdown note tree is complete, linked and stable.
 *
 * Every page of the generated reference is a file in this repository: the notes under
 * `docs/` and the module `README.md` files, which carry a `\page` line because Doxygen
 * renders a subdirectory README as directory documentation otherwise
 * (`docs/DOCUMENTATION.md` §11). These checks are the executable form of that document:
 * they read the files, so a page that has no id, is missing from its folder index, or is
 * named by a typo fails here instead of silently dropping out of the site.
 *
 * The checks are structural on purpose. They cannot tell whether a sentence is true or
 * useful; that remains a review question.
 */
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

/// @brief Repository root, injected by `UnitTests.pro` so the checks never depend on the working directory.
const fs::path& sourceRoot()
{
	static const fs::path root{UNITTESTS_SOURCE_ROOT};
	return root;
}

/// @brief Reads a text file whole.
/// @param path File to read.
/// @return The file's contents, or an empty string when it cannot be opened.
std::string readFile(const fs::path& path)
{
	std::ifstream stream(path, std::ios::binary);
	if (!stream)
		return {};
	return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

/// @brief Every Markdown file directly inside a folder, sorted by name.
/// @param folder Folder to enumerate.
/// @return The `.md` files in @p folder, `index.md` included.
std::vector<fs::path> markdownFiles(const fs::path& folder)
{
	std::vector<fs::path> files;
	if (!fs::is_directory(folder))
		return files;
	for (const fs::directory_entry& entry : fs::directory_iterator(folder)) {
		if (entry.is_regular_file() && entry.path().extension() == ".md")
			files.push_back(entry.path());
	}
	std::sort(files.begin(), files.end());
	return files;
}

/// @brief Extracts the explicit `{#id}` from a note's `# Title` line.
/// @param note Markdown file to inspect.
/// @return The id without braces, or an empty string when the title line has none.
std::string titleId(const fs::path& note)
{
	std::ifstream stream(note);
	std::string line;
	while (std::getline(stream, line)) {
		if (line.rfind("# ", 0) != 0)
			continue;
		const std::size_t open = line.rfind("{#");
		if (open == std::string::npos)
			return {};
		const std::size_t close = line.find('}', open);
		if (close == std::string::npos)
			return {};
		return line.substr(open + 2, close - open - 2);
	}
	return {};
}

/// @brief Reads the id from a module README's `\page <id> <Title>` line.
/// @param readme Module README to inspect.
/// @return The page id, or an empty string when the line is absent.
std::string modulePageId(const fs::path& readme)
{
	std::ifstream stream(readme);
	std::string line;
	while (std::getline(stream, line)) {
		if (line.rfind("\\page ", 0) != 0)
			continue;
		const std::size_t begin = 6;
		const std::size_t end = line.find(' ', begin);
		return line.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
	}
	return {};
}

/// @brief Collects every `\subpage <id>` target in an index page.
/// @param index Index page to scan.
/// @return The ids the page claims as children.
std::set<std::string> subpageRows(const fs::path& index)
{
	static const std::string command = "\\subpage ";
	const std::string text = readFile(index);
	std::set<std::string> rows;
	for (std::size_t at = text.find(command); at != std::string::npos;
	     at = text.find(command, at + command.size())) {
		const std::size_t begin = at + command.size();
		std::size_t end = begin;
		while (end < text.size()
		       && (std::isalnum(static_cast<unsigned char>(text[end])) || text[end] == '_'))
			++end;
		if (end > begin)
			rows.insert(text.substr(begin, end - begin));
	}
	return rows;
}

/// @brief The page id a module folder must declare: `mod_` plus its lower-case name.
/// @param folder Module folder name, for example `KaldiVK`.
/// @return The expected id, for example `mod_kaldivk`.
std::string expectedModuleId(const std::string& folder)
{
	std::string id = "mod_";
	for (const char character : folder)
		id += static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
	return id;
}

/// @brief Every folder that holds notes, relative to `docs/`; the empty string is `docs/` itself.
const std::vector<std::string> kNoteFolders{"", "graphics", "text", "networking"};

/// @brief Pages that live next to the code they document instead of under `docs/`.
///
/// They are listed by a `docs/` index (the MML specification is listed by the text node),
/// so their ids have to be visible to the index-row check even though they are outside the
/// folders that hold notes.
const std::vector<std::string> kExternalPages{"Lotte/Text/MML.md"};

/// @brief `docs/index.md`, the four note-folder indexes, and the index-only `modules/` page.
/// @return The index pages, in tree order.
std::vector<fs::path> indexPages()
{
	std::vector<fs::path> pages{sourceRoot() / "docs" / "index.md"};
	for (const std::string& folder : kNoteFolders) {
		if (!folder.empty())
			pages.push_back(sourceRoot() / "docs" / folder / "index.md");
	}
	pages.push_back(sourceRoot() / "docs" / "modules" / "index.md");
	return pages;
}

/// @brief Every module folder: an immediate subdirectory that has a README and is not tooling.
/// @return The module folders, sorted by name.
std::vector<fs::path> moduleFolders()
{
	std::vector<fs::path> folders;
	for (const fs::directory_entry& entry : fs::directory_iterator(sourceRoot())) {
		const std::string name = entry.path().filename().string();
		if (!entry.is_directory() || name.empty() || name.front() == '.')
			continue;
		if (name == "extern" || name == "build" || name == "docs")
			continue;
		if (fs::is_regular_file(entry.path() / "README.md"))
			folders.push_back(entry.path());
	}
	std::sort(folders.begin(), folders.end());
	return folders;
}


/// @brief Collects every declared page id: note titles, index titles and module `\page` lines.
/// @return The declared ids.
std::set<std::string> declaredPageIds()
{
	std::set<std::string> ids;
	for (const fs::path& index : indexPages()) {
		const std::string id = titleId(index);
		if (!id.empty())
			ids.insert(id);
	}
	for (const std::string& folder : kNoteFolders) {
		for (const fs::path& note : markdownFiles(sourceRoot() / "docs" / folder)) {
			if (note.filename() == "index.md")
				continue;
			const std::string id = titleId(note);
			if (!id.empty())
				ids.insert(id);
		}
	}
	for (const fs::path& module : moduleFolders()) {
		const std::string id = modulePageId(module / "README.md");
		if (!id.empty())
			ids.insert(id);
	}
	for (const std::string& page : kExternalPages) {
		const std::string id = titleId(sourceRoot() / page);
		if (!id.empty())
			ids.insert(id);
	}
	return ids;
}

/// @brief Checks whether a `##` heading names the References section, ignoring a numeric prefix.
/// @param heading Heading text after the `##` marker.
/// @return `true` when the heading is `References` or `<number>. References`.
bool isReferencesHeading(const std::string& heading)
{
	std::string text = heading;
	while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
		text.erase(text.begin());
	std::size_t digits = 0;
	while (digits < text.size() && std::isdigit(static_cast<unsigned char>(text[digits])))
		++digits;
	if (digits > 0 && digits + 1 < text.size() && text[digits] == '.')
		text = text.substr(digits + 1);
	while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
		text.erase(text.begin());
	while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
		text.pop_back();
	return text == "References";
}

}

TEST_CASE("The source root is where the checks expect it", "[docs]")
{
	REQUIRE(fs::is_directory(sourceRoot() / "docs"));
	REQUIRE(fs::is_regular_file(sourceRoot() / "README.md"));
	REQUIRE(fs::is_regular_file(sourceRoot() / "Doxyfile"));
}

TEST_CASE("Every documentation note declares a page id", "[docs]")
{
	for (const std::string& folder : kNoteFolders) {
		for (const fs::path& note : markdownFiles(sourceRoot() / "docs" / folder)) {
			const std::string id = titleId(note);
			INFO("note: " << fs::relative(note, sourceRoot()).string());
			REQUIRE_FALSE(id.empty());
			if (note.filename() == "index.md") {
				REQUIRE(id.rfind("docs_", 0) == 0);
				REQUIRE(id.size() > std::string("docs_").size());
			} else {
				REQUIRE(id.rfind("doc_", 0) == 0);
			}
		}
	}
	for (const fs::path& index : indexPages()) {
		INFO("index: " << fs::relative(index, sourceRoot()).string());
		REQUIRE_FALSE(titleId(index).empty());
	}
}

TEST_CASE("Every documentation note is listed in its folder index", "[docs]")
{
	for (const std::string& folder : kNoteFolders) {
		const fs::path index = sourceRoot() / "docs" / folder / "index.md";
		const std::set<std::string> rows = subpageRows(index);
		for (const fs::path& note : markdownFiles(sourceRoot() / "docs" / folder)) {
			if (note.filename() == "index.md")
				continue;
			INFO("note: " << fs::relative(note, sourceRoot()).string());
			INFO("index: " << fs::relative(index, sourceRoot()).string());
			REQUIRE(rows.count(titleId(note)) == 1);
		}
	}
}


TEST_CASE("Every index row resolves to a declared page", "[docs]")
{
	const std::set<std::string> declared = declaredPageIds();
	for (const fs::path& index : indexPages()) {
		for (const std::string& row : subpageRows(index)) {
			INFO("index: " << fs::relative(index, sourceRoot()).string());
			INFO("row: " << row);
			REQUIRE(declared.count(row) == 1);
		}
	}
}

TEST_CASE("Every module README is a page listed in the modules index", "[docs]")
{
	const fs::path index = sourceRoot() / "docs" / "modules" / "index.md";
	const std::set<std::string> rows = subpageRows(index);
	const std::vector<fs::path> modules = moduleFolders();
	REQUIRE_FALSE(modules.empty());
	for (const fs::path& module : modules) {
		const std::string expected = expectedModuleId(module.filename().string());
		const std::string declared = modulePageId(module / "README.md");
		INFO("module: " << module.filename().string());
		REQUIRE(declared == expected);
		REQUIRE(rows.count(declared) == 1);
	}
}

TEST_CASE("Page ids are unique", "[docs]")
{
	std::map<std::string, std::vector<std::string>> seen;
	for (const fs::path& index : indexPages()) {
		const std::string id = titleId(index);
		if (!id.empty())
			seen[id].push_back(fs::relative(index, sourceRoot()).string());
	}
	for (const std::string& folder : kNoteFolders) {
		for (const fs::path& note : markdownFiles(sourceRoot() / "docs" / folder)) {
			if (note.filename() == "index.md")
				continue;
			const std::string id = titleId(note);
			if (!id.empty())
				seen[id].push_back(fs::relative(note, sourceRoot()).string());
		}
	}
	for (const fs::path& module : moduleFolders()) {
		const std::string id = modulePageId(module / "README.md");
		if (!id.empty())
			seen[id].push_back(fs::relative(module / "README.md", sourceRoot()).string());
	}
	for (const std::string& page : kExternalPages) {
		const std::string id = titleId(sourceRoot() / page);
		if (!id.empty())
			seen[id].push_back(page);
	}
	for (const auto& entry : seen) {
		INFO("id: " << entry.first);
		REQUIRE(entry.second.size() == 1);
	}
}

TEST_CASE("References is the last section of every note", "[docs][references]")
{
	std::vector<fs::path> notes;
	for (const std::string& folder : kNoteFolders) {
		for (const fs::path& note : markdownFiles(sourceRoot() / "docs" / folder))
			notes.push_back(note);
	}
	REQUIRE_FALSE(notes.empty());
	for (const fs::path& note : notes) {
		std::ifstream stream(note);
		std::string line;
		std::string lastHeading;
		while (std::getline(stream, line)) {
			if (line.rfind("## ", 0) == 0)
				lastHeading = line.substr(3);
		}
		INFO("note: " << fs::relative(note, sourceRoot()).string());
		REQUIRE(isReferencesHeading(lastHeading));
	}
}

