#ifndef ELVMATHUTIL_HPP
#define ELVMATHUTIL_HPP
#include <type_traits>
namespace Elv {
namespace Util {

template<typename T> typename std::enable_if<std::is_integral<T>::value, T>::type div_ceil(T a, T b) {
	return (a % b == 0) ? (a / b) : (a / b) + 1;
}

}
}
#endif // ELVMATHUTIL_HPP
