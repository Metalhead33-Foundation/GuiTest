#ifndef LTEMARKUPSTACK_HPP
#define LTEMARKUPSTACK_HPP
/**
 * @file LteMarkupStack.hpp
 * @brief Nesting stack for markup attributes.
 *
 * Salvaged from `Text/RtProcessorStack.{hpp,cpp}` on the `NewBeginning` branch.
 * A markup document nests attributes - `[b]bold [i]bold italic[/i] bold[/b]` -
 * so each attribute needs a depth counter: the innermost tag decides the state,
 * and closing a tag restores whatever was active before it.
 *
 * What changed, and why:
 *
 * - **The `pop*` logic was inverted.** The original read
 *
 *       if (underlineStack > 0) RTP->disableUnderline();
 *       else --underlineStack;
 *
 *   which disabled the attribute on the *first* pop and then decremented a
 *   counter that was already zero, wrapping it to `SIZE_MAX`. Every pop now
 *   decrements first and only clears the attribute when the depth reaches zero.
 * - **Null-processor checks.** The push helpers for the boolean attributes
 *   dereferenced the processor without checking it, unlike the colour/font/size
 *   helpers. All of them check now.
 * - **`std::vector` instead of `std::stack`.** The values are only ever pushed
 *   and popped in LIFO order, and a vector makes the swap-free `top()` access
 *   and the depth queries trivial while staying contiguous.
 * - **Type names.** `pushUnderscore`/`popUnderscore` are spelled
 *   `pushUnderline`/`popUnderline`, matching the attribute they control, and
 *   `RTP` became `processor`.
 */

#include <Lotte/LotteLib.hpp>
#include <Lotte/Text/LteRichText.hpp>
#include <cstddef>
#include <string>
#include <vector>

namespace Lotte {
namespace Text {

/** @brief Tracks the nesting depth of every MML styling attribute. */
class MH_LOTTE_API MarkupStack
{
private:
	RichTextProcessor* processor = nullptr;
	std::vector<Colour> colourStack;
	std::vector<std::string> fontStack;
	std::vector<float> sizeStack;
	std::size_t underlineDepth = 0;
	std::size_t boldDepth = 0;
	std::size_t italicDepth = 0;
	std::size_t strikethroughDepth = 0;
public:
	/** @brief Creates a stack that is not attached to a processor yet. */
	MarkupStack() = default;
	/**
	 * @brief Creates a stack attached to a processor.
	 * @param processor Processor to drive; may be null.
	 */
	explicit MarkupStack(RichTextProcessor* processor);

	/** @brief Returns the attached processor, or null. */
	RichTextProcessor* getProcessor() const { return processor; }
	/**
	 * @brief Attaches a processor.
	 * @param newProcessor Processor to drive; may be null.
	 *
	 * The current default colour of the processor is pushed as the base of the
	 * colour stack, so an unbalanced `[/color]` restores it instead of leaving
	 * the stack empty.
	 */
	void setProcessor(RichTextProcessor* newProcessor);

	/// @name Push
	///@{
	/** @brief Pushes a colour and applies it. */
	void pushColour(const Colour& rgba);
	/** @brief Pushes an opaque colour and applies it. */
	void pushColour(const glm::fvec3& rgb);
	/** @brief Pushes a colour built from 8-bit components and applies it. */
	void pushColour(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 0xFF);
	/** @brief Pushes a font family name and applies it. */
	void pushFont(std::string familyName);
	/** @brief Pushes a pixel size and applies it. */
	void pushSize(float size);
	/** @brief Nests one underline level. */
	void pushUnderline();
	/** @brief Nests one bold level. */
	void pushBold();
	/** @brief Nests one italic level. */
	void pushItalic();
	/** @brief Nests one strikethrough level. */
	void pushStrikethrough();
	///@}

	/// @name Pop
	///@{
	/** @brief Pops one colour level. */
	void popColour();
	/** @brief Pops one font family level. */
	void popFont();
	/** @brief Pops one pixel size level. */
	void popSize();
	/** @brief Pops one underline level. */
	void popUnderline();
	/** @brief Pops one bold level. */
	void popBold();
	/** @brief Pops one italic level. */
	void popItalic();
	/** @brief Pops one strikethrough level. */
	void popStrikethrough();
	///@}

	/** @brief Returns the number of stacked colours. */
	std::size_t getColourDepth() const { return colourStack.size(); }
	/** @brief Returns the number of stacked font families. */
	std::size_t getFontDepth() const { return fontStack.size(); }
	/** @brief Returns the number of stacked sizes. */
	std::size_t getSizeDepth() const { return sizeStack.size(); }
	/** @brief Returns the current underline nesting depth. */
	std::size_t getUnderlineDepth() const { return underlineDepth; }
	/** @brief Returns the current bold nesting depth. */
	std::size_t getBoldDepth() const { return boldDepth; }
	/** @brief Returns the current italic nesting depth. */
	std::size_t getItalicDepth() const { return italicDepth; }
	/** @brief Returns the current strikethrough nesting depth. */
	std::size_t getStrikethroughDepth() const { return strikethroughDepth; }
};

}
}

#endif // LTEMARKUPSTACK_HPP
