#ifndef LTEMML_HPP
#define LTEMML_HPP
/**
 * @file LteMml.hpp
 * @brief Parser for MML, the markup language used by Lotte's rich text.
 *
 * Salvaged from `Text/MmlParser.{hpp,cpp}` and `Text/MML.md` on the
 * `NewBeginning` branch. MML is a union of a BBCode subset and an HTML subset:
 * `<b>` and `[b]` are interchangeable, tags are case-insensitive, whitespace
 * inside a tag is ignored, and HTML character references are required for the
 * characters that would otherwise start markup. The full grammar lives in
 * `Lotte/Text/MML.md`; `LteHtmlEntities.hpp` supplies the reference table.
 *
 * The parser is a byte-at-a-time state machine that feeds a
 * @ref Lotte::Text::RichTextProcessor through its @ref MarkupStack base, so the
 * output is a @ref Lotte::Text::TextRunList.
 *
 * @section lte_mml_changes Changes from the original implementation
 *
 * - **Stray delimiters survive.** The original routed `>` and `]` straight into
 *   `onTagEnd()`, which returned early when no tag was open, so plain text like
 *   `a > b` silently lost the `>` (and `]`). Delimiters outside a tag are now
 *   emitted as text.
 * - **`&` inside a tag.** The original switched into escape mode and dropped the
 *   tag that was being accumulated. Inside a tag, `&` is now literal text.
 * - **Unknown tags and references are preserved.** An unknown reference is
 *   re-emitted as `&name;` instead of vanishing, and an unknown tag with an
 *   argument falls back to a no-argument lookup before being ignored.
 * - **No `std::stringstream`.** The command buffer is a `std::string` plus one
 *   `find('=')`, and numbers are parsed with `std::from_chars`; the original
 *   built a `std::stringstream` per tag and used `std::stoul`, which throws.
 * - **Null-safe.** A parser without a processor records state but emits
 *   nothing; the original dereferenced `RTP` unconditionally in several paths.
 *
 * @note `parse()` never flushes. Call `getProcessor()->flush()` (or
 * `RichTextProcessor::flush()`) once the document is fully parsed, otherwise the
 * trailing text stays buffered.
 */

#include <Lotte/LotteLib.hpp>
#include <Lotte/Text/LteMarkupStack.hpp>
#include <string>
#include <string_view>

namespace Lotte {
namespace Text {

/** @brief Parses MML markup into a @ref Lotte::Text::RichTextProcessor. */
class MH_LOTTE_API MmlParser : public MarkupStack
{
private:
	std::string commandBuffer;
	bool isProcessingTag = false;
	bool isProcessingEscape = false;
	void onTagEnd();
	void onEscapeEnd();
	void processTag(std::string_view tag);
	void processEscape(std::string_view escape);
public:
	/** @brief Creates a parser that is not attached to a processor yet. */
	MmlParser() = default;
	/**
	 * @brief Creates a parser attached to a processor.
	 * @param processor Processor that receives text and attribute changes; may be null.
	 */
	explicit MmlParser(RichTextProcessor* processor);

	/**
	 * @brief Consumes one byte of markup.
	 * @param c Next byte of the document; multi-byte UTF-8 is fine because text
	 *          is buffered and decoded by the processor.
	 */
	void parse(char c);
	/**
	 * @brief Consumes a chunk of markup.
	 * @param text Next bytes of the document.
	 */
	void parse(std::string_view text);
	/**
	 * @brief Consumes a NUL-terminated document fragment.
	 * @param text Next bytes of the document; null is ignored.
	 */
	void parse(const char* text);

	/** @brief Drops any partially accumulated tag or escape and rewinds the state machine. */
	void reset();

	/** @brief Returns true when a tag is currently being accumulated. */
	bool isInTag() const { return isProcessingTag; }
	/** @brief Returns true when a character reference is currently being accumulated. */
	bool isInEscape() const { return isProcessingEscape; }
};

}
}

#endif // LTEMML_HPP
