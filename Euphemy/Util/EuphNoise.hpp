#ifndef EUPHNOISE_HPP
#define EUPHNOISE_HPP
/**
 * @file EuphNoise.hpp
 * @brief Deterministic 1D/2D noise generators shared by textures, terrain and GUI forges.
 *
 * Salvaged from `MhLib/Util/MhNoise.{hpp,cpp}` on the `GuiTest-legacy2` tree. The
 * algorithm set is the same - white, value, Perlin, bilinear Perlin, simplex,
 * Voronoi, sum of sines - but the API and the implementation changed in four
 * ways that matter:
 *
 * - **No global RNG.** The legacy `rand_*` overloads called `srand()`/`rand()`, so two
 *   generators running at once (or a generator and a library) fought over one
 *   process-wide state and a "seeded" result was not reproducible. Every seeded
 *   entry point here owns a `std::mt19937`, and every random entry point takes a
 *   callable or a C++ generator, so the caller decides where the entropy comes from.
 * - **Spans, not raw pointers.** A width/height pair next to a `float*` cannot be
 *   checked; `std::span` makes the size part of the type, and the 2D overloads take
 *   one flat span with explicit extents.
 * - **Degenerate inputs are handled, not asserted.** The legacy code divided by
 *   `size() - 1` (zero for a one-sample span), indexed one past the right edge of a
 *   value-noise lattice, looped `y < width` in the 2D simplex fill, and squared
 *   distances in 32-bit unsigned arithmetic. Each of those is clamped or widened
 *   here, and the behaviour is documented on the function.
 * - **No `glm`.** A two-integer lattice point is `NoisePoint`2D, so Euphemy does
 *   not grow a geometry dependency for a pair of `uint32_t`.
 *
 * Everything is deterministic: the same arguments produce the same bytes on every
 * platform, because the hash and the permutation table are integer operations and
 * the interpolations use `float`.
 *
 * @note The GPU plan is to move the fill loops (not the configuration) to compute
 *       shaders later; this CPU API is the reference the shaders will be checked
 *       against, which is why the degenerate-input behaviour is specified in prose.
 *       The block-based tile Voronoi lives in `EuphVoronoi.hpp`.
 * @note The legacy 1D simplex kernel was not a simplex kernel: it mixed the 2D skew
 *       constants into a 1D expression and returned values in roughly `[-4.6, 3.9]`.
 *       The port uses the standard 1D kernel (and normalizes the 2D kernel scale),
 *       so every simplex entry point here really is bounded by `[-1, 1]`.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace Euph {
namespace Util {
namespace Noise {

/** @brief A two-integer lattice point used by the Voronoi generators. */
struct NoisePoint2D {
	/** @brief X coordinate. */
	std::uint32_t x = 0;
	/** @brief Y coordinate. */
	std::uint32_t y = 0;

	/** @brief Compares both coordinates. */
	friend constexpr bool operator==(const NoisePoint2D&, const NoisePoint2D&) = default;
};

/**
 * @brief Blends four corner values with the bilinear weights of @p tX and @p tY.
 * @param tX Horizontal blend factor, normally in `[0, 1]`.
 * @param tY Vertical blend factor, normally in `[0, 1]`.
 * @param topLeft Value of the top-left corner.
 * @param topRight Value of the top-right corner.
 * @param bottomLeft Value of the bottom-left corner.
 * @param bottomRight Value of the bottom-right corner.
 * @return The weighted average of the four corners.
 */
MH_EUPH_API float bilinearAverage(float tX, float tY, float topLeft, float topRight,
								float bottomLeft, float bottomRight);

/**
 * @brief Rescales a span so its smallest value is 0 and its largest is 1.
 * @param output Samples to rescale in place.
 * @note A span whose range is zero (flat, empty or a single value) is left untouched
 *       instead of producing `NaN` from a division by zero. The legacy version also
 *       started its maximum search at 0, which silently halved an all-negative range.
 */
MH_EUPH_API void normalize(std::span<float> output);

