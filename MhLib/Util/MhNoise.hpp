#ifndef MHNOISE_HPP
#define MHNOISE_HPP
#include <span>
#include <vector>
#include <MhLib/Util/MhRNG.hpp>
#include <MhLib/Util/MhNormDenorm.hpp>
namespace MH33 {
namespace Util {
namespace Noise {
/// REGULAR PERLIN NOISE
float perlin1D(const std::span<const float>& seed, int x, int pitch);
void perlin1D(std::span<float>& output, const std::span<const float>& seed, int octaves, float scaleBias = 2.0f);
float perlin2D(int width, int height, const float* seed, int x, int y, int horizontalPitch, int verticalPitch);
void perlin2D(int width, int height, float* output, const float* seed, int octaves, float scaleBias = 2.0f);
// Regular rand
void rand_perlin1D(std::span<float>& output, unsigned seed, int octaves, float scaleBias = 2.0f);
void rand_perlin2D(int width, int height, float* output, unsigned seed, int octaves, float scaleBias = 2.0f);
// Predictable rand
void lehmer_perlin1D(std::span<float>& output, RNG& rng, int octaves, float scaleBias = 2.0f);
void lehmer_perlin2D(int width, int height, float* output, RNG& rng, int octaves, float scaleBias = 2.0f);
// Generic C++ generators
template <RandomNumberGenerator uintRng> void rng_perlin1D(std::span<float>& output, uintRng& rng, int octaves, float scaleBias = 2.0f) {
	std::vector<float> randVec(output.size());
	for(auto& it : randVec) {
		it = static_cast<float>(rng()) / static_cast<float>(rng.max());
	}
	perlin1D(output,randVec,octaves,scaleBias);
}
template <RandomNumberGenerator uintRng> void rng_perlin2D(int width, int height, float* output, uintRng& rng, int octaves, float scaleBias = 2.0f) {
	std::vector<float> randVec(width * height);
	for(auto& it : randVec) {
		it = static_cast<float>(rng()) / static_cast<float>(rng.max());
	}
	perlin2D(width,height,output,randVec.data(),octaves,scaleBias);
}
/// BILINEAR PERLIN NOISE
void bilinearPerlin1D(std::span<float>& output, const std::span<const float>& seed, int octaves, float valA, float valB, float scaleBias = 2.0f, float cornerBias = 0.5f);
void bilinearPerlin2D(int width, int height, float* output, const float* seed, int octaves, float topLeft, float topRight, float bottomLeft, float bottomRight, float scaleBias = 2.0f, float cornerBias = 0.5f);
// Regular rand
void rand_bilinearPerlin1D(std::span<float>& output, unsigned seed, int octaves, float valA, float valB, float scaleBias = 2.0f, float cornerBias = 0.5f);
void rand_bilinearPerlin2D(int width, int height, float* output, unsigned seed, int octaves, float topLeft, float topRight, float bottomLeft, float bottomRight, float scaleBias = 2.0f, float cornerBias = 0.5f);
// Predictable rand
void lehmer_bilinearPerlin1D(std::span<float>& output, RNG& rng, int octaves, float valA, float valB, float scaleBias = 2.0f, float cornerBias = 0.5f);
void lehmer_bilinearPerlin2D(int width, int height, float* output, RNG& rng, int octaves, float topLeft, float topRight, float bottomLeft, float bottomRight, float scaleBias = 2.0f, float cornerBias = 0.5f);
// Generic C++ generators
template <RandomNumberGenerator uintRng> void rng_bilinearPerlin1D(std::span<float>& output, uintRng& rng, int octaves, float valA, float valB, float scaleBias = 2.0f, float cornerBias = 0.5f) {
	std::vector<float> randVec(output.size());
	for(auto& it : randVec) {
		it = static_cast<float>(rng()) / static_cast<float>(rng.max());
	}
	bilinearPerlin1D(output,randVec,octaves,valA,valB,scaleBias,cornerBias);
}
template <RandomNumberGenerator uintRng> void rng_bilinearPerlin2D(int width, int height, float* output, uintRng& rng, int octaves, float topLeft, float topRight, float bottomLeft, float bottomRight, float scaleBias = 2.0f, float cornerBias = 0.5f) {
	std::vector<float> randVec(width * height);
	for(auto& it : randVec) {
		it = static_cast<float>(rng()) / static_cast<float>(rng.max());
	}
	bilinearPerlin2D(width,height,output,randVec.data(),octaves,topLeft,topRight,bottomLeft,bottomRight,scaleBias,cornerBias);
}

}
}
}

#endif // MHNOISE_HPP
