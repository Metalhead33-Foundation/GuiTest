#include "EuphNoise.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>

namespace Euph {
namespace Util {
namespace Noise {

namespace {

/// @brief Pi as a float, so the easings stay in single precision.
constexpr float Pi = 3.14159265358979323846f;

/// @brief Floors towards negative infinity (the legacy `fastfloor`).
int fastFloor(float x)
{
	return x > 0.0f ? static_cast<int>(x) : static_cast<int>(x) - 1;
}

/// @brief Linear interpolation between two values.
float lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

/// @brief Applies one of the three lattice easings to a `[0, 1]` factor.
float ease(float t, Interpolation interpolation)
{
	switch (interpolation) {
		case Interpolation::Cosine:
			return (1.0f - std::cos(t * Pi)) * 0.5f;
		case Interpolation::SmoothStep:
			return t * t * (3.0f - 2.0f * t);
		case Interpolation::Linear:
		default:
			return t;
	}
}

/// @brief Throws when a destination span cannot hold the requested extent.
void requireExtent(std::size_t available, std::uint64_t needed)
{
	if (available < needed) {
		throw std::invalid_argument("noise: the destination span is smaller than the requested extent");
	}
}

} // namespace

float bilinearAverage(float tX, float tY, float topLeft, float topRight, float bottomLeft,
					float bottomRight)
{
	const float weightTopLeft = (1.0f - tX) * (1.0f - tY);
	const float weightTopRight = tX * (1.0f - tY);
	const float weightBottomLeft = (1.0f - tX) * tY;
	const float weightBottomRight = tX * tY;
	return topLeft * weightTopLeft + topRight * weightTopRight + bottomLeft * weightBottomLeft +
		bottomRight * weightBottomRight;
}

void normalize(std::span<float> output)
{
	if (output.empty()) return;
	float minimum = output.front();
	float maximum = output.front();
	for (const float value : output) {
		minimum = std::min(minimum, value);
		maximum = std::max(maximum, value);
	}
	const float range = maximum - minimum;
	// A flat span has no range to stretch; leaving it alone beats dividing by zero.
	if (!(range > 0.0f)) return;
	for (float& value : output) {
		value = (value - minimum) / range;
	}
}

void remapToSigned(std::span<float> output)
{
	for (float& value : output) {
		value = (value - 0.5f) * 2.0f;
	}
}

void remapToUnsigned(std::span<float> output)
{
	for (float& value : output) {
		value = value * 0.5f + 0.5f;
	}
}

void whiteNoise(std::span<float> output, std::uint32_t seed)
{
	std::mt19937 rng(seed);
	rngWhiteNoise(output, rng);
}

void valueNoise1D(std::span<float> output, std::span<const float> points,
				Interpolation interpolation)
{
	if (output.empty()) return;
	if (points.empty()) {
		std::fill(output.begin(), output.end(), 0.0f);
		return;
	}
	const float outputSpan = static_cast<float>(output.size() - 1);
	const float pointSpan = static_cast<float>(points.size() - 1);
	for (std::size_t x = 0; x < output.size(); ++x) {
		// A single sample or a single lattice value has nothing to interpolate.
		const float position =
			outputSpan > 0.0f ? (static_cast<float>(x) / outputSpan) * pointSpan : 0.0f;
		const auto lower = static_cast<std::size_t>(std::floor(position));
		const std::size_t upper = std::min(lower + 1, points.size() - 1);
		output[x] = lerp(points[lower], points[upper],
						ease(position - static_cast<float>(lower), interpolation));
	}
}

void valueNoise2D(std::span<float> output, std::uint32_t outWidth, std::uint32_t outHeight,
				std::span<const float> input, std::uint32_t inWidth, std::uint32_t inHeight,
				Interpolation interpolation)
{
	requireExtent(output.size(), std::uint64_t(outWidth) * outHeight);
	if (outWidth == 0 || outHeight == 0) return;
	if (inWidth == 0 || inHeight == 0 || input.size() < std::uint64_t(inWidth) * inHeight) {
		std::fill_n(output.begin(), std::size_t(outWidth) * outHeight, 0.0f);
		return;
	}
	const float outWidthSpan = static_cast<float>(outWidth - 1);
	const float outHeightSpan = static_cast<float>(outHeight - 1);
	const float inWidthSpan = static_cast<float>(inWidth - 1);
	const float inHeightSpan = static_cast<float>(inHeight - 1);
	for (std::uint32_t y = 0; y < outHeight; ++y) {
		float* const row = &output[std::size_t(y) * outWidth];
		const float positionY = outHeightSpan > 0.0f
									? (static_cast<float>(y) / outHeightSpan) * inHeightSpan
									: 0.0f;
		const auto lowerY = static_cast<std::uint32_t>(std::floor(positionY));
		const std::uint32_t upperY = std::min(lowerY + 1, inHeight - 1);
		const float tY = ease(positionY - static_cast<float>(lowerY), interpolation);
		for (std::uint32_t x = 0; x < outWidth; ++x) {
			const float positionX = outWidthSpan > 0.0f
										? (static_cast<float>(x) / outWidthSpan) * inWidthSpan
										: 0.0f;
			const auto lowerX = static_cast<std::uint32_t>(std::floor(positionX));
			const std::uint32_t upperX = std::min(lowerX + 1, inWidth - 1);
			const float tX = ease(positionX - static_cast<float>(lowerX), interpolation);
			// Clamping the upper index is what keeps the right/bottom edge inside the
			// lattice; the legacy code read one past it.
			row[x] = bilinearAverage(tX, tY, input[std::size_t(lowerY) * inWidth + lowerX],
									input[std::size_t(lowerY) * inWidth + upperX],
									input[std::size_t(upperY) * inWidth + lowerX],
									input[std::size_t(upperY) * inWidth + upperX]);
		}
	}
}

void valueNoise1D(std::span<float> output, std::uint32_t seed, std::uint32_t pointCount,
				Interpolation interpolation)
{
	std::vector<float> lattice(pointCount);
	whiteNoise(std::span<float>(lattice), seed);
	valueNoise1D(output, std::span<const float>(lattice), interpolation);
}

void valueNoise2D(std::span<float> output, std::uint32_t outWidth, std::uint32_t outHeight,
				std::uint32_t seed, std::uint32_t inWidth, std::uint32_t inHeight,
				Interpolation interpolation)
{
	std::vector<float> lattice(std::size_t(inWidth) * inHeight);
	whiteNoise(std::span<float>(lattice), seed);
	valueNoise2D(output, outWidth, outHeight, std::span<const float>(lattice), inWidth, inHeight,
				interpolation);
}

float perlin1D(std::span<const float> seed, std::uint32_t x, std::uint32_t pitch)
{
	if (seed.empty() || pitch == 0) return 0.0f;
	const auto sampleIndex1 = static_cast<std::uint32_t>((x - (x % pitch)) % seed.size());
	const auto sampleIndex2 = static_cast<std::uint32_t>((sampleIndex1 + pitch) % seed.size());
	const float blend = static_cast<float>(x % pitch) / static_cast<float>(pitch);
	const float fade = blend * blend * (3.0f - 2.0f * blend);
	return lerp(seed[sampleIndex1], seed[sampleIndex2], fade);
}

void perlin1D(std::span<float> output, std::span<const float> seed, std::uint32_t octaves,
			float scaleBias, bool shouldNormalize)
{
	for (std::size_t x = 0; x < output.size(); ++x) {
		float noise = 0.0f;
		float scale = 1.0f;
		float scaleSum = 0.0f;
		for (std::uint32_t octave = 0; octave < octaves; ++octave) {
			const std::uint32_t pitch = static_cast<std::uint32_t>(output.size()) >> octave;
			if (pitch == 0) break;
			noise += perlin1D(seed, static_cast<std::uint32_t>(x), pitch) * scale;
			scaleSum += scale;
			scale /= scaleBias;
		}
		output[x] = scaleSum > 0.0f ? noise / scaleSum : 0.0f;
	}
	if (shouldNormalize) normalize(output);
}

float perlin2D(std::uint32_t width, std::uint32_t height, const float* seed, std::uint32_t x,
			std::uint32_t y, std::uint32_t horizontalPitch, std::uint32_t verticalPitch)
{
	if (seed == nullptr || width == 0 || height == 0 || horizontalPitch == 0 || verticalPitch == 0) {
		return 0.0f;
	}
	const auto sampleIndexX1 = static_cast<std::uint32_t>((x - (x % horizontalPitch)) % width);
	const auto sampleIndexY1 = static_cast<std::uint32_t>((y - (y % verticalPitch)) % height);
	const auto sampleIndexX2 = static_cast<std::uint32_t>((sampleIndexX1 + horizontalPitch) % width);
	const auto sampleIndexY2 = static_cast<std::uint32_t>((sampleIndexY1 + verticalPitch) % height);
	const float blendX = static_cast<float>(x % horizontalPitch) / static_cast<float>(horizontalPitch);
	const float blendY = static_cast<float>(y % verticalPitch) / static_cast<float>(verticalPitch);
	const float fadeX = blendX * blendX * (3.0f - 2.0f * blendX);
	const float fadeY = blendY * blendY * (3.0f - 2.0f * blendY);
	const float sampleTopLeft = seed[std::size_t(sampleIndexY1) * width + sampleIndexX1];
	const float sampleTopRight = seed[std::size_t(sampleIndexY1) * width + sampleIndexX2];
	const float sampleBottomLeft = seed[std::size_t(sampleIndexY2) * width + sampleIndexX1];
	const float sampleBottomRight = seed[std::size_t(sampleIndexY2) * width + sampleIndexX2];
	return bilinearAverage(fadeX, fadeY, sampleTopLeft, sampleTopRight, sampleBottomLeft,
						sampleBottomRight);
}

void perlin2D(std::span<float> output, std::uint32_t width, std::uint32_t height,
			std::span<const float> seed, std::uint32_t octaves, float scaleBias,
			bool shouldNormalize)
{
	requireExtent(output.size(), std::uint64_t(width) * height);
	if (width == 0 || height == 0) return;
	if (seed.size() < std::uint64_t(width) * height) {
		std::fill_n(output.begin(), std::size_t(width) * height, 0.0f);
		return;
	}
	for (std::uint32_t y = 0; y < height; ++y) {
		float* const row = &output[std::size_t(y) * width];
		for (std::uint32_t x = 0; x < width; ++x) {
			float noise = 0.0f;
			float scale = 1.0f;
			float scaleSum = 0.0f;
			for (std::uint32_t octave = 0; octave < octaves; ++octave) {
				const std::uint32_t horizontalPitch = width >> octave;
				const std::uint32_t verticalPitch = height >> octave;
				if (horizontalPitch == 0 || verticalPitch == 0) break;
				noise += perlin2D(width, height, seed.data(), x, y, horizontalPitch, verticalPitch) * scale;
				scaleSum += scale;
				scale /= scaleBias;
			}
			row[x] = scaleSum > 0.0f ? noise / scaleSum : 0.0f;
		}
	}
	if (shouldNormalize) normalize(output.first(std::size_t(width) * height));
}

void perlin1D(std::span<float> output, std::uint32_t seed, std::uint32_t octaves,
			float scaleBias, bool shouldNormalize)
{
	std::vector<float> lattice(output.size());
	whiteNoise(std::span<float>(lattice), seed);
	perlin1D(output, std::span<const float>(lattice), octaves, scaleBias, shouldNormalize);
}

void perlin2D(std::span<float> output, std::uint32_t width, std::uint32_t height,
			std::uint32_t seed, std::uint32_t octaves, float scaleBias,
			bool shouldNormalize)
{
	std::vector<float> lattice(std::size_t(width) * height);
	whiteNoise(std::span<float>(lattice), seed);
	perlin2D(output, width, height, std::span<const float>(lattice), octaves, scaleBias,
			shouldNormalize);
}

void bilinearPerlin1D(std::span<float> output, std::span<const float> seed,
					std::uint32_t octaves, float valA, float valB, float scaleBias,
					float cornerBias)
{
	if (output.empty()) return;
	const float inverseCornerBias = 1.0f - cornerBias;
	const float outputSpan = static_cast<float>(output.size() - 1);
	for (std::size_t x = 0; x < output.size(); ++x) {
		float noise = 0.0f;
		float scale = 1.0f;
		float scaleSum = 0.0f;
		for (std::uint32_t octave = 0; octave < octaves; ++octave) {
			if (octave == 0) {
				const float linearPosition = outputSpan > 0.0f
												? static_cast<float>(x) / outputSpan
												: 0.0f;
				noise += lerp(valA, valB, linearPosition);
				scaleSum += scale;
			} else {
				const std::uint32_t pitch = static_cast<std::uint32_t>(output.size()) >> octave;
				if (pitch == 0) break;
				noise += perlin1D(seed, static_cast<std::uint32_t>(x), pitch) *
						(scale * inverseCornerBias);
				scaleSum += scale * inverseCornerBias;
			}
			scale /= scaleBias;
		}
		output[x] = scaleSum > 0.0f ? noise / scaleSum : 0.0f;
	}
}

void bilinearPerlin2D(std::span<float> output, std::uint32_t width, std::uint32_t height,
					std::span<const float> seed, std::uint32_t octaves, float topLeft,
					float topRight, float bottomLeft, float bottomRight, float scaleBias,
					float cornerBias)
{
	requireExtent(output.size(), std::uint64_t(width) * height);
	if (width == 0 || height == 0) return;
	const float inverseCornerBias = 1.0f - cornerBias;
	for (std::uint32_t y = 0; y < height; ++y) {
		float* const row = &output[std::size_t(y) * width];
		for (std::uint32_t x = 0; x < width; ++x) {
			float noise = 0.0f;
			float scale = 1.0f;
			float scaleSum = 0.0f;
			for (std::uint32_t octave = 0; octave < octaves; ++octave) {
				if (octave == 0) {
					const float scaleX = width > 1 ? static_cast<float>(x) / static_cast<float>(width - 1) : 0.0f;
					const float scaleY = height > 1 ? static_cast<float>(y) / static_cast<float>(height - 1) : 0.0f;
					noise += bilinearAverage(scaleX, scaleY, topLeft, topRight, bottomLeft, bottomRight);
					scaleSum += scale;
				} else {
					const std::uint32_t horizontalPitch = width >> octave;
					const std::uint32_t verticalPitch = height >> octave;
					if (horizontalPitch == 0 || verticalPitch == 0) break;
					noise += perlin2D(width, height, seed.data(), x, y, horizontalPitch,
									verticalPitch) *
							(scale * inverseCornerBias);
					scaleSum += scale * inverseCornerBias;
				}
				scale /= scaleBias;
			}
			row[x] = scaleSum > 0.0f ? noise / scaleSum : 0.0f;
		}
	}
}

void bilinearPerlin1D(std::span<float> output, std::uint32_t seed, std::uint32_t octaves,
					float valA, float valB, float scaleBias, float cornerBias)
{
	std::vector<float> lattice(output.size());
	whiteNoise(std::span<float>(lattice), seed);
	bilinearPerlin1D(output, std::span<const float>(lattice), octaves, valA, valB, scaleBias,
					cornerBias);
}

void bilinearPerlin2D(std::span<float> output, std::uint32_t width, std::uint32_t height,
					std::uint32_t seed, std::uint32_t octaves, float topLeft, float topRight,
					float bottomLeft, float bottomRight, float scaleBias, float cornerBias)
{
	std::vector<float> lattice(std::size_t(width) * height);
	whiteNoise(std::span<float>(lattice), seed);
	bilinearPerlin2D(output, width, height, std::span<const float>(lattice), octaves, topLeft,
					topRight, bottomLeft, bottomRight, scaleBias, cornerBias);
}

SimplexPermutationTable makeSimplexPermutation(std::uint32_t seed)
{
	SimplexPermutationTable table {};
	std::array<std::uint8_t, 256> base {};
	for (std::size_t index = 0; index < base.size(); ++index) {
		base[index] = static_cast<std::uint8_t>(index);
	}
	std::mt19937 rng(seed);
	std::shuffle(base.begin(), base.end(), rng);
	for (std::size_t index = 0; index < table.size(); ++index) {
		table[index] = base[index & 255u];
	}
	return table;
}

float simplexGrad1D(std::uint32_t hash, float x)
{
	const std::uint32_t h = hash & 15u;
	float gradient = 1.0f + static_cast<float>(h & 7u);
	if ((h & 8u) != 0u) gradient = -gradient;
	return gradient * x;
}

float simplexGrad2D(std::uint32_t hash, float x, float y)
{
	const std::uint32_t h = hash & 7u;
	const float u = h < 4u ? x : y;
	const float v = h < 4u ? y : x;
	return ((h & 1u) != 0u ? -u : u) + ((h & 2u) != 0u ? -2.0f * v : 2.0f * v);
}

float simplexNoise1D(const SimplexPermutationTable& perm, float x)
{
	// Gustavson's 1D kernel. The legacy version mixed the 2D skew constants into a
	// 1D expression and returned values in roughly [-4.6, 3.9]; this one keeps
	// `t = 1 - x^2` and the documented 0.395 scale, so the result is in [-1, 1].
	const int i0 = fastFloor(x);
	const int i1 = i0 + 1;
	const float x0 = x - static_cast<float>(i0);
	const float x1 = x0 - 1.0f;

	float n0 = 0.0f;
	float t0 = 1.0f - x0 * x0;
	if (t0 >= 0.0f) {
		t0 *= t0;
		n0 = t0 * t0 * simplexGrad1D(perm[static_cast<std::size_t>(i0) & 255u], x0);
	}
	float n1 = 0.0f;
	float t1 = 1.0f - x1 * x1;
	if (t1 >= 0.0f) {
		t1 *= t1;
		n1 = t1 * t1 * simplexGrad1D(perm[static_cast<std::size_t>(i1) & 255u], x1);
	}
	return 0.395f * (n0 + n1);
}

float simplexNoise2D(const SimplexPermutationTable& perm, float x, float y)
{
	constexpr float F2 = 0.3660254037844386f;
	constexpr float G2 = 0.21132486540518713f;
	const float s = (x + y) * F2;
	const int i = fastFloor(x + s);
	const int j = fastFloor(y + s);
	const float t = static_cast<float>(i + j) * G2;
	const float x0 = x - (static_cast<float>(i) - t);
	const float y0 = y - (static_cast<float>(j) - t);
	const int i1 = x0 > y0 ? 1 : 0;
	const int j1 = x0 > y0 ? 0 : 1;
	const float x1 = x0 - static_cast<float>(i1) + G2;
	const float y1 = y0 - static_cast<float>(j1) + G2;
	const float x2 = x0 - 1.0f + 2.0f * G2;
	const float y2 = y0 - 1.0f + 2.0f * G2;
	const auto gi0 = static_cast<std::size_t>(i + perm[static_cast<std::size_t>(j) & 255u]) & 255u;
	const auto gi1 = static_cast<std::size_t>(i + i1 + perm[static_cast<std::size_t>(j + j1) & 255u]) & 255u;
	const auto gi2 = static_cast<std::size_t>(i + 1 + perm[static_cast<std::size_t>(j + 1) & 255u]) & 255u;

	float n0 = 0.0f;
	float t0 = 0.5f - x0 * x0 - y0 * y0;
	if (t0 >= 0.0f) {
		t0 *= t0;
		n0 = t0 * t0 * simplexGrad2D(perm[gi0], x0, y0);
	}
	float n1 = 0.0f;
	float t1 = 0.5f - x1 * x1 - y1 * y1;
	if (t1 >= 0.0f) {
		t1 *= t1;
		n1 = t1 * t1 * simplexGrad2D(perm[gi1], x1, y1);
	}
	float n2 = 0.0f;
	float t2 = 0.5f - x2 * x2 - y2 * y2;
	if (t2 >= 0.0f) {
		t2 *= t2;
		n2 = t2 * t2 * simplexGrad2D(perm[gi2], x2, y2);
	}
	// The classic 70 from Gustavson's article assumes a normalized gradient table and
	// overshoots to about 1.55 with the eight-direction kernel above. 45.0 is the
	// measured reciprocal of this kernel's maximum, which keeps the output in [-1, 1].
	return 45.0f * (n0 + n1 + n2);
}

void simplexNoise1D(const SimplexPermutationTable& perm, std::span<float> output,
					float frequencyScale)
{
	const float outputSpan = static_cast<float>(output.size() - 1);
	for (std::size_t x = 0; x < output.size(); ++x) {
		const float position = outputSpan > 0.0f ? static_cast<float>(x) / outputSpan : 0.0f;
		output[x] = simplexNoise1D(perm, position * frequencyScale);
	}
}

void simplexNoise2D(const SimplexPermutationTable& perm, std::span<float> output,
					std::uint32_t width, std::uint32_t height, float frequencyScale)
{
	requireExtent(output.size(), std::uint64_t(width) * height);
	if (width == 0 || height == 0) return;
	const float widthSpan = width > 1 ? static_cast<float>(width - 1) : 0.0f;
	const float heightSpan = height > 1 ? static_cast<float>(height - 1) : 0.0f;
	for (std::uint32_t y = 0; y < height; ++y) {
		float* const row = &output[std::size_t(y) * width];
		const float positionY = heightSpan > 0.0f ? static_cast<float>(y) / heightSpan : 0.0f;
		for (std::uint32_t x = 0; x < width; ++x) {
			const float positionX = widthSpan > 0.0f ? static_cast<float>(x) / widthSpan : 0.0f;
			row[x] = simplexNoise2D(perm, positionX * frequencyScale, positionY * frequencyScale);
		}
	}
}

void voronoi1D(std::span<float> output, std::span<const std::uint32_t> points, bool invert)
{
	if (output.empty()) return;
	std::uint64_t maximum = 0;
	for (std::size_t x = 0; x < output.size(); ++x) {
		std::uint64_t minimum = std::numeric_limits<std::uint64_t>::max();
		for (const std::uint32_t point : points) {
			const std::uint64_t distance = x > point ? std::uint64_t(x) - point
													: std::uint64_t(point) - x;
			minimum = std::min(minimum, distance);
		}
		if (minimum == std::numeric_limits<std::uint64_t>::max()) minimum = 0;
		maximum = std::max(maximum, minimum);
		output[x] = static_cast<float>(minimum);
	}
	if (maximum == 0) {
		std::fill(output.begin(), output.end(), invert ? 1.0f : 0.0f);
		return;
	}
	for (float& value : output) {
		value = static_cast<float>(value) / static_cast<float>(maximum);
		if (invert) value = 1.0f - value;
	}
}

void voronoi2D(std::span<float> output, std::uint32_t width, std::uint32_t height,
			std::span<const NoisePoint2D> points, bool squared, bool invert)
{
	requireExtent(output.size(), std::uint64_t(width) * height);
	if (width == 0 || height == 0) return;
	std::uint64_t maximum = 0;
	for (std::uint32_t y = 0; y < height; ++y) {
		float* const row = &output[std::size_t(y) * width];
		for (std::uint32_t x = 0; x < width; ++x) {
			std::uint64_t minimum = std::numeric_limits<std::uint64_t>::max();
			for (const NoisePoint2D& point : points) {
				const std::uint64_t dx = x > point.x ? std::uint64_t(x) - point.x
													: std::uint64_t(point.x) - x;
				const std::uint64_t dy = y > point.y ? std::uint64_t(y) - point.y
													: std::uint64_t(point.y) - y;
				// 64-bit squares: an 8k x 8k image already overflows a uint32.
				const std::uint64_t squaredDistance = dx * dx + dy * dy;
				minimum = std::min(minimum, squaredDistance);
			}
			if (minimum == std::numeric_limits<std::uint64_t>::max()) minimum = 0;
			maximum = std::max(maximum, minimum);
			row[x] = static_cast<float>(minimum);
		}
	}
	if (maximum == 0) {
		std::fill_n(output.begin(), std::size_t(width) * height, invert ? 1.0f : 0.0f);
		return;
	}
	for (float& value : std::span<float>(output.data(), std::size_t(width) * height)) {
		value = static_cast<float>(value) / static_cast<float>(maximum);
		if (!squared) value = std::sqrt(value);
		if (invert) value = 1.0f - value;
	}
}

namespace {

/// @brief Sector count that covers @p size with sectors of @p sectorSize.
std::uint32_t sectorCount(std::uint32_t size, std::uint32_t sectorSize)
{
	if (sectorSize == 0) return 0;
	return (size % sectorSize) ? (size / sectorSize) + 1 : size / sectorSize;
}

} // namespace

void sectorVoronoi1D(std::span<float> output, std::uint32_t sectorSize,
					const RandomOffset1D& offset, bool invert)
{
	if (output.empty() || sectorSize == 0) return;
	const std::uint32_t count = sectorCount(static_cast<std::uint32_t>(output.size()), sectorSize);
	std::vector<std::uint32_t> points(count);
	std::uint32_t sectorOffset = 0;
	for (std::uint32_t& point : points) {
		point = sectorOffset + (offset ? offset(sectorSize) % sectorSize : 0u);
		sectorOffset += sectorSize;
	}
	voronoi1D(output, std::span<const std::uint32_t>(points), invert);
}

void sectorVoronoi2D(std::span<float> output, std::uint32_t width, std::uint32_t height,
					std::uint32_t sectorWidth, std::uint32_t sectorHeight,
					const RandomOffset2D& offset, bool squared, bool invert)
{
	requireExtent(output.size(), std::uint64_t(width) * height);
	const std::uint32_t countX = sectorCount(width, sectorWidth);
	const std::uint32_t countY = sectorCount(height, sectorHeight);
	if (countX == 0 || countY == 0) return;
	std::vector<NoisePoint2D> points(std::size_t(countX) * countY);
	for (std::uint32_t y = 0; y < countY; ++y) {
		NoisePoint2D* const row = &points[std::size_t(y) * countX];
		for (std::uint32_t x = 0; x < countX; ++x) {
			const NoisePoint2D local = offset ? offset(NoisePoint2D{sectorWidth, sectorHeight})
											: NoisePoint2D{0u, 0u};
			row[x] = NoisePoint2D{local.x % sectorWidth + x * sectorWidth,
								local.y % sectorHeight + y * sectorHeight};
		}
	}
	voronoi2D(output, width, height, std::span<const NoisePoint2D>(points), squared, invert);
}

void sectorVoronoi1D(std::span<float> output, std::uint32_t sectorSize, std::uint32_t seed,
					bool invert)
{
	std::mt19937 rng(seed);
	sectorVoronoi1D(output, sectorSize,
					[&rng](std::uint32_t max) {
						return max == 0 ? 0u : std::uniform_int_distribution<std::uint32_t>(0, max - 1)(rng);
					},
					invert);
}

void sectorVoronoi2D(std::span<float> output, std::uint32_t width, std::uint32_t height,
					std::uint32_t sectorWidth, std::uint32_t sectorHeight, std::uint32_t seed,
					bool squared, bool invert)
{
	std::mt19937 rng(seed);
	sectorVoronoi2D(
		output, width, height, sectorWidth, sectorHeight,
		[&rng](NoisePoint2D max) {
			return NoisePoint2D{
				max.x == 0 ? 0u : std::uniform_int_distribution<std::uint32_t>(0, max.x - 1)(rng),
				max.y == 0 ? 0u : std::uniform_int_distribution<std::uint32_t>(0, max.y - 1)(rng)};
		},
		squared, invert);
}

void sumOfSines1D(std::span<float> output, float periodicity, std::uint32_t periods,
				const PeriodicFunction1D& perFunc, float periodicityBias, float amplitudeBias)
{
	const float outputSpan = static_cast<float>(output.size() - 1);
	for (std::size_t x = 0; x < output.size(); ++x) {
		float amplitude = 1.0f;
		float valueSum = 0.0f;
		float amplitudeSum = 0.0f;
		float currentPeriodicity = periodicity;
		for (std::uint32_t period = 0; period < periods; ++period) {
			valueSum += amplitude * perFunc((outputSpan > 0.0f ? static_cast<float>(x) / outputSpan : 0.0f) *
											currentPeriodicity);
			amplitudeSum += amplitude;
			currentPeriodicity *= periodicityBias;
			amplitude *= amplitudeBias;
		}
		output[x] = amplitudeSum > 0.0f ? valueSum / amplitudeSum : 0.0f;
	}
}

void sumOfSines1D(std::span<float> output, float periodicity,
				std::span<const PeriodicFunction1D> perFuncs, float periodicityBias,
				float amplitudeBias)
{
	const float outputSpan = static_cast<float>(output.size() - 1);
	for (std::size_t x = 0; x < output.size(); ++x) {
		float amplitude = 1.0f;
		float valueSum = 0.0f;
		float amplitudeSum = 0.0f;
		float currentPeriodicity = periodicity;
		for (const PeriodicFunction1D& perFunc : perFuncs) {
			if (perFunc) {
				valueSum += amplitude * perFunc((outputSpan > 0.0f ? static_cast<float>(x) / outputSpan : 0.0f) *
												currentPeriodicity);
				amplitudeSum += amplitude;
			}
			currentPeriodicity *= periodicityBias;
			amplitude *= amplitudeBias;
		}
		output[x] = amplitudeSum > 0.0f ? valueSum / amplitudeSum : 0.0f;
	}
}

void sumOfSines2D(std::span<float> output, std::uint32_t outWidth, std::uint32_t outHeight,
				float periodicity, std::uint32_t periods, const PeriodicFunction2D& perFunc,
				float periodicityBias, float amplitudeBias)
{
	requireExtent(output.size(), std::uint64_t(outWidth) * outHeight);
	const float widthSpan = outWidth > 1 ? static_cast<float>(outWidth - 1) : 0.0f;
	const float heightSpan = outHeight > 1 ? static_cast<float>(outHeight - 1) : 0.0f;
	for (std::uint32_t y = 0; y < outHeight; ++y) {
		float* const row = &output[std::size_t(y) * outWidth];
		const float premultipliedY = heightSpan > 0.0f ? static_cast<float>(y) / heightSpan : 0.0f;
		for (std::uint32_t x = 0; x < outWidth; ++x) {
			const float premultipliedX = widthSpan > 0.0f ? static_cast<float>(x) / widthSpan : 0.0f;
			float amplitude = 1.0f;
			float valueSum = 0.0f;
			float amplitudeSum = 0.0f;
			float currentPeriodicity = periodicity;
			for (std::uint32_t period = 0; period < periods; ++period) {
				valueSum += amplitude * perFunc(premultipliedX * currentPeriodicity,
												premultipliedY * currentPeriodicity);
				amplitudeSum += amplitude;
				currentPeriodicity *= periodicityBias;
				amplitude *= amplitudeBias;
			}
			row[x] = amplitudeSum > 0.0f ? valueSum / amplitudeSum : 0.0f;
		}
	}
}

void sumOfSines2D(std::span<float> output, std::uint32_t outWidth, std::uint32_t outHeight,
				float periodicity, std::span<const PeriodicFunction2D> perFuncs,
				float periodicityBias, float amplitudeBias)
{
	requireExtent(output.size(), std::uint64_t(outWidth) * outHeight);
	const float widthSpan = outWidth > 1 ? static_cast<float>(outWidth - 1) : 0.0f;
	const float heightSpan = outHeight > 1 ? static_cast<float>(outHeight - 1) : 0.0f;
	for (std::uint32_t y = 0; y < outHeight; ++y) {
		float* const row = &output[std::size_t(y) * outWidth];
		const float premultipliedY = heightSpan > 0.0f ? static_cast<float>(y) / heightSpan : 0.0f;
		for (std::uint32_t x = 0; x < outWidth; ++x) {
			const float premultipliedX = widthSpan > 0.0f ? static_cast<float>(x) / widthSpan : 0.0f;
			float amplitude = 1.0f;
			float valueSum = 0.0f;
			float amplitudeSum = 0.0f;
			float currentPeriodicity = periodicity;
			for (const PeriodicFunction2D& perFunc : perFuncs) {
				if (perFunc) {
					valueSum += amplitude * perFunc(premultipliedX * currentPeriodicity,
													premultipliedY * currentPeriodicity);
					amplitudeSum += amplitude;
				}
				currentPeriodicity *= periodicityBias;
				amplitude *= amplitudeBias;
			}
			row[x] = amplitudeSum > 0.0f ? valueSum / amplitudeSum : 0.0f;
		}
	}
}


namespace {

/// @brief 32-bit integer hash used by the hashed lattice noise.
uint32_t hashUints(std::uint32_t a, std::uint32_t b, std::uint32_t seed)
{
	std::uint32_t h = a * 0x9E3779B1u ^ (b * 0x85EBCA77u + seed);
	h ^= h >> 16;
	h *= 0x7feb352du;
	h ^= h >> 15;
	h *= 0x846ca68bu;
	h ^= h >> 16;
	return h;
}

/// @brief Maps a hash to a float in `[0, 1]` using its top 24 bits.
float hashToUnit(std::uint32_t hash)
{
	return static_cast<float>(hash >> 8) * (1.0f / 16777216.0f);
}

} // namespace

float latticeNoise2D(float x, float y, std::uint32_t seed)
{
	const int xi = fastFloor(x);
	const int yi = fastFloor(y);
	const float fx = x - static_cast<float>(xi);
	const float fy = y - static_cast<float>(yi);
	const float u = fx * fx * (3.0f - 2.0f * fx);
	const float v = fy * fy * (3.0f - 2.0f * fy);
	const auto at = [seed](int px, int py) {
		return hashToUnit(hashUints(static_cast<std::uint32_t>(px),
									static_cast<std::uint32_t>(py), seed));
	};
	const float top = lerp(at(xi, yi), at(xi + 1, yi), u);
	const float bottom = lerp(at(xi, yi + 1), at(xi + 1, yi + 1), u);
	return lerp(top, bottom, v);
}

float latticeFbm2D(float x, float y, std::uint32_t seed, std::uint32_t octaves)
{
	float sum = 0.0f;
	float amplitude = 0.5f;
	float normalisation = 0.0f;
	for (std::uint32_t octave = 0; octave < octaves; ++octave) {
		sum += amplitude * latticeNoise2D(x, y, seed + octave * 101u);
		normalisation += amplitude;
		x = x * 2.03f + 7.1f;
		y = y * 2.03f - 3.7f;
		amplitude *= 0.5f;
	}
	return normalisation > 0.0f ? sum / normalisation : 0.0f;
}


// Tile Voronoi: block placement, the 3x3 search, then the three projections.

namespace {

/// @brief One tile's point, in tile-local and image-absolute coordinates.
struct Tile {
	std::int64_t startX = 0;
	std::int64_t startY = 0;
	std::int64_t offsetX = 0;
	std::int64_t offsetY = 0;
};

/// @brief Raw distance for a metric: Manhattan is linear, Euclidean is squared.
std::int64_t rawDistance(VoronoiDistance metric, std::int64_t dx, std::int64_t dy)
{
	switch (metric) {
		case VoronoiDistance::Manhattan:
			return dx + dy;
		case VoronoiDistance::Euclidean:
		default:
			return dx * dx + dy * dy;
	}
}

/// @brief Turns a raw metric value back into its comparable distance.
float toComparableDistance(VoronoiDistance metric, std::int64_t raw)
{
	if (metric == VoronoiDistance::Euclidean) {
		return std::sqrt(static_cast<float>(raw));
	}
	return static_cast<float>(raw);
}

/// @brief Calls @p step for every sample of the grid, with the destination index.
std::size_t sampleCount(const VoronoiSettings& settings)
{
	return std::size_t(settings.blockCountX) * settings.blockSizeX * settings.blockCountY *
		settings.blockSizeY;
}

/// @brief Places the points of a grid once, so every pixel can reuse them.
std::vector<Tile> placeTiles(const VoronoiSettings& settings, const VoronoiOffsetGenerator& generator)
{
	std::vector<Tile> tiles(std::size_t(settings.blockCountX) * settings.blockCountY);
	for (std::uint32_t tileY = 0; tileY < settings.blockCountY; ++tileY) {
		for (std::uint32_t tileX = 0; tileX < settings.blockCountX; ++tileX) {
			Tile& tile = tiles[std::size_t(tileY) * settings.blockCountX + tileX];
			VoronoiOffset offset;
			if (generator) {
				offset = generator(tileX, tileY, settings.blockSizeX, settings.blockSizeY);
				// Clamp the generator's answer: a point outside its tile would make the
				// 3x3 neighbourhood search wrong rather than merely ugly.
				const auto clamp = [](std::int32_t value, std::uint32_t limit) {
					if (value < 0) return std::int64_t(0);
					if (static_cast<std::uint32_t>(value) >= limit) return std::int64_t(limit) - 1;
					return std::int64_t(value);
				};
				tile.offsetX = clamp(offset.x, settings.blockSizeX);
				tile.offsetY = clamp(offset.y, settings.blockSizeY);
			} else {
				tile.offsetX = std::int64_t(settings.blockSizeX) / 2;
				tile.offsetY = std::int64_t(settings.blockSizeY) / 2;
			}
			tile.startX = std::int64_t(tileX) * settings.blockSizeX;
			tile.startY = std::int64_t(tileY) * settings.blockSizeY;
		}
	}
	return tiles;
}

} // namespace

void voronoi(std::span<VoronoiSample> output, const VoronoiSettings& settings,
			const VoronoiOffsetGenerator& generator)
{
	const std::size_t needed = sampleCount(settings);
	if (output.size() < needed) {
		throw std::invalid_argument("voronoi: the destination span is smaller than the grid");
	}
	if (settings.blockSizeX == 0 || settings.blockSizeY == 0 || settings.blockCountX == 0 ||
		settings.blockCountY == 0) {
		return;
	}
	const std::vector<Tile> tiles = placeTiles(settings, generator);
	const std::int64_t blockWidth = settings.blockSizeX;
	const std::int64_t blockHeight = settings.blockSizeY;

	for (std::uint32_t tileY = 0; tileY < settings.blockCountY; ++tileY) {
		for (std::uint32_t tileX = 0; tileX < settings.blockCountX; ++tileX) {
			const std::size_t currentIndex = std::size_t(tileY) * settings.blockCountX + tileX;
			const Tile& current = tiles[currentIndex];

			for (std::uint32_t pixelY = 0; pixelY < settings.blockSizeY; ++pixelY) {
				const std::size_t rowBase =
					std::size_t(current.startY + pixelY) * settings.blockCountX * settings.blockSizeX +
					std::size_t(current.startX);
				for (std::uint32_t pixelX = 0; pixelX < settings.blockSizeX; ++pixelX) {
					std::int64_t best = std::numeric_limits<std::int64_t>::max();
					std::int64_t second = std::numeric_limits<std::int64_t>::max();
					std::uint32_t bestCell = 0;

					for (std::int32_t offsetY = -1; offsetY <= 1; ++offsetY) {
						for (std::int32_t offsetX = -1; offsetX <= 1; ++offsetX) {
							// Wrap at the border: that is what makes the field tileable.
							const std::uint32_t neighbourX = static_cast<std::uint32_t>(
								(std::int64_t(tileX) + offsetX + settings.blockCountX) %
								settings.blockCountX);
							const std::uint32_t neighbourY = static_cast<std::uint32_t>(
								(std::int64_t(tileY) + offsetY + settings.blockCountY) %
								settings.blockCountY);
							const std::size_t neighbourIndex =
								std::size_t(neighbourY) * settings.blockCountX + neighbourX;
							const Tile& neighbour = tiles[neighbourIndex];
							const std::int64_t pointX =
								std::int64_t(offsetX) * blockWidth + neighbour.offsetX;
							const std::int64_t pointY =
								std::int64_t(offsetY) * blockHeight + neighbour.offsetY;
							const std::int64_t dx = std::llabs(std::int64_t(pixelX) - pointX);
							const std::int64_t dy = std::llabs(std::int64_t(pixelY) - pointY);
							const std::int64_t distance =
								rawDistance(settings.distance, dx, dy);
							if (distance < best) {
								second = best;
								best = distance;
								bestCell = static_cast<std::uint32_t>(neighbourIndex);
							} else if (distance < second) {
								second = distance;
							}
						}
					}

					VoronoiSample& sample = output[rowBase + pixelX];
					sample.firstDistance = toComparableDistance(settings.distance, best);
					sample.secondDistance = toComparableDistance(settings.distance, second);
					sample.cell = bestCell;
				}
			}
		}
	}
}

void voronoi(std::span<VoronoiSample> output, const VoronoiSettings& settings, std::uint32_t seed)
{
	std::mt19937 rng(seed);
	voronoi(output, settings,
			[&rng](std::uint32_t, std::uint32_t, std::uint32_t tileWidth, std::uint32_t tileHeight) {
				const auto pick = [&rng](std::uint32_t limit) {
					if (limit == 0) return std::int32_t(0);
					return static_cast<std::int32_t>(
						std::uniform_int_distribution<std::uint32_t>(0, limit - 1)(rng));
				};
				return VoronoiOffset{pick(tileWidth), pick(tileHeight)};
			});
}

namespace {

/// @brief Shared body of the distance/border projections.
/// @param output Destination samples, one per pixel.
/// @param settings Tile grid, metric and normalization.
/// @param generator Point placement, or an empty callable for centred points.
/// @param useBorder Project `F2 - F1` instead of `F1`.
void projectField(std::span<float> output, const VoronoiSettings& settings,
				const VoronoiOffsetGenerator& generator, bool useBorder)
{
	std::vector<VoronoiSample> samples(sampleCount(settings));
	voronoi(std::span<VoronoiSample>(samples), settings, generator);
	float maximum = 0.0f;
	for (const VoronoiSample& sample : samples) {
		const float value = useBorder ? sample.secondDistance - sample.firstDistance
									: sample.firstDistance;
		maximum = std::max(maximum, value);
	}
	const float scale = maximum > 0.0f ? 1.0f / maximum : 0.0f;
	for (std::size_t index = 0; index < samples.size(); ++index) {
		const float value = useBorder ? samples[index].secondDistance - samples[index].firstDistance
									: samples[index].firstDistance;
		float normalized = settings.normalize ? value * scale : value;
		if (settings.inverted) normalized = 1.0f - normalized;
		output[index] = normalized;
	}
}

} // namespace

void voronoiDistance(std::span<float> output, const VoronoiSettings& settings,
					const VoronoiOffsetGenerator& generator)
{
	const std::size_t needed = sampleCount(settings);
	if (output.size() < needed) {
		throw std::invalid_argument("voronoiDistance: the destination span is smaller than the grid");
	}
	projectField(output.first(needed), settings, generator, false);
}

void voronoiBorder(std::span<float> output, const VoronoiSettings& settings,
				const VoronoiOffsetGenerator& generator)
{
	const std::size_t needed = sampleCount(settings);
	if (output.size() < needed) {
		throw std::invalid_argument("voronoiBorder: the destination span is smaller than the grid");
	}
	projectField(output.first(needed), settings, generator, true);
}

void voronoiCellIndices(std::span<std::uint32_t> output, const VoronoiSettings& settings,
						const VoronoiOffsetGenerator& generator)
{
	const std::size_t needed = sampleCount(settings);
	if (output.size() < needed) {
		throw std::invalid_argument(
			"voronoiCellIndices: the destination span is smaller than the grid");
	}
	std::vector<VoronoiSample> samples(needed);
	voronoi(std::span<VoronoiSample>(samples), settings, generator);
	for (std::size_t index = 0; index < needed; ++index) {
		output[index] = samples[index].cell;
	}
}

void voronoiDistance(std::span<float> output, const VoronoiSettings& settings, std::uint32_t seed)
{
	std::mt19937 rng(seed);
	voronoiDistance(output, settings,
					[&rng](std::uint32_t, std::uint32_t, std::uint32_t tileWidth,
						std::uint32_t tileHeight) {
						const auto pick = [&rng](std::uint32_t limit) {
							if (limit == 0) return std::int32_t(0);
							return static_cast<std::int32_t>(
								std::uniform_int_distribution<std::uint32_t>(0, limit - 1)(rng));
						};
						return VoronoiOffset{pick(tileWidth), pick(tileHeight)};
					});
}

void voronoiBorder(std::span<float> output, const VoronoiSettings& settings, std::uint32_t seed)
{
	std::mt19937 rng(seed);
	voronoiBorder(output, settings,
				[&rng](std::uint32_t, std::uint32_t, std::uint32_t tileWidth,
						std::uint32_t tileHeight) {
					const auto pick = [&rng](std::uint32_t limit) {
						if (limit == 0) return std::int32_t(0);
						return static_cast<std::int32_t>(
							std::uniform_int_distribution<std::uint32_t>(0, limit - 1)(rng));
					};
					return VoronoiOffset{pick(tileWidth), pick(tileHeight)};
				});
}

void voronoiCellIndices(std::span<std::uint32_t> output, const VoronoiSettings& settings,
						std::uint32_t seed)
{
	std::mt19937 rng(seed);
	voronoiCellIndices(output, settings,
					[&rng](std::uint32_t, std::uint32_t, std::uint32_t tileWidth,
							std::uint32_t tileHeight) {
						const auto pick = [&rng](std::uint32_t limit) {
							if (limit == 0) return std::int32_t(0);
							return static_cast<std::int32_t>(
								std::uniform_int_distribution<std::uint32_t>(0, limit - 1)(rng));
						};
						return VoronoiOffset{pick(tileWidth), pick(tileHeight)};
					});
}

} // namespace Noise
} // namespace Util
} // namespace Euph
