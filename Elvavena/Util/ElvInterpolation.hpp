#ifndef ELVINTERPOLATION_HPP
#define ELVINTERPOLATION_HPP
#include <Elvavena/Util/ElvFixedPoint.hpp>
#include <concepts>
#include <span>
#include <cassert>
namespace Elv {
namespace Util {

/**
 * @brief Concept defining valid interpolation weights.
 * @tparam T Type to be checked.
 *
 * Interpolation weights must be either floating-point numbers or fixed-point types.
 */
template <typename T> concept InterpolationWeight = std::floating_point<T> || FixedPoint<T>;

/**
 * @brief Concept defining interpolable types.
 * @tparam T The main type.
 * @tparam Scalar The scalar type for interpolation.
 *
 * A type is interpolable if it supports scalar multiplication, addition, and compound addition.
 */
template <typename T, typename Scalar> concept Interpolable = std::same_as<T,Scalar> || requires(T a, T b, Scalar s) {
	{ a * s } -> std::same_as<T>;  ///< Scalar multiplication must return T
	{ s * a } -> std::same_as<T>;  ///< Scalar multiplication (commutative)
	{ a + b } -> std::same_as<T>;  ///< Addition must return T
	{ a += b };                    ///< Compound addition must be valid
};

/**
 * @brief Provides interpolation functions.
 * @tparam TData The data type.
 * @tparam TWeight The weight type.
 *
 * Implements various forms of interpolation, including linear, bilinear, triangular, and array-based interpolation.
 */
template <typename TData, typename TWeight>
	requires Interpolable<TData, TWeight> && InterpolationWeight<TWeight>
struct Interpolation {
	/**
	 * @brief Computes 1 - value for interpolation.
	 * @param minus Value to subtract from 1.
	 * @return The complement of the given weight.
	 */
	static constexpr TWeight getOneMinus(const TWeight& minus) {
		return static_cast<TWeight>(1.0) - minus;
	}

	/**
	 * @brief Performs linear interpolation.
	 * @param a First data point.
	 * @param b Second data point.
	 * @param wA Weight for the first data point.
	 * @param wB Weight for the second data point.
	 * @return Interpolated result.
	 */
	inline static TData lerp(const TData& a, const TData& b, const TWeight& wA, const TWeight& wB) {
		return (a * wA) + (b * wB);
	}

	/**
	 * @brief Performs linear interpolation with a single weight.
	 * @param a First data point.
	 * @param b Second data point.
	 * @param w Weight for the second data point.
	 * @return Interpolated result.
	 */
	inline static TData lerp(const TData& a, const TData& b, const TWeight& w) {
		return lerp(a, b, getOneMinus(w), w);
	}

	/**
	 * @brief Performs bilinear interpolation using explicit weights.
	 * @param topLeft Top-left data point.
	 * @param topRight Top-right data point.
	 * @param bottomLeft Bottom-left data point.
	 * @param bottomRight Bottom-right data point.
	 * @param weightTopLeft Weight for top-left.
	 * @param weightTopRight Weight for top-right.
	 * @param weightBottomLeft Weight for bottom-left.
	 * @param weightBottomRight Weight for bottom-right.
	 * @return Interpolated result.
	 */
	inline static TData blerp(
		const TData& topLeft, const TData& topRight,
		const TData& bottomLeft, const TData& bottomRight,
		const TWeight& weightTopLeft, const TWeight& weightTopRight,
		const TWeight& weightBottomLeft, const TWeight& weightBottomRight
		) {
		return (topLeft * weightTopLeft) + (topRight * weightTopRight) + (bottomLeft * weightBottomLeft) + (bottomRight * weightBottomRight);
	}