/**
 * @brief Maps `[0, 1]` values to `[-1, 1]`.
 * @param output Samples to remap in place.
 */
MH_EUPH_API void remapToSigned(std::span<float> output);

/**
 * @brief Maps `[-1, 1]` values to `[0, 1]`.
 * @param output Samples to remap in place.
 */
MH_EUPH_API void remapToUnsigned(std::span<float> output);

/**
 * @brief Fills a span with white noise in `[0, 1]`.
 * @param output Destination samples.
 * @param seed Seed of the local `std::mt19937`.
 */
MH_EUPH_API void whiteNoise(std::span<float> output, std::uint32_t seed);

/**
 * @brief Fills a span with white noise in `[0, 1]` from a caller-owned generator.
 * @tparam Rng Uniform random bit generator type (`std::mt19937`, `std::minstd_rand`, ...).
 * @param output Destination samples.
 * @param rng Generator advanced by the call.
 */
template <typename Rng>
void rngWhiteNoise(std::span<float> output, Rng& rng)
{
	// The generator's own range is the divisor, so the result stays in [0, 1] for any
	// unsigned engine instead of assuming RAND_MAX.
	constexpr float divisor = static_cast<float>(Rng::max());
	for (float& sample : output) {
		sample = static_cast<float>(rng()) / divisor;
	}
}

/** @brief Interpolation used when a noise lattice is resampled. */
enum class Interpolation : std::uint8_t {
	Linear,     /**< Straight `lerp`. */
	Cosine,     /**< Cosine (quarter-wave) easing. */
	SmoothStep  /**< Hermite smoothstep easing. */
};

/// @name Infinite-lattice (hashed) value noise
///@{

/**
 * @brief Samples value noise on an infinite integer lattice, without a seed array.
 * @param x Sample position.
 * @param y Sample position.
 * @param seed Hash seed.
 * @return A value in `[0, 1]`, smooth on the integer lattice.
 * @note This is the form procedural texture code wants: it needs no allocation and no
 *       lattice span, so it can be called per pixel inside an SDF or a material.
 */
MH_EUPH_API float latticeNoise2D(float x, float y, std::uint32_t seed);

/**
 * @brief Octave sum of @ref latticeNoise2D with a 2.03 frequency ratio.
 * @param x Sample position.
 * @param y Sample position.
 * @param seed Hash seed of the first octave; later octaves offset it.
 * @param octaves Number of octaves to sum; zero returns 0.
 * @return The normalized octave sum in `[0, 1]`.
 */
MH_EUPH_API float latticeFbm2D(float x, float y, std::uint32_t seed,
							   std::uint32_t octaves = 5);

///@}


/// @name Value noise
///@{

/**
 * @brief Resamples a 1D lattice into @p output with the requested easing.
 * @param output Destination samples.
 * @param points Lattice values, evenly spaced over the output.
 * @param interpolation Easing applied between two neighbouring lattice values.
 * @note A one-sample output or a one-value lattice repeats the lattice's first value
 *       for every sample instead of dividing by zero.
 */
MH_EUPH_API void valueNoise1D(std::span<float> output, std::span<const float> points,
							Interpolation interpolation = Interpolation::Linear);

/**
 * @brief Resamples a 2D lattice into a flat, row-major destination.
 * @param output Destination samples; `outWidth * outHeight` entries are written.
 * @param outWidth Destination width in samples.
 * @param outHeight Destination height in samples.
 * @param input Source lattice; `inWidth * inHeight` entries are read.
 * @param inWidth Source width in samples.
 * @param inHeight Source height in samples.
 * @param interpolation Easing applied between four neighbouring lattice values.
 * @note Edge samples clamp to the last lattice row/column instead of reading past it.
 */
MH_EUPH_API void valueNoise2D(std::span<float> output, std::uint32_t outWidth,
							std::uint32_t outHeight, std::span<const float> input,
							std::uint32_t inWidth, std::uint32_t inHeight,
							Interpolation interpolation = Interpolation::Linear);

