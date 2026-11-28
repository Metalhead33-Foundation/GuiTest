#ifndef LTEGUIATLAS_HPP
#define LTEGUIATLAS_HPP
/**
 * @file LteGuiAtlas.hpp
 * @brief Builds a whole widget-skin atlas from one seed, with enum-keyed manifests.
 *
 * The element recipes of `Sandbox/slop_advice/gui_procedural_generation/claude/ui_forge.cpp`
 * plus the element types the four reference documents agree on (`grok.md`, `chatgpt.md`,
 * `gemini.md`: arrows, tabs, scroll bars, slot groups, frames), turned into an API a
 * theme can consume:
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
#include <Lotte/Procedural/LteGuiAtlasParts.hpp>
#include <Lotte/Procedural/LteGuiMaterials.hpp>
#include <cstdint>
#include <string_view>
#include <vector>

namespace Lotte {
namespace Procedural {

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
	/** @brief Radio glyph size in pixels. */
	std::uint32_t radioSize = 32;
	/** @brief Arrow button width in pixels. */
	std::uint32_t arrowWidth = 28;
	/** @brief Arrow button height in pixels. */
	std::uint32_t arrowHeight = 32;
	/** @brief Side of the square that holds the dropdown triangle. */
	std::uint32_t triangleSize = 20;
	/** @brief Scroll track width in pixels. */
	std::uint32_t scrollTrackWidth = 24;
	/** @brief Scroll track height in pixels. */
	std::uint32_t scrollTrackHeight = 160;
	/** @brief Scroll thumb width in pixels. */
	std::uint32_t scrollThumbWidth = 20;
	/** @brief Scroll thumb height in pixels. */
	std::uint32_t scrollThumbHeight = 48;
	/** @brief Tab width in pixels. */
	std::uint32_t tabWidth = 128;
	/** @brief Tab height in pixels. */
	std::uint32_t tabHeight = 40;
	/** @brief Text/portrait frame width in pixels. */
	std::uint32_t frameWidth = 320;
	/** @brief Text/portrait frame height in pixels. */
	std::uint32_t frameHeight = 160;
	/** @brief Columns of the generated slot group. */
	std::uint32_t slotGroupColumns = 4;
	/** @brief Rows of the generated slot group. */
	std::uint32_t slotGroupRows = 2;
	/** @brief Length of the nine-sliceable border strip in pixels. */
	std::uint32_t borderStripLength = 256;
	/** @brief Thickness of the border strip in pixels. */
	std::uint32_t borderStripThickness = 16;
	/** @brief Side of a decorated icon tile in pixels. */
	std::uint32_t tileSize = 48;
	/**
	 * @brief Bevel the recipes carve, in pixels; zero keeps each recipe's own default.
	 *
	 * A theme's CSS `border-width` lands here, so the generated skin and the widget
	 * metrics agree on how thick a stroke is.
	 */
	float bevelWidth = 0.0f;
	/**
	 * @brief Multiplier for the stone palette, `(1, 1, 1)` for the recipe's own colours.
	 *
	 * A theme's CSS `background-color` lands here: the recipes multiply their dark/mid/
	 * light/frost tones by it, so the same panel can be navy stone or warm sandstone
	 * without a second palette.
	 */
	glm::fvec3 faceTint = glm::fvec3(1.0f);
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
