#ifndef ELVEDGEFUNCTION_HPP
#define ELVEDGEFUNCTION_HPP
#include <tuple>
#include <concepts>
namespace Elv {
namespace Util {

/*template <typename Vec, typename T>
concept GlmVectorLike = requires(Vec vec, T t) {
	{ vec.x } -> std::convertible_to<T>;
	{ vec.y } -> std::convertible_to<T>;
};

template <typename T, typename Vec1, typename Vec2, typename Vec3>
requires GlmVectorLike<Vec1,T> && GlmVectorLike<Vec2,T> && GlmVectorLike<Vec3,T>
T edgeFunction(const Vec1& a, const Vec2& b, const Vec3& c) {
	return ((c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x));
}*/

template <typename Vec, typename T>
concept Vector2Like = requires(Vec vec) {
	{ vec.x } -> std::convertible_to<T>;
	{ vec.y } -> std::convertible_to<T>;
};

template <typename T, typename... Vecs>
requires (Vector2Like<Vecs, T> && ...)
T edgeFunction(const Vecs&... vecs) {
	const auto& [a, b, c] = std::tie(vecs...);
	return ((c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x));
}


}
}

#endif // ELVEDGEFUNCTION_HPP
