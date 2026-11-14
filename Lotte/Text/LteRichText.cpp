#include "LteRichText.hpp"

#include "LteUtf.hpp"
#include <utility>

namespace Lotte {
namespace Text {

RichTextProcessor::RichTextProcessor()
{
	// `defaultAttributes` is declared after `currentAttributes`, so it cannot be
	// used in the member-initializer list here; both are default-constructed and
	// then synchronised in the body.
	currentAttributes = defaultAttributes;
}

RichTextProcessor::RichTextProcessor(const TextAttributes& defaults)
	: currentAttributes(defaults), defaultAttributes(defaults)
{
}

void RichTextProcessor::appendRun()
{
	if (pendingUtf8.empty()) return;
	TextRun run;
	run.text = decodeUtf8(pendingUtf8);
	run.attributes = currentAttributes;
	pendingUtf8.clear();
	if (!runs.empty() && runs.back().attributes == run.attributes) {
		runs.back().text += run.text;
		return;
	}
	runs.push_back(std::move(run));
}

void RichTextProcessor::flush()
{
	appendRun();
}

void RichTextProcessor::clear()
{
	runs.clear();
	pendingUtf8.clear();
	currentAttributes = defaultAttributes;
}

void RichTextProcessor::setCurrentSize(float size)
{
	flush();
	currentAttributes.size = size;
}

void RichTextProcessor::setCurrentFontName(std::string familyName)
{
	flush();
	currentAttributes.familyName = std::move(familyName);
}

void RichTextProcessor::setCurrentColour(const Colour& rgba)
{
	flush();
	currentAttributes.colour = rgba;
}

void RichTextProcessor::setCurrentColour(const glm::fvec3& rgb)
{
	flush();
	currentAttributes.colour = Colour(rgb, 1.0f);
}

void RichTextProcessor::setCurrentColour(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
{
	flush();
	currentAttributes.colour = Colour(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f, float(a) / 255.0f);
}

void RichTextProcessor::enableBold()
{
	if (currentAttributes.bold) return;
	flush();
	currentAttributes.bold = true;
}

void RichTextProcessor::enableItalic()
{
	if (currentAttributes.italic) return;
	flush();
	currentAttributes.italic = true;
}

void RichTextProcessor::enableUnderline()
{
	if (currentAttributes.underline) return;
	flush();
	currentAttributes.underline = true;
}

void RichTextProcessor::enableStrikethrough()
{
	if (currentAttributes.strikethrough) return;
	flush();
	currentAttributes.strikethrough = true;
}

void RichTextProcessor::disableBold()
{
	if (!currentAttributes.bold) return;
	flush();
	currentAttributes.bold = false;
}

void RichTextProcessor::disableItalic()
{
	if (!currentAttributes.italic) return;
	flush();
	currentAttributes.italic = false;
}

void RichTextProcessor::disableUnderline()
{
	if (!currentAttributes.underline) return;
	flush();
	currentAttributes.underline = false;
}

void RichTextProcessor::disableStrikethrough()
{
	if (!currentAttributes.strikethrough) return;
	flush();
	currentAttributes.strikethrough = false;
}

void RichTextProcessor::resetColour()
{
	flush();
	currentAttributes.colour = defaultAttributes.colour;
}

void RichTextProcessor::resetFont()
{
	flush();
	currentAttributes.familyName = defaultAttributes.familyName;
}

void RichTextProcessor::resetSize()
{
	flush();
	currentAttributes.size = defaultAttributes.size;
}

/*
 * The original factories captured their arguments by reference:
 *
 *     return [&fontname](RichTextProcessor& rtp) { rtp.setCurrentFontName(fontname); };
 *
 * That is a dangling reference as soon as the factory returns. All captures are
 * by value here.
 */

RichTextProcessor::Manipulator RichTextProcessor::ChangeFont(std::string fontName)
{
	return [fontName = std::move(fontName)](RichTextProcessor& rtp) {
		rtp.setCurrentFontName(fontName);
	};
}

RichTextProcessor::Manipulator RichTextProcessor::ChangeColour(const Colour& rgba)
{
	return [rgba](RichTextProcessor& rtp) {
		rtp.setCurrentColour(rgba);
	};
}

RichTextProcessor::Manipulator RichTextProcessor::ChangeColour(const glm::fvec3& rgb)
{
	return [rgb](RichTextProcessor& rtp) {
		rtp.setCurrentColour(rgb);
	};
}

RichTextProcessor::Manipulator RichTextProcessor::ChangeColour(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
{
	return [r, g, b, a](RichTextProcessor& rtp) {
		rtp.setCurrentColour(r, g, b, a);
	};
}

RichTextProcessor::Manipulator RichTextProcessor::ChangeSize(float size)
{
	return [size](RichTextProcessor& rtp) {
		rtp.setCurrentSize(size);
	};
}

RichTextProcessor::Manipulator RichTextProcessor::EnableBold()
{
	return [](RichTextProcessor& rtp) { rtp.enableBold(); };
}

RichTextProcessor::Manipulator RichTextProcessor::EnableItalic()
{
	return [](RichTextProcessor& rtp) { rtp.enableItalic(); };
}

RichTextProcessor::Manipulator RichTextProcessor::EnableUnderline()
{
	return [](RichTextProcessor& rtp) { rtp.enableUnderline(); };
}

RichTextProcessor::Manipulator RichTextProcessor::EnableStrikethrough()
{
	return [](RichTextProcessor& rtp) { rtp.enableStrikethrough(); };
}

RichTextProcessor::Manipulator RichTextProcessor::DisableBold()
{
	return [](RichTextProcessor& rtp) { rtp.disableBold(); };
}

RichTextProcessor::Manipulator RichTextProcessor::DisableItalic()
{
	return [](RichTextProcessor& rtp) { rtp.disableItalic(); };
}

RichTextProcessor::Manipulator RichTextProcessor::DisableUnderline()
{
	return [](RichTextProcessor& rtp) { rtp.disableUnderline(); };
}

RichTextProcessor::Manipulator RichTextProcessor::DisableStrikethrough()
{
	return [](RichTextProcessor& rtp) { rtp.disableStrikethrough(); };
}

RichTextProcessor& RichTextProcessor::operator<<(char c)
{
	pendingUtf8.push_back(c);
	return *this;
}

RichTextProcessor& RichTextProcessor::operator<<(const char* text)
{
	if (text) pendingUtf8.append(text);
	return *this;
}

RichTextProcessor& RichTextProcessor::operator<<(const std::string& text)
{
	pendingUtf8.append(text);
	return *this;
}

RichTextProcessor& RichTextProcessor::operator<<(char32_t codePoint)
{
	appendCodePointUtf8(pendingUtf8, codePoint);
	return *this;
}

RichTextProcessor& RichTextProcessor::operator<<(const Manipulator& manipulator)
{
	if (manipulator) manipulator(*this);
	return *this;
}

}
}
