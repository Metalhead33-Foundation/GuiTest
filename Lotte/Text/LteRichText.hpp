#ifndef LTERICHTEXT_HPP
#define LTERICHTEXT_HPP
/**
 * @file LteRichText.hpp
 * @brief Converts styled character input into a list of @ref Lotte::Text::TextRun.
 *
 * Salvaged from `Text/RichTextProcessor.{hpp,cpp}` on the `NewBeginning` branch.
 * The public surface is deliberately close to the original so that ported call
 * sites stay readable: the `enable*`/`disable*`/`reset*` mutators, the static
 * `Manipulator` factories and the `operator<<` chain all survive.
 *
 * What changed, and why:
 *
 * - **No font repository.** The original held a `sFontRepository` and resolved
 *   `currentFontName` to a `Font*` on flush. Runtime font objects do not exist
 *   in this tree (see `LteTextAttributes.hpp`), so a run stores the requested
 *   family name and size instead, and resolution is the layout stage's job.
 * - **UTF-8 buffering stays, decoding moves.** Text is buffered as UTF-8 and
 *   decoded when a run is flushed, exactly like the original `stringstream` +
 *   `codecvt` pair, but with the portable decoder from `LteUtf.hpp`. Buffering
 *   is what makes `operator<<(char)` safe: the MML parser feeds one byte at a
 *   time, and multi-byte sequences must not be split across attribute changes.
 * - **Every attribute change flushes.** The original `setCurrentSize()` forgot
 *   to flush, so a `[size=20]` tag could retroactively restyle the text before
 *   it. All mutations now flush first.
 * - **Runs merge.** `flush()` appends to the previous run when the attribute set
 *   is unchanged, so `[b]a[/b][b]b[/b]` produces one run instead of two.
 *
 * The `std::ostream`-manipulator overloads (`std::endl` and friends) are gone:
 * they only made sense while the class owned a `std::stringstream`.
 */

#include <Lotte/LotteLib.hpp>
#include <Lotte/Text/LteTextAttributes.hpp>
#include <cstdint>
#include <functional>
#include <string>

namespace Lotte {
namespace Text {

/** @brief Accumulates UTF-8 text with a mutable attribute set. */
class MH_LOTTE_API RichTextProcessor
{
public:
	/** @brief Callable that mutates a processor; used like a stream manipulator. */
	using Manipulator = std::function<void(RichTextProcessor&)>;
private:
	TextRunList runs;
	std::string pendingUtf8;
	TextAttributes currentAttributes;
	TextAttributes defaultAttributes;
	void appendRun();
public:
	/** @brief Creates a processor with the default attribute set. */
	RichTextProcessor();
	/**
	 * @brief Creates a processor with an explicit default attribute set.
	 * @param defaults Attributes used until the first override.
	 */
	explicit RichTextProcessor(const TextAttributes& defaults);

	/** @brief Emits the buffered text as a run. */
	void flush();
	/** @brief Drops every run and any buffered text. */
	void clear();

	/** @brief Returns the runs emitted so far; call @ref flush() first for pending text. */
	const TextRunList& getRuns() const { return runs; }
	/** @brief Returns the runs emitted so far. */
	TextRunList& getRuns() { return runs; }
	/** @brief Returns true when no run has been emitted yet. */
	bool empty() const { return runs.empty(); }
	/** @brief Returns true when text is buffered but not yet emitted. */
	bool hasPendingText() const { return !pendingUtf8.empty(); }

	/** @brief Returns the attributes that will be applied to the next run. */
	const TextAttributes& getCurrentAttributes() const { return currentAttributes; }
	/** @brief Returns the defaults restored by @ref resetColour()/@ref resetFont()/@ref resetSize(). */
	const TextAttributes& getDefaultAttributes() const { return defaultAttributes; }
	/**
	 * @brief Replaces the default attribute set.
	 * @param defaults New defaults; the current attributes are not retroactively changed.
	 */
	void setDefaultAttributes(const TextAttributes& defaults) { defaultAttributes = defaults; }