	/**
	 * @brief Performs bilinear interpolation with X and Y weights.
	 * @param topLeft Top-left data point.
	 * @param topRight Top-right data point.
	 * @param bottomLeft Bottom-left data point.
	 * @param bottomRight Bottom-right data point.
	 * @param wX Horizontal weight.
	 * @param wY Vertical weight.
	 * @return Interpolated result.
	 */
	inline static TData blerp(
		const TData& topLeft, const TData& topRight,
		const TData& bottomLeft, const TData& bottomRight,
		const TWeight& wX, const TWeight& wY
		) {
		const TWeight oneMinusWX = getOneMinus(wX);
		const TWeight oneMinusWY = getOneMinus(wY);
		const TWeight weightTopLeft = oneMinusWX * oneMinusWY;
		const TWeight weightTopRight = wX * oneMinusWY;
		const TWeight weightBottomLeft = oneMinusWX * wY;
		const TWeight weightBottomRight = wX * wY;
		return blerp(topLeft, topRight, bottomLeft, bottomRight, weightTopLeft, weightTopRight, weightBottomLeft, weightBottomRight);
	}

	/**
	 * @brief Performs triangular interpolation.
	 * @param t0 First data point.
	 * @param t1 Second data point.
	 * @param t2 Third data point.
	 * @param w0 Weight for the first data point.
	 * @param w1 Weight for the second data point.
	 * @param w2 Weight for the third data point.
	 * @return Interpolated result.
	 */
	inline static TData triangular_lerp(const TData& t0, const TData& t1, const TData& t2,
										const TWeight& w0, const TWeight& w1, const TWeight& w2) {
		return (t0 * w0) + (t1 * w1) + (t2 * w2);
	}

