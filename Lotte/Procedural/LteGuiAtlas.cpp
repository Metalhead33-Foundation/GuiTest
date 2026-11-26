#include "LteGuiAtlas.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace Lotte {
namespace Procedural {

namespace {

/// @brief Names in the same order as the @ref GuiAtlasPart enumerators.
constexpr std::array<const char*, GuiAtlasPartCount> PartNames = {
"panel",  "button_idle", "button_hover", "button_pressed", "button_disabled",
"slot",   "title_plate",  "checkbox_empty", "checkbox_checked", "slider_track",
"slider_thumb", "parchment", "bar"
};

/// @brief Pointer to a rectangle's top-left corner.
glm::fvec2 corner(float x, float y)
{
	return glm::fvec2(x, y);
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
	paintRaised(canvas, body, stoneStyle(StonePalette::Blue, seed), &cracks, 3.6f, 0.85f);
	paintSunken(canvas, roundedBox(corner(margin + 14.0f, margin + 14.0f),
corner(margin + width - 14.0f, margin + height - 14.0f), 3.0f),
stoneStyle(StonePalette::Void, seed + 2u), &cracks, 2.4f, 0.7f);
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

	GuiStoneStyle style = stoneStyle(StonePalette::Grey, seed);
	if (state == 1) style = stoneStyle(StonePalette::Rim, seed + 2u);
	if (state == 3) style = dimStone(style, 0.58f);
	style.mid = state == 1 ? glm::fvec3(0.659f, 0.663f, 0.706f) : style.mid;
	style.dark = state == 1 ? glm::fvec3(0.486f, 0.490f, 0.533f) : style.dark;

	const SdfFunction body = roughen(
roundedBox(corner(margin, margin), corner(margin + width, margin + height), 4.0f), 0.9f,
0.15f, seed);
	if (state == 2) {
		paintSunken(canvas, body, dimStone(style, 0.8f), &cracks, 3.6f, 0.85f);
	} else {
		paintRaised(canvas, body, style, &cracks, 4.0f, 0.9f);
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
stoneStyle(StonePalette::Rim, seed), &cracks, 1.5f, 0.6f);
	const float rim = 3.6f;
	paintSunken(canvas, roundedBox(corner(margin + rim, margin + rim),
corner(margin + size - rim, margin + size - rim), 1.0f),
stoneStyle(StonePalette::Slot, seed + 1u), &cracks, 3.2f, 0.9f);
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
paintRaised(canvas, body, stoneStyle(StonePalette::Deep, seed), &cracks, 3.6f, 0.85f);

const SdfFunction field = roundedBox(corner(margin + 14.0f, margin + height * 0.27f),
corner(margin + width - 14.0f, margin + height * 0.73f), 2.0f);
paintSunken(canvas, field, dimStone(stoneStyle(StonePalette::Deep, seed + 2u), 0.55f), &cracks,
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
paintRaised(canvas, frame, stoneStyle(StonePalette::Steel, seed), &cracks, 2.6f, 0.8f);
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
stoneStyle(StonePalette::Deep, seed), &cracks, 3.0f, 0.8f);
const float grooveTop = margin + height * 0.34f;
const float grooveBottom = margin + height * 0.66f;
const SdfFunction groove = roundedBox(corner(margin + 16.0f, grooveTop),
corner(margin + width - 16.0f, grooveBottom),
(grooveBottom - grooveTop) * 0.5f);
paintSunken(canvas, groove, stoneStyle(StonePalette::Void, seed + 1u), &cracks, 2.2f, 0.7f);
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
// Torn edge: two octaves of noise displace the outline, which is what makes the
// silhouette irregular instead of a rectangle.
const SdfFunction torn = [base, seed](glm::fvec2 position) {
const float macro = Euph::Util::Noise::latticeFbm2D(position.x * 0.07f, position.y * 0.07f,
seed, 4) - 0.5f;
const float micro = Euph::Util::Noise::latticeFbm2D(position.x * 0.45f, position.y * 0.45f,
seed + 3u, 2) - 0.5f;
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
stoneStyle(StonePalette::Blue, seed), &cracks, 3.0f, 0.85f);
paintSunken(canvas, roundedBox(corner(margin + 5.0f, margin + 5.0f),
corner(margin + width - 5.0f, margin + height - 5.0f), 1.0f),
stoneStyle(StonePalette::Void, seed + 1u), &cracks, 3.6f, 0.95f);
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