/**
 * @brief Fills a span with value noise over a random lattice of @p pointCount values.
 * @param output Destination samples.
 * @param seed Seed of the local `std::mt19937`.
 * @param pointCount Number of lattice values.
 * @param interpolation Easing applied between neighbouring lattice values.
 */
MH_EUPH_API void valueNoise1D(std::span<float> output, std::uint32_t seed,
							std::uint32_t pointCount,
							Interpolation interpolation = Interpolation::Linear);

/**
 * @brief Fills a flat destination with value noise over a random lattice.
 * @param output Destination samples; `outWidth * outHeight` entries are written.
 * @param outWidth Destination width in samples.
 * @param outHeight Destination height in samples.
 * @param seed Seed of the local `std::mt19937`.
 * @param inWidth Lattice width in samples.
 * @param inHeight Lattice height in samples.
 * @param interpolation Easing applied between four neighbouring lattice values.
 */
MH_EUPH_API void valueNoise2D(std::span<float> output, std::uint32_t outWidth,
							std::uint32_t outHeight, std::uint32_t seed,
							std::uint32_t inWidth, std::uint32_t inHeight,
							Interpolation interpolation = Interpolation::Linear);

/**
 * @brief Fills a span with value noise from a caller-owned generator.
 * @tparam Rng Uniform random bit generator type.
 * @param output Destination samples.
 * @param rng Generator advanced by the call.
 * @param pointCount Number of lattice values.
 * @param interpolation Easing applied between neighbouring lattice values.
 */
template <typename Rng>
void rngValueNoise1D(std::span<float> output, Rng& rng, std::uint32_t pointCount,
					Interpolation interpolation = Interpolation::Linear)
{
	std::vector<float> lattice(pointCount);
	rngWhiteNoise(std::span<float>(lattice), rng);
	valueNoise1D(output, std::span<const float>(lattice), interpolation);
}

/**
 * @brief Fills a flat destination with value noise from a caller-owned generator.
 * @tparam Rng Uniform random bit generator type.
 * @param output Destination samples; `outWidth * outHeight` entries are written.
 * @param outWidth Destination width in samples.
 * @param outHeight Destination height in samples.
 * @param rng Generator advanced by the call.
 * @param inWidth Lattice width in samples.
 * @param inHeight Lattice height in samples.
 * @param interpolation Easing applied between four neighbouring lattice values.
 */
template <typename Rng>
void rngValueNoise2D(std::span<float> output, std::uint32_t outWidth, std::uint32_t outHeight,
					Rng& rng, std::uint32_t inWidth, std::uint32_t inHeight,
					Interpolation interpolation = Interpolation::Linear)
{
	std::vector<float> lattice(std::size_t(inWidth) * inHeight);
	rngWhiteNoise(std::span<float>(lattice), rng);
	valueNoise2D(output, outWidth, outHeight, std::span<const float>(lattice), inWidth, inHeight,
				interpolation);
}

///@}

/// @name Perlin noise
///@{

/**
 * @brief Samples a 1D Perlin lattice with smoothstep fading.
 * @param seed Lattice values; the lattice wraps over this span.
 * @param x Sample position.
 * @param pitch Distance between two lattice samples; zero returns 0.
 * @return The interpolated value at @p x, or 0 for an empty @p seed span.
 */
MH_EUPH_API float perlin1D(std::span<const float> seed, std::uint32_t x, std::uint32_t pitch);

/**
 * @brief Fills a span with octave-summed 1D Perlin noise.
 * @param output Destination samples.
 * @param seed Lattice values; the lattice wraps over this span.
 * @param octaves Number of octaves to sum; octaves narrower than one sample are skipped.
 * @param scaleBias Per-octave frequency multiplier.
 * @param shouldNormalize Rescale the finished span into `[0, 1]`.
 * @note An empty @p seed span leaves @p output zero-filled.
 */
MH_EUPH_API void perlin1D(std::span<float> output, std::span<const float> seed,
						std::uint32_t octaves, float scaleBias = 2.0f,
						bool shouldNormalize = false);

