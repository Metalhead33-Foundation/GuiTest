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
/// REGULAR PERLIN NOISE
float MH_UTIL_API perlin1D(const std::span<const float>& seed, unsigned x, int pitch);
void MH_UTIL_API perlin1D(const std::span<float>& output, const std::span<const float>& seed, unsigned octaves, float scaleBias = 2.0f, bool normalize=false);
float MH_UTIL_API perlin2D(unsigned width, unsigned height, const float* seed, unsigned x, unsigned y, int horizontalPitch, int verticalPitch);
void MH_UTIL_API perlin2D(unsigned width, unsigned height, float* output, const float* seed, unsigned octaves, float scaleBias = 2.0f, bool normalize=false);
// Regular rand
void MH_UTIL_API rand_perlin1D(const std::span<float>& output, unsigned seed, unsigned octaves, float scaleBias = 2.0f, bool normalize=false);
void MH_UTIL_API rand_perlin2D(unsigned width, unsigned height, float* output, unsigned seed, unsigned octaves, float scaleBias = 2.0f, bool normalize=false);
// Predictable rand
void MH_UTIL_API lehmer_perlin1D(const std::span<float>& output, RNG& rng, unsigned octaves, float scaleBias = 2.0f, bool normalize=false);
void MH_UTIL_API lehmer_perlin2D(unsigned width, unsigned height, float* output, RNG& rng, unsigned octaves, float scaleBias = 2.0f, bool normalize=false);
// Generic C++ generators
template <RandomNumberGenerator uintRng> void rng_perlin1D(const std::span<float>& output, uintRng& rng, unsigned octaves, float scaleBias = 2.0f, bool normalize=false) {
	std::vector<float> randVec(output.size());
	for(auto& it : randVec) {
		it = static_cast<float>(rng()) / static_cast<float>(rng.max());
	}
	perlin1D(output,randVec,octaves,scaleBias,normalize);
}
template <RandomNumberGenerator uintRng> void rng_perlin2D(unsigned width, unsigned height, float* output, uintRng& rng, unsigned octaves, float scaleBias = 2.0f, bool normalize=false) {
	std::vector<float> randVec(width * height);
	for(auto& it : randVec) {
		it = static_cast<float>(rng()) / static_cast<float>(rng.max());
	}
	perlin2D(width,height,output,randVec.data(),octaves,scaleBias,normalize);
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
	for(auto& it : randVec) {
		it = static_cast<float>(rng()) / static_cast<float>(rng.max());
	}
	bilinearPerlin1D(output,randVec,octaves,valA,valB,scaleBias,cornerBias);
}
template <RandomNumberGenerator uintRng> void rng_bilinearPerlin2D(unsigned width, unsigned height, float* output, uintRng& rng, unsigned octaves, float topLeft, float topRight, float bottomLeft, float bottomRight, float scaleBias = 2.0f, float cornerBias = 0.5f) {
	std::vector<float> randVec(width * height);
	for(auto& it : randVec) {
		it = static_cast<float>(rng()) / static_cast<float>(rng.max());
	}
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
/// WHITE NOISE
void MH_UTIL_API rand_whiteNoise(const std::span<float>& output, unsigned seed);
void MH_UTIL_API lehmer_whiteNoise(const std::span<float>& output, RNG& rng);
template <RandomNumberGenerator uintRng> void rng_whiteNoise(const std::span<float>& output, uintRng& rng)
{
	for(float& it : output) {
		it = static_cast<float>(rng()) / static_cast<float>(rng.max ());
	}
}


}
}
}

#endif // MHNOISE_HPP
