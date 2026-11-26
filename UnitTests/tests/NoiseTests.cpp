/**
 * @file NoiseTests.cpp
 * @brief Checks the salvaged Euphemy noise generators and the tile Voronoi.
 *
 * The noise port replaced three things that were untestable in the legacy code: a
 * global `rand()` seed, unchecked `size() - 1` divisions and out-of-bounds lattice
 * reads. These checks pin the replacements down: determinism, range, the degenerate
 * inputs, and a brute-force cross-check of the tile Voronoi's distances and cell
 * indices.
 */
#include <catch2/catch_test_macros.hpp>
#include <Euphemy/Util/EuphNoise.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

using namespace Euph::Util::Noise;

/// @brief True when every sample of a span is inside `[low, high]`.
bool allWithin(std::span<const float> samples, float low, float high)
{
	for (const float value : samples) {
		if (value < low || value > high) return false;
	}
	return true;
}


} // namespace

TEST_CASE("Euphemy noise: white noise is seeded and bounded", "[noise]")
{
	std::vector<float> first(64);
	std::vector<float> second(64);
	std::vector<float> other(64);
	whiteNoise(std::span<float>(first), 1234u);
	whiteNoise(std::span<float>(second), 1234u);
	whiteNoise(std::span<float>(other), 4321u);
	REQUIRE(first == second);
	REQUIRE(first != other);
	REQUIRE(allWithin(std::span<const float>(first), 0.0f, 1.0f));
}

TEST_CASE("Euphemy noise: normalize handles a flat span and an all-negative span", "[noise]")
{
	std::vector<float> flat(4, 3.5f);
	normalize(std::span<float>(flat));
	REQUIRE(flat[0] == 3.5f);
	REQUIRE(flat[3] == 3.5f);

	std::vector<float> negative = { -8.0f, -6.0f, -4.0f };
	normalize(std::span<float>(negative));
	REQUIRE(negative[0] == 0.0f);
	REQUIRE(negative[1] == 0.5f);
	REQUIRE(negative[2] == 1.0f);
}

TEST_CASE("Euphemy noise: value noise reproduces its lattice and survives one sample", "[noise]")
{
	const std::vector<float> lattice = { 0.0f, 1.0f, 0.0f, 1.0f };
	std::vector<float> output(4);
	valueNoise2D(std::span<float>(output), 2, 2, std::span<const float>(lattice), 2, 2,
	Interpolation::Linear);
	REQUIRE(output == lattice);

	std::vector<float> single(1, -1.0f);
	const std::vector<float> points = { 0.25f };
	valueNoise1D(std::span<float>(single), std::span<const float>(points));
	REQUIRE(single[0] == 0.25f);

	// A span smaller than the requested extent is a programming error, not a crash.
	std::vector<float> tooSmall(3, 0.0f);
	REQUIRE_THROWS_AS(valueNoise2D(std::span<float>(tooSmall), 4, 4,
	std::span<const float>(lattice), 2, 2),
	std::invalid_argument);
}

TEST_CASE("Euphemy noise: hashed lattice noise is deterministic and smooth", "[noise]")
{
	REQUIRE(latticeNoise2D(1.25f, -3.5f, 7u) == latticeNoise2D(1.25f, -3.5f, 7u));
	REQUIRE(latticeNoise2D(1.25f, -3.5f, 7u) != latticeNoise2D(1.25f, -3.5f, 8u));
	for (int index = 0; index < 32; ++index) {
		const float value = latticeFbm2D(static_cast<float>(index) * 0.37f, 1.5f, 11u);
		REQUIRE(value >= 0.0f);
		REQUIRE(value <= 1.0f);
	}
}

TEST_CASE("Euphemy noise: simplex stays inside its documented range", "[noise]")
{
	const SimplexPermutationTable permutation = makeSimplexPermutation(99u);
	std::vector<float> output(64);
	simplexNoise2D(permutation, std::span<float>(output), 8, 8, 4.0f);
	REQUIRE(allWithin(std::span<const float>(output), -1.0f, 1.0f));

	std::vector<float> line(16);
	simplexNoise1D(permutation, std::span<float>(line), 2.0f);
	REQUIRE(allWithin(std::span<const float>(line), -1.0f, 1.0f));
}

TEST_CASE("Euphemy noise: point Voronoi normalizes to its own maximum", "[noise]")
{
	std::vector<float> output(5);
	const std::vector<std::uint32_t> points = { 0u };
	voronoi1D(std::span<float>(output), std::span<const std::uint32_t>(points));
	REQUIRE(output[0] == 0.0f);
	REQUIRE(output[4] == 1.0f);

	const std::vector<NoisePoint2D> corners = { { 0u, 0u }, { 3u, 3u } };
	std::vector<float> field(16);
	voronoi2D(std::span<float>(field), 4, 4, std::span<const NoisePoint2D>(corners), false, false);
	REQUIRE(field[0] == 0.0f);
	REQUIRE(field[15] == 0.0f);
	REQUIRE(field[5] > field[0]);
}

