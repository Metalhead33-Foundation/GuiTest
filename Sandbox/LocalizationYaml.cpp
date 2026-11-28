#include "LocalizationYaml.hpp"
#include <yaml.h>
#include <cctype>
#include <string>

namespace Sandbox {

namespace {

/** @brief One scalar as libyaml delivered it. */
struct ScalarText {
	/** @brief Pointer into the parser's buffer; valid until the event is deleted. */
	const char* data = nullptr;
	/** @brief Length in bytes; the buffer is not NUL-terminated in general. */
	std::size_t size = 0;
	/** @brief True when the scalar was written as an explicit null / left empty. */
	bool isNull = false;

	/** @brief Materialises the scalar as a `std::string`. */
	std::string str() const { return data != nullptr ? std::string(data, size) : std::string(); }
};

/** @brief Copies a scalar event, marking implicit nulls. */
ScalarText readScalar(const yaml_event_t& event)
{
	ScalarText scalar;
	scalar.data = reinterpret_cast<const char*>(event.data.scalar.value);
	scalar.size = event.data.scalar.length;
	const bool nullTag = event.data.scalar.tag != nullptr &&
						 std::string(reinterpret_cast<const char*>(event.data.scalar.tag)) ==
							 "tag:yaml.org,2002:null";
	scalar.isNull = nullTag && event.data.scalar.length == 0;
	return scalar;
}

/** @brief Escapes a string so it can sit inside a double-quoted YAML scalar. */
std::string escapeQuoted(std::string_view text)
{
	std::string escaped;
	escaped.reserve(text.size() + 2);
	for (const char character : text) {
		if (character == '"' || character == '\\') {
			escaped.push_back('\\');
		}
		escaped.push_back(character);
	}
	return escaped;
}

/** @brief True when @p character may appear in an unquoted localization key. */
bool isKeyCharacter(char character)
{
	const unsigned char value = static_cast<unsigned char>(character);
	return std::isalnum(value) != 0 || character == '_' || character == '.' || character == '-';
}

/**
 * @brief Rewrites one `key:0 "text"` line into the strict `key: "text"` form.
 * @param line Source line, without its newline.
 * @param out Receives the rewritten line.
 * @return True when the line had the dialect shape and was rewritten.
 *
 * The reference dialect is *not* YAML: `key:0 "text"` has no space after the colon, so
 * a YAML parser folds it and every following line into one plain scalar. Normalising
 * before parsing is what lets libyaml do the structural work while the text keeps its
 * dialect spelling on disk.
 */
bool normaliseDialectLine(std::string_view line, std::string& out)
{
	std::size_t cursor = 0;
	while (cursor < line.size() && (line[cursor] == ' ' || line[cursor] == '\t')) {
		++cursor;
	}
	const std::size_t indent = cursor;
	const std::size_t keyStart = cursor;
	while (cursor < line.size() && isKeyCharacter(line[cursor])) {
		++cursor;
	}
	const std::size_t keyEnd = cursor;
	if (keyEnd == keyStart || cursor >= line.size() || line[cursor] != ':') {
		return false;
	}
	++cursor;
	// The version counter is what distinguishes the dialect from strict YAML: a shipped
	// entry always has one (`key:0 "text"`), while a strict pair has a space right after
	// the colon (`key: "text"`). Requiring a digit is what keeps an already valid
	// double-quoted YAML value - escapes and all - out of the rewriting below.
	const std::size_t afterColon = cursor;
	while (cursor < line.size() && std::isdigit(static_cast<unsigned char>(line[cursor]))) {
		++cursor;
	}
	if (cursor == afterColon) {
		return false;
	}
	if (cursor < line.size() && line[cursor] != ' ' && line[cursor] != '\t') {
		return false;
	}
	while (cursor < line.size() && (line[cursor] == ' ' || line[cursor] == '\t')) {
		++cursor;
	}
	if (cursor >= line.size() || line[cursor] != '"') {
		return false;
	}
	const std::size_t textStart = cursor + 1;
	const std::size_t textEnd = line.rfind('"');
	if (textEnd == std::string_view::npos || textEnd < textStart) {
		return false;
	}
	// Anything after the closing quote has to be blank or a comment, otherwise this is
	// not the shape we know how to rewrite.
	for (std::size_t tail = textEnd + 1; tail < line.size(); ++tail) {
		if (line[tail] == '#') {
			break;
		}
		if (line[tail] != ' ' && line[tail] != '\t' && line[tail] != '\r') {
			return false;
		}
	}
	out.assign(line.substr(0, indent));
	out.append(line.substr(keyStart, keyEnd - keyStart));
	out.append(": \"");
	out.append(escapeQuoted(line.substr(textStart, textEnd - textStart)));
	out.push_back('"');
	return true;
}

/**
 * @brief Rewrites every dialect line of a document into strict YAML.
 * @param text Whole document.
 * @return The normalised document; unchanged lines are copied verbatim.
 */
std::string normaliseDialect(std::string_view text)
{
	std::string result;
	result.reserve(text.size() + text.size() / 8);
	std::size_t position = 0;
	while (position <= text.size()) {
		const std::size_t newline = text.find('\n', position);
		const std::size_t end = newline == std::string_view::npos ? text.size() : newline;
		const std::string_view line = text.substr(position, end - position);
		std::string rewritten;
		if (normaliseDialectLine(line, rewritten)) {
			result.append(rewritten);
		} else {
			result.append(line);
		}
		if (newline == std::string_view::npos) {
			break;
		}
		result.push_back('\n');
		position = newline + 1;
	}
	return result;
}

/** @brief Appends one formatted problem line. */
void addProblem(LocalizationParseResult& result, const yaml_parser_t& parser, const std::string& message)
{
	std::string line = message;
	if (parser.problem != nullptr) {
		line += ": ";
		line += parser.problem;
	}
	line += " (line " + std::to_string(parser.problem_mark.line + 1) + ", column " +
			std::to_string(parser.problem_mark.column + 1) + ")";
	result.problems.push_back(std::move(line));
}

/** @brief Stores one entry, reporting an empty key. */
void storeEntry(LocalizationParseResult& result, std::string key, std::string text)
{
	if (key.empty()) {
		result.problems.push_back("an entry has an empty key");
		return;
	}
	result.table.insert(std::move(key), std::move(text));
}

/** @brief Where the event loop currently is in the document shape. */
enum class ParseState {
	Root,       /**< Inside the document's outer mapping; keys are language ids. */
	RootValue,  /**< Right after a language id: expect its mapping. */
	Entries,    /**< Inside a language mapping: keys are localization keys. */
	EntryValue, /**< Right after a localization key: expect its text. */
	Skipping    /**< Inside a nested structure nobody consumes. */
};

} // namespace

LocalizationParseResult parseLocalizationYaml(std::string_view text)
{
	LocalizationParseResult result;
	// The dialect is normalised first; see normaliseDialect().
	const std::string document = normaliseDialect(text);
	yaml_parser_t parser;
	if (yaml_parser_initialize(&parser) == 0) {
		result.problems.push_back("could not initialise the YAML parser");
		return result;
	}
	yaml_parser_set_input_string(&parser,
								 reinterpret_cast<const unsigned char*>(document.data()),
								 document.size());

	ParseState state = ParseState::Root;
	std::size_t skipDepth = 0;
	bool sawRootMapping = false;
	bool sawDocument = false;
	bool finished = false;
	std::string pendingKey;

	while (!finished) {
		yaml_event_t event;
		if (yaml_parser_parse(&parser, &event) == 0) {
			addProblem(result, parser, "YAML parser error");
			break;
		}
		const yaml_event_type_t type = event.type;
		switch (type) {
			case YAML_STREAM_START_EVENT:
			case YAML_DOCUMENT_START_EVENT:
			case YAML_DOCUMENT_END_EVENT:
				break;
			case YAML_STREAM_END_EVENT:
				finished = true;
				break;
			case YAML_MAPPING_START_EVENT:
			case YAML_SEQUENCE_START_EVENT: {
				const bool isMapping = type == YAML_MAPPING_START_EVENT;
				if (state == ParseState::RootValue && isMapping) {
					state = ParseState::Entries;
					break;
				}
				if (!sawRootMapping && isMapping && state == ParseState::Root) {
					sawRootMapping = true;
					sawDocument = true;
					break;
				}
				if (state == ParseState::Entries) {
					// Localizations are flat, so anything nested under a key is skipped.
					state = ParseState::Skipping;
					skipDepth = 1;
					result.problems.push_back("nested structure under '" + pendingKey + "' skipped");
					break;
				}
				if (state == ParseState::EntryValue) {
					state = ParseState::Skipping;
					skipDepth = 1;
					result.problems.push_back("value of '" + pendingKey + "' is not a scalar; skipped");
					break;
				}
				if (state == ParseState::Skipping) {
					++skipDepth;
				}
				break;
			}
			case YAML_MAPPING_END_EVENT:
			case YAML_SEQUENCE_END_EVENT:
				if (state == ParseState::Skipping) {
					if (--skipDepth == 0) {
						state = ParseState::Entries;
						pendingKey.clear();
					}
					break;
				}
				if (state == ParseState::Entries) {
					// End of one language table; the outer mapping continues.
					state = ParseState::Root;
					pendingKey.clear();
				}
				break;
			case YAML_SCALAR_EVENT: {
				const ScalarText scalar = readScalar(event);
				if (state == ParseState::Root) {
					// A language id such as `l_english`; which language a file holds is
					// decided by its directory, not by this key.
					pendingKey = scalar.str();
					state = ParseState::RootValue;
					break;
				}
				if (state == ParseState::Entries) {
					pendingKey = scalar.str();
					state = ParseState::EntryValue;
					break;
				}
				if (state == ParseState::EntryValue) {
					storeEntry(result, pendingKey, scalar.isNull ? std::string() : scalar.str());
					pendingKey.clear();
					state = ParseState::Entries;
				}
				break;
			}
			default:
				break;
		}
		yaml_event_delete(&event);
	}
	yaml_parser_delete(&parser);
	result.ok = sawDocument;
	if (!sawDocument) {
		result.problems.push_back("the document has no mapping at its root");
	}
	return result;
}

} // namespace Sandbox
