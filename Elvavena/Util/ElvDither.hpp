#ifndef ELVDITHER_HPP
#define ELVDITHER_HPP
/**
 * @file ElvDither.hpp
 * @brief Declares the ElvDither API in the Elvavena/Util module.
 *
 * This header is part of the public declaration surface for Elvavena/Util.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <algorithm>
#include <glm/glm.hpp>
#include <limits>
#include <Elvavena/Util/half.hpp>
namespace Elv {
namespace Util {

/**
 * @brief Template struct for ordered dithering.
 *
 * This struct implements an ordered dithering algorithm using a 4x4 lookup table.
 * The dithering behavior depends on the type T and its maximum value.
 *
 * @tparam T The data type for the dithering process.
 * @tparam maximum The maximum value for type T. If not specified, it defaults to the maximum value of T if T is not a floating-point type, or 1.0f if T is a floating-point type.
 */
template <typename T, T maximum = (std::is_floating_point<T>::value ? T(1) : std::numeric_limits<T>::max())>
struct OrderedDither {
	/**
	 * @brief Maximum value used in the dithering process.
	 *
	 * This value is calculated based on whether T is a floating-point type.
	 */
	static constexpr const float MAX = (std::is_floating_point<T>::value)
										   ? (8.0f - 1.0f)
										   : ((static_cast<float>(maximum) + 1.0f) * 8.0f - 1.0f);

	/**
	 * @brief Reciprocal of the maximum value used in the dithering process.
	 */
	static constexpr const float MAX_RECIPROCAL = 1.0f / static_cast<float>(MAX);

	/**
	 * @brief 4x4 lookup table for dithering.
	 *
	 * This table contains values scaled by MAX_RECIPROCAL to be used in the dithering algorithm.
	 */
	static constexpr const float LookupTable[4][4] = {
		{0.0f * MAX_RECIPROCAL,  8.0f * MAX_RECIPROCAL,  -2.0f * MAX_RECIPROCAL, 10.0f * MAX_RECIPROCAL},
		{12.0f * MAX_RECIPROCAL,  -4.0f * MAX_RECIPROCAL, 14.0f * MAX_RECIPROCAL,  -6.0f * MAX_RECIPROCAL},
		{-3.0f * MAX_RECIPROCAL, 11.0f * MAX_RECIPROCAL,  -1.0f * MAX_RECIPROCAL,  9.0f * MAX_RECIPROCAL},
		{15.0f * MAX_RECIPROCAL,  -7.0f * MAX_RECIPROCAL, 13.0f * MAX_RECIPROCAL,  -5.0f * MAX_RECIPROCAL}
	};

	/**
	 * @brief Dither a float value upwards.
	 *
	 * This function applies upward dithering to a float value based on the provided coordinates.
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates used to index into the lookup table.
	 * @return The dithered value clamped between 0.0f and 1.0f.
	 */
	static constexpr float ditherUp(float value, const glm::ivec2& coords) {
		return std::clamp(value + LookupTable[coords.y % 4][coords.x % 4], 0.0f, 1.0f);
	}

	/**
	 * @brief Dither a float value downwards.
	 *
	 * This function applies downward dithering to a float value based on the provided coordinates.
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates used to index into the lookup table.
	 * @return The dithered value clamped between 0.0f and 1.0f.
	 */
	static constexpr float ditherDown(float value, const glm::ivec2& coords) {
		return std::clamp(value - LookupTable[coords.y % 4][coords.x % 4], 0.0f, 1.0f);
	}

	/**
	 * @brief Dither a glm::fvec2 value upwards.
	 *
	 * This function applies upward dithering to a glm::fvec2 value based on the provided coordinates.
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates used to index into the lookup table.
	 * @return The dithered value clamped between 0.0f and 1.0f.
	 */
	static constexpr glm::fvec2 ditherUp(const glm::fvec2& value, const glm::ivec2& coords) {
		const auto& lookupVal = LookupTable[coords.y % 4][coords.x % 4];
		return glm::fvec2(
			std::clamp(value.x + lookupVal, 0.0f, 1.0f),
			std::clamp(value.y + lookupVal, 0.0f, 1.0f)
			);
	}

	/**
	 * @brief Dither a glm::fvec2 value downwards.
	 *
	 * This function applies downward dithering to a glm::fvec2 value based on the provided coordinates.
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates used to index into the lookup table.
	 * @return The dithered value clamped between 0.0f and 1.0f.
	 */
	static constexpr glm::fvec2 ditherDown(const glm::fvec2& value, const glm::ivec2& coords) {
		const auto& lookupVal = LookupTable[coords.y % 4][coords.x % 4];
		return glm::fvec2(
			std::clamp(value.x - lookupVal, 0.0f, 1.0f),
			std::clamp(value.y - lookupVal, 0.0f, 1.0f)
			);
	}