/**
 * @brief Samples a 2D Perlin lattice with smoothstep fading.
 * @param width Lattice width in samples.
 * @param height Lattice height in samples.
 * @param seed Lattice values, row-major; `width * height` entries are read.
 * @param x Sample position.
 * @param y Sample position.
 * @param horizontalPitch Horizontal distance between lattice samples; zero returns 0.
 * @param verticalPitch Vertical distance between lattice samples; zero returns 0.
 * @return The bilinearly faded value at `(x, y)`.
 */
MH_EUPH_API float perlin2D(std::uint32_t width, std::uint32_t height, const float* seed,
						std::uint32_t x, std::uint32_t y, std::uint32_t horizontalPitch,
						std::uint32_t verticalPitch);

/**
 * @brief Fills a flat, row-major destination with octave-summed 2D Perlin noise.
 * @param output Destination samples; `width * height` entries are written.
 * @param width Destination width in samples.
 * @param height Destination height in samples.
 * @param seed Lattice values, row-major; `width * height` entries are read.
 * @param octaves Number of octaves to sum.
 * @param scaleBias Per-octave frequency multiplier.
 * @param shouldNormalize Rescale the finished span into `[0, 1]`.
 */
MH_EUPH_API void perlin2D(std::span<float> output, std::uint32_t width, std::uint32_t height,
						std::span<const float> seed, std::uint32_t octaves,
						float scaleBias = 2.0f, bool shouldNormalize = false);

/**
 * @brief Fills a span with octave-summed 1D Perlin noise over a random lattice.
 * @param output Destination samples.
 * @param seed Seed of the local `std::mt19937`, which also generates the lattice.
 * @param octaves Number of octaves to sum.
 * @param scaleBias Per-octave frequency multiplier.
 * @param shouldNormalize Rescale the finished span into `[0, 1]`.
 */
MH_EUPH_API void perlin1D(std::span<float> output, std::uint32_t seed, std::uint32_t octaves,
						float scaleBias = 2.0f, bool shouldNormalize = false);

/**
 * @brief Fills a flat destination with octave-summed 2D Perlin noise over a random lattice.
 * @param output Destination samples; `width * height` entries are written.
 * @param width Destination width in samples.
 * @param height Destination height in samples.
 * @param seed Seed of the local `std::mt19937`, which also generates the lattice.
 * @param octaves Number of octaves to sum.
 * @param scaleBias Per-octave frequency multiplier.
 * @param shouldNormalize Rescale the finished span into `[0, 1]`.
 */
MH_EUPH_API void perlin2D(std::span<float> output, std::uint32_t width, std::uint32_t height,
						std::uint32_t seed, std::uint32_t octaves, float scaleBias = 2.0f,
						bool shouldNormalize = false);

///@}

/// @name Bilinear Perlin noise (perlin detail over a bilinear ramp)
///@{

/**
 * @brief Fills a span with a bilinear ramp plus octave-summed Perlin detail.
 * @param output Destination samples.
 * @param seed Lattice values for the Perlin octaves.
 * @param octaves Number of octaves; octave 0 is the ramp itself.
 * @param valA Ramp value at the first sample.
 * @param valB Ramp value at the last sample.
 * @param scaleBias Per-octave frequency multiplier.
 * @param cornerBias Weight of the ramp relative to the detail, in `[0, 1]`.
 */
MH_EUPH_API void bilinearPerlin1D(std::span<float> output, std::span<const float> seed,
								std::uint32_t octaves, float valA, float valB,
								float scaleBias = 2.0f, float cornerBias = 0.5f);

/**
 * @brief Fills a flat destination with a bilinear ramp plus Perlin detail.
 * @param output Destination samples; `width * height` entries are written.
 * @param width Destination width in samples.
 * @param height Destination height in samples.
 * @param seed Lattice values for the Perlin octaves.
 * @param octaves Number of octaves; octave 0 is the ramp itself.
 * @param topLeft Ramp value at the top-left corner.
 * @param topRight Ramp value at the top-right corner.
 * @param bottomLeft Ramp value at the bottom-left corner.
 * @param bottomRight Ramp value at the bottom-right corner.
 * @param scaleBias Per-octave frequency multiplier.
 * @param cornerBias Weight of the ramp relative to the detail, in `[0, 1]`.
 */
