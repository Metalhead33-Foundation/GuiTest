#ifndef LTETHEME_HPP
#define LTETHEME_HPP
/**
 * @file LteTheme.hpp
 * @brief Abstract theme contract for windows, panels, controls and text styling.
 *
 * There was no theme abstraction in `GuiTest-legacy2`; widgets talked to a texture
 * and a UV table directly. This header introduces the interface only. It exists so
 * concrete widgets can ask a theme for a skin and metrics without depending on a
 * particular atlas layout, and so future theme files can be swapped at runtime.
 *
 * The contract is intentionally descriptor-based:
 *
 * - @ref ThemeSkin identifies a texture region, a tint and optional nine-slice
 *   margins. A window background, a window corner, a button face and a check mark
 *   are all just skins; the widget decides how to compose them.
 * - @ref ThemeSlice plus @ref ThemeSkin::sourceSize describe the source rectangle
 *   needed for nine-slice scaling, which is how fixed-size window corners survive
 *   a resized window.
 * - @ref ThemeMetrics carries padding, spacing, minimum size, border width and
 *   corner radius. Layout algorithms and widgets can consume these without knowing
 *   whether the theme is bitmap-based, vector-based or procedural.
 * - @ref Theme::textStyle gives the rich-text attributes a label/heading should use.
 *
 * No concrete theme is supplied yet. A missing skin is represented by
 * `std::nullopt`, which lets a widget fall back to its own default or to a
 * parent theme.
 */
#include <Lotte/LotteLib.hpp>
#include <Lotte/Gui/LteGuiRenderer.hpp>
#include <Lotte/Gui/LteWidget.hpp>
#include <Lotte/Text/LteTextAttributes.hpp>
#include <glm/glm.hpp>
#include <cstdint>
#include <optional>
#include <string_view>

namespace Lotte {
namespace Gui {

/** @brief A themeable GUI part. `Count` is not a valid part. */
enum class ThemeElement : std::uint8_t {
	WindowBackground,       /**< Client-area background of a window. */
	WindowBorder,           /**< Window frame / edge. */
	WindowCornerTopLeft,    /**< Top-left window corner ornament. */
	WindowCornerTopRight,   /**< Top-right window corner ornament. */
	WindowCornerBottomLeft, /**< Bottom-left window corner ornament. */
	WindowCornerBottomRight, /**< Bottom-right window corner ornament. */
	Panel,                  /**< Generic inset/raised panel. */
	Button,                 /**< Generic button fallback. */
	PushButton,             /**< Push button. */
	ToolButton,             /**< Toolbar button. */
	CheckBox,               /**< Check box glyph and frame. */
	TickBox,                /**< Tick box glyph and frame. */
	RadioButton,            /**< Radio-button glyph and frame. */
	TextBox,                /**< Editable text field frame. */
	SpinBox,                /**< Numeric spin box. */
	ComboBox,               /**< Drop-down combo box. */
	SliderTrack,            /**< Slider groove. */
	SliderThumb,            /**< Slider handle. */
	Dial,                   /**< Rotary dial. */
	ProgressTrack,          /**< Progress-bar groove. */
	ProgressFill,           /**< Progress-bar filled portion. */
	ScrollTrack,            /**< Scroll-bar groove. */
	ScrollThumb,            /**< Scroll-bar handle. */
	ScrollArea,             /**< Scroll-area viewport/background. */
	TabBar,                 /**< Tab strip. */
	TabPage,                /**< Tab page/container. */
	TabWidget,              /**< Full tab control (bar + pages). */
	TableView,              /**< Table body. */
	TableHeader,            /**< Table column header. */
	WindowTitleBar,         /**< Window title bar. */
	ToolTip,                /**< Tooltip background/frame. */
	Count                   /**< Number of elements; not a valid part. */
};

/** @brief Semantic text role a theme can style independently. */
enum class ThemeTextRole : std::uint8_t {
	Body,        /**< Normal paragraph text. */
	Label,       /**< Control label. */
	Heading,     /**< Section or window heading. */
	WindowTitle, /**< Window title bar text. */
	Placeholder, /**< Empty input-field hint. */
	ToolTip,     /**< Tooltip text. */
	Count        /**< Number of roles; not a valid role. */
};

/**
 * @brief Nine-slice source margins, in source pixels.
 *
 * The margins name the fixed left/top/right/bottom bands of the source rectangle;
 * the middle band stretches. All-zero means "draw as a simple quad".
 */
struct ThemeSlice {
	/** @brief Fixed left band width. */
	float left = 0.0f;
	/** @brief Fixed top band height. */
	float top = 0.0f;
	/** @brief Fixed right band width. */
	float right = 0.0f;
	/** @brief Fixed bottom band height. */
	float bottom = 0.0f;
};

/** @brief One texture region a widget can draw, optionally nine-sliced. */
struct ThemeSkin {
	/** @brief Texture and sampler containing the region. */
	GuiTextureBinding binding {};
	/** @brief Top-left and bottom-right UV of the source region. */
	glm::fvec2 uv[2] = { glm::fvec2(0.0f), glm::fvec2(1.0f) };
	/** @brief Tint multiplied into the sampled texels. */
	glm::fvec4 tint = glm::fvec4(1.0f);
	/** @brief Nine-slice source margins; all-zero means a plain quad. */
	ThemeSlice slice {};
	/** @brief Full source-region size in pixels; zero disables nine-slice math. */
	glm::fvec2 sourceSize = glm::fvec2(0.0f);
};

/** @brief Layout and decoration hints associated with a theme element. */
struct ThemeMetrics {
	/** @brief Inner padding around content, in pixels. */
	glm::fvec2 padding = glm::fvec2(0.0f);
	/** @brief Gap between sibling elements, in pixels. */
	glm::fvec2 spacing = glm::fvec2(0.0f);
	/** @brief Minimum element size, in pixels. */
	glm::fvec2 minimumSize = glm::fvec2(0.0f);
	/** @brief Border/stroke width, in pixels. */
	float borderWidth = 0.0f;
	/** @brief Corner radius for procedural drawing, in pixels. */
	float cornerRadius = 0.0f;
};

/**
 * @brief Interface a concrete GUI theme implements.
 *
 * Implementations are expected to be immutable after construction: widgets may
 * query a theme every frame and may cache the returned descriptors. State is passed
 * as the widget's @ref WidgetState bitmask, so a theme can either ignore it or
 * distinguish hovered, pressed and disabled combinations.
 */
class Theme {
public:
	/** @brief Releases the theme through the interface. */
	virtual ~Theme() = default;

	/** @brief Stable display/debug name of the theme. */
	virtual std::string_view name() const = 0;

	/**
	 * @brief Returns the skin for one element/state combination.
	 * @param element Part to draw.
	 * @param state Widget-state bitmask; see @ref WidgetState.
	 * @return Skin, or `std::nullopt` when this theme has no opinion.
	 */
	virtual std::optional<ThemeSkin> skin(ThemeElement element, WidgetState state) const = 0;

	/**
	 * @brief Returns the text styling for one semantic role/state combination.
	 * @param role Text role.
	 * @param state Widget-state bitmask.
	 */
	virtual Text::TextAttributes textStyle(ThemeTextRole role, WidgetState state) const = 0;

	/**
	 * @brief Returns spacing/size hints for one element.
	 * @param element Part whose metrics are requested.
	 */
	virtual ThemeMetrics metrics(ThemeElement element) const = 0;
};

} // namespace Gui
} // namespace Lotte

#endif // LTETHEME_HPP