	/**
	 * @brief Performs interpolation on an array of values.
	 * @param data Data points.
	 * @param weight Corresponding weights.
	 * @return Interpolated result.
	 */
	inline static TData array_lerp(const std::span<const TData>& data, const std::span<const TWeight>& weight) {
		assert(data.size() == weight.size() && !data.empty());
		TData toReturn = data[0] * weight[0];
		for (size_t i = 1; i < data.size(); ++i) {
			toReturn += data[i] * weight[i];
		}
		return toReturn;
	}
};
/**
 * @brief Linear interpolation between two data points.
 *
 * This function performs linear interpolation between two data points using the specified weights.
 * It is a convenience wrapper around the corresponding method in the Interpolation struct.
 *
 * @tparam TData The type of the data to be interpolated.
 * @tparam TWeight The type of the interpolation weights.
 * @param a The first data point.
 * @param b The second data point.
 * @param wA The weight for the first data point.
 * @param wB The weight for the second data point.
 * @return TData The interpolated result.
 */
template <typename TData, typename TWeight> requires Interpolable<TData, TWeight> && InterpolationWeight<TWeight>
inline TData lerp(const TData& a, const TData& b, const TWeight& wA, const TWeight& wB) {
	return Interpolation<TData,TWeight>::lerp(a,b,wA,wB);
}
/**
 * @brief Linear interpolation between two data points with a single weight.
 *
 * This function performs linear interpolation between two data points using a single weight.
 * It is a convenience wrapper around the corresponding method in the Interpolation struct.
 *
 * @tparam TData The type of the data to be interpolated.
 * @tparam TWeight The type of the interpolation weights.
 * @param a The first data point.
 * @param b The second data point.
 * @param w The weight for the first data point.
 * @return TData The interpolated result.
 */
template <typename TData, typename TWeight> requires Interpolable<TData, TWeight> && InterpolationWeight<TWeight>
inline TData lerp(const TData& a, const TData& b, const TWeight& w) {
	return Interpolation<TData,TWeight>::lerp(a,b,w);
}
/**
 * @brief Bilinear interpolation between four data points with explicit weights.
 *
 * This function performs bilinear interpolation between four data points using the specified weights.
 * It is a convenience wrapper around the corresponding method in the Interpolation struct.
 *
 * @tparam TData The type of the data to be interpolated.
 * @tparam TWeight The type of the interpolation weights.
 * @param topLeft The top-left data point.
 * @param topRight The top-right data point.
 * @param bottomLeft The bottom-left data point.
 * @param bottomRight The bottom-right data point.
 * @param weightTopLeft The weight for the top-left data point.
 * @param weightTopRight The weight for the top-right data point.
 * @param weightBottomLeft The weight for the bottom-left data point.
 * @param weightBottomRight The weight for the bottom-right data point.
 * @return TData The interpolated result.
 */
template <typename TData, typename TWeight> requires Interpolable<TData, TWeight> && InterpolationWeight<TWeight>
inline TData blerp(
		const TData& topLeft, const TData& topRight,
		const TData& bottomLeft, const TData& bottomRight,
		const TWeight& weightTopLeft, const TWeight& weightTopRight,
		const TWeight& weightBottomLeft, const TWeight& weightBottomRight
		) {
	return Interpolation<TData,TWeight>::blerp(topLeft, topRight, bottomLeft, bottomRight, weightTopLeft, weightTopRight, weightBottomLeft, weightBottomRight);
}
/**
 * @brief Bilinear interpolation between four data points with X and Y weights.
 *
 * This function performs bilinear interpolation between four data points using X and Y weights.
 * It is a convenience wrapper around the corresponding method in the Interpolation struct.
 *
 * @tparam TData The type of the data to be interpolated.
 * @tparam TWeight The type of the interpolation weights.
 * @param topLeft The top-left data point.
 * @param topRight The top-right data point.
 * @param bottomLeft The bottom-left data point.
 * @param bottomRight The bottom-right data point.
 * @param wX The X weight.
 * @param wY The Y weight.
 * @return TData The interpolated result.
 */
template <typename TData, typename TWeight> requires Interpolable<TData, TWeight> && InterpolationWeight<TWeight>
inline TData blerp(
		const TData& topLeft, const TData& topRight,
		const TData& bottomLeft, const TData& bottomRight,
		const TWeight& wX, const TWeight& wY
		) {
	return Interpolation<TData,TWeight>::blerp(topLeft, topRight, bottomLeft, bottomRight, wX, wY);
}
/**
 * @brief Triangular interpolation between three data points.
 *
 * This function performs triangular interpolation between three data points using the specified weights.
 * It is a convenience wrapper around the corresponding method in the Interpolation struct.
 *
 * @tparam TData The type of the data to be interpolated.
 * @tparam TWeight The type of the interpolation weights.
 * @param t0 The first data point.
 * @param t1 The second data point.
 * @param t2 The third data point.
 * @param w0 The weight for the first data point.
 * @param w1 The weight for the second data point.
 * @param w2 The weight for the third data point.
 * @return TData The interpolated result.
 */
template <typename TData, typename TWeight> requires Interpolable<TData, TWeight> && InterpolationWeight<TWeight>
inline TData triangular_lerp(const TData& t0, const TData& t1, const TData& t2,
						  const TWeight& w0, const TWeight& w1, const TWeight& w2) {
	return Interpolation<TData,TWeight>::triangular_lerp(t0,t1,t2,w0,w1,w2);
}
/**
 * @brief Interpolation from an array of data points with corresponding weights.
 *
 * This function performs interpolation from an array of data points using the specified weights.
 * It is a convenience wrapper around the corresponding method in the Interpolation struct.
 * The number of data points and weights must be the same and non-empty.
 *
 * @tparam TData The type of the data to be interpolated.
 * @tparam TWeight The type of the interpolation weights.
 * @param data The array of data points.
 * @param weight The array of weights.
 * @return TData The interpolated result.
 */
template <typename TData, typename TWeight> requires Interpolable<TData, TWeight> && InterpolationWeight<TWeight>
inline TData array_lerp(const std::span<const TData>& data, const std::span<const TWeight>& weight) {
	return Interpolation<TData,TWeight>::array_lerp(data,weight);
}

}
}
#endif // ELVINTERPOLATION_HPP
