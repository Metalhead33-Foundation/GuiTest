#ifndef LTEGUIATLAS_HPP
#define LTEGUIATLAS_HPP
/**
 * @file LteGuiAtlas.hpp
 * @brief Builds a whole widget-skin atlas from one seed, with enum-keyed manifests.
 *
 * The element recipes of `Sandbox/slop_advice/gui_procedural_generation/claude/ui_forge.cpp`
 * (plus the flat-frame style of `grok.md`) turned into an API a theme can consume:
 * one call produces the atlas image and a manifest that says which rectangle holds
 * which part. Three deliberate differences from the original:
 *
 * - **The manifest is enum-keyed.** The original wrote `atlas.json` with string keys
 *   and a hard-coded layout; here a part is a `GuiAtlasPart`, and the string form
 *   exists only for the theme file that has to name one. That is the project's
 *   "enums over strings" rule applied to the data the GUI actually looks up.
 * - **No file output.** The atlas is an RGBA8 image
 *   (`Lotte::Gfx::Rgba`8Image); saving it is the caller's job and goes through
 *   Euphemy's encoders via `Lotte::Procedural::toDecodeTarget`.
 * - **Sizes are settings, not constants.** The original packed a 1106x826 sheet with
 *   hand-placed coordinates; this builder packs by shelf and takes the element sizes
 *   from `GuiAtlasSettings`, so a theme can ask for a different scale.
 *
 * @note GPU plan: the painter is the same per-pixel work a compute shader would do,
 *       and the packing is already a CPU-side layout step; only the shading loop moves.
 */
#include <Lotte/LotteLib.hpp>
#include <Lotte/GFX/LteImageConvert.hpp>
#include <Lotte/Procedural/LteGuiMaterials.hpp>
#include <cstdint>
#include <string_view>
#include <vector>

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

/** @brief A generated atlas image plus the manifest of its parts. */
struct GuiAtlas {
	/** @brief The packed RGBA8 image. */
	Gfx::Rgba8Image image;
	/** @brief One entry per generated part. */
	std::vector<GuiAtlasRect> parts;

	/**
	 * @brief Looks a part up in the manifest.
	 * @param part Part to find.
	 * @return The rectangle, or null when the part was not generated.
	 */
	const GuiAtlasRect* find(GuiAtlasPart part) const noexcept;
};

/** @brief Everything the atlas builder needs to know. */
struct GuiAtlasSettings {
	/** @brief Seed of every material and every placement. */
	std::uint32_t seed = 1337;
	/** @brief Transparent margin around every element, in pixels (room for shadows). */
	std::uint32_t margin = 8;
	/** @brief Gap between packed elements, in pixels. */
	std::uint32_t gap = 2;
	/** @brief Atlas width in pixels; the builder grows the height as needed. */
	std::uint32_t atlasWidth = 1024;
	/** @brief Panel face size in pixels. */
	std::uint32_t panelWidth = 256;
	/** @brief Panel face height in pixels. */
	std::uint32_t panelHeight = 128;
	/** @brief Button size in pixels. */
	std::uint32_t buttonWidth = 128;
	/** @brief Button height in pixels. */
	std::uint32_t buttonHeight = 48;
	/** @brief Inventory slot size in pixels. */
	std::uint32_t slotSize = 64;
	/** @brief Title plate size in pixels. */
	std::uint32_t titleWidth = 256;
	/** @brief Title plate height in pixels. */
	std::uint32_t titleHeight = 48;
	/** @brief Checkbox size in pixels. */
	std::uint32_t checkboxSize = 32;
	/** @brief Slider track size in pixels. */
	std::uint32_t sliderWidth = 192;
	/** @brief Slider track height in pixels. */
	std::uint32_t sliderHeight = 32;
	/** @brief Slider thumb size in pixels. */
	std::uint32_t thumbWidth = 20;
	/** @brief Slider thumb height in pixels. */
	std::uint32_t thumbHeight = 26;
	/** @brief Parchment sheet size in pixels. */
	std::uint32_t parchmentWidth = 192;
	/** @brief Parchment sheet height in pixels. */
	std::uint32_t parchmentHeight = 256;
	/** @brief Bar size in pixels. */
	std::uint32_t barWidth = 256;
	/** @brief Bar height in pixels. */
	std::uint32_t barHeight = 40;
};

/**
 * @brief Generates and packs the whole atlas.
 * @param settings Element sizes, seed and packing parameters.
 * @return The atlas image and its manifest; empty when the settings have zero sizes.
 */
MH_LOTTE_API GuiAtlas buildGuiAtlas(const GuiAtlasSettings& settings = {});

} // namespace Procedural
} // namespace Lotte

#endif // LTEGUIATLAS_HPP
