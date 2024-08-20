#include "MhNoise.hpp"
#include <cstdlib>
namespace MH33 {
namespace Util {
namespace Noise {

void normalize(const std::span<float>& output)
{
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

float bilinearAverage(float tX, float tY, float topLeft, float topRight, float bottomLeft, float bottomRight)
{
	const float weightTopLeft = (1 - tX) * (1 - tY);
	const float weightTopRight = tX * (1 - tY);
	const float weightBottomLeft = (1 - tX) * tY;
	const float weightBottomRight = tX * tY;
	// Now we calculate the actual bilinear weighted average
	return ((topLeft * weightTopLeft) + (topRight * weightTopRight) + (bottomLeft * weightBottomLeft) + (bottomRight * weightBottomRight));
}

void remapToSigned(const std::span<float>& output)
{
	for(auto& it : output) {
		it = (it - 0.5f) * 2.0f;
	}
}

void remapToUnsigned(const std::span<float>& output)
{
	for(auto& it : output) {
		it = ((it * 0.5f) + 1.0f) * 2.0f;
	}
}

float perlin1D(const std::span<const float>& seed, unsigned x, int pitch)
{
	const unsigned sampleIndex1 = x - (x % pitch);
	const unsigned sampleIndex2 = (sampleIndex1 + pitch) % static_cast<int>(seed.size());
	const float blend = static_cast<float>(x - sampleIndex1) / static_cast<float>(pitch);
	const float fade = blend * blend * (3 - 2 * blend); // Fade function
	return ((1.0f - fade) * seed[sampleIndex1]) + (fade * seed[sampleIndex2]);
}

void perlin1D(const std::span<float>& output, const std::span<const float>& seed, unsigned octaves, float scaleBias, bool shouldNormalize)
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
	if(shouldNormalize) {
		normalize(output);
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
	// Now we calculate the actual bilinear weighted average
	return bilinearAverage(fadeX, fadeY, sampleTopLeft, sampleTopRight, sampleBottomLeft, sampleBottomRight);
}
void perlin2D(unsigned width, unsigned height, float* output, const float* seed, unsigned octaves, float scaleBias, bool shouldNormalize)
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
	if(shouldNormalize) {
		normalize(std::span<float>(output, width * height));
	}
}

void rand_perlin1D(const std::span<float>& output, unsigned seed, unsigned octaves, float scaleBias, bool shouldNormalize)
{
	std::vector<float> randVec(output.size());
	rand_whiteNoise(randVec, seed);
	perlin1D(output,randVec,octaves,scaleBias,shouldNormalize);
}

void rand_perlin2D(unsigned width, unsigned height, float* output, unsigned seed, unsigned octaves, float scaleBias, bool shouldNormalize)
{
	std::vector<float> randVec(width * height);
	rand_whiteNoise(randVec, seed);
	perlin2D(width,height,output,randVec.data(),octaves,scaleBias,shouldNormalize);
}

void lehmer_perlin1D(const std::span<float>& output, RNG& rng, unsigned octaves, float scaleBias, bool shouldNormalize)
{
	std::vector<float> randVec(output.size());
	lehmer_whiteNoise(randVec, rng);
	perlin1D(output,randVec,octaves,scaleBias,shouldNormalize);
}


void lehmer_perlin2D(unsigned width, unsigned height, float* output, RNG& rng, unsigned octaves, float scaleBias, bool shouldNormalize)
{
	std::vector<float> randVec(width * height);
	lehmer_whiteNoise(randVec, rng);
	perlin2D(width,height,output,randVec.data(),octaves,scaleBias,shouldNormalize);
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
					noise = bilinearAverage(scaleX, scaleY, topLeft, topRight, bottomLeft, bottomRight);
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
	rand_whiteNoise(randVec, seed);
	bilinearPerlin1D(output,randVec,octaves,valA,valB,scaleBias,cornerBias);
}

void rand_bilinearPerlin2D(unsigned width, unsigned height, float* output, unsigned seed, unsigned octaves, float topLeft, float topRight, float bottomLeft, float bottomRight, float scaleBias, float cornerBias)
{
	std::vector<float> randVec(width * height);
	rand_whiteNoise(randVec, seed);
	bilinearPerlin2D(width,height,output,randVec.data(),octaves,topLeft,topRight,bottomLeft,bottomRight,scaleBias,cornerBias);
}

void lehmer_bilinearPerlin1D(const std::span<float>& output, RNG& rng, unsigned octaves, float valA, float valB, float scaleBias, float cornerBias)
{
	std::vector<float> randVec(output.size());
	lehmer_whiteNoise(randVec, rng);
	bilinearPerlin1D(output,randVec,octaves,valA,valB,scaleBias,cornerBias);
}

void lehmer_bilinearPerlin2D(unsigned width, unsigned height, float* output, RNG& rng, unsigned octaves, float topLeft, float topRight, float bottomLeft, float bottomRight, float scaleBias, float cornerBias)
{
	std::vector<float> randVec(width * height);
	lehmer_whiteNoise(randVec, rng);
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
	// normalize
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

void valueNoise1D(const std::span<float>& output, const std::span<const float>& points, Interpolation interpolation)
{
	const float outputSizeReciprocal = 1.0f / static_cast<float>(output.size() - 1);
	for(size_t x = 0; x < output.size(); ++x) {
		const float texelIndex = (static_cast<float>(x) * outputSizeReciprocal) * static_cast<float>(points.size() - 1);
		const float t = texelIndex - std::floor(texelIndex);
		const unsigned lowerIndex = static_cast<unsigned>(std::floor(texelIndex));
		const unsigned upperIndex = static_cast<unsigned>(std::ceil(texelIndex));
		const float lowerValue = points[lowerIndex];
		const float upperValue = points[upperIndex];
		switch (interpolation) {
			case MH33::Util::Noise::Interpolation::LINEAR: {
				output[x] = std::lerp(lowerValue,upperValue, t);
				break; }
			case MH33::Util::Noise::Interpolation::COSINE: {
				const float tRemapCosine = (1 - std::cos(t * M_PI)) * 0.5;
				output[x] = std::lerp(lowerValue,upperValue, tRemapCosine);
				break; }
			case MH33::Util::Noise::Interpolation::SMOOTHSTEP: {
				const float tRemapSmoothstep = t * t * (3.0f - 2.0f * t);
				output[x] = std::lerp(lowerValue,upperValue, tRemapSmoothstep);
				break; }
		}
	}
}

void valueNoise2D(float* output, unsigned outWidth, unsigned outHeight, const float* input, unsigned inWidth, unsigned inHeight, Interpolation interpolation)
{
	const float outWidthReciprocal = 1.0f / static_cast<float>(outWidth - 1);
	const float outHeightReciprocal = 1.0f / static_cast<float>(outHeight - 1);
	for(unsigned y = 0; y < outHeight; ++y) {
		float * const outRow = &output[outWidth * y];
		const float texelIndexY = (static_cast<float>(y) * outHeightReciprocal) * static_cast<float>(inHeight - 1);
		const unsigned lowerIndexY = static_cast<unsigned>(std::floor(texelIndexY));
		const unsigned upperIndexY = static_cast<unsigned>(std::ceil(texelIndexY));
		float tY = texelIndexY - std::floor(texelIndexY);
		switch (interpolation) {
			case MH33::Util::Noise::Interpolation::LINEAR:
				break;
			case MH33::Util::Noise::Interpolation::COSINE:
				tY = (1 - std::cos(tY * M_PI)) * 0.5;
				break;
			case MH33::Util::Noise::Interpolation::SMOOTHSTEP:
				tY = tY * tY * (3.0f - 2.0f * tY);
				break;
		}
		for(unsigned x = 0; x < outWidth; ++x) {
			float& outPoint = outRow[x];
			const float texelIndexX = (static_cast<float>(x) * outWidthReciprocal) * static_cast<float>(inWidth - 1);
			const unsigned lowerIndexX = static_cast<unsigned>(std::floor(texelIndexX));
			const unsigned upperIndexX = static_cast<unsigned>(std::ceil(texelIndexX));
			float tX = texelIndexX - std::floor(texelIndexX);
			switch (interpolation) {
				case MH33::Util::Noise::Interpolation::LINEAR:
					break;
				case MH33::Util::Noise::Interpolation::COSINE:
					tX = (1 - std::cos(tX * M_PI)) * 0.5;
					break;
				case MH33::Util::Noise::Interpolation::SMOOTHSTEP:
					tX = tX * tX * (3.0f - 2.0f * tX);
					break;
			}
			const float sampleTopLeft = input[(lowerIndexY * inWidth) + lowerIndexX];
			const float sampleTopRight = input[(lowerIndexY * inWidth) + upperIndexX];
			const float sampleBottomLeft = input[(upperIndexY * inWidth) + lowerIndexX];
			const float sampleBottomRight = input[(upperIndexY * inWidth) + upperIndexX];
			outPoint = bilinearAverage(tX,tY,sampleTopLeft,sampleTopRight,sampleBottomLeft,sampleBottomRight);
		}
	}
}

void rand_valueNoise1D(const std::span<float>& output, unsigned seed, unsigned pointCount, Interpolation interpolation)
{
	std::vector<float> randVec(pointCount);
	rand_whiteNoise(randVec, seed);
	valueNoise1D(output,randVec,interpolation);
}

void rand_valueNoise2D(float* output, unsigned outWidth, unsigned outHeight, unsigned seed, unsigned inWidth, unsigned inHeight, Interpolation interpolation)
{
	std::vector<float> randVec(inWidth * inHeight);
	rand_whiteNoise(randVec, seed);
	valueNoise2D(output, outWidth, outHeight, randVec.data(), inWidth, inHeight, interpolation);
}

void lehmer_valueNoise1D(const std::span<float>& output, RNG& rng, unsigned pointCount, Interpolation interpolation)
{
	std::vector<float> randVec(pointCount);
	lehmer_whiteNoise(randVec, rng);
	valueNoise1D(output,randVec,interpolation);
}

void lehmer_valueNoise2D(float* output, unsigned outWidth, unsigned outHeight, RNG& rng, unsigned inWidth, unsigned inHeight, Interpolation interpolation)
{
	std::vector<float> randVec(inWidth * inHeight);
	lehmer_whiteNoise(randVec, rng);
	valueNoise2D(output, outWidth, outHeight, randVec.data(), inWidth, inHeight, interpolation);
}

}
}
}