	/**
	 * @brief Dither a glm::fvec3 value upwards.
	 *
	 * This function applies upward dithering to a glm::fvec3 value based on the provided coordinates.
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates used to index into the lookup table.
	 * @return The dithered value clamped between 0.0f and 1.0f.
	 */
	static constexpr glm::fvec3 ditherUp(const glm::fvec3& value, const glm::ivec2& coords) {
		const auto& lookupVal = LookupTable[coords.y % 4][coords.x % 4];
		return glm::fvec3(
			std::clamp(value.x + lookupVal, 0.0f, 1.0f),
			std::clamp(value.y + lookupVal, 0.0f, 1.0f),
			std::clamp(value.z + lookupVal, 0.0f, 1.0f)
			);
	}

	/**
	 * @brief Dither a glm::fvec3 value downwards.
	 *
	 * This function applies downward dithering to a glm::fvec3 value based on the provided coordinates.
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates used to index into the lookup table.
	 * @return The dithered value clamped between 0.0f and 1.0f.
	 */
	static constexpr glm::fvec3 ditherDown(const glm::fvec3& value, const glm::ivec2& coords) {
		const auto& lookupVal = LookupTable[coords.y % 4][coords.x % 4];
		return glm::fvec3(
			std::clamp(value.x - lookupVal, 0.0f, 1.0f),
			std::clamp(value.y - lookupVal, 0.0f, 1.0f),
			std::clamp(value.z - lookupVal, 0.0f, 1.0f)
			);
	}

	/**
	 * @brief Dither a glm::fvec4 value upwards.
	 *
	 * This function applies upward dithering to a glm::fvec4 value based on the provided coordinates.
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates used to index into the lookup table.
	 * @return The dithered value clamped between 0.0f and 1.0f.
	 */
	static constexpr glm::fvec4 ditherUp(const glm::fvec4& value, const glm::ivec2& coords) {
		const auto& lookupVal = LookupTable[coords.y % 4][coords.x % 4];
		return glm::fvec4(
			std::clamp(value.x + lookupVal, 0.0f, 1.0f),
			std::clamp(value.y + lookupVal, 0.0f, 1.0f),
			std::clamp(value.z + lookupVal, 0.0f, 1.0f),
			std::clamp(value.w + lookupVal, 0.0f, 1.0f)
			);
	}

	/**
	 * @brief Dither a glm::fvec4 value downwards.
	 *
	 * This function applies downward dithering to a glm::fvec4 value based on the provided coordinates.
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates used to index into the lookup table.
	 * @return The dithered value clamped between 0.0f and 1.0f.
	 */
	static constexpr glm::fvec4 ditherDown(const glm::fvec4& value, const glm::ivec2& coords) {
		const auto& lookupVal = LookupTable[coords.y % 4][coords.x % 4];
		return glm::fvec4(
			std::clamp(value.x - lookupVal, 0.0f, 1.0f),
			std::clamp(value.y - lookupVal, 0.0f, 1.0f),
			std::clamp(value.z - lookupVal, 0.0f, 1.0f),
			std::clamp(value.w - lookupVal, 0.0f, 1.0f)
			);
	}
};

/**
 * @brief Specialization of OrderedDither for float type.
 *
 * This specialization does not apply dithering for float values, as they are already high-precision.
 */
template <>
struct OrderedDither<float> {
	/**
	 * @brief Dither a float value upwards (no-op for float).
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates (unused in this specialization).
	 * @return The original value.
	 */
	static constexpr float ditherUp(float value, const glm::ivec2& coords) {
		(void)coords;
		return value;
	}

	/**
	 * @brief Dither a float value downwards (no-op for float).
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates (unused in this specialization).
	 * @return The original value.
	 */
	static constexpr float ditherDown(float value, const glm::ivec2& coords) {
		(void)coords;
		return value;
	}

	/**
	 * @brief Dither a glm::fvec2 value upwards (no-op for float).
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates (unused in this specialization).
	 * @return The original value.
	 */
	static constexpr glm::fvec2 ditherUp(const glm::fvec2& value, const glm::ivec2& coords) {
		(void)coords;
		return value;
	}

	/**
	 * @brief Dither a glm::fvec2 value downwards (no-op for float).
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates (unused in this specialization).
	 * @return The original value.
	 */
	static constexpr glm::fvec2 ditherDown(const glm::fvec2& value, const glm::ivec2& coords) {
		(void)coords;
		return value;
	}

	/**
	 * @brief Dither a glm::fvec3 value upwards (no-op for float).
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates (unused in this specialization).
	 * @return The original value.
	 */
	static constexpr glm::fvec3 ditherUp(const glm::fvec3& value, const glm::ivec2& coords) {
		(void)coords;
		return value;
	}

	/**
	 * @brief Dither a glm::fvec3 value downwards (no-op for float).
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates (unused in this specialization).
	 * @return The original value.
	 */
	static constexpr glm::fvec3 ditherDown(const glm::fvec3& value, const glm::ivec2& coords) {
		(void)coords;
		return value;
	}

