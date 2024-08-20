#include "MhNoise.hpp"
#include <cstdlib>
namespace MH33 {
namespace Util {
namespace Noise {

float perlin1D(const std::span<const float>& seed, int x, int pitch)
{
	const int sampleIndex1 = x - (x % pitch);
	const int sampleIndex2 = (sampleIndex1 + pitch) % static_cast<int>(seed.size());
	const float blend = static_cast<float>(x - sampleIndex1) / static_cast<float>(pitch);
	const float fade = blend * blend * (3 - 2 * blend); // Fade function
	return ((1.0f - fade) * seed[sampleIndex1]) + (fade * seed[sampleIndex2]);
}

void perlin1D(std::span<float>& output, const std::span<const float>& seed, int octaves, float scaleBias)
{
	for (size_t x = 0; x < output.size(); ++x) {
		float noise = 0.0f;
		float scale = 1.0f;
		float scaleSum = 0.0f;
		for (int o = 0; o < octaves; ++o) {
			const int pitch = static_cast<int>(output.size()) >> o;
			if(!pitch) break;
			const float sample = perlin1D(seed, x, pitch);
			noise += sample * scale;
			scaleSum += scale;
			scale /= scaleBias;
		}
		output[x] = noise / scaleSum;
	}
}

float perlin2D(int width, int height, const float* seed, int x, int y, int horizontalPitch, int verticalPitch)
{
	const int sampleIndexX1 = x - (x % horizontalPitch);
	const int sampleIndexY1 = y - (y % verticalPitch);
	const int sampleIndexX2 = (sampleIndexX1 + horizontalPitch) % width;
	const int sampleIndexY2 = (sampleIndexY1 + verticalPitch) % height;
	const float fBlendX = static_cast<float>(x - sampleIndexX1) / static_cast<float>(horizontalPitch);
	const float fBlendY = static_cast<float>(y - sampleIndexY1) / static_cast<float>(verticalPitch);
	const float fadeX = fBlendX * fBlendX * (3 - 2 * fBlendX); // Fade function
	const float fadeY = fBlendY * fBlendY * (3 - 2 * fBlendY); // Fade function
	// The actual samples
	const float sampleTopLeft = seed[(sampleIndexY1 * width) + sampleIndexX1];
	const float sampleTopRight = seed[(sampleIndexY1 * width) + sampleIndexX2];
	const float sampleBottomLeft = seed[(sampleIndexY2 * width) + sampleIndexX1];
	const float sampleBottomRight = seed[(sampleIndexY2 * width) + sampleIndexX2];
	// Sample weights
	const float weightTopLeft = (1 - fadeX) * (1 - fadeY);
	const float weightTopRight = fadeX * (1 - fadeY);
	const float weightBottomLeft = (1 - fadeX) * fadeY;
	const float weightBottomRight = fadeX * fadeY;
	// Now we calculate the actual bilinear weighted average
	return ((sampleTopLeft * weightTopLeft) + (sampleTopRight * weightTopRight) + (sampleBottomLeft * weightBottomLeft) + (sampleBottomRight * weightBottomRight));
}
void perlin2D(int width, int height, float* output, const float* seed, int octaves, float scaleBias)
{
	for (int y = 0; y < height; ++y) {
		float* const outputRow = &output[y * width];
		for (int x = 0; x < width; ++x) {
			float noise = 0.0f;
			float scale = 1.0f;
			float scaleSum = 0.0f;
			for (int o = 0; o < octaves; ++o) {
				const int horizontalPitch = width >> o;
				const int verticalPitch = height >> o;
				if(!(horizontalPitch && verticalPitch)) break;
				const float sample = perlin2D(width,height,seed,x,y,horizontalPitch,verticalPitch);
				noise += sample * scale;
				scaleSum += scale;
				scale /= scaleBias;
			}
			outputRow[x] = noise / scaleSum;
		}
	}
}

void rand_perlin1D(std::span<float>& output, unsigned seed, int octaves, float scaleBias)
{
	std::vector<float> randVec(output.size());
	srand(seed);
	for(auto& it : randVec) {
		it = static_cast<float>(rand()) / float(RAND_MAX);
	}
	perlin1D(output,randVec,octaves,scaleBias);
}

void rand_perlin1D(std::span<float>& output, unsigned seed, int octaves, float valA, float valB, float scaleBias)
{
	std::vector<float> randVec(output.size());
	srand(seed);
	for(auto& it : randVec) {
		it = static_cast<float>(rand()) / float(RAND_MAX);
	}
	randVec[0] = valA;
	randVec[randVec.size() - 1] = valB;
	perlin1D(output,randVec,octaves,scaleBias);
}

void rand_perlin2D(int width, int height, float* output, unsigned seed, int octaves, float scaleBias)
{
	std::vector<float> randVec(width * height);
	srand(seed);
	for(auto& it : randVec) {
		it = static_cast<float>(rand()) / float(RAND_MAX);
	}
	perlin2D(width,height,output,randVec.data(),octaves,scaleBias);
}

void lehmer_perlin1D(std::span<float>& output, RNG& rng, int octaves, float scaleBias)
{
	std::vector<float> randVec(output.size());
	for(auto& it : randVec) {
		it = rng.generateDouble();
	}
	perlin1D(output,randVec,octaves,scaleBias);
}


void lehmer_perlin2D(int width, int height, float* output, RNG& rng, int octaves, float scaleBias)
{
	std::vector<float> randVec(width * height);
	for(auto& it : randVec) {
		it = rng.generateDouble();
	}
	perlin2D(width,height,output,randVec.data(),octaves,scaleBias);
}

void bilinearPerlin1D(std::span<float>& output, const std::span<const float>& seed, int octaves, float valA, float valB, float scaleBias, float cornerBias)
{
	const float inverseCornerBias = 1.0f - cornerBias;
	for (size_t x = 0; x < output.size(); ++x) {
		float noise = 0.0f;
		float scale = 1.0f;
		float scaleSum = 0.0f;
		for (int o = 0; o < octaves; ++o) {
			if(o == 0) {
				const float linearPos = static_cast<float>(x) / static_cast<float>(output.size() - 1);
				const float interpolated = ((1.0f - linearPos) * valA) + (linearPos * valB);
				noise = interpolated;
				scaleSum += scale;
			} else {
				const int pitch = static_cast<int>(output.size()) >> o;
				if(!pitch) break;
				noise += perlin1D(seed, x, pitch) * (scale * inverseCornerBias);
				scaleSum += scale * inverseCornerBias;
			}
			scale /= scaleBias;
		}
		output[x] = noise / scaleSum;
	}
}

void bilinearPerlin2D(int width, int height, float* output, const float* seed, int octaves, float topLeft, float topRight, float bottomLeft, float bottomRight, float scaleBias, float cornerBias)
{
	const float inverseCornerBias = 1.0f - cornerBias;
	for (int y = 0; y < height; ++y) {
		float* const outputRow = &output[y * width];
		for (int x = 0; x < width; ++x) {
			float noise = 0.0f;
			float scale = 1.0f;
			float scaleSum = 0.0f;
			for (int o = 0; o < octaves; ++o) {
				if(o == 0) {
					const float scaleX = static_cast<float>(x) / static_cast<float>(width - 1);
					const float scaleY = static_cast<float>(y) / static_cast<float>(height - 1);
					// Sample weights
					const float weightTopLeft = (1 - scaleX) * (1 - scaleY);
					const float weightTopRight = scaleX * (1 - scaleY);
					const float weightBottomLeft = (1 - scaleX) * scaleY;
					const float weightBottomRight = scaleX * scaleY;
					noise = ((topLeft * weightTopLeft) + (topRight * weightTopRight) + (bottomLeft * weightBottomLeft) + (bottomRight * weightBottomRight));
					scaleSum += scale;
				}
				else {
					const int horizontalPitch = width >> o;
					const int verticalPitch = height >> o;
					if(!(horizontalPitch && verticalPitch)) break;
					noise += perlin2D(width,height,seed,x,y,horizontalPitch,verticalPitch) * (scale * inverseCornerBias);
					scaleSum += scale * inverseCornerBias;
				}
				scale /= scaleBias;
			}
			outputRow[x] = noise / scaleSum;
		}
	}
}

void rand_bilinearPerlin1D(std::span<float>& output, unsigned seed, int octaves, float valA, float valB, float scaleBias, float cornerBias)
{
	std::vector<float> randVec(output.size());
	srand(seed);
	for(auto& it : randVec) {
		it = static_cast<float>(rand()) / float(RAND_MAX);
	}
	bilinearPerlin1D(output,randVec,octaves,valA,valB,scaleBias,cornerBias);
}

void rand_bilinearPerlin2D(int width, int height, float* output, unsigned seed, int octaves, float topLeft, float topRight, float bottomLeft, float bottomRight, float scaleBias, float cornerBias)
{
	std::vector<float> randVec(width * height);
	srand(seed);
	for(auto& it : randVec) {
		it = static_cast<float>(rand()) / float(RAND_MAX);
	}
	bilinearPerlin2D(width,height,output,randVec.data(),octaves,topLeft,topRight,bottomLeft,bottomRight,scaleBias,cornerBias);
}

void lehmer_bilinearPerlin1D(std::span<float>& output, RNG& rng, int octaves, float valA, float valB, float scaleBias, float cornerBias)
{
	std::vector<float> randVec(output.size());
	for(auto& it : randVec) {
		it = rng.generateDouble();
	}
	bilinearPerlin1D(output,randVec,octaves,valA,valB,scaleBias,cornerBias);
}

void lehmer_bilinearPerlin2D(int width, int height, float* output, RNG& rng, int octaves, float topLeft, float topRight, float bottomLeft, float bottomRight, float scaleBias, float cornerBias)
{
	std::vector<float> randVec(width * height);
	for(auto& it : randVec) {
		it = rng.generateDouble();
	}
	bilinearPerlin2D(width,height,output,randVec.data(),octaves,topLeft,topRight,bottomLeft,bottomRight,scaleBias,cornerBias);
}

}
}
}
