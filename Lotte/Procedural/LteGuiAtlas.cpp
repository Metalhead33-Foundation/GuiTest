#include "LteGuiAtlas.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace Lotte {
namespace Procedural {

namespace {

/// @brief Names in the same order as the @ref GuiAtlasPart enumerators.
constexpr std::array<const char*, GuiAtlasPartCount> PartNames = {
	"panel",          "button_idle",  "button_hover",     "button_pressed",
	"button_disabled", "slot",        "title_plate",      "checkbox_empty",
	"checkbox_checked", "slider_track", "slider_thumb",    "parchment",
	"bar",            "radio_empty",  "radio_checked",    "arrow_up",
	"arrow_down",     "tri_down",     "scroll_track",     "scroll_thumb",
	"tab_active",     "tab_inactive", "frame_text",       "slot_group",
	"border_strip",   "gem_tile"
};

/// @brief Pointer to a rectangle's top-left corner.
glm::fvec2 corner(float x, float y)
{
	return glm::fvec2(x, y);
}

/// @brief Multiplies a stone palette by the theme's face tint.
GuiStoneStyle tinted(GuiStoneStyle style, glm::fvec3 tint)
{
	style.dark *= tint;
	style.mid *= tint;
	style.light *= tint;
	style.frost *= tint;
	return style;
}

/// @brief The bevel a recipe carves: the theme's CSS border width when it set one.
float bevelWidth(const GuiAtlasSettings& settings, float fallback)
{
	return settings.bevelWidth > 0.0f ? settings.bevelWidth : fallback;
}

/// @brief Paints a raised chamfered body with a stone face.
void paintRaised(SurfaceCanvas& canvas, const SdfFunction& shape, const GuiStoneStyle& style,
				 const GuiField* cracks, float bevel = 3.0f, float slope = 0.85f)
{
	PaintOptions options;
	options.bevel = bevel;
	options.slope = slope;
	options.sign = 1.0f;
	canvas.paint(shape, stoneMaterial(style, cracks), options);
}

/// @brief Paints a sunken chamfered body with a stone face.
void paintSunken(SurfaceCanvas& canvas, const SdfFunction& shape, const GuiStoneStyle& style,
				 const GuiField* cracks, float bevel = 3.0f, float slope = 0.9f)
{
	PaintOptions options;
	options.bevel = bevel;
	options.slope = slope;
	options.sign = -1.0f;
	canvas.paint(shape, stoneMaterial(style, cracks), options);
}

/// @brief Paints gold with a thin dark engraving line under it, so it separates from stone.
void paintGold(SurfaceCanvas& canvas, const SdfFunction& shape, std::uint32_t seed,
			   bool engrave = true)
{
	if (engrave) {
		PaintOptions options;
		options.bevel = 1.0f;
		options.slope = 0.0f;
		canvas.paint(inset(shape, -1.5f), flatMaterial(glm::fvec3(0.043f, 0.039f, 0.071f)), options);
	}
	PaintOptions options;
	options.bevel = 1.6f;
	options.slope = 1.0f;
	canvas.paint(shape, goldMaterial(seed), options);
}

/// @brief Adds a gem: gold bezel, faceted body, highlight dot.
void paintGem(SurfaceCanvas& canvas, glm::fvec2 centre, float halfWidth, float halfHeight,
			  std::uint32_t seed)
{
	paintGold(canvas, diamond(centre, halfWidth + 3.4f, halfHeight + 4.0f), seed);
	const float innerRadius = halfWidth * halfHeight / std::hypot(halfWidth, halfHeight);
	PaintOptions options;
	options.bevel = innerRadius * 0.62f;
	options.slope = 1.35f;
	canvas.paint(diamond(centre, halfWidth, halfHeight),
gemMaterial(glm::fvec3(0.557f, 0.0f, 0.078f), glm::fvec3(1.0f, 0.227f, 0.282f),
innerRadius),
options);
	PaintOptions glint;
	glint.bevel = 0.5f;
	glint.slope = 0.2f;
	canvas.paint(circle(centre + corner(-halfWidth * 0.28f, -halfHeight * 0.30f), 1.5f),
flatMaterial(glm::fvec3(1.0f, 0.91f, 0.91f), 0.0f, 1.0f), glint);
}

SurfaceCanvas paintPanel(const GuiAtlasSettings& settings, std::uint32_t seed)
{
	const float margin = static_cast<float>(settings.margin);
	const float width = static_cast<float>(settings.panelWidth);
	const float height = static_cast<float>(settings.panelHeight);
	SurfaceCanvas canvas(settings.panelWidth + settings.margin * 2,
settings.panelHeight + settings.margin * 2);
	const GuiField cracks = makeVoronoiCrackField(canvas.width(), canvas.height(), 22, seed);
	const SdfFunction body = roughen(
roundedBox(corner(margin, margin), corner(margin + width, margin + height), 6.0f), 1.3f,
0.12f, seed);
	paintRaised(canvas, body, tinted(stoneStyle(StonePalette::Blue, seed), settings.faceTint), &cracks,
				bevelWidth(settings, 3.6f), 0.85f);
	paintSunken(canvas, roundedBox(corner(margin + 14.0f, margin + 14.0f),
corner(margin + width - 14.0f, margin + height - 14.0f), 3.0f),
tinted(stoneStyle(StonePalette::Void, seed + 2u), settings.faceTint), &cracks, 2.4f, 0.7f);
	paintGold(canvas, outline(roundedBox(corner(margin + 18.0f, margin + 18.0f),
corner(margin + width - 18.0f, margin + height - 18.0f), 2.0f),
2.0f, 0.0f),
seed + 3u);
	return canvas;
}

SurfaceCanvas paintButton(const GuiAtlasSettings& settings, std::uint32_t seed, int state)
{
	const float margin = static_cast<float>(settings.margin);
	const float width = static_cast<float>(settings.buttonWidth);
	const float height = static_cast<float>(settings.buttonHeight);
	SurfaceCanvas canvas(settings.buttonWidth + settings.margin * 2,
settings.buttonHeight + settings.margin * 2);
	const GuiField cracks = makeVoronoiCrackField(canvas.width(), canvas.height(), 16, seed);

	GuiStoneStyle style = tinted(stoneStyle(StonePalette::Grey, seed), settings.faceTint);
	if (state == 1) style = tinted(stoneStyle(StonePalette::Rim, seed + 2u), settings.faceTint);
	if (state == 3) style = dimStone(style, 0.58f);
	style.mid = state == 1 ? glm::fvec3(0.659f, 0.663f, 0.706f) : style.mid;
	style.dark = state == 1 ? glm::fvec3(0.486f, 0.490f, 0.533f) : style.dark;

	const SdfFunction body = roughen(
roundedBox(corner(margin, margin), corner(margin + width, margin + height), 4.0f), 0.9f,
0.15f, seed);
	if (state == 2) {
		paintSunken(canvas, body, dimStone(style, 0.8f), &cracks, bevelWidth(settings, 3.6f), 0.85f);
	} else {
		paintRaised(canvas, body, style, &cracks, bevelWidth(settings, 4.0f), 0.9f);
	}
	if (state == 1) {
		paintGold(canvas, outline(roundedBox(corner(margin, margin),
corner(margin + width, margin + height), 4.0f),
2.0f, 1.0f),
seed + 9u, false);
	}
	return canvas;
}

SurfaceCanvas paintSlot(const GuiAtlasSettings& settings, std::uint32_t seed)
{
	const float margin = static_cast<float>(settings.margin);
	const float size = static_cast<float>(settings.slotSize);
	SurfaceCanvas canvas(settings.slotSize + settings.margin * 2,
settings.slotSize + settings.margin * 2);
	const GuiField cracks = makeVoronoiCrackField(canvas.width(), canvas.height(), 14, seed);
	paintRaised(canvas, roundedBox(corner(margin, margin), corner(margin + size, margin + size), 1.5f),
tinted(stoneStyle(StonePalette::Rim, seed), settings.faceTint), &cracks, 1.5f, 0.6f);
	const float rim = 3.6f;
	paintSunken(canvas, roundedBox(corner(margin + rim, margin + rim),
corner(margin + size - rim, margin + size - rim), 1.0f),
tinted(stoneStyle(StonePalette::Slot, seed + 1u), settings.faceTint), &cracks, 3.2f, 0.9f);
	return canvas;
}

SurfaceCanvas paintTitlePlate(const GuiAtlasSettings& settings, std::uint32_t seed)
{
const float margin = static_cast<float>(settings.margin);
const float width = static_cast<float>(settings.titleWidth);
const float height = static_cast<float>(settings.titleHeight);
SurfaceCanvas canvas(settings.titleWidth + settings.margin * 2,
settings.titleHeight + settings.margin * 2);
const GuiField cracks = makeVoronoiCrackField(canvas.width(), canvas.height(), 20, seed);

// Stepped ends plus a slightly inset bar: the classic banner silhouette.
const SdfFunction ends = unite(
roundedBox(corner(margin, margin), corner(margin + 40.0f, margin + height), 3.0f),
roundedBox(corner(margin + width - 40.0f, margin), corner(margin + width, margin + height),
3.0f));
const SdfFunction body = roughen(
unite(ends, roundedBox(corner(margin, margin + height * 0.14f),
corner(margin + width, margin + height * 0.86f), 3.0f)),
0.9f, 0.15f, seed);
paintRaised(canvas, body, tinted(stoneStyle(StonePalette::Deep, seed), settings.faceTint), &cracks, 3.6f, 0.85f);

const SdfFunction field = roundedBox(corner(margin + 14.0f, margin + height * 0.27f),
corner(margin + width - 14.0f, margin + height * 0.73f), 2.0f);
paintSunken(canvas, field, dimStone(tinted(stoneStyle(StonePalette::Deep, seed + 2u), settings.faceTint), 0.55f), &cracks,
2.8f, 0.8f);
PaintOptions trim;
trim.bevel = 0.8f;
trim.slope = 0.4f;
canvas.paint(outline(field, 1.6f, -2.2f), flatMaterial(glm::fvec3(0.624f, 0.690f, 0.878f)), trim);
return canvas;
}

SurfaceCanvas paintCheckbox(const GuiAtlasSettings& settings, std::uint32_t seed, bool checked)
{
const float margin = static_cast<float>(settings.margin);
const float size = static_cast<float>(settings.checkboxSize);
SurfaceCanvas canvas(settings.checkboxSize + settings.margin * 2,
settings.checkboxSize + settings.margin * 2);
const GuiField cracks = makeVoronoiCrackField(canvas.width(), canvas.height(), 12, seed);
const SdfFunction frame = roundedBox(corner(margin, margin), corner(margin + size, margin + size),
2.0f);
if (checked) {
paintGold(canvas, frame, seed, false);
} else {
paintRaised(canvas, frame, tinted(stoneStyle(StonePalette::Steel, seed), settings.faceTint), &cracks, 2.6f, 0.8f);
}
const float border = checked ? 4.2f : 5.0f;
paintSunken(canvas, roundedBox(corner(margin + border, margin + border),
corner(margin + size - border, margin + size - border), 1.0f),
stoneStyle(checked ? StonePalette::Void : StonePalette::Navy, seed + 1u), &cracks, 2.2f,
0.7f);
if (checked) {
const SdfFunction tick = unite(
capsule(corner(margin + size * 0.24f, margin + size * 0.53f),
corner(margin + size * 0.43f, margin + size * 0.74f), 2.6f),
capsule(corner(margin + size * 0.43f, margin + size * 0.74f),
corner(margin + size * 0.80f, margin + size * 0.27f), 2.6f));
canvas.glow(tick, glm::fvec3(1.0f, 0.659f, 0.125f), 5.0f, 0.55f);
PaintOptions options;
options.bevel = 1.6f;
options.slope = 0.5f;
canvas.paint(tick, flatMaterial(glm::fvec3(1.0f, 0.941f, 0.627f), 0.0f, 0.6f), options);
canvas.glow(tick, glm::fvec3(1.0f, 0.878f, 0.502f), 1.5f, 0.45f);
}
return canvas;
}

SurfaceCanvas paintSliderTrack(const GuiAtlasSettings& settings, std::uint32_t seed)
{
const float margin = static_cast<float>(settings.margin);
const float width = static_cast<float>(settings.sliderWidth);
const float height = static_cast<float>(settings.sliderHeight);
SurfaceCanvas canvas(settings.sliderWidth + settings.margin * 2,
settings.sliderHeight + settings.margin * 2);
const GuiField cracks = makeVoronoiCrackField(canvas.width(), canvas.height(), 18, seed);
paintRaised(canvas,
roughen(roundedBox(corner(margin, margin), corner(margin + width, margin + height), 9.0f),
0.7f, 0.15f, seed),
tinted(stoneStyle(StonePalette::Deep, seed), settings.faceTint), &cracks, 3.0f, 0.8f);
const float grooveTop = margin + height * 0.34f;
const float grooveBottom = margin + height * 0.66f;
const SdfFunction groove = roundedBox(corner(margin + 16.0f, grooveTop),
corner(margin + width - 16.0f, grooveBottom),
(grooveBottom - grooveTop) * 0.5f);
paintSunken(canvas, groove, tinted(stoneStyle(StonePalette::Void, seed + 1u), settings.faceTint), &cracks, 2.2f, 0.7f);
paintGold(canvas, outline(groove, 2.4f, 0.0f), seed + 2u);
return canvas;
}

SurfaceCanvas paintSliderThumb(const GuiAtlasSettings& settings, std::uint32_t seed)
{
const float margin = static_cast<float>(settings.margin);
SurfaceCanvas canvas(settings.thumbWidth + settings.margin * 2,
settings.thumbHeight + settings.margin * 2);
paintGem(canvas,
corner(margin + static_cast<float>(settings.thumbWidth) * 0.5f,
margin + static_cast<float>(settings.thumbHeight) * 0.5f),
static_cast<float>(settings.thumbWidth) * 0.45f,
static_cast<float>(settings.thumbHeight) * 0.46f, seed);
return canvas;
}

SurfaceCanvas paintParchment(const GuiAtlasSettings& settings, std::uint32_t seed)
{
const float margin = static_cast<float>(settings.margin);
const float width = static_cast<float>(settings.parchmentWidth);
const float height = static_cast<float>(settings.parchmentHeight);
SurfaceCanvas canvas(settings.parchmentWidth + settings.margin * 2,
settings.parchmentHeight + settings.margin * 2);
const SdfFunction base = roundedBox(corner(margin + 3.0f, margin + 3.0f),
corner(margin + width - 3.0f, margin + height - 3.0f), 3.0f);
// Torn edge: gradient noise displaces the outline, which is what makes the silhouette
// irregular instead of a rectangle. The finest octave stays near 2 px so the edge reads
// as fibre rather than as aliasing.
const SdfFunction torn = [base, seed](glm::fvec2 position) {
const float macro = Euph::Util::Noise::latticeGradientFbm2D(position.x * 0.07f,
position.y * 0.07f, seed, 4) - 0.5f;
const float micro = Euph::Util::Noise::latticeGradientFbm2D(position.x * 0.22f,
position.y * 0.22f, seed + 3u, 2) - 0.5f;
return base(position) + macro * 11.0f + micro * 2.4f;
};
PaintOptions options;
options.bevel = 2.0f;
options.slope = 0.6f;
canvas.paint(torn, parchmentMaterial(seed), options);

const SdfFunction band = outline(roundedBox(corner(margin + 17.0f, margin + 17.0f),
corner(margin + width - 17.0f, margin + height - 17.0f), 2.0f),
7.0f, 0.0f);
const SurfaceMaterial rope = [](glm::fvec2 position, float distance) -> MaterialSample {
const float strand = position.x + position.y;
const float weave = (std::sin(strand * 0.82f) + 1.0f) * 0.25f + 0.5f;
MaterialSample sample;
sample.albedo = std::fabs(distance) > 2.3f
? glm::fvec3(0.290f, 0.173f, 0.078f)
: glm::mix(glm::fvec3(0.353f, 0.216f, 0.102f), glm::fvec3(0.753f, 0.557f, 0.337f),
weave);
return sample;
};
PaintOptions bandOptions;
bandOptions.bevel = 0.8f;
bandOptions.slope = 0.25f;
canvas.paint(band, rope, bandOptions);
return canvas;
}

SurfaceCanvas paintBar(const GuiAtlasSettings& settings, std::uint32_t seed)
{
const float margin = static_cast<float>(settings.margin);
const float width = static_cast<float>(settings.barWidth);
const float height = static_cast<float>(settings.barHeight);
SurfaceCanvas canvas(settings.barWidth + settings.margin * 2,
settings.barHeight + settings.margin * 2);
const GuiField cracks = makeVoronoiCrackField(canvas.width(), canvas.height(), 18, seed);
paintRaised(canvas,
roughen(roundedBox(corner(margin, margin), corner(margin + width, margin + height), 2.0f),
0.6f, 0.1f, seed),
tinted(stoneStyle(StonePalette::Blue, seed), settings.faceTint), &cracks, 3.0f, 0.85f);
paintSunken(canvas, roundedBox(corner(margin + 5.0f, margin + 5.0f),
corner(margin + width - 5.0f, margin + height - 5.0f), 1.0f),
tinted(stoneStyle(StonePalette::Void, seed + 1u), settings.faceTint), &cracks, 3.6f, 0.95f);
return canvas;
}

/// @brief Paints a gold glyph with a soft glow, the way every arrow and triangle here is made.
void paintGoldGlyph(SurfaceCanvas& canvas, const SdfFunction& glyph, std::uint32_t seed)
{
canvas.glow(glyph, glm::fvec3(1.0f, 0.760f, 0.240f), 4.0f, 0.30f);
paintGold(canvas, glyph, seed, false);
canvas.glow(glyph, glm::fvec3(1.0f, 0.878f, 0.502f), 1.2f, 0.30f);
}

/// @brief Radio glyph: a round stone bezel, a sunken well and a gem core when selected.
SurfaceCanvas paintRadio(const GuiAtlasSettings& settings, std::uint32_t seed, bool checked)
{
const float margin = static_cast<float>(settings.margin);
const float size = static_cast<float>(settings.radioSize);
SurfaceCanvas canvas(settings.radioSize + settings.margin * 2,
 settings.radioSize + settings.margin * 2);
const GuiField cracks = makeVoronoiCrackField(canvas.width(), canvas.height(), 12, seed);
const glm::fvec2 centre = corner(margin + size * 0.5f, margin + size * 0.5f);
const SdfFunction rim = circle(centre, size * 0.5f);
paintRaised(canvas, rim, tinted(stoneStyle(StonePalette::Steel, seed), settings.faceTint), &cracks, 2.6f, 0.8f);
paintSunken(canvas, circle(centre, size * 0.5f - 4.4f),
tinted(stoneStyle(StonePalette::Navy, seed + 1u), settings.faceTint), &cracks, 2.0f, 0.7f);
if (!checked) return canvas;
paintGold(canvas, outline(rim, 2.0f, -1.2f), seed + 2u);
const float coreRadius = size * 0.19f;
canvas.glow(circle(centre, coreRadius), glm::fvec3(1.0f, 0.659f, 0.125f), 6.0f, 0.55f);
PaintOptions options;
options.bevel = coreRadius * 0.7f;
options.slope = 1.0f;
canvas.paint(circle(centre, coreRadius),
 gemMaterial(glm::fvec3(0.557f, 0.0f, 0.078f), glm::fvec3(1.0f, 0.227f, 0.282f),
 coreRadius), options);
return canvas;
}

/// @brief Scroll/spin arrow: a stone button with a solid gold triangle on it.
SurfaceCanvas paintArrow(const GuiAtlasSettings& settings, std::uint32_t seed, bool up)
{
const float margin = static_cast<float>(settings.margin);
const float width = static_cast<float>(settings.arrowWidth);
const float height = static_cast<float>(settings.arrowHeight);
SurfaceCanvas canvas(settings.arrowWidth + settings.margin * 2,
 settings.arrowHeight + settings.margin * 2);
const GuiField cracks = makeVoronoiCrackField(canvas.width(), canvas.height(), 12, seed);
paintRaised(canvas, roundedBox(corner(margin, margin), corner(margin + width, margin + height), 3.0f),
tinted(stoneStyle(StonePalette::Steel, seed), settings.faceTint), &cracks, 3.0f, 0.85f);
// The triangle is inset so the stone bevel stays visible all the way round.
const float centreX = margin + width * 0.5f;
const float tip = margin + height * (up ? 0.28f : 0.72f);
const float base = margin + height * (up ? 0.70f : 0.30f);
const float halfWidth = width * 0.26f;
const std::array<glm::fvec2, 3> points = { corner(centreX, tip), corner(centreX + halfWidth, base),
  corner(centreX - halfWidth, base) };
paintGoldGlyph(canvas, polygon(std::span<const glm::fvec2>(points)), seed + 1u);
return canvas;
}

/// @brief Dropdown triangle: the small inset button a combo box shows on its right.
SurfaceCanvas paintTriangle(const GuiAtlasSettings& settings, std::uint32_t seed)
{
const float margin = static_cast<float>(settings.margin);
const float size = static_cast<float>(settings.triangleSize);
SurfaceCanvas canvas(settings.triangleSize + settings.margin * 2,
 settings.triangleSize + settings.margin * 2);
const GuiField cracks = makeVoronoiCrackField(canvas.width(), canvas.height(), 10, seed);
paintRaised(canvas, roundedBox(corner(margin, margin), corner(margin + size, margin + size), 2.0f),
tinted(stoneStyle(StonePalette::Steel, seed), settings.faceTint), &cracks, 2.2f, 0.8f);
paintSunken(canvas, roundedBox(corner(margin + 2.4f, margin + 2.4f),
  corner(margin + size - 2.4f, margin + size - 2.4f), 1.4f),
tinted(stoneStyle(StonePalette::Void, seed + 1u), settings.faceTint), &cracks, 1.6f, 0.6f);
const float centreX = margin + size * 0.5f;
const float top = margin + size * 0.38f;
const float bottom = margin + size * 0.62f;
const float halfWidth = size * 0.22f;
const std::array<glm::fvec2, 3> points = { corner(centreX, bottom), corner(centreX + halfWidth, top),
  corner(centreX - halfWidth, top) };
paintGoldGlyph(canvas, polygon(std::span<const glm::fvec2>(points)), seed + 2u);
return canvas;
}

/// @brief Vertical scroll groove: a stone column with a sunken, gold-trimmed channel.
SurfaceCanvas paintScrollTrack(const GuiAtlasSettings& settings, std::uint32_t seed)
{
const float margin = static_cast<float>(settings.margin);
const float width = static_cast<float>(settings.scrollTrackWidth);
const float height = static_cast<float>(settings.scrollTrackHeight);
SurfaceCanvas canvas(settings.scrollTrackWidth + settings.margin * 2,
 settings.scrollTrackHeight + settings.margin * 2);
const GuiField cracks = makeVoronoiCrackField(canvas.width(), canvas.height(), 16, seed);
paintRaised(canvas, roundedBox(corner(margin, margin), corner(margin + width, margin + height), 6.0f),
tinted(stoneStyle(StonePalette::Deep, seed), settings.faceTint), &cracks, 2.8f, 0.8f);
const float insetX = width * 0.32f;
const SdfFunction groove = roundedBox(corner(margin + insetX, margin + 10.0f),
 corner(margin + width - insetX, margin + height - 10.0f),
 insetX);
paintSunken(canvas, groove, tinted(stoneStyle(StonePalette::Void, seed + 1u), settings.faceTint), &cracks, 2.0f, 0.7f);
paintGold(canvas, outline(groove, 2.0f, 0.0f), seed + 2u);
return canvas;
}

/// @brief Vertical scroll handle: a stone pill with a gold band at each end.
SurfaceCanvas paintScrollThumb(const GuiAtlasSettings& settings, std::uint32_t seed)
{
const float margin = static_cast<float>(settings.margin);
const float width = static_cast<float>(settings.scrollThumbWidth);
const float height = static_cast<float>(settings.scrollThumbHeight);
SurfaceCanvas canvas(settings.scrollThumbWidth + settings.margin * 2,
 settings.scrollThumbHeight + settings.margin * 2);
const GuiField cracks = makeVoronoiCrackField(canvas.width(), canvas.height(), 10, seed);
const SdfFunction body = roundedBox(corner(margin, margin), corner(margin + width, margin + height),
width * 0.45f);
paintRaised(canvas, body, tinted(stoneStyle(StonePalette::Rim, seed), settings.faceTint), &cracks, 2.6f, 0.85f);
const float band = 4.0f;
paintGold(canvas, roundedBox(corner(margin + 3.0f, margin + band),
 corner(margin + width - 3.0f, margin + band + 1.8f), 1.0f),
  seed + 1u, false);
paintGold(canvas, roundedBox(corner(margin + 3.0f, margin + height - band - 1.8f),
 corner(margin + width - 3.0f, margin + height - band), 1.0f),
  seed + 2u, false);
return canvas;
}

/// @brief Tab face: a trapezoid that is wider at the bottom, so it reads as part of a strip.
SurfaceCanvas paintTab(const GuiAtlasSettings& settings, std::uint32_t seed, bool active)
{
const float margin = static_cast<float>(settings.margin);
const float width = static_cast<float>(settings.tabWidth);
const float height = static_cast<float>(settings.tabHeight);
SurfaceCanvas canvas(settings.tabWidth + settings.margin * 2,
 settings.tabHeight + settings.margin * 2);
const GuiField cracks = makeVoronoiCrackField(canvas.width(), canvas.height(), 14, seed);
const float slant = height * 0.42f;
const std::array<glm::fvec2, 4> outlinePoints = {
corner(margin, margin + height), corner(margin + slant, margin),
corner(margin + width - slant, margin), corner(margin + width, margin + height)
};
const SdfFunction body = roughen(polygon(std::span<const glm::fvec2>(outlinePoints)), 0.7f, 0.15f, seed);
const GuiStoneStyle style = active ? tinted(stoneStyle(StonePalette::Deep, seed), settings.faceTint)
   : dimStone(tinted(stoneStyle(StonePalette::Navy, seed), settings.faceTint), 0.85f);
paintRaised(canvas, body, style, &cracks, bevelWidth(settings, 3.2f), 0.85f);
if (active) {
// The selected tab carries the gold trim and a lit field; the others stay quiet.
paintGold(canvas, outline(body, 2.0f, -1.4f), seed + 1u);
PaintOptions options;
options.bevel = 1.2f;
options.slope = 0.4f;
canvas.paint(inset(body, 6.0f), flatMaterial(glm::fvec3(0.100f, 0.135f, 0.243f)), options);
}
return canvas;
}

/// @brief Wide frame with a deep recessed field: a portrait slot or a text area.
SurfaceCanvas paintFrameText(const GuiAtlasSettings& settings, std::uint32_t seed)
{
const float margin = static_cast<float>(settings.margin);
const float width = static_cast<float>(settings.frameWidth);
const float height = static_cast<float>(settings.frameHeight);
SurfaceCanvas canvas(settings.frameWidth + settings.margin * 2,
 settings.frameHeight + settings.margin * 2);
const GuiField cracks = makeVoronoiCrackField(canvas.width(), canvas.height(), 24, seed);
const SdfFunction body = roughen(
roundedBox(corner(margin, margin), corner(margin + width, margin + height), 5.0f), 1.1f,
0.12f, seed);
paintRaised(canvas, body, tinted(stoneStyle(StonePalette::Blue, seed), settings.faceTint), &cracks, bevelWidth(settings, 3.2f), 0.85f);
paintGold(canvas, outline(inset(body, 7.0f), 1.8f, 0.0f), seed + 1u);
const SdfFunction field = inset(body, 12.0f);
paintSunken(canvas, field, tinted(stoneStyle(StonePalette::Void, seed + 2u), settings.faceTint), &cracks, 3.0f, 0.9f);
paintGold(canvas, outline(field, 1.4f, -2.0f), seed + 3u, false);
return canvas;
}

/// @brief Inventory grid: a stone backing plate, a nameplate strip and a grid of wells.
SurfaceCanvas paintSlotGroup(const GuiAtlasSettings& settings, std::uint32_t seed)
{
const float margin = static_cast<float>(settings.margin);
const float well = static_cast<float>(settings.slotSize);
const float pad = 12.0f;
const float gap = 8.0f;
const float nameHeight = 22.0f;
const float columns = static_cast<float>(std::max(settings.slotGroupColumns, 1u));
const float rows = static_cast<float>(std::max(settings.slotGroupRows, 1u));
const float width = pad * 2.0f + columns * well + (columns - 1.0f) * gap;
const float height = pad * 2.0f + nameHeight + 10.0f + rows * well + (rows - 1.0f) * gap;
SurfaceCanvas canvas(static_cast<std::uint32_t>(width) + settings.margin * 2,
 static_cast<std::uint32_t>(height) + settings.margin * 2);
const GuiField cracks = makeVoronoiCrackField(canvas.width(), canvas.height(), 26, seed);
const SdfFunction body = roughen(
roundedBox(corner(margin, margin), corner(margin + width, margin + height), 6.0f), 1.2f,
0.12f, seed);
paintRaised(canvas, body, tinted(stoneStyle(StonePalette::Blue, seed), settings.faceTint), &cracks, bevelWidth(settings, 3.6f), 0.85f);

// Nameplate: the same recessed field a title plate uses, so a caption reads on it.
const SdfFunction name = roundedBox(corner(margin + pad, margin + pad),
corner(margin + width - pad, margin + pad + nameHeight), 2.0f);
paintSunken(canvas, name, dimStone(tinted(stoneStyle(StonePalette::Deep, seed + 1u), settings.faceTint), 0.6f), &cracks, 2.4f,
0.8f);
paintGold(canvas, outline(name, 1.4f, -1.6f), seed + 2u, false);

// Wells: a raised rim and a sunken face each, the same pair a loose slot uses.
const float wellsTop = margin + pad + nameHeight + 10.0f;
for (std::uint32_t row = 0; row < settings.slotGroupRows; ++row) {
for (std::uint32_t column = 0; column < settings.slotGroupColumns; ++column) {
const float left = margin + pad + static_cast<float>(column) * (well + gap);
const float top = wellsTop + static_cast<float>(row) * (well + gap);
paintRaised(canvas, roundedBox(corner(left, top), corner(left + well, top + well), 1.5f),
tinted(stoneStyle(StonePalette::Rim, seed), settings.faceTint), &cracks, 1.5f, 0.6f);
const float rim = 3.6f;
paintSunken(canvas, roundedBox(corner(left + rim, top + rim),
  corner(left + well - rim, top + well - rim), 1.0f),
tinted(stoneStyle(StonePalette::Slot, seed + 3u), settings.faceTint), &cracks, 3.2f, 0.9f);
}
}
return canvas;
}

/// @brief Nine-sliceable window edge: a beveled stone plank with a gold inlay.
SurfaceCanvas paintBorderStrip(const GuiAtlasSettings& settings, std::uint32_t seed)
{
const float margin = static_cast<float>(settings.margin);
const float length = static_cast<float>(settings.borderStripLength);
const float thickness = static_cast<float>(settings.borderStripThickness);
SurfaceCanvas canvas(settings.borderStripLength + settings.margin * 2,
 settings.borderStripThickness + settings.margin * 2);
const GuiField cracks = makeVoronoiCrackField(canvas.width(), canvas.height(), 14, seed);
const SdfFunction body = roughen(
roundedBox(corner(margin, margin), corner(margin + length, margin + thickness), 2.0f),
0.6f, 0.12f, seed);
paintRaised(canvas, body, tinted(stoneStyle(StonePalette::Deep, seed), settings.faceTint), &cracks, 2.6f, 0.8f);
paintGold(canvas, outline(inset(body, 3.5f), 1.6f, 0.0f), seed + 1u, false);
return canvas;
}

/// @brief Icon tile: a slot frame with a gem medallion in the middle.
SurfaceCanvas paintGemTile(const GuiAtlasSettings& settings, std::uint32_t seed)
{
const float margin = static_cast<float>(settings.margin);
const float size = static_cast<float>(settings.tileSize);
SurfaceCanvas canvas(settings.tileSize + settings.margin * 2,
 settings.tileSize + settings.margin * 2);
const GuiField cracks = makeVoronoiCrackField(canvas.width(), canvas.height(), 12, seed);
paintRaised(canvas, roundedBox(corner(margin, margin), corner(margin + size, margin + size), 2.0f),
tinted(stoneStyle(StonePalette::Rim, seed), settings.faceTint), &cracks, 2.0f, 0.7f);
const float border = 4.0f;
paintSunken(canvas, roundedBox(corner(margin + border, margin + border),
  corner(margin + size - border, margin + size - border), 1.0f),
tinted(stoneStyle(StonePalette::Slot, seed + 1u), settings.faceTint), &cracks, 2.6f, 0.9f);
paintGem(canvas, corner(margin + size * 0.5f, margin + size * 0.5f), size * 0.20f,
 size * 0.26f, seed + 2u);
return canvas;
}

/// @brief One generated element waiting to be packed.
struct PendingElement {
GuiAtlasPart part = GuiAtlasPart::Panel;
Gfx::Rgba8Image image;
};

/// @brief Alpha-composites a shaded element into the atlas.
void blitImage(Gfx::Rgba8Image& atlas, const Gfx::Rgba8Image& source, std::uint32_t x, std::uint32_t y)
{
for (std::uint32_t sy = 0; sy < source.height; ++sy) {
const std::uint32_t ty = y + sy;
if (ty >= atlas.height) break;
for (std::uint32_t sx = 0; sx < source.width; ++sx) {
const std::uint32_t tx = x + sx;
if (tx >= atlas.width) break;
const std::size_t sourceIndex = (std::size_t(sy) * source.width + sx) * 4u;
const std::size_t targetIndex = (std::size_t(ty) * atlas.width + tx) * 4u;
const float alpha = static_cast<float>(source.pixels[sourceIndex + 3u]) / 255.0f;
for (std::uint32_t channel = 0; channel < 3; ++channel) {
const float from = static_cast<float>(source.pixels[sourceIndex + channel]);
const float to = static_cast<float>(atlas.pixels[targetIndex + channel]);
atlas.pixels[targetIndex + channel] =
static_cast<std::uint8_t>(from * alpha + to * (1.0f - alpha) + 0.5f);
}
const float targetAlpha = static_cast<float>(atlas.pixels[targetIndex + 3u]) / 255.0f;
atlas.pixels[targetIndex + 3u] =
static_cast<std::uint8_t>((alpha + targetAlpha * (1.0f - alpha)) * 255.0f + 0.5f);
}
}
}

} // namespace

const char* guiAtlasPartName(GuiAtlasPart part) noexcept
{
const auto index = static_cast<std::size_t>(part);
return index < PartNames.size() ? PartNames[index] : "";
}

bool parseGuiAtlasPart(std::string_view name, GuiAtlasPart& part) noexcept
{
for (std::size_t index = 0; index < PartNames.size(); ++index) {
if (name == PartNames[index]) {
part = static_cast<GuiAtlasPart>(index);
return true;
}
}
return false;
}

const GuiAtlasRect* GuiAtlas::find(GuiAtlasPart part) const noexcept
{
for (const GuiAtlasRect& rectangle : parts) {
if (rectangle.part == part) return &rectangle;
}
return nullptr;
}

GuiAtlas buildGuiAtlas(const GuiAtlasSettings& settings)
{
GuiAtlas atlas;
if (settings.atlasWidth == 0) return atlas;

std::vector<PendingElement> elements;
elements.reserve(GuiAtlasPartCount);
const auto add = [&elements](GuiAtlasPart part, SurfaceCanvas&& canvas) {
PendingElement element;
element.part = part;
element.image = canvas.shade();
elements.push_back(std::move(element));
};

// Every recipe seeds itself from the caller's seed plus a fixed offset, so the
	// same @ref GuiAtlasSettings always produces the same atlas byte for byte.
	const std::uint32_t seed = settings.seed;
	add(GuiAtlasPart::Panel, paintPanel(settings, seed));
	add(GuiAtlasPart::ButtonIdle, paintButton(settings, seed + 1u, 0));
	add(GuiAtlasPart::ButtonHover, paintButton(settings, seed + 2u, 1));
	add(GuiAtlasPart::ButtonPressed, paintButton(settings, seed + 3u, 2));
	add(GuiAtlasPart::ButtonDisabled, paintButton(settings, seed + 4u, 3));
	add(GuiAtlasPart::Slot, paintSlot(settings, seed + 5u));
	add(GuiAtlasPart::TitlePlate, paintTitlePlate(settings, seed + 6u));
	add(GuiAtlasPart::CheckboxEmpty, paintCheckbox(settings, seed + 7u, false));
	add(GuiAtlasPart::CheckboxChecked, paintCheckbox(settings, seed + 8u, true));
	add(GuiAtlasPart::SliderTrack, paintSliderTrack(settings, seed + 9u));
	add(GuiAtlasPart::SliderThumb, paintSliderThumb(settings, seed + 10u));
	add(GuiAtlasPart::Parchment, paintParchment(settings, seed + 11u));
	add(GuiAtlasPart::Bar, paintBar(settings, seed + 12u));
	add(GuiAtlasPart::RadioEmpty, paintRadio(settings, seed + 13u, false));
	add(GuiAtlasPart::RadioChecked, paintRadio(settings, seed + 14u, true));
	add(GuiAtlasPart::ArrowUp, paintArrow(settings, seed + 15u, true));
	add(GuiAtlasPart::ArrowDown, paintArrow(settings, seed + 16u, false));
	add(GuiAtlasPart::TriDown, paintTriangle(settings, seed + 17u));
	add(GuiAtlasPart::ScrollTrack, paintScrollTrack(settings, seed + 18u));
	add(GuiAtlasPart::ScrollThumb, paintScrollThumb(settings, seed + 19u));
	add(GuiAtlasPart::TabActive, paintTab(settings, seed + 20u, true));
	add(GuiAtlasPart::TabInactive, paintTab(settings, seed + 21u, false));
	add(GuiAtlasPart::FrameText, paintFrameText(settings, seed + 22u));
	add(GuiAtlasPart::SlotGroup, paintSlotGroup(settings, seed + 23u));
	add(GuiAtlasPart::BorderStrip, paintBorderStrip(settings, seed + 24u));
	add(GuiAtlasPart::GemTile, paintGemTile(settings, seed + 25u));

	// Shelf packing: tallest first keeps the rows tight, and the builder grows the
	// height instead of requiring a hand-tuned sheet size.
	std::stable_sort(elements.begin(), elements.end(),
					 [](const PendingElement& left, const PendingElement& right) {
						 return left.image.height > right.image.height;
					 });
	std::vector<GuiAtlasRect> rectangles(elements.size());
	std::uint32_t cursorX = 0;
	std::uint32_t cursorY = 0;
	std::uint32_t rowHeight = 0;
	for (std::size_t index = 0; index < elements.size(); ++index) {
		const Gfx::Rgba8Image& image = elements[index].image;
		if (cursorX != 0 && cursorX + image.width > settings.atlasWidth) {
			cursorX = 0;
			cursorY += rowHeight + settings.gap;
			rowHeight = 0;
		}
		GuiAtlasRect& rectangle = rectangles[index];
		rectangle.part = elements[index].part;
		rectangle.x = cursorX;
		rectangle.y = cursorY;
		rectangle.width = image.width;
		rectangle.height = image.height;
		cursorX += image.width + settings.gap;
		rowHeight = std::max(rowHeight, static_cast<std::uint32_t>(image.height));
	}
	const std::uint32_t atlasHeight = cursorY + rowHeight;
	if (atlasHeight == 0 || atlasHeight > 0xFFFFu) return atlas;

	atlas.image.width = static_cast<std::uint16_t>(settings.atlasWidth);
	atlas.image.height = static_cast<std::uint16_t>(atlasHeight);
	atlas.image.pixels.assign(std::size_t(settings.atlasWidth) * atlasHeight * 4u, 0u);
	for (std::size_t index = 0; index < elements.size(); ++index) {
		blitImage(atlas.image, elements[index].image, rectangles[index].x, rectangles[index].y);
	}
	atlas.parts = std::move(rectangles);
	return atlas;
}

} // namespace Procedural
} // namespace Lotte
