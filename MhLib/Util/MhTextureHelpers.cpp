#include "MhTextureHelpers.hpp"
#include <cmath>
#include <algorithm>

namespace MH33 {
namespace Util {
uint32_t argb32(const float alpha)
{
	return static_cast<uint32_t>(std::round(alpha*255.0f)) << 24u;
}

uint32_t argb32(const glm::fvec3& rgbF)
{
	const uint32_t a = 255;
	const uint32_t r = static_cast<uint32_t>(std::round(rgbF.r * 255.0f));
	const uint32_t g = static_cast<uint32_t>(std::round(rgbF.g * 255.0f));
	const uint32_t b = static_cast<uint32_t>(std::round(rgbF.b * 255.0f));
	return (a << 24u) | (r << 16u) << (g << 8u) | b;
}

uint32_t argb32(const glm::fvec4& rgbaF)
{
	const uint32_t a = static_cast<uint32_t>(std::round(rgbaF.a * 255.0f));
	const uint32_t r = static_cast<uint32_t>(std::round(rgbaF.r * 255.0f));
	const uint32_t g = static_cast<uint32_t>(std::round(rgbaF.g * 255.0f));
	const uint32_t b = static_cast<uint32_t>(std::round(rgbaF.b * 255.0f));
	return (a << 24u) | (r << 16u) << (g << 8u) | b;
}

uint32_t rgba32(const float alpha)
{
	return static_cast<uint32_t>(std::round(alpha*255.0f));
}

uint32_t rgba32(const glm::fvec3& rgbF)
{
	const uint32_t a = 255;
	const uint32_t r = static_cast<uint32_t>(std::round(rgbF.r * 255.0f));
	const uint32_t g = static_cast<uint32_t>(std::round(rgbF.g * 255.0f));
	const uint32_t b = static_cast<uint32_t>(std::round(rgbF.b * 255.0f));
	return (r << 24u) | (g << 16u) << (b << 8u) | a;
}

uint32_t rgba32(const glm::fvec4& rgbaF)
{
	const uint32_t a = static_cast<uint32_t>(std::round(rgbaF.a * 255.0f));
	const uint32_t r = static_cast<uint32_t>(std::round(rgbaF.r * 255.0f));
	const uint32_t g = static_cast<uint32_t>(std::round(rgbaF.g * 255.0f));
	const uint32_t b = static_cast<uint32_t>(std::round(rgbaF.b * 255.0f));
	return (r << 24u) | (g << 16u) << (b << 8u) | a;
}


void createButtons(ButtonTextureCollection& buttons, int width, int height, const glm::fvec3& baseColour, const glm::fvec3& borderColour)
{
	const int size = width * height;
	const auto shadowColourA = argb32(glm::mix(baseColour,glm::fvec3(0.0f,0.0f,0.0f),0.5f));
	const auto shadowColourB = argb32(glm::mix(baseColour,glm::fvec3(0.0f,0.0f,0.0f),0.25f));
	const auto baseColour32 = argb32(baseColour);
	const auto borderColour32 = argb32(borderColour);
	const auto borderA = argb32( 0.75f );
	const auto borderB = argb32( 0.75f * 0.5f );
	const auto borderC = argb32( 0.75f * 0.25f );

	buttons.ENABLED_NORMAL.format = MH33::Image::Format::ARGB8U;
	buttons.ENABLED_NORMAL.isAnimated = false;
	buttons.ENABLED_CLICKED.format = MH33::Image::Format::ARGB8U;
	buttons.ENABLED_CLICKED.isAnimated = false;
	buttons.DISABLED_NORMAL.format = MH33::Image::Format::ARGB8U;
	buttons.DISABLED_NORMAL.isAnimated = false;
	buttons.DISABLED_CLICEKD.format = MH33::Image::Format::ARGB8U;
	buttons.DISABLED_CLICEKD.isAnimated = false;
	buttons.ENABLED_NORMAL.frames.resize(1);
	buttons.ENABLED_CLICKED.frames.resize(1);
	buttons.DISABLED_NORMAL.frames.resize(1);
	buttons.DISABLED_CLICEKD.frames.resize(1);

	buttons.ENABLED_NORMAL.frames.push_back({});
	buttons.ENABLED_NORMAL.frames[0].width = width;
	buttons.ENABLED_NORMAL.frames[0].height = height;
	buttons.ENABLED_NORMAL.frames[0].stride = width * sizeof(uint32_t);
	buttons.ENABLED_NORMAL.frames[0].imageData.resize(size * sizeof(uint32_t));

	buttons.ENABLED_CLICKED.frames.push_back({});
	buttons.ENABLED_CLICKED.frames[0].width = width;
	buttons.ENABLED_CLICKED.frames[0].height = height;
	buttons.ENABLED_CLICKED.frames[0].stride = width * sizeof(uint32_t);
	buttons.ENABLED_CLICKED.frames[0].imageData.resize(size * sizeof(uint32_t));

	buttons.DISABLED_NORMAL.frames.push_back({});
	buttons.DISABLED_NORMAL.frames[0].width = width;
	buttons.DISABLED_NORMAL.frames[0].height = height;
	buttons.DISABLED_NORMAL.frames[0].stride = width * sizeof(uint32_t);
	buttons.DISABLED_NORMAL.frames[0].imageData.resize(size * sizeof(uint32_t));

	buttons.DISABLED_CLICEKD.frames.push_back({});
	buttons.DISABLED_CLICEKD.frames[0].width = width;
	buttons.DISABLED_CLICEKD.frames[0].height = height;
	buttons.DISABLED_CLICEKD.frames[0].stride = width * sizeof(uint32_t);
	buttons.DISABLED_CLICEKD.frames[0].imageData.resize(size * sizeof(uint32_t));

	auto ENABLED_NORMAL = std::span<uint32_t>(reinterpret_cast<uint32_t*>(buttons.ENABLED_NORMAL.frames[0].imageData.data()),size);
	auto ENABLED_CLICKED = std::span<uint32_t>(reinterpret_cast<uint32_t*>(buttons.ENABLED_CLICKED.frames[0].imageData.data()),size);
	auto DISABLED_NORMAL = std::span<uint32_t>(reinterpret_cast<uint32_t*>(buttons.DISABLED_NORMAL.frames[0].imageData.data()),size);
	auto DISABLED_CLICEKD = std::span<uint32_t>(reinterpret_cast<uint32_t*>(buttons.DISABLED_CLICEKD.frames[0].imageData.data()),size);

	for(int y = 0; y < height; ++y) {
		uint32_t * const line1 = ENABLED_NORMAL.data() + (y*width);
		uint32_t * const line2 = ENABLED_CLICKED.data() + (y*width);
		uint32_t * const line3 = DISABLED_NORMAL.data() + (y*width);
		uint32_t * const line4 = DISABLED_CLICEKD.data() + (y*width);
		for(int x = 0; x < width; ++x) {
			if(x == 0 || x == (width-1) || y == 0 || y == (height-1)) {
				line1[x] = borderColour32;
				line2[x] = shadowColourA;
				line3[x] = borderA;
				line4[x] = borderA;
			} else if(x == (width-2) || y == (height-2)) {
				line1[x] = shadowColourA;
				line2[x] = borderColour32;
				line3[x] = borderB;
				line4[x] = 0;
			} else if(x == (width-3) || y == (height-3)) {
				line1[x] = shadowColourB;
				line2[x] = shadowColourB;
				line3[x] = borderC;
				line4[x] = borderC;
			} else {
				line1[x] = baseColour32;
				line2[x] = shadowColourA;
				line3[x] = 0;
				line4[x] = borderB;
			}
		}
	}
}

void createCircleTextures(MH33::Image::DecodeTarget& redCircle, MH33::Image::DecodeTarget& greenCircle, MH33::Image::DecodeTarget& blueCircle, int CIRCLE_W, int CIRCLE_H, bool argb)
{
	const int CIRCLE_SIZE = CIRCLE_W * CIRCLE_H;
	const int CIRCLE_ORIGO_X = CIRCLE_W / 2;
	const int CIRCLE_ORIGO_Y = CIRCLE_H / 2;
	const float CIRCLE_RADIUS = static_cast<float>(std::min(CIRCLE_ORIGO_X,CIRCLE_ORIGO_Y));

	//const auto format = argb ? MH33::Image::Format::ARGB8U : MH33::Image::Format::RGBA8U;
	const auto format = argb ? MH33::Image::Format::RGBA8U : MH33::Image::Format::ARGB8U;

	redCircle.frames.push_back({});
	redCircle.format = format;
	redCircle.isAnimated = false;
	redCircle.frames[0].height = CIRCLE_H;
	redCircle.frames[0].width = CIRCLE_W;
	redCircle.frames[0].stride = CIRCLE_W * sizeof(uint32_t);
	redCircle.frames[0].imageData.resize(CIRCLE_SIZE * sizeof(uint32_t));
	auto redCircleF = std::span<uint32_t>(reinterpret_cast<uint32_t*>(redCircle.frames[0].imageData.data()),CIRCLE_SIZE);

	greenCircle.frames.push_back({});
	greenCircle.format = format;
	greenCircle.isAnimated = false;
	greenCircle.frames[0].imageData.resize(CIRCLE_SIZE * sizeof(uint32_t));
	greenCircle.frames[0].height = CIRCLE_H;
	greenCircle.frames[0].width = CIRCLE_W;
	greenCircle.frames[0].stride = CIRCLE_W * sizeof(uint32_t);
	auto greenCircleF = std::span<uint32_t>(reinterpret_cast<uint32_t*>(greenCircle.frames[0].imageData.data()),CIRCLE_SIZE);

	blueCircle.frames.push_back({});
	blueCircle.format = format;
	blueCircle.isAnimated = false;
	blueCircle.frames[0].imageData.resize(CIRCLE_SIZE * sizeof(uint32_t));
	blueCircle.frames[0].height = CIRCLE_H;
	blueCircle.frames[0].width = CIRCLE_W;
	blueCircle.frames[0].stride = CIRCLE_W * sizeof(uint32_t);
	auto blueCircleF = std::span<uint32_t>(reinterpret_cast<uint32_t*>(blueCircle.frames[0].imageData.data()),CIRCLE_SIZE);

	//redCircle.resize(CIRCLE_SIZE);
	//greenCircle.resize(CIRCLE_SIZE);
	//blueCircle.resize(CIRCLE_SIZE);
	for(int y = 0; y < CIRCLE_H; ++y) {
		uint32_t * const line1 = &redCircleF[y*CIRCLE_W];
		uint32_t * const line2 = &greenCircleF[y*CIRCLE_W];
		uint32_t * const line3 = &blueCircleF[y*CIRCLE_W];
		const float distanceY = std::abs(static_cast<float>(y) - static_cast<float>(CIRCLE_ORIGO_Y));
		for(int x = 0; x < CIRCLE_W; ++x) {
			const float distanceX = std::abs(static_cast<float>(x) - static_cast<float>(CIRCLE_ORIGO_X));
			const float distance = std::sqrt(distanceX * distanceX + distanceY * distanceY);
			if(distance >= CIRCLE_RADIUS) {
				line1[x] = 0;
				line2[x] = 0;
				line3[x] = 0;
			} else {
				const float intens_base = 1.0f - (distance / CIRCLE_RADIUS);
				const float intens_base2 = std::clamp((intens_base - 0.33f) * 2.0f ,0.0f,1.0f);
				const uint32_t intensity = static_cast<uint32_t>( ((intens_base >= 0.5f) ? 1.0f : intens_base * 2.0f) * 255.0f);
				const uint32_t intensity2 = static_cast<uint32_t>(intens_base2 * 255.0f);
				if(argb) {
					line1[x] = (intensity << 24u) | (intensity2 << 16u) | (intensity2 << 8u) |  255u;
					line2[x] = (intensity << 24u) | (intensity2 << 16u) | (255u << 8u) | intensity2;
					line3[x] = (intensity << 24u) | (255u << 16u) | (intensity2 << 8u) | intensity2;
				} else {
					line1[x] = (intensity2 << 24u) | (intensity2 << 16u) |  (255u << 8u) | (intensity);
					line2[x] = (intensity2 << 24u) | (255u << 16u) | (intensity2 << 8u) | (intensity);
					line3[x] = (255u << 24u) | (intensity2 << 16u) | (intensity2 << 8u) | (intensity);
				}
			}
		}
	}
}

void createCircleTexture(MH33::Image::DecodeTarget& output, const glm::fvec3& colour, int CIRCLE_W, int CIRCLE_H, bool argb)
{
	const int CIRCLE_SIZE = CIRCLE_W * CIRCLE_H;
	const int CIRCLE_ORIGO_X = CIRCLE_W / 2;
	const int CIRCLE_ORIGO_Y = CIRCLE_H / 2;
	const float CIRCLE_RADIUS = static_cast<float>(std::min(CIRCLE_ORIGO_X,CIRCLE_ORIGO_Y));

	output.frames.push_back({});
	output.format = MH33::Image::Format::ARGB8U;
	output.isAnimated = false;
	output.frames[0].height = CIRCLE_H;
	output.frames[0].width = CIRCLE_W;
	output.frames[0].stride = CIRCLE_W * sizeof(uint32_t);
	output.frames[0].imageData.resize(CIRCLE_SIZE * sizeof(uint32_t));
	auto outputF = std::span<uint32_t>(reinterpret_cast<uint32_t*>(output.frames[0].imageData.data()),CIRCLE_SIZE);

	//output.resize(CIRCLE_SIZE);
	for(int y = 0; y < CIRCLE_H; ++y) {
		const float distanceY = std::abs(static_cast<float>(y) - static_cast<float>(CIRCLE_ORIGO_Y));
		uint32_t * const line = &outputF[y*CIRCLE_W];
		for(int x = 0; x < CIRCLE_W; ++x) {
			const float distanceX = std::abs(static_cast<float>(x) - static_cast<float>(CIRCLE_ORIGO_X));
			const float distance = std::sqrt(distanceX * distanceX + distanceY * distanceY);
			if(distance >= CIRCLE_RADIUS) {
				line[x] = 0;
			} else {
				const float intens_base = 1.0f - (distance / CIRCLE_RADIUS);
				const float intens_base2 = std::clamp((intens_base - 0.33f) * 2.0f ,0.0f,1.0f);
				if(argb) line[x] = argb32(glm::fvec4(
							((intens_base >= 0.5f) ? 1.0f : intens_base * 2.0f),
							std::max(colour.r,intens_base2),
							std::max(colour.g,intens_base2),
							std::max(colour.b,intens_base2)
							));
				else  line[x] = rgba32(glm::fvec4(
										   ((intens_base >= 0.5f) ? 1.0f : intens_base * 2.0f),
										   std::max(colour.r,intens_base2),
										   std::max(colour.g,intens_base2),
										   std::max(colour.b,intens_base2)
										   ));
			}
		}
	}
}

}
}
