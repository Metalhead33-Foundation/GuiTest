#include "LteCssStyle.hpp"

#ifdef LTE_WITH_LIBCSS

// libcss is a C library whose public headers are not C++-clean: `computed.h` uses
// the `restrict` keyword and `css_unit_ctx` has a const member. Both quirks are
// confined to this translation unit, and `LteCssStyle.hpp` exposes no libcss type.
#if !defined(restrict)
#define restrict __restrict__
#endif
extern "C" {
#include <libcss/libcss.h>
#include <libcss/fpmath.h>
}
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Lotte {
namespace Text {

namespace {

/** @brief Style reset plus the element defaults MML tags have always had. */
constexpr const char* markupDefaultsCss =
	"b, strong { font-weight: bold; }\n"
	"i, em { font-style: italic; }\n"
	"u { text-decoration: underline; }\n"
	"s, del, strike { text-decoration: line-through; }\n"
	"code, tt, pre, kbd, samp { font-family: monospace; }\n"
	"h1 { font-size: 2em; font-weight: bold; }\n"
	"h2 { font-size: 1.5em; font-weight: bold; }\n"
	"h3 { font-size: 1.17em; font-weight: bold; }\n"
	"h4, h5, h6 { font-size: 1em; font-weight: bold; }\n"
	"small { font-size: 0.85em; }\n"
	"sub, sup { font-size: 0.75em; }\n";

/** @brief One MML element as libcss sees it: a node in a tree. */
struct CssNode {
	std::string name;
	std::string classesText;
	std::string idText;
	CssNode* parent = nullptr;
	std::vector<CssNode*> children;
	/** @brief Interned copies handed to libcss, owned here. */
	lwc_string* nameString = nullptr;
	std::vector<lwc_string*> classStrings;
	lwc_string* idString = nullptr;
	/** @brief Result set of the selection; owns the specified style. */
	css_select_results* results = nullptr;
	/** @brief Inheritance-resolved style, or null when `specified` is already final. */
	css_computed_style* composed = nullptr;
	/** @brief Attributes this element's text runs use. */
	TextAttributes attributes;
	/**
	 * @brief Arrays handed to libcss, one per `node_classes` call.
	 *
	 * libcss may release the references it receives (it does not always, depending
	 * on which selectors key on the classes), so every call gets a fresh array of
	 * fresh references and the node keeps its own separately.
	 */
	mutable std::vector<std::vector<lwc_string*> > classHandouts;

	CssNode() = default;
	~CssNode();
	CssNode(const CssNode&) = delete;
	CssNode& operator=(const CssNode&) = delete;

	/** @brief The style children compose against: the resolved one when present. */
	const css_computed_style* effectiveStyle() const noexcept;
};

CssNode::~CssNode()
{
	if (composed != nullptr) css_computed_style_destroy(composed);
	if (results != nullptr) css_select_results_destroy(results);
	// libcss refs the strings a handler returns for as long as it keeps them, and
	// releases those refs before the selection call returns, so the node's own
	// references are the only ones left here.
	if (nameString != nullptr) lwc_string_unref(nameString);
	if (idString != nullptr) lwc_string_unref(idString);
	for (lwc_string* cls : classStrings) lwc_string_unref(cls);
}

const css_computed_style* CssNode::effectiveStyle() const noexcept
{
	if (composed != nullptr) return composed;
	if (results != nullptr) return results->styles[CSS_PSEUDO_ELEMENT_NONE];
	return nullptr;
}

/** @brief Interns a lowercase copy of @p text, or returns null for empty text. */
lwc_string* internText(std::string_view text)
{
	if (text.empty()) return nullptr;
	std::string lowered;
	lowered.reserve(text.size());
	for (char c : text) lowered.push_back(char(std::tolower(static_cast<unsigned char>(c))));
	lwc_string* interned = nullptr;
	if (lwc_intern_string(lowered.data(), lowered.size(), &interned) != lwc_error_ok) {
		return nullptr;
	}
	return interned;
}

/** @brief Splits a space-separated class list into interned strings. */
void internClasses(std::string_view classes, std::vector<lwc_string*>& out)
{
	std::size_t index = 0;
	while (index < classes.size()) {
		while (index < classes.size() && std::isspace(static_cast<unsigned char>(classes[index]))) ++index;
		const std::size_t begin = index;
		while (index < classes.size() && !std::isspace(static_cast<unsigned char>(classes[index]))) ++index;
		if (index > begin) {
			if (lwc_string* cls = internText(classes.substr(begin, index - begin))) out.push_back(cls);
		}
	}
}

/** @brief Converts a CSS length to pixels using the document metrics. */
float lengthToPixels(css_fixed value, css_unit unit, float parentSize,
		     const CssMetrics& metrics)
{
	const float raw = FIXTOFLT(value);
	const float dpi = metrics.deviceDpi > 0.0f ? metrics.deviceDpi : 96.0f;
	switch (unit) {
		case CSS_UNIT_PX: return raw;
		case CSS_UNIT_IN: return raw * dpi;
		case CSS_UNIT_CM: return raw * dpi / 2.54f;
		case CSS_UNIT_MM: return raw * dpi / 25.4f;
		case CSS_UNIT_Q: return raw * dpi / 101.6f;
		case CSS_UNIT_PT: return raw * dpi / 72.0f;
		case CSS_UNIT_PC: return raw * dpi / 6.0f;
		case CSS_UNIT_EM: return raw * parentSize;
		// Without font metrics, `ex`/`ch` use the fallback the CSS specification
		// suggests for fonts that cannot be measured.
		case CSS_UNIT_EX:
		case CSS_UNIT_CH: return raw * parentSize * 0.5f;
		case CSS_UNIT_PCT: return raw * parentSize / 100.0f;
		case CSS_UNIT_REM: return raw * metrics.defaultFontSize;
		case CSS_UNIT_LH: return raw * parentSize * 1.2f;
		case CSS_UNIT_VW: return raw * metrics.viewportWidth / 100.0f;
		case CSS_UNIT_VH: return raw * metrics.viewportHeight / 100.0f;
		case CSS_UNIT_VI: return raw * metrics.viewportWidth / 100.0f;
		case CSS_UNIT_VB: return raw * metrics.viewportHeight / 100.0f;
		case CSS_UNIT_VMIN:
			return raw * std::min(metrics.viewportWidth, metrics.viewportHeight) / 100.0f;
		case CSS_UNIT_VMAX:
			return raw * std::max(metrics.viewportWidth, metrics.viewportHeight) / 100.0f;
		default: return raw;
	}
}

/** @brief Scale factor of a `font-size` keyword, matching libcss's own table. */
float fontSizeKeywordScale(uint8_t status)
{
	switch (status) {
		case CSS_FONT_SIZE_XX_SMALL: return 0.5625f;
		case CSS_FONT_SIZE_X_SMALL: return 0.75f;
		case CSS_FONT_SIZE_SMALL: return 0.875f;
		case CSS_FONT_SIZE_MEDIUM: return 1.0f;
		case CSS_FONT_SIZE_LARGE: return 1.125f;
		case CSS_FONT_SIZE_X_LARGE: return 1.5f;
		case CSS_FONT_SIZE_XX_LARGE: return 2.0f;
		case CSS_FONT_SIZE_LARGER: return 1.2f;
		case CSS_FONT_SIZE_SMALLER: return 1.0f / 1.2f;
		default: return 1.0f;
	}
}

/** @brief Converts a computed CSS font weight to MML's numeric weight. */
FontWeight fontWeightFromCss(uint8_t weight, FontWeight parent)
{
	switch (weight) {
		case CSS_FONT_WEIGHT_NORMAL: return FontWeight::Normal;
		case CSS_FONT_WEIGHT_BOLD: return FontWeight::Bold;
		case CSS_FONT_WEIGHT_BOLDER: return fontWeightBolder(parent);
		case CSS_FONT_WEIGHT_LIGHTER: return fontWeightLighter(parent);
		case CSS_FONT_WEIGHT_100: return FontWeight::Thin;
		case CSS_FONT_WEIGHT_200: return FontWeight::ExtraLight;
		case CSS_FONT_WEIGHT_300: return FontWeight::Light;
		case CSS_FONT_WEIGHT_400: return FontWeight::Normal;
		case CSS_FONT_WEIGHT_500: return FontWeight::Medium;
		case CSS_FONT_WEIGHT_600: return FontWeight::SemiBold;
		case CSS_FONT_WEIGHT_700: return FontWeight::Bold;
		case CSS_FONT_WEIGHT_800: return FontWeight::ExtraBold;
		case CSS_FONT_WEIGHT_900: return FontWeight::Black;
		case CSS_FONT_WEIGHT_INHERIT:
		default: return parent;
	}
}

/**
 * @brief Converts a computed style into MML text attributes.
 * @param style Inheritance-resolved style of the element.
 * @param parent Attributes of the parent element (or the caller's base).
 * @param metrics Unit-resolution metrics.
 *
 * Only the properties MML can express are read; everything else libcss computed is
 * left on the floor, which is what keeps the promise in the header honest.
 */
TextAttributes composeAttributes(const css_computed_style* style, const TextAttributes& parent,
				 const CssMetrics& metrics)
{
	TextAttributes result = parent;
	if (style == nullptr) return result;

	css_color colour = 0;
	if (css_computed_color(style, &colour) == CSS_COLOR_COLOR) {
		result.colour = Colour(float((colour >> 16) & 0xFFu) / 255.0f,
				       float((colour >> 8) & 0xFFu) / 255.0f,
				       float(colour & 0xFFu) / 255.0f,
				       float((colour >> 24) & 0xFFu) / 255.0f);
	}

	const uint8_t weight = css_computed_font_weight(style);
	result.weight = fontWeightFromCss(weight, parent.effectiveWeight());
	result.bold = fontWeightIsBold(result.weight);

	const uint8_t slant = css_computed_font_style(style);
	if (slant == CSS_FONT_STYLE_ITALIC || slant == CSS_FONT_STYLE_OBLIQUE) result.italic = true;
	else if (slant == CSS_FONT_STYLE_NORMAL) result.italic = false;

	// text-decoration is not inherited, but browsers paint an ancestor's decoration
	// across descendant boxes, so it can only ever be added here.
	const uint8_t decoration = css_computed_text_decoration(style);
	if (decoration != CSS_TEXT_DECORATION_INHERIT) {
		if ((decoration & CSS_TEXT_DECORATION_UNDERLINE) != 0) result.underline = true;
		if ((decoration & CSS_TEXT_DECORATION_LINE_THROUGH) != 0) result.strikethrough = true;
	}

	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	const uint8_t sizeStatus = css_computed_font_size(style, &length, &unit);
	if (sizeStatus == CSS_FONT_SIZE_DIMENSION) {
		result.size = lengthToPixels(length, unit, parent.size, metrics);
	} else if (sizeStatus != CSS_FONT_SIZE_INHERIT) {
		result.size = parent.size * fontSizeKeywordScale(sizeStatus);
	}
	if (metrics.minimumFontSize > 0.0f) result.size = std::max(result.size, metrics.minimumFontSize);

	// A named family becomes the MML face name. A generic family (`serif`,
	// `monospace`, ...) has no MML face behind it, so the previous name stays.
	lwc_string** families = nullptr;
	css_computed_font_family(style, &families);
	if (families != nullptr && families[0] != nullptr) {
		const char* data = lwc_string_data(families[0]);
		const std::size_t count = lwc_string_length(families[0]);
		std::string name;
		name.reserve(count);
		for (std::size_t i = 0; i < count; ++i) {
			name.push_back(char(std::tolower(static_cast<unsigned char>(data[i]))));
		}
		if (!name.empty()) result.familyName = std::move(name);
	}
	return result;
}

/** @brief Client data of one selection pass. */
struct SelectContext {
	/** @brief Colour reported as the user-agent default, i.e. the caller's base. */
	css_color baseColour = 0xFF000000u;
};

/** @brief Packs an RGBA colour into libcss's `css_color` (`0xAARRGGBB`). */
css_color packColour(const Colour& colour) noexcept
{
	const auto byte = [](float value) {
		const float clamped = std::clamp(value, 0.0f, 1.0f);
		return std::uint32_t(clamped * 255.0f + 0.5f) & 0xFFu;
	};
	return (byte(colour.a) << 24) | (byte(colour.r) << 16) | (byte(colour.g) << 8) | byte(colour.b);
}

css_error hNodeName(void*, void* node, css_qname* qname)
{
	lwc_string* name = static_cast<CssNode*>(node)->nameString;
	qname->name = name != nullptr ? lwc_string_ref(name) : nullptr;
	qname->ns = nullptr;
	return CSS_OK;
}

css_error hNodeClasses(void*, void* node, lwc_string*** classes, uint32_t* count)
{
	CssNode* element = static_cast<CssNode*>(node);
	*classes = nullptr;
	*count = 0;
	if (element->classStrings.empty()) return CSS_OK;

	std::vector<lwc_string*> handout;
	handout.reserve(element->classStrings.size());
	for (lwc_string* cls : element->classStrings) handout.push_back(lwc_string_ref(cls));
	element->classHandouts.push_back(std::move(handout));
	*classes = element->classHandouts.back().data();
	*count = static_cast<uint32_t>(element->classStrings.size());
	return CSS_OK;
}

css_error hNodeId(void*, void* node, lwc_string** id)
{
	lwc_string* value = static_cast<CssNode*>(node)->idString;
	*id = value != nullptr ? lwc_string_ref(value) : nullptr;
	return CSS_OK;
}

css_error hParent(void*, void* node, void** parent)
{
	*parent = static_cast<CssNode*>(node)->parent;
	return CSS_OK;
}

css_error hSibling(void*, void* node, void** sibling)
{
	CssNode* element = static_cast<CssNode*>(node);
	*sibling = nullptr;
	if (element->parent == nullptr) return CSS_OK;
	const auto& siblings = element->parent->children;
	for (std::size_t i = 1; i < siblings.size(); ++i) {
		if (siblings[i] == element) {
			*sibling = siblings[i - 1];
			return CSS_OK;
		}
	}
	return CSS_OK;
}

css_error hNamedNode(void*, void*, const css_qname*, void** out)
{
	*out = nullptr;
	return CSS_OK;
}

css_error hHasName(void*, void* node, const css_qname* qname, bool* match)
{
	*match = false;
	lwc_string* name = static_cast<CssNode*>(node)->nameString;
	if (name != nullptr && qname->name != nullptr) {
		lwc_string_caseless_isequal(name, qname->name, match);
	}
	return CSS_OK;
}

css_error hHasClass(void*, void* node, lwc_string* name, bool* match)
{
	*match = false;
	for (lwc_string* cls : static_cast<CssNode*>(node)->classStrings) {
		bool equal = false;
		if (lwc_string_caseless_isequal(cls, name, &equal) == lwc_error_ok && equal) {
			*match = true;
			return CSS_OK;
		}
	}
	return CSS_OK;
}

css_error hHasId(void*, void* node, lwc_string* name, bool* match)
{
	*match = false;
	lwc_string* id = static_cast<CssNode*>(node)->idString;
	if (id != nullptr) lwc_string_caseless_isequal(id, name, match);
	return CSS_OK;
}

css_error hHasAttribute(void*, void*, const css_qname*, bool* match)
{
	*match = false;
	return CSS_OK;
}

css_error hHasAttributeValue(void*, void*, const css_qname*, lwc_string*, bool* match)
{
	*match = false;
	return CSS_OK;
}

css_error hIsRoot(void*, void* node, bool* match)
{
	*match = static_cast<CssNode*>(node)->parent == nullptr;
	return CSS_OK;
}

css_error hCountSiblings(void*, void* node, bool, bool after, int32_t* count)
{
	CssNode* element = static_cast<CssNode*>(node);
	*count = 0;
	if (element->parent == nullptr) return CSS_OK;
	int32_t seen = 0;
	bool reached = false;
	for (CssNode* sibling : element->parent->children) {
		if (sibling == element) {
			reached = true;
			continue;
		}
		if (after) {
			if (reached) ++seen;
		} else if (!reached) {
			++seen;
		}
	}
	*count = seen;
	return CSS_OK;
}

css_error hIsEmpty(void*, void* node, bool* match)
{
	*match = static_cast<CssNode*>(node)->children.empty();
	return CSS_OK;
}

css_error hFalseFlag(void*, void*, bool* match)
{
	*match = false;
	return CSS_OK;
}

css_error hIsLang(void*, void*, lwc_string*, bool* match)
{
	*match = false;
	return CSS_OK;
}

css_error hPresentationalHint(void*, void*, uint32_t* count, css_hint** hints)
{
	*count = 0;
	*hints = nullptr;
	return CSS_OK;
}

css_error hUaDefault(void* pw, uint32_t property, css_hint* hint)
{
	const SelectContext* context = static_cast<const SelectContext*>(pw);
	if (property == CSS_PROP_COLOR && context != nullptr) {
		hint->data.color = context->baseColour;
		hint->status = CSS_COLOR_COLOR;
		return CSS_OK;
	}
	if (property == CSS_PROP_FONT_FAMILY) {
		// `CSS_PROPERTY_NOT_SET` is *not* safe here: libcss's `initial_*` helpers feed
		// the hint straight into the property setter, which then reads
		// `data.strings` as a real family array. A generic family is the honest answer
		// anyway - MML resolves generic families through the caller's base face.
		hint->data.strings = nullptr;
		hint->status = CSS_FONT_FAMILY_SANS_SERIF;
		return CSS_OK;
	}
	if (property == CSS_PROP_QUOTES) {
		// Same trap: with the pointer null, `CSS_QUOTES_NONE` is "no quotation marks",
		// which is what a rich-text document wants anyway.
		hint->data.strings = nullptr;
		hint->status = CSS_QUOTES_NONE;
		return CSS_OK;
	}
	hint->status = CSS_PROPERTY_NOT_SET;
	return CSS_OK;
}

css_error hSetNodeData(void*, void*, void*)
{
	return CSS_OK;
}

css_error hGetNodeData(void*, void*, void** data)
{
	*data = nullptr;
	return CSS_OK;
}

/**
 * @brief Fills the selector handler vtable.
 *
 * Every slot is populated: libcss calls several of them without a null check, so a
 * partial vtable segfaults inside `css_select_style()`.
 */
css_select_handler makeHandler() noexcept
{
	css_select_handler handler{};
	handler.handler_version = CSS_SELECT_HANDLER_VERSION_1;
	handler.node_name = hNodeName;
	handler.node_classes = hNodeClasses;
	handler.node_id = hNodeId;
	handler.named_ancestor_node = hNamedNode;
	handler.named_parent_node = hNamedNode;
	handler.named_sibling_node = hNamedNode;
	handler.named_generic_sibling_node = hNamedNode;
	handler.parent_node = hParent;
	handler.sibling_node = hSibling;
	handler.node_has_name = hHasName;
	handler.node_has_class = hHasClass;
	handler.node_has_id = hHasId;
	handler.node_has_attribute = hHasAttribute;
	handler.node_has_attribute_equal = hHasAttributeValue;
	handler.node_has_attribute_dashmatch = hHasAttributeValue;
	handler.node_has_attribute_includes = hHasAttributeValue;
	handler.node_has_attribute_prefix = hHasAttributeValue;
	handler.node_has_attribute_suffix = hHasAttributeValue;
	handler.node_has_attribute_substring = hHasAttributeValue;
	handler.node_is_root = hIsRoot;
	handler.node_count_siblings = hCountSiblings;
	handler.node_is_empty = hIsEmpty;
	handler.node_is_link = hFalseFlag;
	handler.node_is_visited = hFalseFlag;
	handler.node_is_hover = hFalseFlag;
	handler.node_is_active = hFalseFlag;
	handler.node_is_focus = hFalseFlag;
	handler.node_is_enabled = hFalseFlag;
	handler.node_is_disabled = hFalseFlag;
	handler.node_is_checked = hFalseFlag;
	handler.node_is_target = hFalseFlag;
	handler.node_is_lang = hIsLang;
	handler.node_presentational_hint = hPresentationalHint;
	handler.ua_default_for_property = hUaDefault;
	handler.set_libcss_node_data = hSetNodeData;
	handler.get_libcss_node_data = hGetNodeData;
	return handler;
}

/** @brief URL resolver libcss insists on having, even for in-memory sheets. */
css_error resolveUrl(void*, const char*, lwc_string* relative, lwc_string** absolute)
{
	*absolute = lwc_string_ref(relative);
	return CSS_OK;
}

/** @brief Compiles one in-memory stylesheet, reporting parse problems. */
css_stylesheet* compileSheet(const char* css, std::size_t length, const char* url, bool& problem)
{
	css_stylesheet_params params{};
	params.params_version = CSS_STYLESHEET_PARAMS_VERSION_1;
	params.level = CSS_LEVEL_DEFAULT;
	params.charset = "UTF-8";
	params.url = url;
	params.title = "";
	params.allow_quirks = false;
	params.inline_style = false;
	// libcss answers CSS_BADPARM when `resolve` is null, even for a sheet that can
	// never import anything, so the pass-through resolver above is mandatory.
	params.resolve = resolveUrl;
	params.resolve_pw = nullptr;

	css_stylesheet* sheet = nullptr;
	if (css_stylesheet_create(&params, &sheet) != CSS_OK) {
		throw std::runtime_error("Lotte::Text::CssStylesheet: libcss could not create a stylesheet");
	}
	const css_error appended = css_stylesheet_append_data(sheet,
							      reinterpret_cast<const uint8_t*>(css),
							      length);
	// CSS_NEEDDATA means "want more input"; anything else is a real problem.
	if (appended != CSS_OK && appended != CSS_NEEDDATA) problem = true;
	if (css_stylesheet_data_done(sheet) != CSS_OK) problem = true;
	return sheet;
}

} // namespace

struct CssStylesheet::Impl {
	CssMetrics metrics;
	css_stylesheet* markupSheet = nullptr;
	css_stylesheet* authorSheet = nullptr;
	css_select_ctx* selectContext = nullptr;
	css_media media{};
	/** @brief `css_unit_ctx` has a const member, so it is aggregate-initialised once. */
	css_unit_ctx units;
	bool parseProblem = false;

	Impl(const CssMetrics& values, std::string_view authorCss);
	~Impl();
	Impl(const Impl&) = delete;
	Impl& operator=(const Impl&) = delete;

	/** @brief Selects the specified style of one element. */
	css_error select(CssNode* node, css_select_handler& handler, void* pw,
			 const css_unit_ctx* unitContext, css_select_results** out) noexcept
	{
		return css_select_style(selectContext, node, unitContext, &media, nullptr, &handler, pw,
					out);
	}
	/** @brief Resolves inheritance by composing a child against its parent. */
	css_error compose(const css_computed_style* parent, const css_computed_style* child,
			  const css_unit_ctx* unitContext, css_computed_style** out) noexcept
	{
		return css_computed_style_compose(parent, child, unitContext, out);
	}
	/**
	 * @brief Unit context for a document whose root font size is @p rootFontSize.
	 *
	 * The base attributes are per document, so `medium` has to mean *their* size,
	 * which is why this is not just a copy of the sheet's context.
	 */
	css_unit_ctx unitsFor(float rootFontSize) const noexcept
	{
		const float root = rootFontSize > 0.0f ? rootFontSize : metrics.defaultFontSize;
		return css_unit_ctx{ FLTTOFIX(metrics.viewportWidth), FLTTOFIX(metrics.viewportHeight),
				     FLTTOFIX(root), FLTTOFIX(metrics.minimumFontSize),
				     FLTTOFIX(metrics.deviceDpi), nullptr, nullptr, nullptr };
	}
};

CssStylesheet::Impl::Impl(const CssMetrics& values, std::string_view authorCss)
	: metrics(values),
	  units{ FLTTOFIX(values.viewportWidth), FLTTOFIX(values.viewportHeight),
		 FLTTOFIX(values.defaultFontSize), FLTTOFIX(values.minimumFontSize),
		 FLTTOFIX(values.deviceDpi), nullptr, nullptr, nullptr }
{
	media.type = CSS_MEDIA_SCREEN;
	media.width = FLTTOFIX(values.viewportWidth);
	media.height = FLTTOFIX(values.viewportHeight);

	markupSheet = compileSheet(markupDefaultsCss, std::strlen(markupDefaultsCss),
				   "lotte:mml-defaults.css", parseProblem);
	authorSheet = compileSheet(authorCss.data(), authorCss.size(), "lotte:author.css", parseProblem);

	if (css_select_ctx_create(&selectContext) != CSS_OK) {
		throw std::runtime_error("Lotte::Text::CssStylesheet: libcss could not create a selection context");
	}
	// The markup defaults are a user-agent sheet, so every author rule outranks
	// them: `b { font-weight: normal }` really does un-bold `<b>`.
	if (css_select_ctx_append_sheet(selectContext, markupSheet, CSS_ORIGIN_UA, nullptr) !=
		    CSS_OK ||
	    css_select_ctx_append_sheet(selectContext, authorSheet, CSS_ORIGIN_AUTHOR, nullptr) != CSS_OK) {
		throw std::runtime_error("Lotte::Text::CssStylesheet: libcss rejected a stylesheet");
	}
}

CssStylesheet::Impl::~Impl()
{
	if (selectContext != nullptr) css_select_ctx_destroy(selectContext);
	if (authorSheet != nullptr) css_stylesheet_destroy(authorSheet);
	if (markupSheet != nullptr) css_stylesheet_destroy(markupSheet);
}

CssStylesheet::CssStylesheet(std::string_view css, const CssMetrics& metrics)
	: impl(std::make_unique<Impl>(metrics, css))
{
}

CssStylesheet::~CssStylesheet() = default;

const CssMetrics& CssStylesheet::metrics() const noexcept
{
	return impl->metrics;
}

bool CssStylesheet::hadParseProblem() const noexcept
{
	return impl->parseProblem;
}

std::string_view CssStylesheet::markupDefaults() noexcept
{
	return markupDefaultsCss;
}

namespace detail {

/** @brief Reaches into `CssStylesheet`'s pimpl without exposing libcss types. */
struct CssAccess {
	static CssStylesheet::Impl& engine(const CssStylesheet& sheet) noexcept { return *sheet.impl; }
};

} // namespace detail

struct CssStyleResolver::Impl {
	RichTextProcessor* processor = nullptr;
	const CssStylesheet* sheet = nullptr;
	TextAttributes base;
	std::vector<std::unique_ptr<CssNode> > stack;
	css_select_handler handler = makeHandler();
	SelectContext selectContext;
	/** @brief Selection units, with the caller's base size as `medium`. */
	css_unit_ctx units;

	Impl(RichTextProcessor& target, const CssStylesheet& styles, TextAttributes attributes,
	     const CssMetrics& metrics)
		: processor(&target), sheet(&styles), base(std::move(attributes)),
		  units{ FLTTOFIX(metrics.viewportWidth), FLTTOFIX(metrics.viewportHeight),
			 FLTTOFIX(base.size > 0.0f ? base.size : metrics.defaultFontSize),
			 FLTTOFIX(metrics.minimumFontSize), FLTTOFIX(metrics.deviceDpi), nullptr, nullptr,
			 nullptr }
	{
		selectContext.baseColour = packColour(base.colour);
	}

	CssNode* top() const noexcept { return stack.empty() ? nullptr : stack.back().get(); }
	/** @brief Attributes the next text byte will use. */
	const TextAttributes& current() const noexcept
	{
		return stack.empty() ? base : stack.back()->attributes;
	}
	void apply() { processor->setCurrentAttributes(current()); }
	void open(std::string_view name, std::string_view classes, std::string_view id);
	void close(std::string_view name);
};

void CssStyleResolver::Impl::open(std::string_view name, std::string_view classes,
				  std::string_view id)
{
	auto node = std::make_unique<CssNode>();
	node->name.assign(name);
	node->classesText.assign(classes);
	node->idText.assign(id);
	node->parent = top();
	node->nameString = internText(node->name);
	node->idString = internText(node->idText);
	internClasses(node->classesText, node->classStrings);

	CssNode* element = node.get();
	if (element->parent != nullptr) element->parent->children.push_back(element);

	// The accessor returns the private engine type, so take it by `auto&`: naming
	// `CssStylesheet::Impl` here would require the access rights `CssAccess` has.
	auto& engine = detail::CssAccess::engine(*sheet);
	css_select_results* results = nullptr;
	const css_error selectStatus = engine.select(element, handler, &selectContext, &units, &results);
	if (selectStatus == CSS_OK && results != nullptr) {
		const css_computed_style* specified = results->styles[CSS_PSEUDO_ELEMENT_NONE];
		if (element->parent != nullptr) {
			const css_computed_style* parentStyle = element->parent->effectiveStyle();
			if (parentStyle == nullptr ||
			    engine.compose(parentStyle, specified, &units, &element->composed) != CSS_OK) {
				element->composed = nullptr;
			}
		}
		element->results = results;
	}

	// A failed or empty selection simply inherits: styling is best-effort, and a
	// document must still render when a rule cannot be matched.
	element->attributes = composeAttributes(element->effectiveStyle(), current(),
						engine.metrics);
	stack.push_back(std::move(node));
	apply();
}

void CssStyleResolver::Impl::close(std::string_view name)
{
	std::size_t index = stack.size();
	while (index > 0) {
		--index;
		if (stack[index]->name == name) {
			// MML is not strictly nested, so closing an outer element also closes
			// everything that was opened inside it.
			stack.resize(index);
			apply();
			return;
		}
	}
}

CssStyleResolver::CssStyleResolver(RichTextProcessor& processor, const CssStylesheet& sheet,
				   TextAttributes base)
	: impl(std::make_unique<Impl>(processor, sheet, std::move(base),
				      detail::CssAccess::engine(sheet).metrics))
{
}

CssStyleResolver::~CssStyleResolver() = default;

void CssStyleResolver::onElementOpen(std::string_view name, std::string_view classes,
				     std::string_view id)
{
	impl->open(name, classes, id);
}

void CssStyleResolver::onElementClose(std::string_view name)
{
	impl->close(name);
}

const TextAttributes& CssStyleResolver::currentAttributes() const noexcept
{
	return impl->current();
}

std::size_t CssStyleResolver::depth() const noexcept
{
	return impl->stack.size();
}

std::string_view CssStyleResolver::currentElement() const noexcept
{
	const CssNode* node = impl->top();
	return node != nullptr ? std::string_view(node->name) : std::string_view{};
}

TextRunList styleMarkup(std::string_view mml, const CssStylesheet& sheet, const TextAttributes& base)
{
	RichTextProcessor processor(base);
	CssStyleResolver resolver(processor, sheet, base);
	MmlParser parser(&processor);
	parser.setElementObserver(&resolver);
	parser.parse(mml);
	processor.flush();
	return processor.getRuns();
}

} // namespace Text
} // namespace Lotte

#endif // LTE_WITH_LIBCSS