MH_EUPH_API void bilinearPerlin2D(std::span<float> output, std::uint32_t width,
								std::uint32_t height, std::span<const float> seed,
								std::uint32_t octaves, float topLeft, float topRight,
								float bottomLeft, float bottomRight,
								float scaleBias = 2.0f, float cornerBias = 0.5f);

/**
 * @brief Seeded convenience overload of @ref bilinearPerlin1D.
 * @param output Destination samples.
 * @param seed Seed of the local `std::mt19937`.
 * @param octaves Number of octaves.
 * @param valA Ramp value at the first sample.
 * @param valB Ramp value at the last sample.
 * @param scaleBias Per-octave frequency multiplier.
 * @param cornerBias Weight of the ramp relative to the detail.
 */
MH_EUPH_API void bilinearPerlin1D(std::span<float> output, std::uint32_t seed,
								std::uint32_t octaves, float valA, float valB,
								float scaleBias = 2.0f, float cornerBias = 0.5f);

/**
 * @brief Seeded convenience overload of @ref bilinearPerlin2D.
 * @param output Destination samples.
 * @param width Destination width in samples.
 * @param height Destination height in samples.
 * @param seed Seed of the local `std::mt19937`.
 * @param octaves Number of octaves.
 * @param topLeft Ramp value at the top-left corner.
 * @param topRight Ramp value at the top-right corner.
 * @param bottomLeft Ramp value at the bottom-left corner.
 * @param bottomRight Ramp value at the bottom-right corner.
 * @param scaleBias Per-octave frequency multiplier.
 * @param cornerBias Weight of the ramp relative to the detail.
 */
MH_EUPH_API void bilinearPerlin2D(std::span<float> output, std::uint32_t width,
								std::uint32_t height, std::uint32_t seed,
								std::uint32_t octaves, float topLeft, float topRight,
								float bottomLeft, float bottomRight,
								float scaleBias = 2.0f, float cornerBias = 0.5f);

///@}

/// @name Simplex noise
///@{

/** @brief A 512-entry simplex permutation table (`uint8_t`, so every lookup masks). */
using SimplexPermutationTable = std::array<std::uint8_t, 512>;

/**
 * @brief Builds a deterministically shuffled simplex permutation table.
 * @param seed Seed of the local `std::mt19937`.
 * @return A 512-entry table (the 0..255 permutation emitted twice).
 */
MH_EUPH_API SimplexPermutationTable makeSimplexPermutation(std::uint32_t seed);

/**
 * @brief Gradient of the 1D simplex kernel.
 * @param hash Permutation hash value; only the low four bits are used.
 * @param x Distance along the axis.
 * @return The signed gradient times @p x.
 */
MH_EUPH_API float simplexGrad1D(std::uint32_t hash, float x);

/**
 * @brief Gradient of the 2D simplex kernel.
 * @param hash Permutation hash value; only the low three bits are used.
 * @param x X distance to the corner.
 * @param y Y distance to the corner.
 * @return The dot product of the corner's gradient with `(x, y)`.
 */
MH_EUPH_API float simplexGrad2D(std::uint32_t hash, float x, float y);

/**
 * @brief Evaluates 1D simplex noise at one position.
 * @param perm Permutation table.
 * @param x Sample position.
 * @return A value in `[-1, 1]`.
 */
MH_EUPH_API float simplexNoise1D(const SimplexPermutationTable& perm, float x);

/**
 * @brief Evaluates 2D simplex noise at one position.
 * @param perm Permutation table.
 * @param x Sample position.
 * @param y Sample position.
 * @return A value in `[-1, 1]`.
 */
MH_EUPH_API float simplexNoise2D(const SimplexPermutationTable& perm, float x, float y);

/**
 * @brief Fills a span with 1D simplex noise.
 * @param perm Permutation table.
 * @param output Destination samples.
 * @param frequencyScale Cycles across the span; the sample position is
 *        `x / (size - 1) * frequencyScale`.
 */