	/** @brief Returns the current pixel size. */
	float getCurrentSize() const { return currentAttributes.size; }
	/** @brief Sets the pixel size used by subsequent runs. */
	void setCurrentSize(float size);
	/** @brief Returns the current font family name. */
	const std::string& getCurrentFontName() const { return currentAttributes.familyName; }
	/** @brief Sets the font family name used by subsequent runs. */
	void setCurrentFontName(std::string familyName);
	/** @brief Returns the current colour. */
	const Colour& getCurrentColour() const { return currentAttributes.colour; }
	/** @brief Sets the colour used by subsequent runs. */
	void setCurrentColour(const Colour& rgba);
	/** @brief Sets the colour used by subsequent runs, with an opaque alpha. */
	void setCurrentColour(const glm::fvec3& rgb);
	/** @brief Sets the colour used by subsequent runs from 8-bit components. */
	void setCurrentColour(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 0xFF);

	/** @brief Returns whether bold is currently requested. */
	bool getIsBold() const { return currentAttributes.bold; }
	/** @brief Returns whether italic is currently requested. */
	bool getIsItalic() const { return currentAttributes.italic; }
	/** @brief Returns whether underline is currently requested. */
	bool getIsUnderline() const { return currentAttributes.underline; }
	/** @brief Returns whether strikethrough is currently requested. */
	bool getIsStrikethrough() const { return currentAttributes.strikethrough; }

	/** @brief Enables bold for subsequent runs. */
	void enableBold();
	/** @brief Enables italic for subsequent runs. */
	void enableItalic();
	/** @brief Enables underline for subsequent runs. */
	void enableUnderline();
	/** @brief Enables strikethrough for subsequent runs. */
	void enableStrikethrough();
	/** @brief Disables bold for subsequent runs. */
	void disableBold();
	/** @brief Disables italic for subsequent runs. */
	void disableItalic();
	/** @brief Disables underline for subsequent runs. */
	void disableUnderline();
	/** @brief Disables strikethrough for subsequent runs. */
	void disableStrikethrough();

	/** @brief Restores the default colour. */
	void resetColour();
	/** @brief Restores the default font family name. */
	void resetFont();
	/** @brief Restores the default size. */
	void resetSize();

	/// @name Manipulator factories
	///@{
	/** @brief Returns a manipulator that changes the font family. */
	static Manipulator ChangeFont(std::string fontName);
	/** @brief Returns a manipulator that changes the colour. */
	static Manipulator ChangeColour(const Colour& rgba);
	/** @brief Returns a manipulator that changes the colour, with an opaque alpha. */
	static Manipulator ChangeColour(const glm::fvec3& rgb);
	/** @brief Returns a manipulator that changes the colour from 8-bit components. */
	static Manipulator ChangeColour(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 0xFF);
	/** @brief Returns a manipulator that changes the pixel size. */
	static Manipulator ChangeSize(float size);
	/** @brief Returns a manipulator that enables bold. */
	static Manipulator EnableBold();
	/** @brief Returns a manipulator that enables italic. */
	static Manipulator EnableItalic();
	/** @brief Returns a manipulator that enables underline. */
	static Manipulator EnableUnderline();
	/** @brief Returns a manipulator that enables strikethrough. */
	static Manipulator EnableStrikethrough();
	/** @brief Returns a manipulator that disables bold. */
	static Manipulator DisableBold();
	/** @brief Returns a manipulator that disables italic. */
	static Manipulator DisableItalic();
	/** @brief Returns a manipulator that disables underline. */
	static Manipulator DisableUnderline();
	/** @brief Returns a manipulator that disables strikethrough. */
	static Manipulator DisableStrikethrough();
	///@}

	/// @name Text input
	///@{
	/** @brief Appends one raw byte of UTF-8 text. */
	RichTextProcessor& operator<<(char c);
	/** @brief Appends a UTF-8 string. */
	RichTextProcessor& operator<<(const char* text);
	/** @brief Appends a UTF-8 string. */
	RichTextProcessor& operator<<(const std::string& text);
	/** @brief Appends one code point. */
	RichTextProcessor& operator<<(char32_t codePoint);
	/** @brief Applies a manipulator. */
	RichTextProcessor& operator<<(const Manipulator& manipulator);
	///@}
};

}
}

#endif // LTERICHTEXT_HPP
