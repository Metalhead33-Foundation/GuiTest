#include "EuphVoronoi.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

namespace Euph {
namespace Util {
namespace Noise {

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