	/**
	 * @brief Dither a glm::fvec4 value upwards (no-op for float).
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates (unused in this specialization).
	 * @return The original value.
	 */
	static constexpr glm::fvec4 ditherUp(const glm::fvec4& value, const glm::ivec2& coords) {
		(void)coords;
		return value;
	}

	/**
	 * @brief Dither a glm::fvec4 value downwards (no-op for float).
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates (unused in this specialization).
	 * @return The original value.
	 */
	static constexpr glm::fvec4 ditherDown(const glm::fvec4& value, const glm::ivec2& coords) {
		(void)coords;
		return value;
	}
};

/**
 * @brief Specialization of OrderedDither for double type.
 *
 * This specialization does not apply dithering for double values, as they are already high-precision.
 */
template <>
struct OrderedDither<double> {
	/**
	 * @brief Dither a float value upwards (no-op for double).
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates (unused in this specialization).
	 * @return The original value.
	 */
	static constexpr float ditherUp(float value, const glm::ivec2& coords) {
		(void)coords;
		return value;
	}

	/**
	 * @brief Dither a float value downwards (no-op for double).
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates (unused in this specialization).
	 * @return The original value.
	 */
	static constexpr float ditherDown(float value, const glm::ivec2& coords) {
		(void)coords;
		return value;
	}

	/**
	 * @brief Dither a glm::fvec2 value upwards (no-op for double).
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates (unused in this specialization).
	 * @return The original value.
	 */
	static constexpr glm::fvec2 ditherUp(const glm::fvec2& value, const glm::ivec2& coords) {
		(void)coords;
		return value;
	}

	/**
	 * @brief Dither a glm::fvec2 value downwards (no-op for double).
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates (unused in this specialization).
	 * @return The original value.
	 */
	static constexpr glm::fvec2 ditherDown(const glm::fvec2& value, const glm::ivec2& coords) {
		(void)coords;
		return value;
	}

	/**
	 * @brief Dither a glm::fvec3 value upwards (no-op for double).
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates (unused in this specialization).
	 * @return The original value.
	 */
	static constexpr glm::fvec3 ditherUp(const glm::fvec3& value, const glm::ivec2& coords) {
		(void)coords;
		return value;
	}

	/**
	 * @brief Dither a glm::fvec3 value downwards (no-op for double).
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates (unused in this specialization).
	 * @return The original value.
	 */
	static constexpr glm::fvec3 ditherDown(const glm::fvec3& value, const glm::ivec2& coords) {
		(void)coords;
		return value;
	}

	/**
	 * @brief Dither a glm::fvec4 value upwards (no-op for double).
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates (unused in this specialization).
	 * @return The original value.
	 */
	static constexpr glm::fvec4 ditherUp(const glm::fvec4& value, const glm::ivec2& coords) {
		(void)coords;
		return value;
	}

	/**
	 * @brief Dither a glm::fvec4 value downwards (no-op for double).
	 *
	 * @param value The value to be dithered.
	 * @param coords The 2D coordinates (unused in this specialization).
	 * @return The original value.
	 */
	static constexpr glm::fvec4 ditherDown(const glm::fvec4& value, const glm::ivec2& coords) {
		(void)coords;
		return value;
	}
};

/**
 * @brief 4x4 threshold matrix for stippling.
 *
 * This matrix is used in the stippling algorithm to determine which pixels to turn on or off based on their alpha value.
 */
constexpr const glm::fvec4 thresholdMatrix[4] = {
	glm::fvec4(1.0 / 17.0,  9.0 / 17.0,  3.0 / 17.0, 11.0 / 17.0),
	glm::fvec4(13.0 / 17.0,  5.0 / 17.0, 15.0 / 17.0,  7.0 / 17.0),
	glm::fvec4(4.0 / 17.0, 12.0 / 17.0,  2.0 / 17.0, 10.0 / 17.0),
	glm::fvec4(16.0 / 17.0,  8.0 / 17.0, 14.0 / 17.0,  6.0 / 17.0)
};

/**
 * @brief Stipple alpha value based on threshold matrix.
 *
 * This function determines whether a pixel should be turned on or off based on its alpha value and the threshold matrix.
 *
 * @param alpha The alpha value to be stippled.
 * @param coords The 2D coordinates used to index into the threshold matrix.
 * @return true if the alpha value is greater than or equal to the threshold, false otherwise.
 */
constexpr bool stippleAlpha(float alpha, const glm::ivec2& coords) {
	return (alpha >= thresholdMatrix[coords.x % 4][coords.y % 4]);
}

/**
 * @brief 2x2 lookup table for some purposes.
 *
 * This table contains values used in various algorithms that require a 2x2 lookup.
 */
constexpr const glm::fvec2 LOOKUP[2][2] = {
	{glm::fvec2(0.25f, 0.00f), glm::fvec2(0.50f, 0.75f)},
	{glm::fvec2(0.75f, 0.50f), glm::fvec2(0.00f, 0.25f)}
};

}
}

#endif // ELVDITHER_HPP
