#ifndef ELVNORMDENORM_HPP
#define ELVNORMDENORM_HPP
/**
 * @file ElvNormDenorm.hpp
 * @brief Declares the ElvNormDenorm API in the Elvavena/Util module.
 *
 * This header is part of the public declaration surface for Elvavena/Util.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <limits>
#include <cmath>
#include <type_traits>
#include <Elvavena/Util/ElvSpanHelpers.hpp>
#include <Elvavena/Util/half.hpp>
namespace Elv {
namespace Util {

/**
 * @defgroup Normalization Utilities for normalizing and denormalizing values between different data types.
 * @{
 */

/**
 * @brief Template struct for float normalization and denormalization.
 *
 * Provides methods to normalize a value of type T to a float in the range [-1.0f, 1.0f] and denormalize a float back to type T.
 *
 * @tparam T Type of the value to be normalized/denormalized.
 */
template <typename T> struct _fnorm_fdenorm {
	/**
	 * @brief Minimum float value for normalization (derived from T's min limit).
	 */
	constexpr static const float minF = -1.0f * float(std::numeric_limits<T>::min());

	/**
	 * @brief Maximum float value for normalization (derived from T's max limit).
	 */
	constexpr static const float maxF = float(std::numeric_limits<T>::max());

	/**
	 * @brief Reciprocal of minF for normalization calculations.
	 */
	constexpr static const float recMinF = ((std::numeric_limits<T>::min()))? 1.0f / minF : 0.0f;

	/**
	 * @brief Reciprocal of maxF for normalization calculations.
	 */
	constexpr static const float recMaxF = 1.0f / maxF;

	/**
	 * @brief Normalize a value of type T to a float in the range [-1.0f, 1.0f].
	 *
	 * @param val Value to be normalized.
	 * @return float Normalized value.
	 */
	inline static constexpr float normalize(const T& val) {
		return (val < 0)? (float(val) * recMinF) : (float(val) * recMaxF);
	}

	/**
	 * @brief Denormalize a float value back to type T.
	 *
	 * @param val Float value to be denormalized.
	 * @return T Denormalized value.
	 */
	inline static T denormalize(const float& val) {
		return (std::signbit(val))? T(val*minF) : T(val*maxF);
	}
};

/**
 * @brief Specialization for half_float::half type.
 */
template <> struct _fnorm_fdenorm<half_float::half> {
	/**
	 * @brief Normalize half_float::half to float (direct conversion).
	 *
	 * @param val half_float::half value.
	 * @return float Converted value.
	 */
	inline static float normalize(const half_float::half& val) {
		return float(val);
	}

	/**
	 * @brief Denormalize float back to half_float::half (direct conversion).
	 *
	 * @param val Float value.
	 * @return half_float::half Converted value.
	 */
	inline static half_float::half denormalize(const float& val) {
		return half_float::half(val);
	}
};

/**
 * @brief Specialization for float type (no-op).
 */
template <> struct _fnorm_fdenorm<float> {
	/**
	 * @brief Normalize float (identity function).
	 *
	 * @param val Float value.
	 * @return float Same value.
	 */
	inline constexpr static float normalize(const float& val) {
		return val;
	}

	/**
	 * @brief Denormalize float (identity function).
	 *
	 * @param val Float value.
	 * @return float Same value.
	 */
	inline constexpr static float denormalize(const float& val) {
		return val;
	}
};

/**
 * @brief Specialization for double type.
 */
template <> struct _fnorm_fdenorm<double> {
	/**
	 * @brief Normalize double to float.
	 *
	 * @param val Double value.
	 * @return float Normalized value.
	 */
	inline constexpr static float normalize(const double& val) {
		return float(val);
	}

	/**
	 * @brief Denormalize float back to double.
	 *
	 * @param val Float value.
	 * @return double Denormalized value.
	 */
	inline constexpr static double denormalize(const float& val) {
		return double(val);
	}
};

/**
 * @brief Convenience function for normalizing a value of type T to a float.
 *
 * @tparam T Type of the value to normalize.
 * @param val Value to normalize.
 * @return float Normalized value.
 */
template <typename T> inline float fnormalize(const T& val) {
	return _fnorm_fdenorm<T>::normalize(val);
}

/**
 * @brief Convenience function for denormalizing a float value to type T.
 *
 * @tparam T Target type of the denormalized value.
 * @param val Float value to denormalize.
 * @return T Denormalized value.
 */
template <typename T> inline T fdenormalize(const float& val) {
	return _fnorm_fdenorm<T>::denormalize(val);
}

/**
 * @brief Normalize an array of T values to an array of floats.
 *
 * @tparam T Type of values to normalize.
 * @param input Array of T values.
 * @param output Array to store float normalized values.
 */
template <typename T> inline void fnormalize(const std::span<const T>& input, const std::span<float>& output) {
	transform(input, output, [](const T& val) { return _fnorm_fdenorm<T>::normalize(val); });
}

/**
 * @brief Denormalize an array of floats to an array of T values.
 *
 * @tparam T Target type of the denormalized values.
 * @param input Array of float values.
 * @param output Array to store T denormalized values.
 */
template <typename T> inline void fdenormalize(const std::span<const float>& input, const std::span<T>& output) {
	transform(input, output, [](const float& val) { return _fnorm_fdenorm<T>::denormalize(val); });
}

/**
 * @brief Template struct for double normalization and denormalization.
 *
 * Similar to _fnorm_fdenorm but targets double instead of float.
 *
 * @tparam T Type of the value to be normalized/denormalized.
 */
