#include "LteSurfaceCanvas.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

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

/// @brief Separable box blur of a single-channel field.
void boxBlur(std::vector<float>& field, int width, int height, int radius)
{
	std::vector<float> temporary(field.size());
	const int diameter = radius * 2 + 1;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			float sum = 0.0f;
			for (int k = -radius; k <= radius; ++k) {
				sum += field[std::size_t(y) * width + std::clamp(x + k, 0, width - 1)];
			}
			temporary[std::size_t(y) * width + x] = sum / static_cast<float>(diameter);
		}
	}
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			float sum = 0.0f;
			for (int k = -radius; k <= radius; ++k) {
				sum += temporary[std::size_t(std::clamp(y + k, 0, height - 1)) * width + x];
			}
			field[std::size_t(y) * width + x] = sum / static_cast<float>(diameter);
		}
	}
}

/// @brief Converts a `[0, 1]` value to an 8-bit channel.
std::uint8_t toByte(float value)
{
	return static_cast<std::uint8_t>(clamp01(value) * 255.0f + 0.5f);
}

} // namespace

SurfaceCanvas::SurfaceCanvas(std::uint32_t width, std::uint32_t height)
: widthValue(width), heightValue(height), texels(std::size_t(width) * height)
{
}

void SurfaceCanvas::paint(const SdfFunction& shape, const SurfaceMaterial& material,
const PaintOptions& options)
{
	if (!shape || !material) return;
	for (std::uint32_t y = 0; y < heightValue; ++y) {
		for (std::uint32_t x = 0; x < widthValue; ++x) {
			const glm::fvec2 position(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
			const float distance = shape(position);
			if (distance >= 0.5f) continue;
			const float coverage = clamp01(0.5f - distance);
			const MaterialSample sample = material(position, distance);
			// A chamfered bevel: the height ramps up over the first `bevel` pixels inside
			// the edge, which is what reads as a carved face instead of a flat cut-out.
			const float depth = std::max(-distance, 0.0f);
			const float delta = options.sign * options.slope * std::min(depth, options.bevel) +
			options.lift;
			SurfacePixel& pixel = at(x, y);
			const float newAlpha = pixel.alpha + coverage * (1.0f - pixel.alpha);
			const float oldWeight = pixel.alpha * (1.0f - coverage) / std::max(newAlpha, 1.0e-5f);
			const float newWeight = coverage / std::max(newAlpha, 1.0e-5f);
			pixel.albedo = pixel.albedo * oldWeight + sample.albedo * newWeight;
			pixel.alpha = newAlpha;
			pixel.height += coverage * delta;
			pixel.micro = lerp(pixel.micro, sample.micro, coverage);
			pixel.metal = lerp(pixel.metal, sample.metal, coverage);
			pixel.shine = lerp(pixel.shine, sample.shine, coverage);
			pixel.gloss = lerp(pixel.gloss, sample.gloss, coverage);
			// A new layer occludes whatever glow was underneath it.
			pixel.emissive *= (1.0f - coverage);
		}
	}
}

void SurfaceCanvas::glow(const SdfFunction& shape, glm::fvec3 colour, float radius, float intensity)
{
	if (!shape || radius <= 0.0f) return;
	for (std::uint32_t y = 0; y < heightValue; ++y) {
		for (std::uint32_t x = 0; x < widthValue; ++x) {
			const glm::fvec2 position(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
			const float distance = shape(position);
			const float energy = intensity * std::exp(-std::max(distance, 0.0f) / radius);
			if (energy < 0.004f) continue;
			at(x, y).emissive += colour * energy;
		}
	}
}

void SurfaceCanvas::blit(const SurfaceCanvas& source, glm::ivec2 destination)
{
	for (std::uint32_t sourceY = 0; sourceY < source.height(); ++sourceY) {
		for (std::uint32_t sourceX = 0; sourceX < source.width(); ++sourceX) {
			const std::int32_t targetX = destination.x + static_cast<std::int32_t>(sourceX);
			const std::int32_t targetY = destination.y + static_cast<std::int32_t>(sourceY);
			if (targetX < 0 || targetY < 0 || targetX >= static_cast<std::int32_t>(widthValue) ||
			targetY >= static_cast<std::int32_t>(heightValue)) {
				continue;
			}
			const SurfacePixel& from = source.at(sourceX, sourceY);
			SurfacePixel& into = at(static_cast<std::uint32_t>(targetX),
			static_cast<std::uint32_t>(targetY));
			const float weight = from.alpha;
			into.albedo = from.albedo * weight + into.albedo * (1.0f - weight);
			into.height = from.height * weight + into.height * (1.0f - weight);
			into.micro = from.micro * weight + into.micro * (1.0f - weight);
			into.metal = from.metal * weight + into.metal * (1.0f - weight);
			into.shine = from.shine * weight + into.shine * (1.0f - weight);
			into.gloss = from.gloss * weight + into.gloss * (1.0f - weight);
			into.emissive = from.emissive + into.emissive * (1.0f - weight);
			into.alpha = 1.0f - (1.0f - into.alpha) * (1.0f - from.alpha);
		}
	}
}

Gfx::Rgba8Image SurfaceCanvas::shade(const ShadeOptions& options) const
{
	Gfx::Rgba8Image image;
	if (!isValid()) return image;
	const int width = static_cast<int>(widthValue);
	const int height = static_cast<int>(heightValue);
	const std::size_t count = texels.size();

	// The normal comes from the height field, so the height field is what gets blurred
	// for the ambient-occlusion term: a pixel lower than its neighbourhood sits in a pit.
	std::vector<float> heightField(count);
	for (std::size_t index = 0; index < count; ++index) {
		heightField[index] = texels[index].height + texels[index].micro * options.microScale;
	}
	std::vector<float> blurred = heightField;
	boxBlur(blurred, width, height, 5);
	boxBlur(blurred, width, height, 5);

	// Drop shadow: the alpha channel stamped slightly up-left, then blurred.
	std::vector<float> shadow;
	if (options.dropShadow) {
		shadow.assign(count, 0.0f);
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				const int sourceX = x + options.shadowOffset.x;
				const int sourceY = y + options.shadowOffset.y;
				if (sourceX < 0 || sourceY < 0 || sourceX >= width || sourceY >= height) continue;
				shadow[std::size_t(y) * width + x] =
				texels[std::size_t(sourceY) * width + sourceX].alpha;
			}
		}
		boxBlur(shadow, width, height, 2);
		boxBlur(shadow, width, height, 2);
	}

	const glm::fvec3 light = glm::normalize(options.lightDirection);
	const glm::fvec3 half = glm::normalize(light + glm::fvec3(0.0f, 0.0f, 1.0f));
	image.width = static_cast<std::uint16_t>(widthValue);
	image.height = static_cast<std::uint16_t>(heightValue);
	image.pixels.assign(count * 4u, 0u);

	const auto sampleHeight = [&](int x, int y) {
		return heightField[std::size_t(std::clamp(y, 0, height - 1)) * width +
		std::clamp(x, 0, width - 1)];
	};

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			const std::size_t index = std::size_t(y) * width + x;
			const SurfacePixel& pixel = texels[index];
			// Central differences give the slope; the z component stays 1 because the
			// height field is in pixel units, not a physical surface.
			glm::fvec3 normal(sampleHeight(x - 1, y) - sampleHeight(x + 1, y),
			sampleHeight(x, y - 1) - sampleHeight(x, y + 1), 2.0f);
			normal = glm::normalize(normal) * 0.5f + glm::fvec3(0.0f, 0.0f, 0.5f);
			normal = glm::normalize(normal);

			const float diffuse = std::max(0.0f, glm::dot(normal, light));
			const float lit = options.ambient + (1.0f - options.ambient) *
			(diffuse / std::max(light.z, 1.0e-4f));
			const float occlusion = 1.0f - clamp01((blurred[index] - heightField[index]) * 0.11f);
			float specular = 0.0f;
			if (pixel.metal > 0.01f || pixel.shine > 0.01f) {
				specular = std::pow(std::max(0.0f, glm::dot(normal, half)), pixel.gloss);
			}

			glm::fvec3 colour = pixel.albedo * (lit * occlusion);
			if (pixel.metal > 0.01f) {
				const glm::fvec3 metalColour =
				pixel.albedo * (std::pow(std::min(lit, 1.8f), 1.8f) * 1.08f * occlusion) +
				glm::fvec3(1.0f, 0.90f, 0.62f) * (specular * 0.75f);
				colour = glm::mix(colour, metalColour, pixel.metal);
			}
			colour += glm::fvec3(1.0f) * (specular * pixel.shine * 0.85f);

			const float emissive = std::min(1.0f, std::max({ pixel.emissive.r, pixel.emissive.g,
			pixel.emissive.b }));
			const float alpha = pixel.alpha;
			const float shadowAlpha = options.dropShadow ? shadow[index] * options.shadowAlpha : 0.0f;
			const glm::fvec3 premultiplied = colour * alpha + pixel.emissive;
			const float coverage = 1.0f - (1.0f - alpha) * (1.0f - emissive);
			const float finalAlpha = coverage + (1.0f - coverage) * shadowAlpha;
			if (finalAlpha < 1.0e-4f) continue;
			image.pixels[index * 4u + 0u] = toByte(premultiplied.r / finalAlpha);
			image.pixels[index * 4u + 1u] = toByte(premultiplied.g / finalAlpha);
			image.pixels[index * 4u + 2u] = toByte(premultiplied.b / finalAlpha);
			image.pixels[index * 4u + 3u] = toByte(finalAlpha);
		}
	}
	return image;
}

Euph::Media::Image::DecodeTarget toDecodeTarget(const Gfx::Rgba8Image& image)
{
	Euph::Media::Image::DecodeTarget target;
	if (!image.isValid()) return target;
	target.setFormat(Euph::Media::Image::Format::RGBA8U);
	Euph::Media::Image::Frame& frame = target.addFrame(image.width, image.height);
	frame.stride = static_cast<unsigned>(image.rowBytes());
	frame.data.assign(reinterpret_cast<const std::byte*>(image.pixels.data()),
	reinterpret_cast<const std::byte*>(image.pixels.data()) + image.pixels.size());
	return target;
}

} // namespace Procedural
} // namespace Lotte