MH_EUPH_API void simplexNoise1D(const SimplexPermutationTable& perm, std::span<float> output,
								float frequencyScale = 1.0f);

/**
 * @brief Fills a flat, row-major destination with 2D simplex noise.
 * @param perm Permutation table.
 * @param output Destination samples; `width * height` entries are written.
 * @param width Destination width in samples.
 * @param height Destination height in samples.
 * @param frequencyScale Cycles across each axis.
 */
MH_EUPH_API void simplexNoise2D(const SimplexPermutationTable& perm, std::span<float> output,
								std::uint32_t width, std::uint32_t height,
								float frequencyScale = 1.0f);

///@}

/// @name Voronoi / Worley noise over explicit points
///@{

/** @brief Returns a random 1D lattice offset in `[0, max)`; used by sector Voronoi. */
using RandomOffset1D = std::function<std::uint32_t(std::uint32_t max)>;

/** @brief Returns a random 2D lattice offset in `[0, max)` per axis. */
using RandomOffset2D = std::function<NoisePoint2D(NoisePoint2D max)>;

/**
 * @brief Fills a span with the distance to the nearest of @p points.
 * @param output Destination samples.
 * @param points Point positions in sample units.
 * @param invert Mirror the normalized result (`1 - d`).
 * @note The result is normalized by its own maximum; a span with no in-range distance
 *       (no points) is zero-filled.
 */
MH_EUPH_API void voronoi1D(std::span<float> output, std::span<const std::uint32_t> points,
						bool invert = false);

/**
 * @brief Fills a flat, row-major destination with the distance to the nearest point.
 * @param output Destination samples; `width * height` entries are written.
 * @param width Destination width in samples.
 * @param height Destination height in samples.
 * @param points Point positions in sample units.
 * @param squared Keep the squared distance instead of the Euclidean one.
 * @param invert Mirror the normalized result (`1 - d`).
 * @note Distances are accumulated in 64-bit integers, so large coordinates cannot wrap.
 */
MH_EUPH_API void voronoi2D(std::span<float> output, std::uint32_t width, std::uint32_t height,
						std::span<const NoisePoint2D> points, bool squared = true,
						bool invert = false);

/**
 * @brief Fills a span with sector-based Voronoi noise.
 * @param output Destination samples.
 * @param sectorSize Width of one sector in samples.
 * @param offset Callable that returns a point offset inside a sector.
 * @param invert Mirror the normalized result.
 */
MH_EUPH_API void sectorVoronoi1D(std::span<float> output, std::uint32_t sectorSize,
								const RandomOffset1D& offset, bool invert = false);

/**
 * @brief Fills a flat destination with sector-based 2D Voronoi noise.
 * @param output Destination samples; `width * height` entries are written.
 * @param width Destination width in samples.
 * @param height Destination height in samples.
 * @param sectorWidth Width of one sector in samples.
 * @param sectorHeight Height of one sector in samples.
 * @param offset Callable that returns a point offset inside a sector.
 * @param squared Keep the squared distance instead of the Euclidean one.
 * @param invert Mirror the normalized result.
 */
MH_EUPH_API void sectorVoronoi2D(std::span<float> output, std::uint32_t width,
								std::uint32_t height, std::uint32_t sectorWidth,
								std::uint32_t sectorHeight, const RandomOffset2D& offset,
								bool squared = true, bool invert = false);

/**
 * @brief Seeded convenience overload of @ref sectorVoronoi1D.
 * @param output Destination samples.
 * @param sectorSize Width of one sector in samples.
 * @param seed Seed of the local `std::mt19937`.
 * @param invert Mirror the normalized result.
 */
MH_EUPH_API void sectorVoronoi1D(std::span<float> output, std::uint32_t sectorSize,
								std::uint32_t seed, bool invert = false);

