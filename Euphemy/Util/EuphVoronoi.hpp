#ifndef EUPHVORONOI_HPP
#define EUPHVORONOI_HPP
/**
 * @file EuphVoronoi.hpp
 * @brief Tile-based Voronoi/Worley noise that also reports which point won each pixel.
 *
 * Ported from `ProceduralGenerationTest/Voronoi.{hpp,cpp}` (the Qt playground) into
 * the Qt-free library. The algorithm is the same **tile Voronoi**: the image is cut
 * into `blockCountX x blockCountY` tiles of `blockSizeX x blockSizeY` pixels, every
 * tile owns one point at an offset inside it, and each pixel searches the 3x3
 * neighbourhood of its own tile - wrapping at the image border, which is what makes
 * the result tile seamlessly.
 *
 * The port keeps the two ideas that made the original useful and adds the one it
 * lacked:
 *
 * - **Distance output** (`voronoiDistance`) - the classic Worley field. The first
 *   and second nearest distances are both computed, because `F2 - F1` is what draws
 *   cracks and cell borders; `firstDistance` alone is the legacy behaviour.
 * - **Cell output** (`voronoiCellIndices`) - every pixel stores the index of the
 *   *nearest point*, not its distance. That turns the field into a tile map: index a
 *   palette, an atlas or a per-cell random value with it to get tiles instead of a
 *   height ramp.
 * - **One traversal for everything** (`voronoi`) - the block search is the
 *   expensive part, so the raw overload fills a full `VoronoiSample` (both
 *   distances plus the cell index) in one pass, and the two convenience functions
 *   are cheap projections of it.
 *
 * Differences from the original implementation:
 *
 * - No `rand()`: point placement is either a caller-supplied
 *   `VoronoiOffsetGenerator` or a `std::mt19937` seeded overload.
 * - `Distance::Eucledian` became `VoronoiDistance::Euclidean` (the typo was in the
 *   original enum) and the metric list is documented instead of inferred.
 * - Distances are compared as 64-bit squares and converted to `float` once, so a
 *   large image or a large tile cannot wrap the squared distance.
 * - Normalization is a projection (`voronoiDistance`), not a side effect of the raw
 *   fill, so a caller that wants *both* F1 and F2 normalized consistently can do it
 *   itself.
 *
 * @note The GPU plan is to evaluate the 3x3 neighbourhood in a compute shader later;
 *       this CPU path stays as the reference and as the editor-time generator.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <cstdint>
#include <functional>
#include <span>

namespace Euph {
namespace Util {
namespace Noise {

/** @brief Metric used to measure the distance from a pixel to a tile's point. */
enum class VoronoiDistance : std::uint8_t {
	Euclidean, /**< `sqrt(dx^2 + dy^2)`; the usual Worley look. */
	Manhattan  /**< `|dx| + |dy|`; diamond-shaped cells. */
};

/** @brief Geometry of the tile grid and the shape of the output. */
struct VoronoiSettings {
	/** @brief Width of one tile in pixels; must be non-zero. */
	std::uint32_t blockSizeX = 128;
	/** @brief Height of one tile in pixels; must be non-zero. */
	std::uint32_t blockSizeY = 128;
	/** @brief Number of tiles horizontally; must be non-zero. */
	std::uint32_t blockCountX = 8;
	/** @brief Number of tiles vertically; must be non-zero. */
	std::uint32_t blockCountY = 8;
	/** @brief Rescale the distance projection into `[0, 1]` (ignored by the cell output). */
	bool normalize = true;
	/** @brief Mirror the distance projection (`1 - d`) after normalization. */
	bool inverted = false;
	/** @brief Metric used for the distances. */
	VoronoiDistance distance = VoronoiDistance::Euclidean;
};

/** @brief Width in pixels of a @ref VoronoiSettings grid. */
constexpr std::uint32_t voronoiWidthPixels(const VoronoiSettings& settings) noexcept
{
	return settings.blockCountX * settings.blockSizeX;
}

/** @brief Height in pixels of a @ref VoronoiSettings grid. */
constexpr std::uint32_t voronoiHeightPixels(const VoronoiSettings& settings) noexcept
{
	return settings.blockCountY * settings.blockSizeY;
}

/** @brief A point offset inside one tile, in `[0, tileSize)` per axis. */
struct VoronoiOffset {
	/** @brief Horizontal offset inside the tile. */
	std::int32_t x = 0;
	/** @brief Vertical offset inside the tile. */
	std::int32_t y = 0;
};

