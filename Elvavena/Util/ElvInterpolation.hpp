#ifndef ELVINTERPOLATION_HPP
#define ELVINTERPOLATION_HPP
#include <Elvavena/Util/ElvFixedPoint.hpp>
#include <concepts>
#include <span>
#include <cassert>
namespace Elv {
namespace Util {

template <typename T> concept InterpolationWeight = std::floating_point<T> || FixedPoint<T>;

template <typename T, typename Scalar> concept Interpolable = std::same_as<T,Scalar> || requires(T a, T b, Scalar s) {
	{ a * s } -> std::same_as<T>;      // Scalar multiplication must return T
	{ s * a } -> std::same_as<T>;      // Scalar multiplication (commutative)
	{ a + b } -> std::same_as<T>;      // Addition must return T
	{ a += b };                        // Compound addition must be valid
};

template <typename TData, typename TWeight>
requires Interpolable<TData, TWeight> && InterpolationWeight<TWeight>
struct Interpolation {
	static constexpr TWeight getOneMinus(const TWeight& minus)
	{
		return static_cast<TWeight>(1) - minus;
	}
	inline static TData lerp(const TData& a, const TData& b, const TWeight& wA, const TWeight& wB) {
		return (a * wA) + (b * wB);
	}
	inline static TData lerp(const TData& a, const TData& b, const TWeight& w) {
		return lerp(a, b, getOneMinus(w), w);
	}
	inline static TData blerp(
			const TData& topLeft, const TData& topRight,
			const TData& bottomLeft, const TData& bottomRight,
			const TWeight& weightTopLeft, const TWeight& weightTopRight,
			const TWeight& weightBottomLeft, const TWeight& weightBottomRight
			) {
		return (topLeft * weightTopLeft) + (topRight * weightTopRight) + (bottomLeft * weightBottomLeft) + (bottomRight * weightBottomRight);
	}
	inline static TData blerp(
			const TData& topLeft, const TData& topRight,
			const TData& bottomLeft, const TData& bottomRight,
			const TWeight& wX, const TWeight& wY
			) {
		const TWeight& oneMinusWX = getOneMinus(wX);
		const TWeight& oneMinusWY = getOneMinus(wY);
		const TWeight& weightTopLeft = oneMinusWX * oneMinusWY;
		const TWeight& weightTopRight = wX * oneMinusWY;
		const TWeight& weightBottomLeft = oneMinusWX * wY;
		const TWeight& weightBottomRight = wX * wY;
		return blerp(topLeft, topRight, bottomLeft, bottomRight, weightTopLeft, weightTopRight, weightBottomLeft, weightBottomRight);
	}
	inline static TData triangular_lerp(const TData& t0, const TData& t1, const TData& t2,
								 const TWeight& w0, const TWeight& w1, const TWeight& w2)
	{
		return (t0 * w0) + (t1 * w1) + (t2 * w2);
	}
	inline static TData array_lerp(const std::span<const TData>& data, const std::span<const TWeight>& weight) {
		assert(data.size() == weight.size() && !data.empty());
		TData toReturn = data[0] * weight[0];
		for(size_t i = 1; i < data.size(); ++i) {
			toReturn += data[i] * weight[i];
		}
		return toReturn;
	}
};
template <typename TData, typename TWeight> requires Interpolable<TData, TWeight> && InterpolationWeight<TWeight>
inline TData lerp(const TData& a, const TData& b, const TWeight& wA, const TWeight& wB) {
	return Interpolation<TData,TWeight>::lerp(a,b,wA,wB);
}
template <typename TData, typename TWeight> requires Interpolable<TData, TWeight> && InterpolationWeight<TWeight>
inline TData lerp(const TData& a, const TData& b, const TWeight& w) {
	return Interpolation<TData,TWeight>::lerp(a,b,w);
}
template <typename TData, typename TWeight> requires Interpolable<TData, TWeight> && InterpolationWeight<TWeight>
inline TData blerp(
		const TData& topLeft, const TData& topRight,
		const TData& bottomLeft, const TData& bottomRight,
		const TWeight& weightTopLeft, const TWeight& weightTopRight,
		const TWeight& weightBottomLeft, const TWeight& weightBottomRight
		) {
	return Interpolation<TData,TWeight>::blerp(topLeft, topRight, bottomLeft, bottomRight, weightTopLeft, weightTopRight, weightBottomLeft, weightBottomRight);
}
template <typename TData, typename TWeight> requires Interpolable<TData, TWeight> && InterpolationWeight<TWeight>
inline TData blerp(
		const TData& topLeft, const TData& topRight,
		const TData& bottomLeft, const TData& bottomRight,
		const TWeight& wX, const TWeight& wY
		) {
	return Interpolation<TData,TWeight>::blerp(topLeft, topRight, bottomLeft, bottomRight, wX, wY);
}
template <typename TData, typename TWeight> requires Interpolable<TData, TWeight> && InterpolationWeight<TWeight>
inline TData triangular_lerp(const TData& t0, const TData& t1, const TData& t2,
						  const TWeight& w0, const TWeight& w1, const TWeight& w2) {
	return Interpolation<TData,TWeight>::triangular_lerp(t0,t1,t2,w0,w1,w2);
}
template <typename TData, typename TWeight> requires Interpolable<TData, TWeight> && InterpolationWeight<TWeight>
inline TData array_lerp(const std::span<const TData>& data, const std::span<const TWeight>& weight) {
	return Interpolation<TData,TWeight>::array_lerp(data,weight);
}

}
}
#endif // ELVINTERPOLATION_HPP
