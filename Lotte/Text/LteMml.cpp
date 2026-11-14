#include "LteMml.hpp"

#include "LteHtmlEntities.hpp"
#include "LteMarkupColour.hpp"
#include <cctype>
#include <charconv>
#include <string>
#include <unordered_map>

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

/** @brief Trims ASCII whitespace from both ends of a view. */
std::string_view trim(std::string_view text)
{
	while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) text.remove_prefix(1);
	while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.remove_suffix(1);
	return text;
}

} // namespace

MmlParser::MmlParser(RichTextProcessor* processor)
	: MarkupStack(processor)
{
}

void MmlParser::reset()
{
	commandBuffer.clear();
	isProcessingTag = false;
	isProcessingEscape = false;
}

void MmlParser::processTag(std::string_view tag)
{
	if (tag.empty()) return;

	const std::size_t equals = tag.find('=');
	if (equals != std::string_view::npos) {
		const std::string_view name = trim(tag.substr(0, equals));
		const std::string_view argument = trim(tag.substr(equals + 1));
		const auto withArgument = TagWithArgumentHandlers.find(name);
		if (withArgument != TagWithArgumentHandlers.end()) {
			withArgument->second(*this, argument);
			return;
		}
		// A tag that carries an argument may still be a plain tag; `<br/>` only
		// differs from `<br>` after whitespace removal, so fall through.
	}

	const auto plain = TagHandlers.find(tag);
	if (plain != TagHandlers.end()) plain->second(*this);
}

void MmlParser::onTagEnd()
{
	if (!isProcessingTag) return;
	std::string tag;
	tag.swap(commandBuffer);
	isProcessingTag = false;

	// MML treats tags as case-insensitive and ignores all whitespace inside them.
	std::string compacted;
	compacted.reserve(tag.size());
	for (char c : tag) {
		const auto lowered = char(std::tolower(static_cast<unsigned char>(c)));
		if (!std::isspace(static_cast<unsigned char>(lowered))) compacted.push_back(lowered);
	}
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
