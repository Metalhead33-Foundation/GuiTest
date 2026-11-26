#ifndef LTEGUIATLASPART_HPP
#define LTEGUIATLASPART_HPP
/**
 * @file LteGuiAtlasParts.hpp
 * @brief The enum-keyed vocabulary of a generated GUI atlas.
 *
 * Split out of `LteGuiAtlas.hpp` so the theme schema can name a part without pulling
 * in the SDF/material/canvas stack the builder needs. A part is an enum, and this
 * header is the one place that maps it to a stable name - which is what lets a theme
 * file say `"part": "button_hover"` while the C++ side never juggles a string map.
 */
#include <Lotte/LotteLib.hpp>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Lotte {
namespace Procedural {

/** @brief One addressable rectangle of a generated atlas. */
enum class GuiAtlasPart : std::uint8_t {
	Panel,            /**< Generic stone panel face. */
	ButtonIdle,       /**< Button in its normal state. */
	ButtonHover,      /**< Button under the pointer. */
	ButtonPressed,    /**< Button held down. */
	ButtonDisabled,   /**< Button with the `Enabled` bit clear. */
	Slot,             /**< Sunken inventory slot. */
	TitlePlate,       /**< Stepped title bar with a recessed caption field. */
	CheckboxEmpty,    /**< Unchecked box. */
	CheckboxChecked,  /**< Checked box with a gold tick and glow. */
	SliderTrack,      /**< Horizontal slider groove. */
	SliderThumb,      /**< Gem thumb for the slider. */
	Parchment,        /**< Torn parchment sheet. */
	Bar,              /**< Inset text/log bar. */
	RadioEmpty,       /**< Unselected radio glyph. */
	RadioChecked,     /**< Selected radio glyph with a lit gem core. */
	ArrowUp,          /**< Scroll/spin button pointing up. */
	ArrowDown,        /**< Scroll/spin button pointing down. */
	TriDown,          /**< Dropdown triangle of a combo box. */
	ScrollTrack,      /**< Vertical scroll-bar groove. */
	ScrollThumb,      /**< Vertical scroll-bar handle. */
	TabActive,        /**< Selected tab face. */
	TabInactive,      /**< Unselected tab face. */
	FrameText,        /**< Wide frame with a recessed field (portrait, text area). */
	SlotGroup,        /**< Panel with a nameplate and a grid of wells. */
	BorderStrip,      /**< Nine-sliceable window edge. */
	GemTile,          /**< Decorated icon tile with a gem. */
	Count             /**< Number of parts; not a valid part. */
};

/** @brief Number of @ref GuiAtlasPart values that name a rectangle. */
inline constexpr std::size_t GuiAtlasPartCount = static_cast<std::size_t>(GuiAtlasPart::Count);

/**
 * @brief Stable file/JSON name of a part.
 * @param part Part to name.
 * @return A lower-case identifier (`"button_idle"`, ...); never null.
 */
MH_LOTTE_API const char* guiAtlasPartName(GuiAtlasPart part) noexcept;

/**
 * @brief Resolves a part from its stable name.
 * @param name Identifier as returned by @ref guiAtlasPartName.
 * @param part Receives the part on success; untouched otherwise.
 * @return True when @p name named a part.
 */
MH_LOTTE_API bool parseGuiAtlasPart(std::string_view name, GuiAtlasPart& part) noexcept;

/** @brief Where one part landed in the atlas, in pixels. */
struct GuiAtlasRect {
	/** @brief Which part this rectangle holds. */
	GuiAtlasPart part = GuiAtlasPart::Panel;
	/** @brief Left edge in the atlas. */
	std::uint32_t x = 0;
	/** @brief Top edge in the atlas. */
	std::uint32_t y = 0;
	/** @brief Width including the transparent margin. */
	std::uint32_t width = 0;
	/** @brief Height including the transparent margin. */
	std::uint32_t height = 0;
};

} // namespace Procedural
} // namespace Lotte

#endif // LTEGUIATLASPART_HPP