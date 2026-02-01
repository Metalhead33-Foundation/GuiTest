#ifndef EUPHCONFIGURATIONELEMENT_H
#define EUPHCONFIGURATIONELEMENT_H
#include <Euphemy/Config/EuphLib.hpp>
#include <cstdint>
#include <string>
#include <array>
#include <sstream>
/**
 * @file
 * @brief Configuration data types and utilities for the Euph namespace
 */

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <iostream>

namespace Euph {
namespace Conf {

/**
 * @def LARGE_CONF_DATA
 * @brief Macro to enable large configuration data types
 *
 * When defined, uses 64-bit types for ConfFloat, ConfInt, and ConfUint.
 * Otherwise, uses 32-bit types.
 */

#ifdef LARGE_CONF_DATA
typedef double ConfFloat;      ///< Floating-point type for configuration (64-bit when LARGE_CONF_DATA is defined)
typedef int64_t ConfInt;       ///< Signed integer type for configuration (64-bit when LARGE_CONF_DATA is defined)
typedef uint64_t ConfUint;     ///< Unsigned integer type for configuration (64-bit when LARGE_CONF_DATA is defined)
#else
typedef float ConfFloat;       ///< Floating-point type for configuration (32-bit by default)
typedef int32_t ConfInt;       ///< Signed integer type for configuration (32-bit by default)
typedef uint32_t ConfUint;     ///< Unsigned integer type for configuration (32-bit by default)
#endif

/**
 * @class SizedString
 * @brief Compact string storage using a shared string pool
 *
 * Stores strings in a shared pool and uses bit-packing to store index and length.
 * The string data itself is stored in a static array (string_pool).
 */
class MH_EUPH_API SizedString {
public:
	static constexpr const ConfUint MAX_STRING_POOL_SIZE = 4 * 1024 * 1024; ///< Maximum size of the string pool (4MB)

#ifdef LARGE_CONF_DATA
	static constexpr const ConfUint LENGTH_BITS = 32; ///< Number of bits used for string length (32 in large config mode)
#else
	static constexpr const ConfUint LENGTH_BITS = 12; ///< Number of bits used for string length (12 by default)
#endif

	static_assert (LENGTH_BITS <= sizeof(ConfUint)*4,"LENGTH_BITS must be half or less the size of ConfUint!");

	static constexpr const ConfUint INDEX_BITS = static_cast<ConfUint>(sizeof(ConfUint)*8) - LENGTH_BITS; ///< Number of bits used for string index
	static constexpr const ConfUint LENGTH_MASK = (static_cast<ConfUint>(1) << LENGTH_BITS) - 1; ///< Bitmask for length portion
	static constexpr const ConfUint INDEX_MASK = ~LENGTH_MASK; ///< Bitmask for index portion
	static constexpr const ConfUint STRING_POOL_SIZE = std::min((static_cast<ConfUint>(1) << INDEX_BITS), MAX_STRING_POOL_SIZE); ///< Actual size of string pool

	typedef std::array<char,STRING_POOL_SIZE> StringPool; ///< Type for the string pool storage

	ConfUint container; ///< Bit-packed container storing index and length

	static StringPool string_pool; ///< Shared storage for all strings

	/**
	 * @brief Unpack the container into index and length
	 * @param[out] startIndex The starting index in the string pool
	 * @param[out] length The length of the string
	 */
	void unpack(ConfUint& startIndex, ConfUint& length) const;

	/**
	 * @brief Pack index and length into the container
	 * @param startIndex The starting index in the string pool
	 * @param length The length of the string
	 */
	void pack(ConfUint startIndex, ConfUint length);

	/**
	 * @brief Get the string as a string_view
	 * @return std::string_view view of the stored string
	 */
	std::string_view asStringView() const;

	/**
	 * @brief Copy the string into a new std::string
	 * @return std::string copy of the stored string
	 */
	std::string copyIntoString() const;

	/**
	 * @brief Get the string as a C-style string
	 * @return const char* pointer to the string data
	 */
	const char* c_str() const;

	/**
	 * @brief Store a string from a string_view
	 * @param strView The string to store
	 */
	void fromStringView(const std::string_view& strView);
};

/**
 * @enum ConfigurationElementType
 * @brief Types of configuration elements
 */
enum class ConfigurationElementType : uint8_t {
	CONF_FLOAT,  ///< Floating-point value
	CONF_INT,    ///< Signed integer value
	CONF_UINT,   ///< Unsigned integer value
	CONF_BOOL,   ///< Boolean value
	CONF_STRING  ///< String value
};

/**
 * @union ConfigurationElementUnion
 * @brief Union storing the actual configuration value
 */
union ConfigurationElementUnion {
	ConfFloat f;      ///< Floating-point value
	ConfInt i;        ///< Signed integer value
	ConfUint u;       ///< Unsigned integer value
	bool b;           ///< Boolean value
	SizedString s;    ///< String value
};

/**
 * @class ConfigurationElement
 * @brief A type-safe container for configuration values
 */
class MH_EUPH_API ConfigurationElement {
public:
	ConfigurationElementType type; ///< Type of the stored value
	ConfigurationElementUnion storage; ///< Storage for the actual value

	/**
	 * @brief Copy constructor
	 * @param cpy Element to copy
	 */
	ConfigurationElement(const ConfigurationElement& cpy);

	/**
	 * @brief Copy assignment operator
	 * @param cpy Element to copy
	 * @return Reference to this object
	 */
	ConfigurationElement& operator=(const ConfigurationElement& cpy);

	/**
	 * @brief Construct from type and string view
	 * @param type Type of the element
	 * @param str String value to initialize with
	 */
	ConfigurationElement(ConfigurationElementType type, const std::string_view& str);

	/**
	 * @brief Construct from floating-point value
	 * @param flt Floating-point value
	 */
	explicit ConfigurationElement(ConfFloat flt);

	/**
	 * @brief Construct from signed integer value
	 * @param nnt Signed integer value
	 */
	explicit ConfigurationElement(ConfInt nnt);

	/**
	 * @brief Construct from unsigned integer value
	 * @param uint Unsigned integer value
	 */
	explicit ConfigurationElement(ConfUint uint);

	/**
	 * @brief Construct from boolean value
	 * @param boolean Boolean value
	 */
	explicit ConfigurationElement(bool boolean);

	/**
	 * @brief Construct from string view
	 * @param strview String value
	 */
	explicit ConfigurationElement(const std::string_view& strview);
};

} // namespace Conf
} // namespace Euph

/**
 * @brief Stream output operator for ConfigurationElement
 * @tparam CharT Character type
 * @tparam Traits Character traits
 * @param left Output stream
 * @param right Configuration element to output
 * @return Reference to the output stream
 */
template<class CharT, class Traits = std::char_traits<CharT>>
std::basic_ostream<CharT,Traits>& operator<<(std::basic_ostream<CharT,Traits>& left, Euph::Conf::ConfigurationElement& right) {
	switch (right.type) {
		case Euph::Conf::ConfigurationElementType::CONF_FLOAT:
			left << right.storage.f;
			break;
		case Euph::Conf::ConfigurationElementType::CONF_INT:
			left << right.storage.i;
			break;
		case Euph::Conf::ConfigurationElementType::CONF_UINT:
			left << right.storage.u;
			break;
		case Euph::Conf::ConfigurationElementType::CONF_BOOL:
			left << right.storage.b;
			break;
		case Euph::Conf::ConfigurationElementType::CONF_STRING:
			left << right.storage.s.asStringView();
			break;
	}
	return left;
}

#endif // EUPHCONFIGURATIONELEMENT_H
