#include "LteGuiMaterials.hpp"
#include <Euphemy/Util/EuphNoise.hpp>
#include <Euphemy/Util/EuphVoronoi.hpp>
#include <algorithm>
#include <cmath>

namespace Lotte {
namespace Procedural {

namespace {

/// @brief Clamps a float into `[0, 1]`.
float clamp01(float value)
{
return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
}

/// @brief Linear interpolation.
float lerp(float a, float b, float t)
{
return a + (b - a) * t;
}

/// @brief Hermite smoothstep between two edges.
float smooth(float edge0, float edge1, float value)
{
const float t = clamp01((value - edge0) / (edge1 - edge0));
return t * t * (3.0f - 2.0f * t);
}

/// @brief Linear RGB mix.
glm::fvec3 mix(glm::fvec3 a, glm::fvec3 b, float t)
{
return a + (b - a) * t;
}

/// @brief Shortcut for `noise::latticeFbm2D` with a seed offset.
float fbm(glm::fvec2 position, std::uint32_t seed, std::uint32_t octaves = 4)
{
return Euph::Util::Noise::latticeFbm2D(position.x, position.y, seed, octaves);
}

} // namespace

float GuiField::sample(glm::fvec2 position) const noexcept
{
if (!isValid()) return 0.0f;
const float maxX = static_cast<float>(width - 1);
const float maxY = static_cast<float>(height - 1);
const float x = std::clamp(position.x, 0.0f, maxX);
const float y = std::clamp(position.y, 0.0f, maxY);
const auto x0 = static_cast<std::uint32_t>(x);
const auto y0 = static_cast<std::uint32_t>(y);
const std::uint32_t x1 = std::min(x0 + 1, width - 1);
const std::uint32_t y1 = std::min(y0 + 1, height - 1);
const float tx = x - static_cast<float>(x0);
const float ty = y - static_cast<float>(y0);
const float top = lerp(samples[std::size_t(y0) * width + x0], samples[std::size_t(y0) * width + x1], tx);
const float bottom = lerp(samples[std::size_t(y1) * width + x0], samples[std::size_t(y1) * width + x1], tx);
return lerp(top, bottom, ty);
}

GuiStoneStyle stoneStyle(StonePalette palette, std::uint32_t seed)
{
GuiStoneStyle style;
switch (palette) {
case StonePalette::Deep:
style.dark = glm::fvec3(0.141f, 0.208f, 0.416f);
style.mid = glm::fvec3(0.239f, 0.341f, 0.608f);
style.light = glm::fvec3(0.424f, 0.518f, 0.761f);
style.frost = glm::fvec3(0.627f, 0.698f, 0.886f);
style.angle = -0.45f;
break;
case StonePalette::Navy:
style.dark = glm::fvec3(0.039f, 0.047f, 0.086f);
style.mid = glm::fvec3(0.102f, 0.125f, 0.204f);
style.light = glm::fvec3(0.200f, 0.235f, 0.369f);
style.frost = glm::fvec3(0.357f, 0.396f, 0.612f);
style.crack = 0.5f;
break;
case StonePalette::Void:
style.dark = glm::fvec3(0.004f, 0.008f, 0.024f);
style.mid = glm::fvec3(0.027f, 0.035f, 0.067f);
style.light = glm::fvec3(0.090f, 0.106f, 0.176f);
style.frost = glm::fvec3(0.173f, 0.200f, 0.329f);
style.crack = 0.3f;
style.wear = 0.4f;
break;
case StonePalette::Slot:
style.dark = glm::fvec3(0.078f, 0.078f, 0.165f);
style.mid = glm::fvec3(0.169f, 0.169f, 0.278f);
style.light = glm::fvec3(0.322f, 0.318f, 0.486f);
style.frost = glm::fvec3(0.561f, 0.576f, 0.741f);
style.wear = 0.3f;
break;
case StonePalette::Rim:
style.dark = glm::fvec3(0.565f, 0.592f, 0.749f);
style.mid = glm::fvec3(0.706f, 0.729f, 0.871f);
style.light = glm::fvec3(0.851f, 0.867f, 0.949f);
style.frost = glm::fvec3(0.945f, 0.953f, 0.984f);
style.wearWidth = 5.0f;
style.crack = 0.3f;
break;
case StonePalette::Grey:
style.dark = glm::fvec3(0.373f, 0.376f, 0.400f);
style.mid = glm::fvec3(0.541f, 0.545f, 0.569f);
style.light = glm::fvec3(0.710f, 0.714f, 0.733f);
style.frost = glm::fvec3(0.827f, 0.831f, 0.847f);
style.angle = -0.7f;
style.crack = 0.8f;
break;
case StonePalette::Steel:
style.dark = glm::fvec3(0.114f, 0.125f, 0.149f);
style.mid = glm::fvec3(0.231f, 0.247f, 0.286f);
style.light = glm::fvec3(0.392f, 0.416f, 0.467f);
style.frost = glm::fvec3(0.573f, 0.596f, 0.647f);
style.crack = 0.2f;
style.wearWidth = 2.0f;
break;
case StonePalette::Blue:
default:
break;
}
style.seed = seed;
return style;
}

GuiStoneStyle dimStone(GuiStoneStyle style, float factor)
{
style.dark *= factor;
style.mid *= factor;
style.light *= factor;
style.frost *= factor;
return style;
}

GuiField makeVoronoiCrackField(std::uint32_t width, std::uint32_t height,
std::uint32_t tileSize, std::uint32_t seed)
{
GuiField field;
if (width == 0 || height == 0 || tileSize == 0) return field;
field.width = width;
field.height = height;
field.samples.resize(std::size_t(width) * height);
Euph::Util::Noise::VoronoiSettings settings;
settings.blockSizeX = tileSize;
settings.blockSizeY = tileSize;
settings.blockCountX = (width + tileSize - 1) / tileSize;
settings.blockCountY = (height + tileSize - 1) / tileSize;
settings.normalize = true;
std::vector<float> border(std::size_t(settings.blockCountX) * settings.blockSizeX *
settings.blockCountY * settings.blockSizeY);
Euph::Util::Noise::voronoiBorder(std::span<float>(border), settings, seed);
// The Voronoi grid is rounded up to whole tiles, so copy the region that exists.
for (std::uint32_t y = 0; y < height; ++y) {
for (std::uint32_t x = 0; x < width; ++x) {
field.samples[std::size_t(y) * width + x] = border[std::size_t(y) * settings.blockCountX *
settings.blockSizeX + x];
}
}
return field;
}

SurfaceMaterial stoneMaterial(const GuiStoneStyle& style, const GuiField* cracks)
{
return [style, cracks](glm::fvec2 position, float distance) -> MaterialSample {
const float cosine = std::cos(style.angle);
const float sine = std::sin(style.angle);
// Rotate the streak coordinates so the chisel marks run in one direction.
const float u = position.x * cosine + position.y * sine;
const float v = -position.x * sine + position.y * cosine;
const float scale = style.scale;

const float coarse = fbm(position * (0.018f * scale), style.seed, 4);
const float strokes = fbm(glm::fvec2(u * 0.030f * scale, v * 0.30f * scale), style.seed + 17u, 4);
const float fineStrokes = fbm(glm::fvec2(u * 0.085f * scale, v * 0.65f * scale), style.seed + 31u, 3);

// Fissures: the Voronoi border mask when the recipe supplied one, otherwise the
// hash noise's own high-frequency component. Gating with a zone mask keeps them
		// sparse instead of covering the face with a crack net.
		float crack = 0.0f;
		if (cracks != nullptr) {
			const float zone = smooth(0.52f, 0.74f, fbm(position * 0.040f, style.seed + 233u, 3));
			crack = (1.0f - smooth(0.0f, 0.055f, cracks->sample(position))) * zone;
		} else {
			const float zone = smooth(0.52f, 0.74f, fbm(position * 0.040f, style.seed + 233u, 3));
			const float ridge = std::abs(fbm(position * 0.05f, style.seed + 5u, 3) - 0.5f) * 2.0f;
			crack = (1.0f - smooth(0.0f, 0.30f, ridge)) * zone;
		}

		const float pits = smooth(0.66f, 0.82f, fbm(position * 0.11f, style.seed + 77u, 3));
		const float grain = Euph::Util::Noise::latticeNoise2D(position.x * 0.85f, position.y * 0.85f,
style.seed + 9u);
		float tone = 0.28f * coarse + 0.44f * strokes * style.streak + 0.17f * fineStrokes + 0.11f * grain;
		const float bias = 0.5f * (0.28f + 0.44f * style.streak + 0.28f);
		tone = clamp01((tone - bias) * 2.8f + 0.5f);
		glm::fvec3 colour = tone < 0.5f ? mix(style.dark, style.mid, tone * 2.0f)
										: mix(style.mid, style.light, (tone - 0.5f) * 2.0f);
		colour *= (1.0f - style.crack * 0.62f * crack) * (1.0f - 0.22f * pits);

		// Chipped, frosted edge wear: irregular so it reads as stone rather than a rim.
		const float wearNoise = fbm(position * 0.24f, style.seed + 55u, 3);
		const float wearMask = (1.0f - smooth(0.0f, style.wearWidth, -distance)) *
							   smooth(0.30f, 0.60f, wearNoise + 0.3f * (1.0f - smooth(0.0f, 1.6f, -distance)));
		colour = mix(colour, style.frost, clamp01(wearMask * style.wear));

		MaterialSample sample;
		sample.albedo = colour;
		sample.micro = (0.50f * strokes + 0.25f * fineStrokes + 0.25f * grain - 0.5f) -
					   0.7f * style.crack * crack - 0.25f * pits;
		sample.micro *= style.microAmplitude;
		return sample;
	};
}

SurfaceMaterial goldMaterial(std::uint32_t seed, float brightness)
{
	return [seed, brightness](glm::fvec2 position, float) -> MaterialSample {
		const float noise = fbm(position * 0.2f, seed, 3);
		MaterialSample sample;
		sample.albedo = mix(glm::fvec3(0.486f, 0.290f, 0.063f), glm::fvec3(0.890f, 0.675f, 0.251f),
clamp01(0.30f + noise * 0.85f)) * brightness;
		sample.micro = noise * 0.12f;
		sample.metal = 1.0f;
		sample.gloss = 26.0f;
		return sample;
	};
}

SurfaceMaterial flatMaterial(glm::fvec3 colour, float metal, float shine, float gloss)
{
	return [colour, metal, shine, gloss](glm::fvec2, float) -> MaterialSample {
		MaterialSample sample;
		sample.albedo = colour;
		sample.metal = metal;
		sample.shine = shine;
		sample.gloss = gloss;
		return sample;
	};
}

SurfaceMaterial gemMaterial(glm::fvec3 deep, glm::fvec3 bright, float depth)
{
	return [deep, bright, depth](glm::fvec2, float distance) -> MaterialSample {
		MaterialSample sample;
		const float t = depth > 0.0f ? clamp01(-distance / depth) : 1.0f;
		sample.albedo = mix(deep, bright, t);
		sample.shine = 0.95f;
		sample.gloss = 46.0f;
		return sample;
	};
}

SurfaceMaterial parchmentMaterial(std::uint32_t seed)
{
	return [seed](glm::fvec2 position, float distance) -> MaterialSample {
		const float mottle = fbm(position * 0.03f, seed + 1u, 4);
		const float fibreA = fbm(glm::fvec2(position.x * 0.55f, position.y * 0.05f), seed + 2u, 3);
		const float fibreB = fbm(glm::fvec2(position.x * 0.05f, position.y * 0.5f), seed + 12u, 3);
		glm::fvec3 colour = mix(glm::fvec3(0.839f, 0.612f, 0.369f),
glm::fvec3(0.922f, 0.741f, 0.486f), mottle);
		colour *= 0.965f + 0.06f * (fibreA * 0.4f + fibreB * 0.6f);
		const float edge = 1.0f - smooth(0.0f, 30.0f, -distance);
		const float burn = edge * (0.4f + 0.6f * fbm(position * 0.09f, seed + 9u, 3));
		colour = mix(colour, glm::fvec3(0.576f, 0.349f, 0.165f), burn * 0.6f);
		const float stain = smooth(0.62f, 0.8f, fbm(position * 0.02f, seed + 21u, 3));
		colour = mix(colour, glm::fvec3(0.690f, 0.478f, 0.267f), stain * 0.5f);
		const float blot = smooth(0.55f, 0.75f, fbm(position * 0.06f, seed + 41u, 3));
		colour = mix(colour, glm::fvec3(0.788f, 0.561f, 0.333f), blot * 0.35f);

		MaterialSample sample;
		sample.albedo = colour;
		sample.micro = (fibreA - 0.5f) * 0.5f;
		return sample;
	};
}

} // namespace Procedural
} // namespace Lotte
