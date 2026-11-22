#include "LteMml.hpp"

#include "LteHtmlEntities.hpp"
#include "LteMarkupColour.hpp"
#include <cctype>
#include <charconv>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Lotte {
namespace Text {

namespace {

/** @brief Handler for a tag without arguments. */
using TagHandler = void (*)(MmlParser&);
/** @brief Handler for a tag with one `name=argument` parameter. */
using TagHandlerWithArgument = void (*)(MmlParser&, std::string_view);

/** @brief Emits a newline into the current text buffer. */
void tagLineBreak(MmlParser& parser)
{
	if (RichTextProcessor* processor = parser.getProcessor()) *processor << '\n';
}

/** @brief Does nothing; used for markers such as `<p>` that only exist for symmetry. */
void tagNoop(MmlParser&) {}

void tagBold(MmlParser& parser) { parser.pushBold(); }
void tagNoBold(MmlParser& parser) { parser.popBold(); }
void tagItalic(MmlParser& parser) { parser.pushItalic(); }
void tagNoItalic(MmlParser& parser) { parser.popItalic(); }
void tagUnderline(MmlParser& parser) { parser.pushUnderline(); }
void tagNoUnderline(MmlParser& parser) { parser.popUnderline(); }
void tagStrikethrough(MmlParser& parser) { parser.pushStrikethrough(); }
void tagNoStrikethrough(MmlParser& parser) { parser.popStrikethrough(); }
void tagNoFont(MmlParser& parser) { parser.popFont(); }
void tagNoSize(MmlParser& parser) { parser.popSize(); }
void tagNoColour(MmlParser& parser) { parser.popColour(); }

/** @brief `[font=Name]`: pushes a font family request. */
void tagFont(MmlParser& parser, std::string_view argument)
{
	parser.pushFont(std::string(argument));
}

/** @brief `[size=N]`: pushes a pixel size request. */
void tagSize(MmlParser& parser, std::string_view argument)
{
	float size = 0.0f;
	const auto result = std::from_chars(argument.data(), argument.data() + argument.size(), size);
	if (result.ec != std::errc() || size <= 0.0f) return;
	parser.pushSize(size);
}

/** @brief `[color=...]`: pushes a colour, leaving the current colour untouched if unparsable. */
void tagColour(MmlParser& parser, std::string_view argument)
{
	Colour colour;
	if (!parseMarkupColour(argument, colour)) return;
	parser.pushColour(colour);
}

/**
 * @brief Tags that take no argument.
 *
 * The table keeps the original spellings and aliases: HTML-style (`b`, `strong`,
 * `em`, `del`) and the shorter BBCode-style forms (`s`, `i`, `u`).
 */
const std::unordered_map<std::string_view, TagHandler> TagHandlers = {
	{ "b", tagBold },
	{ "/b", tagNoBold },
	{ "strong", tagBold },
	{ "/strong", tagNoBold },
	{ "br", tagLineBreak },
	{ "/br", tagLineBreak },
	{ "br/", tagLineBreak },
	{ "p", tagNoop },
	{ "/p", tagLineBreak },
	{ "strike", tagStrikethrough },
	{ "/strike", tagNoStrikethrough },
	{ "s", tagStrikethrough },
	{ "/s", tagNoStrikethrough },
	{ "del", tagStrikethrough },
	{ "/del", tagNoStrikethrough },
	{ "i", tagItalic },
	{ "/i", tagNoItalic },
	{ "em", tagItalic },
	{ "/em", tagNoItalic },
	{ "u", tagUnderline },
	{ "/u", tagNoUnderline },
	{ "/font", tagNoFont },
	{ "/size", tagNoSize },
	{ "/color", tagNoColour },
	{ "/colour", tagNoColour }
};

/** @brief Tags that take one `name=argument` parameter. */
const std::unordered_map<std::string_view, TagHandlerWithArgument> TagWithArgumentHandlers = {
	{ "font", tagFont },
	{ "size", tagSize },
	{ "color", tagColour },
	{ "colour", tagColour }
};

/** @brief One tag split into the parts the element observer needs. */
struct TagParts {
	/** @brief Element name without the leading `/`. */
	std::string_view name;
	/** @brief True for a closing tag. */
	bool closing = false;
	/** @brief Space-separated `class` values. */
	std::string_view classes;
	/** @brief `id` value. */
	std::string_view id;
	/** @brief Positional argument of the legacy `font=`/`size=`/`color=` tags. */
	std::string_view argument;
	/** @brief True when the tag carried a parameter list at all. */
	bool hasParameters = false;
};

/** @brief True when @p text starts with `class=` or `id=`. */
bool startsWithAttributeKey(std::string_view text)
{
	return text.rfind("class=", 0) == 0 || text.rfind("id=", 0) == 0;
}

/**
 * @brief Splits a compacted tag into name, closing flag, attributes and argument.
 *
 * The parameter list is only read as `class=`/`id=` pairs when it contains one of
 * those keys; otherwise the whole parameter text stays the positional argument, so
 * the legacy forms (`[font=Arial]`, `[color=1.0,0.332,0.75]`) keep working.
 */
TagParts parseTag(std::string_view tag)
{
	TagParts parts;
	if (!tag.empty() && tag.front() == '/') {
		parts.closing = true;
		tag.remove_prefix(1);
	}

	const std::size_t separator = tag.find_first_of("= ");
	parts.name = tag.substr(0, separator);
	if (separator == std::string_view::npos) return parts;

	std::string_view parameters = tag.substr(separator + 1);
	parts.hasParameters = true;
	if (parameters.find("class=") == std::string_view::npos &&
	    parameters.find("id=") == std::string_view::npos) {
		parts.argument = parameters;
		return parts;
	}

	while (!parameters.empty()) {
		const std::size_t cut = parameters.find_first_of(", ");
		const std::string_view item = parameters.substr(0, cut);
		parameters = cut == std::string_view::npos ? std::string_view{}
							   : parameters.substr(cut + 1);
		if (item.rfind("class=", 0) == 0) parts.classes = item.substr(6);
		else if (item.rfind("id=", 0) == 0) parts.id = item.substr(3);
		else if (parts.argument.empty()) parts.argument = item;
	}
	return parts;
}

/** @brief Tags whose text-flow effect always applies, even when an observer styles. */
bool alwaysRunsHandler(std::string_view key)
{
	// `br` and `p` both carry a line break that belongs to the text, not to the look.
	return key == "br" || key == "/br" || key == "br/" || key == "p" || key == "/p";
}

/** @brief Tags that are pure text flow and therefore never elements. */
bool isPureTextTag(std::string_view key)
{
	// `p` *is* an element - HTML styles it, so does MML - but `br` is only a break.
	return key == "br" || key == "/br" || key == "br/";
}

/**
 * @brief Lowercases a tag and removes its whitespace, keeping `class=`/`id=` apart.
 *
 * MML removes all whitespace inside a tag, which would merge `b class=warn` into
 * `bclass=warn`. The one exception is whitespace that separates the element name
 * from a `class=`/`id=` parameter, which is kept as a single space so the readable
 * `[b class=warn]` spelling works. Everything else behaves as documented.
 */
std::string compactTag(std::string_view tag)
{
	std::string lowered;
	lowered.reserve(tag.size());
	for (char c : tag) lowered.push_back(char(std::tolower(static_cast<unsigned char>(c))));

	std::vector<std::string_view> tokens;
	std::size_t index = 0;
	while (index < lowered.size()) {
		while (index < lowered.size() && std::isspace(static_cast<unsigned char>(lowered[index]))) ++index;
		const std::size_t begin = index;
		while (index < lowered.size() && !std::isspace(static_cast<unsigned char>(lowered[index]))) ++index;
		if (index > begin) tokens.push_back(std::string_view(lowered).substr(begin, index - begin));
	}

	bool hasAttributeSeparation = false;
	for (std::size_t i = 1; i < tokens.size(); ++i) {
		if (startsWithAttributeKey(tokens[i])) {
			hasAttributeSeparation = true;
			break;
		}
	}

	std::string compacted;
	compacted.reserve(lowered.size());
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (i > 0 && hasAttributeSeparation) compacted.push_back(' ');
		compacted.append(tokens[i]);
	}
	return compacted;
}

} // namespace