template <typename T> struct _dnorm_ddenorm {
	/**
	 * @brief Minimum double value for normalization (derived from T's min limit).
	 */
	constexpr static const double minF = -1.0 * double(std::numeric_limits<T>::min());

	/**
	 * @brief Maximum double value for normalization (derived from T's max limit).
	 */
	constexpr static const double maxF = double(std::numeric_limits<T>::max());

	/**
	 * @brief Reciprocal of minF for normalization calculations.
	 */
	constexpr static const double recMinF = ((std::numeric_limits<T>::min()))? 1.0 / minF : 0.0;

	/**
	 * @brief Reciprocal of maxF for normalization calculations.
	 */
	constexpr static const double recMaxF = 1.0 / maxF;

	/**
	 * @brief Normalize a value of type T to a double in the range [-1.0, 1.0].
	 *
	 * @param val Value to be normalized.
	 * @return double Normalized value.
	 */
	inline static constexpr double normalize(const T& val) {
		return (val < 0)? (double(val) * recMinF) : (double(val) * recMaxF);
	}

	/**
	 * @brief Denormalize a double value back to type T.
	 *
	 * @param val Double value to be denormalized.
	 * @return T Denormalized value.
	 */
	inline static T denormalize(const double& val) {
		return (std::signbit(val))? T(val*minF) : T(val*maxF);
	}
};

/**
 * @brief Specialization for half_float::half type (double normalization).
 */
template <> struct _dnorm_ddenorm<half_float::half> {
	/**
	 * @brief Normalize half_float::half to double (direct conversion).
	 *
	 * @param val half_float::half value.
	 * @return double Converted value.
	 */
	inline static double normalize(const half_float::half& val) {
		return double(val);
	}

	/**
	 * @brief Denormalize double back to half_float::half (conversion via float).
	 *
	 * @param val Double value.
	 * @return half_float::half Converted value.
	 */
	inline static half_float::half denormalize(const double& val) {
		return half_float::half(float(val));
	}
};

/**
 * @brief Specialization for float type (double normalization).
 */
template <> struct _dnorm_ddenorm<float> {
	/**
	 * @brief Normalize float to double (direct conversion).
	 *
	 * @param val Float value.
	 * @return double Converted value.
	 */
	inline constexpr static double normalize(const float& val) {
		return double(val);
	}

	/**
	 * @brief Denormalize double back to float (direct conversion).
	 *
	 * @param val Double value.
	 * @return float Converted value.
	 */
	inline constexpr static float denormalize(const double& val) {
		return float(val);
	}
};

/**
 * @brief Specialization for double type (no-op for normalization).
 */
template <> struct _dnorm_ddenorm<double> {
	/**
	 * @brief Normalize double (identity function).
	 *
	 * @param val Double value.
	 * @return double Same value.
	 */
	inline constexpr static double normalize(const double& val) {
		return val;
	}

	/**
	 * @brief Denormalize double (identity function).
	 *
	 * @param val Double value.
	 * @return double Same value.
	 */
	inline constexpr static double denormalize(const double& val) {
		return val;
	}
};
/**
 * @brief Convenience function for normalizing a value of type T to a double.
 *
 * @tparam T Type of the value to normalize.
 * @param val Value to normalize.
 * @return double Normalized value.
 */
template <typename T> inline double dnormalize(const T& val) {
	return _dnorm_ddenorm<T>::normalize(val);
}

/**
 * @brief Convenience function for denormalizing a double value to type T.
 *
 * @tparam T Target type of the denormalized value.
 * @param val Double value to denormalize.
 * @return T Denormalized value.
 */
template <typename T> inline T ddenormalize(const double& val) {
	return _dnorm_ddenorm<T>::denormalize(val);
}

/**
 * @brief Normalize an array of T values to an array of doubles.
 *
 * @tparam T Type of values to normalize.
 * @param input Array of T values.
 * @param output Array to store double normalized values.
 */
template <typename T> inline void dnormalize(const std::span<const T>& input, const std::span<double>& output) {
	transform(input, output, [](const T& val) { return _dnorm_ddenorm<T>::normalize(val); });
}

/**
 * @brief Denormalize an array of doubles to an array of T values.
 *
 * @tparam T Target type of the denormalized values.
 * @param input Array of double values.
 * @param output Array to store T denormalized values.
 */
template <typename T> inline void ddenormalize(const std::span<const double>& input, const std::span<T>& output) {
	transform(input, output, [](const double& val) { return _dnorm_ddenorm<T>::denormalize(val); });
}

/**
 * @brief Perform a normalizing cast from one type to another.
 *
 * If types are the same, performs a direct copy. Otherwise, normalizes the source value to a float and then denormalizes it to the target type.
 *
 * @tparam T1 Source type.
 * @tparam T2 Target type.
 * @param src Source value.
 * @param dst Target value.
 */
template <typename T1, typename T2> void normalizing_cast(const T1& src, T2& dst) {
	if constexpr(std::is_same<T1,T2>())
		dst = src;
	else
		dst = fdenormalize<T2>(fnormalize(src));
}

/**
 * @brief Perform a normalizing cast for arrays.
 *
 * Similar to the single-value version but for arrays.
 *
 * @tparam T1 Source type.
 * @tparam T2 Target type.
 * @param src Source array.
 * @param dst Target array.
 */
template <typename T1, typename T2> void normalizing_cast(const std::span<const T1>& src, const std::span<T2>& dst) {
	if constexpr(std::is_same<T1,T2>())
		transform(src, dst, [](const T1& val) { return val; });
	else
		transform(src, dst, [](const T1& val) { return fdenormalize<T2>(fnormalize(val)); });
}

/** @} */ // End of Normalization group

}
}
#endif // ELVNORMDENORM_HPP
