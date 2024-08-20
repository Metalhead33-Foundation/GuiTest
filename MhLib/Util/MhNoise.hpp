#ifndef MHNOISE_HPP
#define MHNOISE_HPP
#include <span>
#include <vector>
#include <MhLib/Util/MhRNG.hpp>
#include <MhLib/Util/MhNormDenorm.hpp>
#include <glm/glm.hpp>
#include <functional>
#include <MhLib/Util/MhGlobals.hpp>
namespace MH33 {
namespace Util {
namespace Noise {
/// Convenience functions
float bilinearAverage(float tX, float tY, float topLeft, float topRight, float bottomLeft, float bottomRight);
void MH_UTIL_API normalize(const std::span<float>& output);
void MH_UTIL_API remapToSigned(const std::span<float>& output);
void MH_UTIL_API remapToUnsigned(const std::span<float>& output);
/// WHITE NOISE
void MH_UTIL_API rand_whiteNoise(const std::span<float>& output, unsigned seed);
void MH_UTIL_API lehmer_whiteNoise(const std::span<float>& output, RNG& rng);
template <RandomNumberGenerator uintRng> void rng_whiteNoise(const std::span<float>& output, uintRng& rng)
{
	for(float& it : output) {
		it = static_cast<float>(rng()) / static_cast<float>(rng.max ());
	}
}
/// REGULAR PERLIN NOISE
float MH_UTIL_API perlin1D(const std::span<const float>& seed, unsigned x, int pitch);
void MH_UTIL_API perlin1D(const std::span<float>& output, const std::span<const float>& seed, unsigned octaves, float scaleBias = 2.0f, bool shouldNormalize=false);
float MH_UTIL_API perlin2D(unsigned width, unsigned height, const float* seed, unsigned x, unsigned y, int horizontalPitch, int verticalPitch);
void MH_UTIL_API perlin2D(unsigned width, unsigned height, float* output, const float* seed, unsigned octaves, float scaleBias = 2.0f, bool shouldNormalize=false);
// Regular rand
void MH_UTIL_API rand_perlin1D(const std::span<float>& output, unsigned seed, unsigned octaves, float scaleBias = 2.0f, bool shouldNormalize=false);
void MH_UTIL_API rand_perlin2D(unsigned width, unsigned height, float* output, unsigned seed, unsigned octaves, float scaleBias = 2.0f, bool shouldNormalize=false);
// Predictable rand
void MH_UTIL_API lehmer_perlin1D(const std::span<float>& output, RNG& rng, unsigned octaves, float scaleBias = 2.0f, bool shouldNormalize=false);
void MH_UTIL_API lehmer_perlin2D(unsigned width, unsigned height, float* output, RNG& rng, unsigned octaves, float scaleBias = 2.0f, bool shouldNormalize=false);
// Generic C++ generators
template <RandomNumberGenerator uintRng> void rng_perlin1D(const std::span<float>& output, uintRng& rng, unsigned octaves, float scaleBias = 2.0f, bool shouldNormalize=false) {
	std::vector<float> randVec(output.size());
	rng_whiteNoise(randVec, rng);
	perlin1D(output,randVec,octaves,scaleBias,shouldNormalize);
}
template <RandomNumberGenerator uintRng> void rng_perlin2D(unsigned width, unsigned height, float* output, uintRng& rng, unsigned octaves, float scaleBias = 2.0f, bool shouldNormalize=false) {
	std::vector<float> randVec(width * height);
	rng_whiteNoise(randVec, rng);
	perlin2D(width,height,output,randVec.data(),octaves,scaleBias,shouldNormalize);
}
/// BILINEAR PERLIN NOISE
void MH_UTIL_API bilinearPerlin1D(const std::span<float>& output, const std::span<const float>& seed, unsigned octaves, float valA, float valB, float scaleBias = 2.0f, float cornerBias = 0.5f);
void MH_UTIL_API bilinearPerlin2D(unsigned width, unsigned height, float* output, const float* seed, unsigned octaves, float topLeft, float topRight, float bottomLeft, float bottomRight, float scaleBias = 2.0f, float cornerBias = 0.5f);
// Regular rand
void MH_UTIL_API rand_bilinearPerlin1D(const std::span<float>& output, unsigned seed, unsigned octaves, float valA, float valB, float scaleBias = 2.0f, float cornerBias = 0.5f);
void MH_UTIL_API rand_bilinearPerlin2D(unsigned width, unsigned height, float* output, unsigned seed, unsigned octaves, float topLeft, float topRight, float bottomLeft, float bottomRight, float scaleBias = 2.0f, float cornerBias = 0.5f);
// Predictable rand
void MH_UTIL_API lehmer_bilinearPerlin1D(const std::span<float>& output, RNG& rng, unsigned octaves, float valA, float valB, float scaleBias = 2.0f, float cornerBias = 0.5f);
void MH_UTIL_API lehmer_bilinearPerlin2D(unsigned width, unsigned height, float* output, RNG& rng, unsigned octaves, float topLeft, float topRight, float bottomLeft, float bottomRight, float scaleBias = 2.0f, float cornerBias = 0.5f);
// Generic C++ generators
template <RandomNumberGenerator uintRng> void rng_bilinearPerlin1D(const std::span<float>& output, uintRng& rng, unsigned octaves, float valA, float valB, float scaleBias = 2.0f, float cornerBias = 0.5f) {
	std::vector<float> randVec(output.size());
	rng_whiteNoise(randVec, rng);
	bilinearPerlin1D(output,randVec,octaves,valA,valB,scaleBias,cornerBias);
}
template <RandomNumberGenerator uintRng> void rng_bilinearPerlin2D(unsigned width, unsigned height, float* output, uintRng& rng, unsigned octaves, float topLeft, float topRight, float bottomLeft, float bottomRight, float scaleBias = 2.0f, float cornerBias = 0.5f) {
	std::vector<float> randVec(width * height);
	rng_whiteNoise(randVec, rng);
	bilinearPerlin2D(width,height,output,randVec.data(),octaves,topLeft,topRight,bottomLeft,bottomRight,scaleBias,cornerBias);
}
/// Worley Noise / Voronoi noise
// Pregenerated points
void MH_UTIL_API voronoi1D(const std::span<float>& output, const std::span<const unsigned>& points, bool invert=false);
void MH_UTIL_API voronoi2D(unsigned width, unsigned height, float* output, const std::span<const glm::uvec2>& points, bool squared=true, bool invert=false);
typedef std::function<unsigned(unsigned)> RandFunction1D;
typedef std::function<glm::uvec2(const glm::uvec2&)> RandFunction2D;
void MH_UTIL_API rand_voronoi1D(const std::span<float>& output, unsigned sectorSize, const RandFunction1D& seed, bool invert=false);
void MH_UTIL_API rand_voronoi2D(unsigned width, unsigned height, float* output, unsigned sectorWidth, unsigned sectorHeight, const RandFunction2D& seed, bool squared=true, bool invert=false);
// Regular rand
void MH_UTIL_API rand_voronoi1D(const std::span<float>& output, unsigned sectorSize, unsigned seed, bool invert=false);
void MH_UTIL_API rand_voronoi2D(unsigned width, unsigned height, float* output, unsigned sectorWidth, unsigned sectorHeight, unsigned seed, bool squared=true, bool invert=false);
// Predictable rand
void MH_UTIL_API lehmer_voronoi1D(const std::span<float>& output, unsigned sectorSize, RNG& rng, bool invert=false);
void MH_UTIL_API lehmer_voronoi2D(unsigned width, unsigned height, float* output, unsigned sectorWidth, unsigned sectorHeight, RNG& rng, bool squared=true, bool invert=false);
// Generic C++ generators
template <RandomNumberGenerator uintRng> void rng_voronoi1D(const std::span<float>& output, unsigned sectorSize, uintRng& rng, bool invert=false)
{
	rand_voronoi1D(output,sectorSize,[&rng](unsigned max){ return rng() % max; }, invert);
}
template <RandomNumberGenerator uintRng> void rng_voronoi2D(unsigned width, unsigned height, float* output, unsigned sectorWidth, unsigned sectorHeight, uintRng& rng, bool squared=true, bool invert=false)
{
	rand_voronoi2D(width,height,output,sectorWidth,sectorHeight,
				   [&rng](const glm::uvec2& sectorSize) { return glm::uvec2(rng() % sectorSize.x, rng() % sectorSize.y); },
	squared, invert);
}
/// Value noise
enum class Interpolation {
	LINEAR,
	COSINE,
	SMOOTHSTEP
};
void MH_UTIL_API valueNoise1D(const std::span<float>& output, const std::span<const float>& points, Interpolation interpolation = Interpolation::LINEAR);
void MH_UTIL_API valueNoise2D(float* output, unsigned outWidth, unsigned outHeight, const float* input, unsigned inWidth, unsigned inHeight, Interpolation interpolation = Interpolation::LINEAR);
// Regular rand
void MH_UTIL_API rand_valueNoise1D(const std::span<float>& output, unsigned seed, unsigned pointCount, Interpolation interpolation = Interpolation::LINEAR);
void MH_UTIL_API rand_valueNoise2D(float* output, unsigned outWidth, unsigned outHeight, unsigned seed, unsigned inWidth, unsigned inHeight, Interpolation interpolation = Interpolation::LINEAR);
// Predictable rand
void MH_UTIL_API lehmer_valueNoise1D(const std::span<float>& output, RNG& rng, unsigned pointCount, Interpolation interpolation = Interpolation::LINEAR);
void MH_UTIL_API lehmer_valueNoise2D(float* output, unsigned outWidth, unsigned outHeight, RNG& rng, unsigned inWidth, unsigned inHeight, Interpolation interpolation = Interpolation::LINEAR);
// Generic C++ generators
template <RandomNumberGenerator uintRng> void rng_valueNoise1D(const std::span<float>& output, uintRng& rng, unsigned pointCount, Interpolation interpolation = Interpolation::LINEAR) {
	std::vector<float> randVec(pointCount);
	rng_whiteNoise(randVec, rng);
	valueNoise1D(output,randVec,interpolation);
}
template <RandomNumberGenerator uintRng> void rng_valueNoise2D(float* output, unsigned outWidth, unsigned outHeight, uintRng& rng, unsigned inWidth, unsigned inHeight, Interpolation interpolation = Interpolation::LINEAR) {
	std::vector<float> randVec(inWidth * inHeight);
	rng_whiteNoise(randVec, rng);
	valueNoise2D(output, outWidth, outHeight, randVec.data(), inWidth, inHeight, interpolation);
}

}
}
}

#endif // MHNOISE_HPP