MmlParser::MmlParser(RichTextProcessor* processor)
	: MarkupStack(processor)
{
}

MmlElementObserver::~MmlElementObserver() = default;

bool MmlElementObserver::ownsElementStyling() const
{
	return true;
}

void MmlParser::reset()
{
	commandBuffer.clear();
	isProcessingTag = false;
	isProcessingEscape = false;
}

void MmlParser::processTag(std::string_view compactedTag)
{
	if (compactedTag.empty()) return;

	const TagParts parts = parseTag(compactedTag);
	if (parts.name.empty()) return;

	std::string key;
	key.reserve(parts.name.size() + 1);
	if (parts.closing) key.push_back('/');
	key.append(parts.name);

	// Structural tags are part of the text, not of the look, so they always run.
	// The legacy styling handlers only run while no observer owns the styling.
	const bool fullControl = elementObserver != nullptr && elementObserver->ownsElementStyling();
	const auto tagKey = std::string_view(key);

	if (alwaysRunsHandler(tagKey) || !fullControl) {
		const auto plain = TagHandlers.find(tagKey);
		if (plain != TagHandlers.end()) {
			plain->second(*this);
		} else if (!parts.closing && parts.hasParameters) {
			// A tag that carries an argument may still be a plain tag; `<br/>` only
			// differs from `<br>` after whitespace removal, so fall through.
			const auto withArgument = TagWithArgumentHandlers.find(parts.name);
			if (withArgument != TagWithArgumentHandlers.end()) {
				withArgument->second(*this, parts.argument);
			}
		}
	}

	// The observer sees every element (including `p`, which a stylesheet can style),
	// and sees it *after* the legacy handler so a stylesheet always wins over an MML
	// tag attribute. Closing happens in the opposite order, which keeps the
	// processor's attribute stack balanced.
	if (elementObserver == nullptr || isPureTextTag(tagKey)) return;
	if (parts.closing) elementObserver->onElementClose(parts.name);
	else elementObserver->onElementOpen(parts.name, parts.classes, parts.id);
}