/**
 * @brief Places the point of one tile.
 *
 * The tile coordinates are given so a generator can make the field non-repeating
 * (a hash of the tile index) or keep it uniform (a constant offset).
 */
using VoronoiOffsetGenerator =
	std::function<VoronoiOffset(std::uint32_t tileX, std::uint32_t tileY, std::uint32_t tileWidth,
								std::uint32_t tileHeight)>;

/** @brief Everything one pixel needs to know about its neighbourhood. */
struct VoronoiSample {
	/** @brief Distance to the nearest point in the chosen metric (F1). */
	float firstDistance = 0.0f;
	/** @brief Distance to the second nearest point (F2); `F2 - F1` draws cell borders. */
	float secondDistance = 0.0f;
	/** @brief Index of the nearest point, `tileY * blockCountX + tileX`. */
	std::uint32_t cell = 0;
};

/**
 * @brief Fills a grid with the nearest/second-nearest point records.
 * @param output Destination samples; `blockCountX * blockSizeX * blockCountY * blockSizeY`
 *        entries are written starting at index 0.
 * @param settings Tile grid, metric and normalization.
 * @param generator Point placement; an empty callable centres every point in its tile.
 * @throws std::invalid_argument When the destination span is too small.
 */
MH_EUPH_API void voronoi(std::span<VoronoiSample> output, const VoronoiSettings& settings,
						const VoronoiOffsetGenerator& generator);

/**
 * @brief Fills a grid with point records placed by a local `std::mt19937`.
 * @param output Destination samples.
 * @param settings Tile grid, metric and normalization.
 * @param seed Seed of the local `std::mt19937`.
 * @throws std::invalid_argument When the destination span is too small.
 */
MH_EUPH_API void voronoi(std::span<VoronoiSample> output, const VoronoiSettings& settings,
						std::uint32_t seed);

/**
 * @brief Fills a grid with the normalized F1 distance.
 * @param output Destination samples; one per pixel.
 * @param settings Tile grid, metric and normalization.
 * @param generator Point placement; an empty callable centres every point in its tile.
 * @throws std::invalid_argument When the destination span is too small.
 */
MH_EUPH_API void voronoiDistance(std::span<float> output, const VoronoiSettings& settings,
								const VoronoiOffsetGenerator& generator);

/**
 * @brief Fills a grid with the normalized `F2 - F1` border field.
 * @param output Destination samples; one per pixel.
 * @param settings Tile grid, metric and normalization.
 * @param generator Point placement; an empty callable centres every point in its tile.
 * @throws std::invalid_argument When the destination span is too small.
 */
MH_EUPH_API void voronoiBorder(std::span<float> output, const VoronoiSettings& settings,
							const VoronoiOffsetGenerator& generator);

/**
 * @brief Fills a grid with the index of the nearest point of every pixel.
 * @param output Destination samples; one per pixel, in `[0, blockCountX * blockCountY)`.
 * @param settings Tile grid and metric; normalization and inversion are ignored.
 * @param generator Point placement; an empty callable centres every point in its tile.
 * @throws std::invalid_argument When the destination span is too small.
 */
MH_EUPH_API void voronoiCellIndices(std::span<std::uint32_t> output,
									const VoronoiSettings& settings,
									const VoronoiOffsetGenerator& generator);

/**
 * @brief Seeded convenience overload of @ref voronoiDistance.
 * @param output Destination samples; one per pixel.
 * @param settings Tile grid, metric and normalization.
 * @param seed Seed of the local `std::mt19937`.
 * @throws std::invalid_argument When the destination span is too small.
 */
MH_EUPH_API void voronoiDistance(std::span<float> output, const VoronoiSettings& settings,
								std::uint32_t seed);

/**
 * @brief Seeded convenience overload of @ref voronoiBorder.
 * @param output Destination samples; one per pixel.
 * @param settings Tile grid, metric and normalization.
 * @param seed Seed of the local `std::mt19937`.
 * @throws std::invalid_argument When the destination span is too small.
 */
MH_EUPH_API void voronoiBorder(std::span<float> output, const VoronoiSettings& settings,
							std::uint32_t seed);

/**
 * @brief Seeded convenience overload of @ref voronoiCellIndices.
 * @param output Destination samples; one per pixel.
 * @param settings Tile grid and metric.
 * @param seed Seed of the local `std::mt19937`.
 * @throws std::invalid_argument When the destination span is too small.
 */
MH_EUPH_API void voronoiCellIndices(std::span<std::uint32_t> output,
									const VoronoiSettings& settings, std::uint32_t seed);

} // namespace Noise
} // namespace Util
} // namespace Euph

#endif // EUPHVORONOI_HPP