TEST_CASE("Euphemy Voronoi: a single tile tiles periodically", "[noise][voronoi]")
{
	VoronoiSettings settings;
	settings.blockSizeX = 8;
	settings.blockSizeY = 8;
	settings.blockCountX = 1;
	settings.blockCountY = 1;
	settings.normalize = false;
	settings.distance = VoronoiDistance::Euclidean;
	const auto generator = [](std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) {
		return VoronoiOffset{ 3, 5 };
	};

	std::vector<float> distance(64);
	voronoiDistance(std::span<float>(distance), settings, generator);
	std::vector<std::uint32_t> cells(64);
	voronoiCellIndices(std::span<std::uint32_t>(cells), settings, generator);

	// The field tiles with the grid, so a pixel's nearest point may be the image of
	// the single point in a neighbouring period. That is the property the tile
	// Voronoi is for, and the 3x3 neighbourhood search is what produces it.
	for (std::uint32_t y = 0; y < 8; ++y) {
		for (std::uint32_t x = 0; x < 8; ++x) {
			double expected = std::numeric_limits<double>::max();
			for (int wrapX = -1; wrapX <= 1; ++wrapX) {
				for (int wrapY = -1; wrapY <= 1; ++wrapY) {
					const double pointX = 3.0 + wrapX * 8.0;
					const double pointY = 5.0 + wrapY * 8.0;
					expected = std::min(expected, std::hypot(double(x) - pointX, double(y) - pointY));
				}
			}
			REQUIRE(std::abs(distance[std::size_t(y) * 8 + x] - expected) < 1.0e-5);
			REQUIRE(cells[std::size_t(y) * 8 + x] == 0u);
		}
	}
}

TEST_CASE("Euphemy Voronoi: cells and distances agree with a brute-force search", "[noise][voronoi]")
{
	VoronoiSettings settings;
	settings.blockSizeX = 6;
	settings.blockSizeY = 6;
	settings.blockCountX = 3;
	settings.blockCountY = 2;
	settings.normalize = false;
	const auto generator = [](std::uint32_t tileX, std::uint32_t tileY, std::uint32_t width,
	std::uint32_t height) {
		return VoronoiOffset{ static_cast<std::int32_t>((tileX * 7 + tileY * 3) % width),
		static_cast<std::int32_t>((tileX * 5 + tileY * 11) % height) };
	};

	// The points are recomputed exactly as the generator defines them.
	std::vector<NoisePoint2D> points;
	for (std::uint32_t tileY = 0; tileY < settings.blockCountY; ++tileY) {
		for (std::uint32_t tileX = 0; tileX < settings.blockCountX; ++tileX) {
			const VoronoiOffset offset = generator(tileX, tileY, settings.blockSizeX,
			settings.blockSizeY);
			points.push_back(NoisePoint2D{
				static_cast<std::uint32_t>(tileX) * settings.blockSizeX +
				static_cast<std::uint32_t>(offset.x),
				static_cast<std::uint32_t>(tileY) * settings.blockSizeY +
				static_cast<std::uint32_t>(offset.y) });
			}
		}

		std::vector<VoronoiSample> samples(std::size_t(18) * 12);
		voronoi(std::span<VoronoiSample>(samples), settings, generator);

		for (std::uint32_t y = 0; y < 12; ++y) {
			for (std::uint32_t x = 0; x < 18; ++x) {
				const VoronoiSample& sample = samples[std::size_t(y) * 18 + x];
				// The grid wraps, so the brute force must consider the point's periodic
				// images too: that is exactly what the 3x3 neighbourhood search does.
				double best = std::numeric_limits<double>::max();
				for (std::uint32_t index = 0; index < points.size(); ++index) {
					for (int wrapX = -1; wrapX <= 1; ++wrapX) {
						for (int wrapY = -1; wrapY <= 1; ++wrapY) {
							const double px = double(points[index].x) + wrapX * 18.0;
							const double py = double(points[index].y) + wrapY * 12.0;
							const double candidate = std::hypot(double(x) - px, double(y) - py);
							if (candidate < best) {
								best = candidate;
							}
						}
					}
				}
				REQUIRE(std::abs(sample.firstDistance - best) < 1.0e-4);
				REQUIRE(sample.cell < settings.blockCountX * settings.blockCountY);
				// Ties are resolved by iteration order, so the reported cell only has to
				// be *a* nearest point - checked by measuring the distance to it.
				double cellDistance = std::numeric_limits<double>::max();
				for (int wrapX = -1; wrapX <= 1; ++wrapX) {
					for (int wrapY = -1; wrapY <= 1; ++wrapY) {
						const double px = double(points[sample.cell].x) + wrapX * 18.0;
						const double py = double(points[sample.cell].y) + wrapY * 12.0;
						cellDistance = std::min(cellDistance, std::hypot(double(x) - px, double(y) - py));
					}
				}
				REQUIRE(std::abs(cellDistance - best) < 1.0e-4);
				REQUIRE(sample.secondDistance >= sample.firstDistance);
			}
		}
	}

	TEST_CASE("Euphemy Voronoi: the seeded overload is deterministic", "[noise][voronoi]")
	{
		VoronoiSettings settings;
		settings.blockSizeX = 4;
		settings.blockSizeY = 4;
		settings.blockCountX = 2;
		settings.blockCountY = 2;

		std::vector<std::uint32_t> first(64);
		std::vector<std::uint32_t> second(64);
		std::vector<std::uint32_t> other(64);
		voronoiCellIndices(std::span<std::uint32_t>(first), settings, 7u);
		voronoiCellIndices(std::span<std::uint32_t>(second), settings, 7u);
		voronoiCellIndices(std::span<std::uint32_t>(other), settings, 8u);
		REQUIRE(first == second);
		REQUIRE(first != other);
	}