/**
 * @brief Seeded convenience overload of @ref sectorVoronoi2D.
 * @param output Destination samples; `width * height` entries are written.
 * @param width Destination width in samples.
 * @param height Destination height in samples.
 * @param sectorWidth Width of one sector in samples.
 * @param sectorHeight Height of one sector in samples.
 * @param seed Seed of the local `std::mt19937`.
 * @param squared Keep the squared distance instead of the Euclidean one.
 * @param invert Mirror the normalized result.
 */
MH_EUPH_API void sectorVoronoi2D(std::span<float> output, std::uint32_t width,
								std::uint32_t height, std::uint32_t sectorWidth,
								std::uint32_t sectorHeight, std::uint32_t seed,
								bool squared = true, bool invert = false);

///@}

/// @name Sum of sines / fractional Brownian motion
///@{

/** @brief Periodic shaping function of the 1D sum-of-sines generator. */
using PeriodicFunction1D = std::function<float(float)>;

/** @brief Periodic shaping function of the 2D sum-of-sines generator. */
using PeriodicFunction2D = std::function<float(float, float)>;

/**
 * @brief Fills a span with the sum of @p periods shaped by one periodic function.
 * @param output Destination samples.
 * @param periodicity Cycles of the first period across the span.
 * @param periods Number of harmonics.
 * @param perFunc Shaping function, called with the current phase.
 * @param periodicityBias Per-harmonic frequency multiplier.
 * @param amplitudeBias Per-harmonic amplitude multiplier; the reciprocal of
 *        @p periodicityBias keeps the result bounded, as the legacy header advised.
 */
MH_EUPH_API void sumOfSines1D(std::span<float> output, float periodicity, std::uint32_t periods,
							const PeriodicFunction1D& perFunc, float periodicityBias = 2.0f,
							float amplitudeBias = 0.5f);

/**
 * @brief Fills a span with the sum of @p perFuncs harmonics, one per function.
 * @param output Destination samples.
 * @param periodicity Cycles of the first harmonic across the span.
 * @param perFuncs Shaping functions in harmonic order.
 * @param periodicityBias Per-harmonic frequency multiplier.
 * @param amplitudeBias Per-harmonic amplitude multiplier.
 */
MH_EUPH_API void sumOfSines1D(std::span<float> output, float periodicity,
							std::span<const PeriodicFunction1D> perFuncs,
							float periodicityBias = 2.0f, float amplitudeBias = 0.5f);

/**
 * @brief Fills a flat destination with the 2D sum of @p periods of one function.
 * @param output Destination samples; `outWidth * outHeight` entries are written.
 * @param outWidth Destination width in samples.
 * @param outHeight Destination height in samples.
 * @param periodicity Cycles of the first harmonic across each axis.
 * @param periods Number of harmonics.
 * @param perFunc Shaping function, called with the current `(x, y)` phase.
 * @param periodicityBias Per-harmonic frequency multiplier.
 * @param amplitudeBias Per-harmonic amplitude multiplier.
 */
MH_EUPH_API void sumOfSines2D(std::span<float> output, std::uint32_t outWidth,
							std::uint32_t outHeight, float periodicity, std::uint32_t periods,
							const PeriodicFunction2D& perFunc, float periodicityBias = 2.0f,
							float amplitudeBias = 0.5f);

/**
 * @brief Fills a flat destination with one harmonic per shaping function.
 * @param output Destination samples; `outWidth * outHeight` entries are written.
 * @param outWidth Destination width in samples.
 * @param outHeight Destination height in samples.
 * @param periodicity Cycles of the first harmonic across each axis.
 * @param perFuncs Shaping functions in harmonic order.
 * @param periodicityBias Per-harmonic frequency multiplier.
 * @param amplitudeBias Per-harmonic amplitude multiplier.
 */
MH_EUPH_API void sumOfSines2D(std::span<float> output, std::uint32_t outWidth,
							std::uint32_t outHeight, float periodicity,
							std::span<const PeriodicFunction2D> perFuncs,
							float periodicityBias = 2.0f, float amplitudeBias = 0.5f);

///@}

} // namespace Noise
} // namespace Util
} // namespace Euph

#endif // EUPHNOISE_HPP
