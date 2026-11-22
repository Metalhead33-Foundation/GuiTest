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

/**
 * @brief Receives MML element boundaries, so an external styling layer can drive
 *        the processor instead of the built-in tag handlers.
 *
 * MML tags are attribute toggles (`[b]`, `[size=20]`), but they are also *elements*:
 * a document is a tree of `<tag>` instances, and CSS is a language for styling such
 * a tree. Attaching an observer turns the parser into an element stream - every
 * non-structural tag becomes an open/close event carrying its name and its optional
 * `class`/`id` parameters - and the observer decides which attributes the processor
 * sees. That is what `Text/CssStyle.hpp` does with libcss; see `docs/MML_CSS.md`.
 *
 * @section lte_mml_element_observer_syntax Parameter syntax
 *
 * A tag may carry a parameter list after its name, introduced by `=` or by
 * whitespace (the latter only when the next parameter is `class=`/`id=`, because
 * MML otherwise removes all whitespace inside a tag):
 *
 *     <b class=warning>
 *     <b=class=warning,id=loud>
 *     [span class=note]
 *
 * `class` and `id` are the only recognised parameter names. Any other single
 * parameter keeps MML's positional meaning and is passed to the built-in handlers
 * when no observer is attached; with an observer it is ignored, because the
 * stylesheet decides the presentation.
 */
class MH_LOTTE_API MmlElementObserver
{
public:
	/** @brief Destroys the observer; the parser never owns it. */
	virtual ~MmlElementObserver();

	/**
	 * @brief Called when an opening tag is processed.
	 * @param name Lowercase tag name, without `<`, `>` or the leading `/`.
	 * @param classes Space-separated `class` values, possibly empty.
	 * @param id The `id` value, possibly empty.
	 */
	virtual void onElementOpen(std::string_view name, std::string_view classes,
				   std::string_view id) = 0;
	/**
	 * @brief Called when a closing tag is processed.
	 * @param name Lowercase tag name of the element being closed.
	 */
	virtual void onElementClose(std::string_view name) = 0;
	/**
	 * @brief True when the observer is the only source of styling.
	 *
	 * When true (the default) the parser stops applying MML's built-in attribute
	 * handlers - `[b]`, `[i]`, `[u]`, `[s]`, `[font=]`, `[size=]`, `[color=]` - and
	 * leaves their meaning to the observer's stylesheet. Structural tags (`br`, `p`)
	 * still emit their line breaks, because those are part of the text, not the look.
	 */
	virtual bool ownsElementStyling() const;
};

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

	/**
	 * @brief Attaches an element observer, or detaches the current one.
	 * @param newObserver Observer that receives element boundaries; may be null.
	 *
	 * The parser never owns the observer. See @ref MmlElementObserver for the
	 * parameter syntax and for what changes when an observer is attached.
	 */
	void setElementObserver(MmlElementObserver* newObserver) { elementObserver = newObserver; }
	/** @brief Returns the attached element observer, or null. */
	MmlElementObserver* getElementObserver() const { return elementObserver; }

private:
	/** @brief Optional styling layer; not owned. */
	MmlElementObserver* elementObserver = nullptr;
};

}
}

#endif // LTEMML_HPP
