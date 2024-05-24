#ifndef MHDITHER_HPP
#define MHDITHER_HPP
#include <cstddef>
#include <algorithm>
#include <glm/glm.hpp>
#include <limits>

namespace MH33 {
namespace Util {
template <typename T, T maximum = std::numeric_limits<T>::max() > struct OrderedDither {
	static constexpr const float MAX = ((static_cast<float>(maximum)+1.0f) * 8.0f) - 1.0f;
	static constexpr const float MAX_RECIPROCAL = 1.0f/static_cast<float>(MAX);

	static constexpr const float LookupTable[4][4] = {
		{0.0f * MAX_RECIPROCAL,  8.0f * MAX_RECIPROCAL,  -2.0f * MAX_RECIPROCAL, 10.0f * MAX_RECIPROCAL},
	   {12.0f * MAX_RECIPROCAL,  -4.0f * MAX_RECIPROCAL, 14.0f * MAX_RECIPROCAL,  -6.0f * MAX_RECIPROCAL},
		{-3.0f * MAX_RECIPROCAL, 11.0f * MAX_RECIPROCAL,  -1.0f * MAX_RECIPROCAL,  9.0f * MAX_RECIPROCAL},
	   {15.0f * MAX_RECIPROCAL,  -7.0f * MAX_RECIPROCAL, 13.0f * MAX_RECIPROCAL,  -5.0f * MAX_RECIPROCAL}
	};
	static constexpr float ditherUp(float value, const glm::ivec2& coords) {
		return std::clamp(value + LookupTable[coords.y%4][coords.x%4],0.0f,1.0f);
	}
	static constexpr float ditherDown(float value, const glm::ivec2& coords) {
		return std::clamp(value - LookupTable[coords.y%4][coords.x%4],0.0f,1.0f);
	}
	static constexpr glm::fvec2 ditherUp(const glm::fvec2& value, const glm::ivec2& coords) {
		const auto& lookupVal = LookupTable[coords.y%4][coords.x%4];
		return glm::fvec2(
					std::clamp(value.x + lookupVal, 0.0f, 1.0f),
					std::clamp(value.y + lookupVal, 0.0f, 1.0f)
						 );
	}
	static constexpr glm::fvec2 ditherDown(const glm::fvec2& value, const glm::ivec2& coords) {
		const auto& lookupVal = LookupTable[coords.y%4][coords.x%4];
		return glm::fvec2(
					std::clamp(value.x - lookupVal, 0.0f, 1.0f),
					std::clamp(value.y - lookupVal, 0.0f, 1.0f)
						 );
	}
	static constexpr glm::fvec3 ditherUp(const glm::fvec3& value, const glm::ivec2& coords) {
		const auto& lookupVal = LookupTable[coords.y%4][coords.x%4];
		return glm::fvec3(
					std::clamp(value.x + lookupVal, 0.0f, 1.0f),
					std::clamp(value.y + lookupVal, 0.0f, 1.0f),
					std::clamp(value.z + lookupVal, 0.0f, 1.0f)
						 );
	}
	static constexpr glm::fvec3 ditherDown(const glm::fvec3& value, const glm::ivec2& coords) {
		const auto& lookupVal = LookupTable[coords.y%4][coords.x%4];
		return glm::fvec3(
					std::clamp(value.x - lookupVal, 0.0f, 1.0f),
					std::clamp(value.y - lookupVal, 0.0f, 1.0f),
					std::clamp(value.z - lookupVal, 0.0f, 1.0f)
						 );
	}
	static constexpr glm::fvec4 ditherUp(const glm::fvec4& value, const glm::ivec2& coords) {
		const auto& lookupVal = LookupTable[coords.y%4][coords.x%4];
		return glm::fvec4(
					std::clamp(value.x + lookupVal, 0.0f, 1.0f),
					std::clamp(value.y + lookupVal, 0.0f, 1.0f),
					std::clamp(value.z + lookupVal, 0.0f, 1.0f),
					std::clamp(value.w + lookupVal, 0.0f, 1.0f)
						 );
	}
	static constexpr glm::fvec4 ditherDown(const glm::fvec4& value, const glm::ivec2& coords) {
		const auto& lookupVal = LookupTable[coords.y%4][coords.x%4];
		return glm::fvec4(
					std::clamp(value.x - lookupVal, 0.0f, 1.0f),
					std::clamp(value.y - lookupVal, 0.0f, 1.0f),
					std::clamp(value.z - lookupVal, 0.0f, 1.0f),
					std::clamp(value.w - lookupVal, 0.0f, 1.0f)
						 );
	}

};

constexpr const glm::fvec4 thresholdMatrix[4] = {
glm::fvec4(1.0 / 17.0,  9.0 / 17.0,  3.0 / 17.0, 11.0 / 17.0),
glm::fvec4(13.0 / 17.0,  5.0 / 17.0, 15.0 / 17.0,  7.0 / 17.0),
glm::fvec4(4.0 / 17.0, 12.0 / 17.0,  2.0 / 17.0, 10.0 / 17.0),
glm::fvec4(16.0 / 17.0,  8.0 / 17.0, 14.0 / 17.0,  6.0 / 17.0)
};
constexpr bool stippleAlpha(float alpha, const glm::ivec2& coords) {
	return (alpha >= thresholdMatrix[coords.x % 4][coords.y % 4]);
}


constexpr const glm::fvec2 LOOKUP[2][2] = {
{  glm::fvec2( 0.25f, 0.00f ), glm::fvec2( 0.50f, 0.75f ) },
{  glm::fvec2( 0.75f, 0.50f ), glm::fvec2( 0.00f, 0.25f ) }
};

}
}
#endif // DITHER_HPP
