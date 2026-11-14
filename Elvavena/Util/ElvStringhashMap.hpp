#ifndef ELVSTRINGHASHMAP_HPP
#define ELVSTRINGHASHMAP_HPP
/**
 * @file ElvStringhashMap.hpp
 * @brief Declares the ElvStringhashMap API in the Elvavena/Util module.
 *
 * This header is part of the public declaration surface for Elvavena/Util.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */

#include <map>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Elv {
namespace Util {

/**
 * @struct StringHash
 * @brief Transparent string hash functor for heterogeneous lookup.
 *
 * Supports hashing `const char*`, `std::string_view`, and `std::string`.
 * This enables lookups in hash maps without constructing temporary `std::string` objects.
 */
struct StringHash {
	/** @brief Marker type enabling transparent lookup in associative containers. */
	using is_transparent = void;

	/**
	 * @brief Hashes a null-terminated C string.
	 * @param txt Input C string.
	 * @return Hash value.
	 */
	[[nodiscard]] size_t operator()(const char* txt) const {
		return std::hash<std::string_view>{}(txt);
	}

	/**
	 * @brief Hashes a string view.
	 * @param txt Input string view.
	 * @return Hash value.
	 */
	[[nodiscard]] size_t operator()(std::string_view txt) const {
		return std::hash<std::string_view>{}(txt);
	}

	/**
	 * @brief Hashes a string object.
	 * @param txt Input string.
	 * @return Hash value.
	 */
	[[nodiscard]] size_t operator()(const std::string& txt) const {
		return std::hash<std::string>{}(txt);
	}
};

/**
 * @brief Alias for an unordered map keyed by `std::string` with transparent lookup support.
 * @tparam T Mapped value type.
 */
template <typename T>
using UnorderedStrMap = std::unordered_map<std::string, T, StringHash, std::equal_to<>>;

/**
 * @brief Alias for an ordered map keyed by `std::string` with transparent lookup support.
 * @tparam T Mapped value type.
 */
template <typename T>
using StrMap = std::map<std::string, T, std::less<>>;

} // namespace Util
} // namespace Elv

#endif // ELVSTRINGHASHMAP_HPP
