#include "MhNoise.hpp"
#include <cstdlib>
namespace MH33 {
namespace Util {
namespace Noise {

float perlin1D(const std::span<const float>& seed, unsigned x, int pitch)
{
	const unsigned sampleIndex1 = x - (x % pitch);
	const unsigned sampleIndex2 = (sampleIndex1 + pitch) % static_cast<int>(seed.size());
	const float blend = static_cast<float>(x - sampleIndex1) / static_cast<float>(pitch);
	const float fade = blend * blend * (3 - 2 * blend); // Fade function
	return ((1.0f - fade) * seed[sampleIndex1]) + (fade * seed[sampleIndex2]);
}

void perlin1D(const std::span<float>& output, const std::span<const float>& seed, unsigned octaves, float scaleBias, bool normalize)
{
	for (size_t x = 0; x < output.size(); ++x) {
		float noise = 0.0f;
		float scale = 1.0f;
		float scaleSum = 0.0f;
		for (unsigned o = 0; o < octaves; ++o) {
			const unsigned pitch = static_cast<unsigned>(output.size()) >> o;
			if(!pitch) break;
			const float sample = perlin1D(seed, x, pitch);
			noise += sample * scale;
			scaleSum += scale;
			scale /= scaleBias;
		}
		output[x] = noise / scaleSum;
	}
	if(normalize) {
		float min = std::numeric_limits<float>::max();
		float max = 0.0f;
		for (const float it : output) {
			min = std::min(min,it);
			max = std::max(max,it);
		}
		const float dist = max - min;
		for (float& it : output) {
			it = (it - min) / dist;
		}
	}
}

float perlin2D(unsigned width, unsigned height, const float* seed, unsigned x, unsigned y, int horizontalPitch, int verticalPitch)
{
	const unsigned sampleIndexX1 = x - (x % horizontalPitch);
	const unsigned sampleIndexY1 = y - (y % verticalPitch);
	const unsigned sampleIndexX2 = (sampleIndexX1 + horizontalPitch) % width;
	const unsigned sampleIndexY2 = (sampleIndexY1 + verticalPitch) % height;
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
void perlin2D(unsigned width, unsigned height, float* output, const float* seed, unsigned octaves, float scaleBias, bool normalize)
{
	for (unsigned y = 0; y < height; ++y) {
		float* const outputRow = &output[y * width];
		for (unsigned x = 0; x < width; ++x) {
			float noise = 0.0f;
			float scale = 1.0f;
			float scaleSum = 0.0f;
			for (unsigned o = 0; o < octaves; ++o) {
				const unsigned horizontalPitch = width >> o;
				const unsigned verticalPitch = height >> o;
				if(!(horizontalPitch && verticalPitch)) break;
				const float sample = perlin2D(width,height,seed,x,y,horizontalPitch,verticalPitch);
				noise += sample * scale;
				scaleSum += scale;
				scale /= scaleBias;
			}
			outputRow[x] = noise / scaleSum;
		}
	}
	if(normalize) {
		std::span<float> outSpan(output, width * height);
		float min = std::numeric_limits<float>::max();
		float max = 0.0f;
		for (const float it : outSpan) {
			min = std::min(min,it);
			max = std::max(max,it);
		}
		const float dist = max - min;
		for (float& it : outSpan) {
			it = (it - min) / dist;
		}
	}
}

void rand_perlin1D(std::span<float>& output, unsigned seed, unsigned octaves, float scaleBias, bool normalize)
{
	std::vector<float> randVec(output.size());
	srand(seed);
	for(auto& it : randVec) {
		it = static_cast<float>(rand()) / float(RAND_MAX);
	}
	perlin1D(output,randVec,octaves,scaleBias,normalize);
}

void rand_perlin2D(unsigned width, unsigned height, float* output, unsigned seed, unsigned octaves, float scaleBias, bool normalize)
{
	std::vector<float> randVec(width * height);
	srand(seed);
	for(auto& it : randVec) {
		it = static_cast<float>(rand()) / float(RAND_MAX);
	}
	perlin2D(width,height,output,randVec.data(),octaves,scaleBias,normalize);
}

void lehmer_perlin1D(const std::span<float>& output, RNG& rng, unsigned octaves, float scaleBias, bool normalize)
{
	std::vector<float> randVec(output.size());
	for(auto& it : randVec) {
		it = rng.generateFloat();
	}
	perlin1D(output,randVec,octaves,scaleBias,normalize);
}


void lehmer_perlin2D(unsigned width, unsigned height, float* output, RNG& rng, unsigned octaves, float scaleBias, bool normalize)
{
	std::vector<float> randVec(width * height);
	for(auto& it : randVec) {
		it = rng.generateFloat();
	}
	perlin2D(width,height,output,randVec.data(),octaves,scaleBias,normalize);
}

void bilinearPerlin1D(const std::span<float>& output, const std::span<const float>& seed, unsigned octaves, float valA, float valB, float scaleBias, float cornerBias)
{
	const float inverseCornerBias = 1.0f - cornerBias;
	for (size_t x = 0; x < output.size(); ++x) {
		float noise = 0.0f;
		float scale = 1.0f;
		float scaleSum = 0.0f;
		for (unsigned o = 0; o < octaves; ++o) {
			if(o == 0) {
				const float linearPos = static_cast<float>(x) / static_cast<float>(output.size() - 1);
				const float interpolated = ((1.0f - linearPos) * valA) + (linearPos * valB);
				noise = interpolated;
				scaleSum += scale;
			} else {
				const unsigned pitch = static_cast<unsigned>(output.size()) >> o;
				if(!pitch) break;
				noise += perlin1D(seed, x, pitch) * (scale * inverseCornerBias);
				scaleSum += scale * inverseCornerBias;
			}
			scale /= scaleBias;
		}
		output[x] = noise / scaleSum;
	}
}

void bilinearPerlin2D(unsigned width, unsigned height, float* output, const float* seed, unsigned octaves, float topLeft, float topRight, float bottomLeft, float bottomRight, float scaleBias, float cornerBias)
{
	const float inverseCornerBias = 1.0f - cornerBias;
	for (unsigned y = 0; y < height; ++y) {
		float* const outputRow = &output[y * width];
		for (unsigned x = 0; x < width; ++x) {
			float noise = 0.0f;
			float scale = 1.0f;
			float scaleSum = 0.0f;
			for (unsigned o = 0; o < octaves; ++o) {
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
					const unsigned horizontalPitch = width >> o;
					const unsigned verticalPitch = height >> o;
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

void rand_bilinearPerlin1D(const std::span<float>& output, unsigned seed, unsigned octaves, float valA, float valB, float scaleBias, float cornerBias)
{
	std::vector<float> randVec(output.size());
	srand(seed);
	for(auto& it : randVec) {
		it = static_cast<float>(rand()) / float(RAND_MAX);
	}
	bilinearPerlin1D(output,randVec,octaves,valA,valB,scaleBias,cornerBias);
}

void rand_bilinearPerlin2D(unsigned width, unsigned height, float* output, unsigned seed, unsigned octaves, float topLeft, float topRight, float bottomLeft, float bottomRight, float scaleBias, float cornerBias)
{
	std::vector<float> randVec(width * height);
	srand(seed);
	for(auto& it : randVec) {
		it = static_cast<float>(rand()) / float(RAND_MAX);
	}
	bilinearPerlin2D(width,height,output,randVec.data(),octaves,topLeft,topRight,bottomLeft,bottomRight,scaleBias,cornerBias);
}

void lehmer_bilinearPerlin1D(const std::span<float>& output, RNG& rng, unsigned octaves, float valA, float valB, float scaleBias, float cornerBias)
{
	std::vector<float> randVec(output.size());
	for(auto& it : randVec) {
		it = rng.generateFloat();
	}
	bilinearPerlin1D(output,randVec,octaves,valA,valB,scaleBias,cornerBias);
}

void lehmer_bilinearPerlin2D(unsigned width, unsigned height, float* output, RNG& rng, unsigned octaves, float topLeft, float topRight, float bottomLeft, float bottomRight, float scaleBias, float cornerBias)
{
	std::vector<float> randVec(width * height);
	for(auto& it : randVec) {
		it = rng.generateFloat();
	}
	bilinearPerlin2D(width,height,output,randVec.data(),octaves,topLeft,topRight,bottomLeft,bottomRight,scaleBias,cornerBias);
}

void voronoi1D(const std::span<float>& output, const std::span<const unsigned>& points, bool invert)
{
	unsigned maxDistance = 0;
	for(unsigned x = 0; x < output.size(); ++x) {
		unsigned minDistance = std::numeric_limits<unsigned>::max();
		for(const auto point : points) {
			const unsigned distance = std::max(x,point) - std::min(x,point);
			minDistance = std::min(minDistance,distance);
		}
		maxDistance = std::max(maxDistance,minDistance);
		output[x] = static_cast<float>(minDistance);
	}
	// Normalize
	for(float& it : output) {
		it /= maxDistance;
		if(invert) it = 1.0f - it;
	}
}

void voronoi2D(unsigned width, unsigned height, float* output, const std::span<const glm::uvec2>& points, bool squared, bool invert)
{
	float maxDistance = 0.0f;
	for(unsigned y = 0; y < height; ++y) {
		float * const outputRow = &output[y * width];
		for(unsigned x = 0; x < width; ++x) {
			float& outputPoint = outputRow[x];
			unsigned minDistance = std::numeric_limits<unsigned>::max();
			for(const auto& point : points) {
				const unsigned distanceX = std::max(x,point.x) - std::min(x,point.x);
				const unsigned distanceY = std::max(y,point.y) - std::min(y,point.y);
				const unsigned squaredDistance = (distanceX * distanceX) + (distanceY * distanceY);
				minDistance = std::min(minDistance,squaredDistance);
			}
			outputPoint = squared ? static_cast<float>(minDistance) : std::sqrt(static_cast<float>(minDistance));
			maxDistance = std::max(maxDistance,outputPoint);
		}
	}
	std::span<float> outSpan(output,width * height);
	for(float& it : outSpan) {
		it /= maxDistance;
		if(invert) it = 1.0f - it;
	}
}

void rand_voronoi1D(const std::span<float>& output, unsigned sectorSize, const RandFunction1D& seed, bool invert)
{
	std::vector<unsigned> randVec((output.size() % sectorSize) ? ((output.size() / sectorSize)+1) : (output.size() / sectorSize));
	unsigned sectorOffset = 0;
	for(auto& it : randVec) {
		it = sectorOffset + seed(sectorSize);
		sectorOffset += sectorSize;
	}
	voronoi1D(output,randVec,invert);
}

void rand_voronoi2D(unsigned width, unsigned height, float* output, unsigned sectorWidth, unsigned sectorHeight, const RandFunction2D& seed, bool squared, bool invert)
{
	const unsigned sectorCountX = (width % sectorWidth) ? ((width / sectorWidth)+1) : (width / sectorWidth);
	const unsigned sectorCountY = (height % sectorHeight) ? ((height / sectorHeight)+1) : (height / sectorHeight);
	std::vector<glm::uvec2> randVec(sectorCountX*sectorCountY);
	for(unsigned y = 0; y < sectorCountY; ++y) {
		auto * const rowPtr = &randVec[y * sectorCountX];
		for(unsigned x = 0; x < sectorCountX; ++x) {
			auto& dataRef = rowPtr[x];
			dataRef = seed(glm::uvec2(sectorWidth,sectorHeight)) + (glm::uvec2(sectorWidth,sectorHeight) * glm::uvec2(x,y));
		}
	}
	voronoi2D(width,height,output,randVec,squared,invert);
}

void rand_voronoi1D(const std::span<float>& output, unsigned sectorSize, unsigned seed, bool invert)
{
	srand(seed);
	rand_voronoi1D(output,sectorSize,[](unsigned max){ return rand() % max; }, invert);
}

void rand_voronoi2D(unsigned width, unsigned height, float* output, unsigned sectorWidth, unsigned sectorHeight, unsigned seed, bool squared, bool invert)
{
	srand(seed);
	rand_voronoi2D(width,height,output,sectorWidth,sectorHeight,
				   [](const glm::uvec2& sectorSize) { return glm::uvec2(rand() % sectorSize.x, rand() % sectorSize.y ); },
	squared, invert);
}

void lehmer_voronoi1D(const std::span<float>& output, unsigned sectorSize, RNG& rng, bool invert)
{
	rand_voronoi1D(output,sectorSize,[&rng](unsigned max){ return rng.generate(max); }, invert);
}

void lehmer_voronoi2D(unsigned width, unsigned height, float* output, unsigned sectorWidth, unsigned sectorHeight, RNG& rng, bool squared, bool invert)
{
	rand_voronoi2D(width,height,output,sectorWidth,sectorHeight,
				   [&rng](const glm::uvec2& sectorSize) { return glm::uvec2(rng.generate(sectorSize.x), rng.generate(sectorSize.y) ); },
	squared, invert);
}

void rand_whiteNoise(const std::span<float>& output, unsigned seed)
{
	srand(seed);
	for(float& it : output) {
		it = static_cast<float>(rand()) / float(RAND_MAX);
	}
}

void lehmer_whiteNoise(const std::span<float>& output, RNG& rng)
{
	for(float& it : output) {
		it = rng.generateFloat();
	}
}

}
}
}
