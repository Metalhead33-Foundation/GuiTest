#ifndef LTECSSSTYLE_HPP
#define LTECSSSTYLE_HPP
/**
 * @file LteCssStyle.hpp
 * @brief Optional CSS styling for MML, backed by NetSurf's libcss.
 *
 * MML tags are attribute toggles, but they are also elements, and libcss is a
 * complete CSS parser + selector matcher + cascade. This file is the bridge:
 * MML's element stack becomes a node tree that libcss can select against, and the
 * computed styles are converted into the handful of attributes
 * @ref Lotte::Text::TextAttributes can express.
 *
 * @section lte_css_scope What actually works
 *
 * Supported, verified by `UnitTests/tests/CssStyleTests.cpp`:
 *
 * - **Parsing and cascade**: rules, selector lists, `!important`, specificity,
 *   inherited vs non-inherited properties, `@media screen`.
 * - **Selectors**: type (`b`), class (`.warn`), id (`#loud`), attribute selectors,
 *   descendant/child/sibling combinators, `:first-child`/`:nth-child`/`:not(...)`
 *   and the other tree-structural pseudo-classes. `:hover`/`:focus`/`:active`
 *   always report false, because rich text has no interaction state.
 * - **Properties that map to `TextAttributes`**: `color`, `font-family`,
 *   `font-size`, `font-weight` (numeric 100-900), `font-style`, `text-decoration`.
 *
 * Parsed but **without effect**, because Lotte has no layout engine and no other
 * text attributes: `margin`, `padding`, `border`, `background`, `width`/`height`,
 * `position`, `display`, `float`, `line-height`, `text-align`, `letter-spacing`,
 * `white-space`, `text-transform`, transforms, columns, flexbox, everything else.
 * libcss will happily parse and cascade them; the resolver simply does not read
 * them. Do not promise more than this to document authors.
 *
 * @section lte_css_units Units
 *
 * libcss resolves `em` against the composed parent style and keeps `pt`/`pc`/`in`/
 * `cm`/`mm` as-is; this bridge converts absolute units to pixels at
 * `CssMetrics::deviceDpi` and resolves `%`/`ex`/`ch` against the parent's size.
 * `font-size` keywords (`large`, `smaller`, ...) use the CSS scaling table.
 *
 * @section lte_css_threading Threading and lifetime
 *
 * libcss has process-global state (string internment, property strings) and is not
 * documented as thread-safe: use these classes from one thread. A
 * `CssStylesheet` must outlive every resolver using it; the resolver borrows the
 * processor and must die before it.
 *
 * @note Availability is a build-time question: the whole header is inert unless
 * `LTE_WITH_LIBCSS` is defined, which `Lotte.pro` does when `pkg-config` finds
 * libcss. See `Lotte/Text/MML.md` for the study behind this module.
 */
#include <Lotte/LotteLib.hpp>

#ifdef LTE_WITH_LIBCSS

#include <Lotte/Text/LteMml.hpp>
#include <Lotte/Text/LteRichText.hpp>
#include <Lotte/Text/LteTextAttributes.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

namespace Lotte {
namespace Text {

namespace detail {
/** @brief Internal bridge between the two pimpl types; not part of the public API. */
struct CssAccess;
}

/** @brief Metrics libcss needs to resolve relative units. */
struct CssMetrics {
	/** @brief Viewport width in CSS pixels, for `vw`/`vmin`/`vmax`. */
	float viewportWidth = 640.0f;
	/** @brief Viewport height in CSS pixels, for `vh`/`vmin`/`vmax`. */
	float viewportHeight = 480.0f;
	/** @brief Font size of the document root in pixels, i.e. what `medium` means. */
	float defaultFontSize = 16.0f;
	/** @brief Lower clamp applied to computed font sizes; zero disables it. */
	float minimumFontSize = 0.0f;
	/** @brief Device DPI used to convert `pt`/`pc`/`in`/`cm`/`mm` to pixels. */
	float deviceDpi = 96.0f;
};

/**
 * @brief A compiled stylesheet pair: MML's built-in element defaults plus an
 *        author stylesheet, ready for selector matching.
 *
 * The built-in sheet is registered as `CSS_ORIGIN_USER_AGENT`, so author rules win
 * the cascade: `b { font-weight: normal }` really does un-bold `<b>`.
 */
class MH_LOTTE_API CssStylesheet {
public:
	/**
	 * @brief Parses @p css and pairs it with the built-in MML element styles.
	 * @param css Author stylesheet text; may be empty.
	 * @param metrics Unit-resolution metrics.
	 * @throws std::runtime_error When libcss cannot allocate or rejects the
	 *         stylesheet outright (syntax errors are recovered from, not thrown).
	 */
	explicit CssStylesheet(std::string_view css, const CssMetrics& metrics = {});
	~CssStylesheet();

	CssStylesheet(const CssStylesheet&) = delete;
	CssStylesheet& operator=(const CssStylesheet&) = delete;
	CssStylesheet(CssStylesheet&&) = delete;
	CssStylesheet& operator=(CssStylesheet&&) = delete;

	/** @brief The metrics this sheet was built with. */
	const CssMetrics& metrics() const noexcept;
	/** @brief True when libcss reported a problem while parsing the author sheet. */
	bool hadParseProblem() const noexcept;