void MmlParser::onTagEnd()
{
	if (!isProcessingTag) return;
	std::string tag;
	tag.swap(commandBuffer);
	isProcessingTag = false;

	// MML treats tags as case-insensitive and ignores all whitespace inside them,
	// except for the `class=`/`id=` separator that `compactTag()` explains.
	const std::string compacted = compactTag(tag);
	processTag(compacted);
}

void MmlParser::processEscape(std::string_view escape)
{
	RichTextProcessor* processor = getProcessor();
	if (!processor) return;
	std::string utf8;
	if (decodeHtmlEntity(escape, utf8)) {
		*processor << utf8;
		return;
	}
	// Unknown reference: keep it visible rather than dropping input.
	*processor << '&';
	*processor << std::string(escape);
	*processor << ';';
}

void MmlParser::onEscapeEnd()
{
	if (!isProcessingEscape) return;
	std::string escape;
	escape.swap(commandBuffer);
	isProcessingEscape = false;
	processEscape(escape);
}

void MmlParser::parse(char c)
{
	RichTextProcessor* processor = getProcessor();

	if (isProcessingEscape) {
		if (c == ';') onEscapeEnd();
		else commandBuffer.push_back(c);
		return;
	}

	switch (c) {
		case '<':
		case '[':
			// A second opener restarts the tag instead of corrupting it.
			commandBuffer.clear();
			isProcessingTag = true;
			break;
		case '>':
		case ']':
			if (isProcessingTag) onTagEnd();
			else if (processor) *processor << c;
			break;
		case '&':
			if (isProcessingTag) commandBuffer.push_back(c);
			else {
				commandBuffer.clear();
				isProcessingEscape = true;
			}
			break;
		default:
			if (isProcessingTag) commandBuffer.push_back(c);
			else if (processor) *processor << c;
			break;
	}
}

void MmlParser::parse(std::string_view text)
{
	for (char c : text) parse(c);
}

void MmlParser::parse(const char* text)
{
	if (!text) return;
	for (const char* c = text; *c; ++c) parse(*c);
}

}
}
