#include "MhNoise.hpp"
#include <cstdlib>
namespace MH33 {
namespace Util {
namespace Noise {

void perlin1D(std::span<float>& output, const std::span<const float>& seed, int octaves, float scaleBias)
{
	for (size_t x = 0; x < output.size(); ++x) {
		float noise = 0.0f;
		float scale = 1.0f;
		float scaleSum = 0.0f;
		for (int o = 0; o < octaves; ++o) {
			const int pitch = static_cast<int>(output.size()) >> o;
			if(!pitch) break;
			const int sampleIndex1 = static_cast<int>(x) - (static_cast<int>(x) % pitch);
			const int sampleIndex2 = (sampleIndex1 + pitch) % static_cast<int>(output.size());
			const float blend = static_cast<float>(x - sampleIndex1) / static_cast<float>(pitch);
			const float fade = blend * blend * (3 - 2 * blend); // Fade function
			const float sample = ((1.0f - fade) * seed[sampleIndex1]) + (fade * seed[sampleIndex2]);
			noise += sample * scale;
			scaleSum += scale;
			scale /= scaleBias;
		}
		output[x] = noise / scaleSum;
	}
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
				const float sample = (sampleTopLeft * weightTopLeft) + (sampleTopRight * weightTopRight) + (sampleBottomLeft * weightBottomLeft) + (sampleBottomRight * weightBottomRight);
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

void rand_perlin2D(int width, int height, float* output, unsigned seed, int octaves, float valA, float valB, float valC, float valD, float scaleBias)
{
	std::vector<float> randVec(width * height);
	srand(seed);
	for(auto& it : randVec) {
		it = static_cast<float>(rand()) / float(RAND_MAX);
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

void lehmer_perlin1D(std::span<float>& output, RNG& rng, int octaves, float scaleBias)
{
	std::vector<float> randVec(output.size());
	for(auto& it : randVec) {
		it = rng.generateDouble();
	}
	perlin1D(output,randVec,octaves,scaleBias);
}

void lehmer_perlin1D(std::span<float>& output, RNG& rng, int octaves, float valA, float valB, float scaleBias)
{
	std::vector<float> randVec(output.size());
	for(auto& it : randVec) {
		it = rng.generateDouble();
	}
	randVec[0] = valA;
	randVec[randVec.size() - 1] = valB;
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

void lehmer_perlin2D(int width, int height, float* output, RNG& rng, int octaves, float valA, float valB, float valC, float valD, float scaleBias)
{
	std::vector<float> randVec(width * height);
	for(auto& it : randVec) {
		it = rng.generateDouble();
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