	/**
	 * @brief The built-in stylesheet that gives MML elements their usual meaning.
	 *
	 * Exposed so an application can display it (a style editor needs to know the
	 * baseline) or copy it. It is `b, strong { font-weight: bold }`,
	 * `i, em { font-style: italic }`, `u { text-decoration: underline }`,
	 * `s, del, strike { text-decoration: line-through }`, monospace for
	 * `code`/`tt`/`pre`, and `h1`-`h3` heading sizes.
	 */
	static std::string_view markupDefaults() noexcept;

private:
	friend struct detail::CssAccess;
	struct Impl;
	std::unique_ptr<Impl> impl;
};

/**
 * @brief Applies a @ref CssStylesheet to MML elements and drives a processor.
 *
 * Attach it with @ref Lotte::Text::MmlParser::setElementObserver; every element the
 * parser opens is composed against its parent's style, converted to
 * @ref Lotte::Text::TextAttributes and pushed into the processor, so text between
 * two tags picks up exactly the cascaded style.
 *
 * `text-decoration` is *propagated* the way browsers paint it: a `u` element
 * underlines its descendants' text too, even though CSS does not inherit the
 * property. `font-family` keeps the first named family, lowercased, because MML
 * font repositories use lowercase names; a generic family (`serif`, `monospace`)
 * leaves the previous family in place rather than inventing a name.
 *
 * @code
 * Lotte::Text::CssStylesheet sheet(cssText);
 * Lotte::Text::CssStyleResolver style(processor, sheet, baseAttributes);
 * Lotte::Text::MmlParser parser(&processor);
 * parser.setElementObserver(&style);
 * parser.parse(markup);
 * processor.flush();
 * @endcode
 */
class MH_LOTTE_API CssStyleResolver final : public MmlElementObserver {
public:
	/**
	 * @brief Binds a resolver to a processor.
	 * @param processor Processor that receives text and attribute changes.
	 * @param sheet Stylesheet to select against; must outlive the resolver.
	 * @param base Attributes used for the document root (colour, size, family).
	 */
	CssStyleResolver(RichTextProcessor& processor, const CssStylesheet& sheet,
			 TextAttributes base = {});
	~CssStyleResolver() override;

	CssStyleResolver(const CssStyleResolver&) = delete;
	CssStyleResolver& operator=(const CssStyleResolver&) = delete;
	CssStyleResolver(CssStyleResolver&&) = delete;
	CssStyleResolver& operator=(CssStyleResolver&&) = delete;

	void onElementOpen(std::string_view name, std::string_view classes,
			   std::string_view id) override;
	void onElementClose(std::string_view name) override;

	/** @brief Attributes currently pushed into the processor. */
	const TextAttributes& currentAttributes() const noexcept;
	/** @brief Number of open elements, i.e. the document depth. */
	std::size_t depth() const noexcept;
	/** @brief Name of the innermost open element, empty at the document root. */
	std::string_view currentElement() const noexcept;

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};

/**
 * @brief Parses @p mml with @p sheet and returns the styled runs.
 * @param mml MML document.
 * @param sheet Stylesheet to apply.
 * @param base Attributes used before any rule matches.
 * @return One run per distinct attribute set, in document order.
 */
MH_LOTTE_API TextRunList styleMarkup(std::string_view mml, const CssStylesheet& sheet,
				     const TextAttributes& base = {});

/**
 * @brief Everything the cascade computes for one element outside a document.
 *
 * This is the door a *theme* uses: a theme document carries a declaration block such as
 * `font-size: 18px; color: #ffd27f; padding: 6px; border-width: 3px`, the theme wraps it
 * in a universal rule, and this call answers with the attributes and box hints libcss
 * computed. No markup, no processor, no document tree.
 */
struct CssElementStyle {
	/** @brief Colour, family, size, weight, slant and decoration of the element's text. */
	TextAttributes text;
	/** @brief Computed padding in pixels, in CSS order: top, right, bottom, left. */
	float padding[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	/** @brief Computed border width in pixels, in CSS order: top, right, bottom, left. */
	float border[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	/** @brief Computed `background-color`; read it only when @ref hasBackground is set. */
	Colour background = Colour(0.0f, 0.0f, 0.0f, 0.0f);
	/** @brief True when the cascade gave `background-color` a value. */
	bool hasBackground = false;
};

/**
 * @brief Computes one element's style against a stylesheet, with no document.
 * @param sheet Stylesheet to match; its universal rules apply to the element.
 * @param element Element name to select, e.g. `label`, `button`, `body`.
 * @param classes Space-separated class list for the element; may be empty.
 * @param base Attributes of the parent, i.e. what the element inherits before any rule.
 * @return The composed attributes and box hints.
 * @note A failed selection is not an error: the result simply equals @p base, because a
 *       theme must still render when one of its declarations does not parse.
 */
MH_LOTTE_API CssElementStyle computeElementStyle(const CssStylesheet& sheet,
						 std::string_view element,
						 std::string_view classes = {},
						 const TextAttributes& base = {});


} // namespace Text
} // namespace Lotte

#endif // LTE_WITH_LIBCSS

#endif // LTECSSSTYLE_HPP
