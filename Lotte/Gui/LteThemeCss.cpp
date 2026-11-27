#include "LteThemeCss.hpp"
#include <algorithm>
#include <cstdio>
#include <exception>

#ifdef LTE_WITH_LIBCSS
#include <Lotte/Text/LteCssStyle.hpp>
#endif

namespace Lotte {
namespace Gui {

namespace {

/// @brief Copies the attributes libcss computed back into the theme's own style struct.
ThemeTextStyleDefinition toTextStyle(const Text::TextAttributes& attributes)
{
ThemeTextStyleDefinition style;
style.face = attributes.familyName;
style.weight = attributes.weight;
style.size = attributes.size;
style.colour = attributes.colour;
style.italic = attributes.italic;
style.underline = attributes.underline;
style.strikethrough = attributes.strikethrough;
return style;
}

#ifdef LTE_WITH_LIBCSS
/** @brief What the block starts from: the theme's explicit values. */
Text::TextAttributes toAttributes(const ThemeTextStyleDefinition& style)
{
	Text::TextAttributes attributes;
	attributes.familyName = style.face;
	attributes.size = style.size;
	attributes.colour = style.colour;
	attributes.weight = style.weight;
	attributes.italic = style.italic;
	attributes.underline = style.underline;
	attributes.strikethrough = style.strikethrough;
	return attributes;
}
#endif

#ifdef LTE_WITH_LIBCSS
/**
 * @brief Builds the `* { ... }` rule that carries the theme's explicit values.
 *
 * The block the document writes is appended *after* this rule, so CSS wins where it says
 * something and the JSON keeps its meaning everywhere else. Doing it this way (rather
 * than passing the base style as a parent) is what makes libcss's own initial values -
 * weight 400, size 16 px - unable to flatten a theme that asked for 700 at 32 px.
 */
std::string baseRuleFor(const ThemeTextStyleDefinition& style)
{
	const auto hex = [](float value) {
		const float clamped = std::clamp(value, 0.0f, 1.0f);
		const unsigned channel = unsigned(clamped * 255.0f + 0.5f);
		char text[3];
		std::snprintf(text, sizeof(text), "%02x", channel);
		return std::string(text);
	};
	char size[32];
	std::snprintf(size, sizeof(size), "%.4gpx", double(std::max(style.size, 1.0f)));
	std::string rule = "* {";
	rule += " font-size: ";
	rule += size;
	rule += "; font-weight: ";
	rule += std::to_string(unsigned(style.weight));
	rule += "; font-style: ";
	rule += style.italic ? "italic;" : "normal;";
	rule += " color: #" + hex(style.colour.r) + hex(style.colour.g) + hex(style.colour.b) + ";";
	if (!style.face.empty()) rule += " font-family: "" + style.face + "";";
	if (style.underline || style.strikethrough) {
		rule += " text-decoration:";
		if (style.underline) rule += " underline";
		if (style.strikethrough) rule += " line-through";
		rule += ";";
	}
	rule += " }";
	return rule;
}
#endif

} // namespace

ThemeCssStyle resolveThemeCss(std::string_view css, const ThemeTextStyleDefinition& base)
{
	ThemeCssStyle result;
	result.text = base;
	if (css.empty()) {
		result.problems.push_back("theme CSS: the declaration block is empty");
		return result;
	}
#ifdef LTE_WITH_LIBCSS
	try {
		// A universal rule is the smallest valid stylesheet that carries a bare
		// declaration block, and it matches the `label` element this call selects.
		// Explicit values first, the document's declarations second: the cascade then
		// decides, and nothing the JSON said is lost to libcss's initial values.
		const std::string sheetText = baseRuleFor(base) + "\n* { " + std::string(css) + " }";
		const Text::CssStylesheet sheet(sheetText, Text::CssMetrics{});
		if (sheet.hadParseProblem()) {
			result.problems.push_back("theme CSS: libcss reported a parse problem, using what parsed");
		}
		const Text::CssElementStyle computed =
			Text::computeElementStyle(sheet, "label", {}, toAttributes(base));
		result.text = toTextStyle(computed.text);
		// `ThemeMetrics` keeps one padding pair, so the four CSS sides collapse to the
		// horizontal and vertical averages; the border keeps a single width as well.
		result.metrics.padding =
			glm::fvec2((computed.padding[1] + computed.padding[3]) * 0.5f,
					   (computed.padding[0] + computed.padding[2]) * 0.5f);
		const float border =
			(computed.border[0] + computed.border[1] + computed.border[2] + computed.border[3]) *
			0.25f;
		result.metrics.borderWidth = border;
		if (border > 0.0f) {
			result.atlas.hasBorderWidth = true;
			result.atlas.borderWidth = border;
		}
		if (computed.hasBackground) {
			result.atlas.hasBackground = true;
			result.atlas.background =
				glm::fvec3(computed.background.r, computed.background.g, computed.background.b);
		}
		result.valid = true;
	} catch (const std::exception& error) {
		result.problems.push_back(std::string("theme CSS: ") + error.what());
	}
#else
	result.problems.push_back("theme CSS: this build has no libcss (LTE_WITH_LIBCSS is off)");
#endif
	return result;
}

std::size_t resolveThemeCssInPlace(ThemeDefinition& definition, std::vector<std::string>* problems)
{
	std::size_t applied = 0;
	const auto report = [problems](const std::vector<std::string>& lines) {
		if (problems == nullptr) return;
		problems->insert(problems->end(), lines.begin(), lines.end());
	};
	for (std::optional<ThemeElementDefinition>& slot : definition.elements) {
		if (!slot.has_value() || !slot->textStyle.has_value() || slot->textStyle->css.empty()) continue;
		const ThemeCssStyle resolved = resolveThemeCss(slot->textStyle->css, *slot->textStyle);
		report(resolved.problems);
		if (!resolved.valid) continue;
		slot->textStyle = resolved.text;
		if (resolved.metrics.padding != glm::fvec2(0.0f)) slot->metrics.padding = resolved.metrics.padding;
		if (resolved.metrics.borderWidth > 0.0f) slot->metrics.borderWidth = resolved.metrics.borderWidth;
		++applied;
	}
	for (std::optional<ThemeTextStyleDefinition>& role : definition.roles) {
		if (!role.has_value() || role->css.empty()) continue;
		const ThemeCssStyle resolved = resolveThemeCss(role->css, *role);
		report(resolved.problems);
		if (!resolved.valid) continue;
		role = resolved.text;
		++applied;
	}
	return applied;
}

} // namespace Gui
} // namespace Lotte
