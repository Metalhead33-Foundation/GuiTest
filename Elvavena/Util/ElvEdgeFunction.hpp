#ifndef ELVEDGEFUNCTION_HPP
#define ELVEDGEFUNCTION_HPP
/**
 * @file ElvEdgeFunction.hpp
 * @brief Declares the ElvEdgeFunction API in the Elvavena/Util module.
 *
 * This header is part of the public declaration surface for Elvavena/Util.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <tuple>
#include <concepts>
namespace Elv {
namespace Util {

/**
 * @brief Concept to check if a type behaves like a 2D vector with x and y members.
 *
 * This concept requires that the type `Vec` has `x` and `y` members that are convertible to type `T`.
 *
 * @tparam Vec The type to check.
 * @tparam T The type that `x` and `y` members should be convertible to.
 */
template <typename Vec, typename T>
concept Vector2Like = requires(Vec vec) {
	{ vec.x } -> std::convertible_to<T>;  /**< The `x` member of `Vec` should be convertible to `T`. */
	{ vec.y } -> std::convertible_to<T>;  /**< The `y` member of `Vec` should be convertible to `T`. */
};

/**
 * @brief Computes the edge function for a set of 2D vectors.
 *
 * The edge function is used in barycentric coordinate calculations and computes the determinant of a 2x2 matrix formed by the vectors.
 * This function requires at least three vectors that satisfy the `Vector2Like` concept.
 *
 * @tparam T The type of the vector components.
 * @tparam Vecs The types of the vectors, each of which must satisfy the `Vector2Like` concept.
 * @param vecs The vectors for which to compute the edge function. At least three vectors are required.
 * @return The result of the edge function, which is a scalar value of type `T`.
 */
template <typename T, typename... Vecs>
	requires (Vector2Like<Vecs, T> && ...)
T edgeFunction(const Vecs&... vecs) {
	const auto& [a, b, c] = std::tie(vecs...);  /**< Unpack the input vectors into `a`, `b`, and `c`. */
	return ((c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x));  /**< Compute and return the edge function value. */
}


}
}

#endif // ELVEDGEFUNCTION_HPP
