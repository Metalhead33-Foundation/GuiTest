#ifndef MHNOISE_HPP
#define MHNOISE_HPP
#include <span>
#include <vector>
#include <MhLib/Util/MhRNG.hpp>
#include <MhLib/Util/MhNormDenorm.hpp>
namespace MH33 {
namespace Util {
namespace Noise {
/// PERLIN NOISE
void perlin1D(std::span<float>& output, const std::span<const float>& seed, int octaves, float scaleBias = 2.0f);
void perlin2D(int width, int height, float* output, const float* seed, int octaves, float scaleBias = 2.0f);
// Regular rand
void rand_perlin1D(std::span<float>& output, unsigned seed, int octaves, float scaleBias = 2.0f);
void rand_perlin1D(std::span<float>& output, unsigned seed, int octaves, float valA, float valB, float scaleBias = 2.0f);
void rand_perlin2D(int width, int height, float* output, unsigned seed, int octaves, float scaleBias = 2.0f);
void rand_perlin2D(int width, int height, float* output, unsigned seed, int octaves, float valA, float valB, float valC, float valD, float scaleBias = 2.0f);
// Predictable rand
void lehmer_perlin1D(std::span<float>& output, RNG& rng, int octaves, float scaleBias = 2.0f);
void lehmer_perlin1D(std::span<float>& output, RNG& rng, int octaves, float valA, float valB, float scaleBias = 2.0f);
void lehmer_perlin2D(int width, int height, float* output, RNG& rng, int octaves, float scaleBias = 2.0f);
void lehmer_perlin2D(int width, int height, float* output, RNG& rng, int octaves, float valA, float valB, float valC, float valD, float scaleBias = 2.0f);
// Generic C++ generators
template <RandomNumberGenerator uintRng> void rng_perlin1D(std::span<float>& output, uintRng& rng, int octaves, float scaleBias = 2.0f) {
	std::vector<float> randVec(output.size());
	for(auto& it : randVec) {
		it = static_cast<float>(rng()) / static_cast<float>(rng.max());
	}
	perlin1D(output,randVec,octaves,scaleBias);
}
template <RandomNumberGenerator uintRng> void rng_perlin1D(std::span<float>& output, uintRng& rng, int octaves, float valA, float valB, float scaleBias = 2.0f) {
	std::vector<float> randVec(output.size());
	for(auto& it : randVec) {
		it = static_cast<float>(rng()) / static_cast<float>(rng.max());
	}
	randVec[0] = valA;
	randVec[randVec.size() - 1] = valB;
	perlin1D(output,randVec,octaves,scaleBias);
}
template <RandomNumberGenerator uintRng> void rng_perlin2D(int width, int height, float* output, uintRng& rng, int octaves, float scaleBias = 2.0f) {
	std::vector<float> randVec(width * height);
	for(auto& it : randVec) {
		it = static_cast<float>(rng()) / static_cast<float>(rng.max());
	}
	perlin2D(width,height,output,randVec.data(),octaves,scaleBias);
}
template <RandomNumberGenerator uintRng> void rng_perlin2D(int width, int height, float* output, uintRng& rng, int octaves, float valA, float valB, float valC, float valD, float scaleBias = 2.0f) {
	std::vector<float> randVec(width * height);
	for(auto& it : randVec) {
		it = static_cast<float>(rng()) / static_cast<float>(rng.max());
	}
	// Top left corner
	randVec[0] = valA;
	// Top right corner
	randVec[width-1] = valB;
	// Bottom left corner
	randVec[width*(height-1)] = valC;
	// Bottom right corner
	randVec[(width*height)-1] = valD;
	perlin2D(width,height,output,randVec.data(),octaves,scaleBias);
}

}
}
}

#endif // MHNOISE_HPP
