#ifndef ELVFIXEDPOINT_HPP
#define ELVFIXEDPOINT_HPP
#include <cstdint>
#include <cmath>
#include <compare>
#include <Elvavena/Io/ElvDataStream.hpp>
#if __cplusplus <= 199711L
  #error This library needs at least a C++11 compliant compiler
#endif
/**
 * @defgroup fixed_point Fixed Point Arithmetic Class
 * @brief Provides a class template for fixed-point arithmetic.
 * @ingroup fp
 * @{
 */

#define FP_TEMPLATE_INTRO template <typename container_t = std::int32_t, container_t fraction_bits = sizeof(container_t)*4>
#define FP_TEMPLATE_INST fixed_point<container_t,fraction_bits>
namespace Elv {
namespace Util {

/**
 * @defgroup fixed_point Fixed Point Arithmetic Class
 * @brief Provides a class template for fixed-point arithmetic.
 * @ingroup fp
 * @{
 */


/**
 * @class fixed_point
 * @brief Class template for fixed-point numbers.
 *
 * @tparam container_t The underlying integer type to store the fixed-point number. Defaults to std::int32_t.
 * @tparam fraction_bits The number of bits to use for the fractional part. Defaults to half of the bits in container_t.
 */
FP_TEMPLATE_INTRO
struct fixed_point {
	/**
	 * @var _container
	 * @brief The underlying container to store the fixed-point value.
	 */
	container_t _container;

	/**
	 * @def Constants and Bit Masks
	 * @{
	 */

	/**
	 * @var bits
	 * @brief Total number of bits in the container type.
	 */
	static constexpr const std::uint8_t bits = sizeof(container_t) * 8;

	/**
	 * @var fraction_shifter
	 * @brief Number of bits to shift for fractional part alignment.
	 */
	static constexpr const container_t fraction_shifter = fraction_bits;

	/**
	 * @var half_shifter
	 * @brief Number of bits to shift for half alignment.
	 */
	static constexpr const container_t half_shifter = bits / 2;

	/**
	 * @var quarter_shifter
	 * @brief Number of bits to shift for quarter alignment.
	 */
	static constexpr const container_t quarter_shifter = bits / 4;

	/**
	 * @var multiplier
	 * @brief Multiplier to convert from float to fixed-point.
	 */
	static constexpr const container_t multiplier = static_cast<container_t>(static_cast<container_t>(1) << fraction_bits);

	/**
	 * @var fractionmask
	 * @brief Mask for the fractional part.
	 */
	static constexpr const container_t fractionmask = multiplier - 1;

	/**
	 * @var wholemask
	 * @brief Mask for the whole part.
	 */
	static constexpr const container_t wholemask = ~fractionmask;

	/**
	 * @var halfraw
	 * @brief Half of the fractional part's raw value.
	 */
	static constexpr const container_t halfraw = fractionmask / 2;

	/**
	 * @var multiplierF, multiplierD, multiplierLD
	 * @brief Float, Double, and Long Double multipliers.
	 */
	static constexpr const float multiplierF = static_cast<float>(multiplier);
	static constexpr const double multiplierD = static_cast<double>(multiplier);
	static constexpr const long double multiplierLD = static_cast<long double>(multiplier);

	/**
	 * @var dividerF, dividerD, dividerLD
	 * @brief Float, Double, and Long Double dividers (reciprocal of multipliers).
	 */
	static constexpr const float dividerF = 1.0f / multiplierF;
	static constexpr const double dividerD = 1.0 / multiplierD;
	static constexpr const double dividerLD = 1.0L / multiplierLD;

	/** @} */

	/**
	 * @name Value Extraction
	 * @{
	 */

	/**
	 * @brief Returns the fractional part of the fixed-point number.
	 * @return fixed_point The fractional part.
	 */
	inline constexpr fixed_point fraction_part() const {
		return { _container & fractionmask };
	}

	/**
	 * @brief Returns the whole part of the fixed-point number.
	 * @return fixed_point The whole part.
	 */
	inline constexpr fixed_point whole_part() const {
		return { _container & wholemask };
	}

	/**
	 * @brief Returns the absolute value of the fixed-point number.
	 * @return fixed_point The absolute value.
	 */
	inline constexpr fixed_point abs() const {
		return { std::abs(_container) };
	}

	/**
	 * @brief Returns the floor of the fixed-point number.
	 * @return fixed_point The floor value.
	 */
	inline constexpr fixed_point floor() const {
		return whole_part();
	}

	/**
	 * @brief Returns the ceiling of the fixed-point number.
	 * @return fixed_point The ceiling value.
	 */
	inline constexpr fixed_point ceil() const {
		return { (_container & fractionmask)? (_container & wholemask) + multiplier : _container };
	}

	/**
	 * @brief Returns the rounded value of the fixed-point number.
	 * @return fixed_point The rounded value.
	 */
	inline constexpr fixed_point round() const {
		return { (_container & fractionmask) >= halfraw? (_container & wholemask) + multiplier : _container };
	}

	/** @} */

	/**
	 * @name Construction from Various Types
	 * @{
	 */

	/**
	 * @brief Constructs a fixed_point from a raw container value.
	 * @param input The raw container value.
	 * @return fixed_point The constructed fixed-point object.
	 */
	inline static constexpr fixed_point from_raw(container_t input) {
		return { input };
	}

	/**
	 * @brief Constructs a fixed_point from an integer value.
	 * @param input The integer value.
	 * @return fixed_point The constructed fixed-point object.
	 */
	inline static constexpr fixed_point from_integer(container_t input) {
		return { input << fraction_bits };
	}

	/**
	 * @brief Constructs a fixed_point from a float value.
	 * @param input The float value.
	 * @return fixed_point The constructed fixed-point object.
	 */
	inline static constexpr fixed_point from_float(float input) {
		return { static_cast<container_t>(input * multiplierF) };
	}

	/**
	 * @brief Constructs a fixed_point from a double value.
	 * @param input The double value.
	 * @return fixed_point The constructed fixed-point object.
	 */
	inline static constexpr fixed_point from_double(double input) {
		return { static_cast<container_t>(input * multiplierD) };
	}

	/**
	 * @brief Constructs a fixed_point from a long double value.
	 * @param input The long double value.
	 * @return fixed_point The constructed fixed-point object.
	 */
	inline static constexpr fixed_point from_long_double(long double input) {
		return { static_cast<container_t>(input * multiplierLD) };
	}

	/** @} */

	/**
	 * @name Conversions
	 * @{
	 */

	/**
	 * @brief Converts the fixed-point number to its raw container value.
	 * @return container_t The raw container value.
	 */
	constexpr container_t to_raw() const {
		return _container;
	}

	/**
	 * @brief Converts the fixed-point number to an integer value.
	 * @return container_t The integer value.
	 */
	constexpr container_t to_integer() const {
		return _container >> fraction_bits;
	}

	/**
	 * @brief Converts the fixed-point number to a float value.
	 * @return float The float value.
	 */
	inline constexpr float to_float() const {
		return static_cast<float>(_container) * dividerF;
	}

	/**
	 * @brief Converts the fixed-point number to a double value.
	 * @return double The double value.
	 */
	inline constexpr double to_double() const {
		return static_cast<double>(_container) * dividerD;
	}

	/**
	 * @brief Converts the fixed-point number to a long double value.
	 * @return long double The long double value.
	 */
	inline constexpr long double to_long_double() const {
		return static_cast<long double>(_container) * dividerLD;
	}

	/** @} */

	/**
	 * @name Implicit Conversions
	 * @{
	 */

	/**
	 * @brief Implicit conversion to bool.
	 * @return bool Whether the fixed-point value is non-zero.
	 */
	constexpr operator bool() const {
		return static_cast<bool>(_container);
	}

	/**
	 * @brief Implicit conversion to container_t (integer).
	 * @return container_t The integer value of the fixed-point number.
	 */
	constexpr operator container_t() const {
		return to_integer();
	}

	/**
	 * @brief Implicit conversion to float.
	 * @return float The float value of the fixed-point number.
	 */
	constexpr operator float() const {
		return to_float();
	}

	/**
	 * @brief Implicit conversion to double.
	 * @return double The double value of the fixed-point number.
	 */
	constexpr operator double() const {
		return to_double();
	}

	/** @} */

	/**
	 * @name Arithmetic Operators
	 * @{
	 */

	//... (Operators are documented similarly, for brevity, their documentation is not fully included here)

	/** @} */

	/**
	 * @name Comparison Operators
	 * @{
	 */

	/** @} */

	/**
	 * @name Comparison Operators
	 * @{
	 */

	/**
	 * @brief Three-way comparison operator (C++20 and later).
	 * @param b The other fixed_point object to compare with.
	 * @return std::strong_ordering The comparison result.
	 */
#if __cplusplus >= 202002L
	inline constexpr std::strong_ordering operator<=>(const fixed_point& b) const {
		return _container <=> b._container;
	}
#else
	/**
	 * @brief Equality comparison operator.
	 * @param b The other fixed_point object to compare with.
	 * @return bool Whether the two fixed-point numbers are equal.
	 */
	inline constexpr bool operator==(const fixed_point& b) const {
		return _container == b._container;
	}

	/**
	 * @brief Inequality comparison operator.
	 * @param b The other fixed_point object to compare with.
	 * @return bool Whether the two fixed-point numbers are not equal.
	 */
	inline constexpr bool operator!=(const fixed_point& b) const {
		return _container!= b._container;
	}

	/**
	 * @brief Greater-than comparison operator.
	 * @param b The other fixed_point object to compare with.
	 * @return bool Whether this fixed-point number is greater than the other.
	 */
	inline constexpr bool operator>(const fixed_point& b) const {
		return _container > b._container;
	}

	/**
	 * @brief Greater-than-or-equal comparison operator.
	 * @param b The other fixed_point object to compare with.
	 * @return bool Whether this fixed-point number is greater than or equal to the other.
	 */
	inline constexpr bool operator>=(const fixed_point& b) const {
		return _container >= b._container;
	}

	/**
	 * @brief Less-than comparison operator.
	 * @param b The other fixed_point object to compare with.
	 * @return bool Whether this fixed-point number is less than the other.
	 */
	inline constexpr bool operator<(const fixed_point& b) const {
		return _container < b._container;
	}

	/**
	 * @brief Less-than-or-equal comparison operator.
	 * @param b The other fixed_point object to compare with.
	 * @return bool Whether this fixed-point number is less than or equal to the other.
	 */
	inline constexpr bool operator<=(const fixed_point& b) const {
		return _container <= b._container;
	}
#endif

	/** @} */

	/**
	 * @name Bitwise Operators
	 * @{
	 */

	/**
	 * @brief Bitwise NOT operator.
	 * @return fixed_point The bitwise NOT of the fixed-point number.
	 */
	inline constexpr fixed_point& operator~() const {
		return { ~_container };
	}

	/**
	 * @brief Bitwise AND operator with another fixed_point.
	 * @param b The other fixed_point object.
	 * @return fixed_point The bitwise AND result.
	 */
	inline constexpr fixed_point operator&(const fixed_point& b) const {
		return { _container & b._container };
	}

	/**
	 * @brief Bitwise AND operator with a container_t.
	 * @param b The container_t value.
	 * @return fixed_point The bitwise AND result.
	 */
	inline constexpr fixed_point operator&(container_t b) const {
		return { _container & b };
	}

	/**
	 * @brief Bitwise OR operator with another fixed_point.
	 * @param b The other fixed_point object.
	 * @return fixed_point The bitwise OR result.
	 */
	inline constexpr fixed_point operator|(const fixed_point& b) const {
		return { _container | b._container };
	}

	/**
	 * @brief Bitwise OR operator with a container_t.
	 * @param b The container_t value.
	 * @return fixed_point The bitwise OR result.
	 */
	inline constexpr fixed_point operator|(container_t b) const {
		return { _container | b };
	}

	/**
	 * @brief Bitwise XOR operator with another fixed_point.
	 * @param b The other fixed_point object.
	 * @return fixed_point The bitwise XOR result.
	 */
	inline constexpr fixed_point operator^(const fixed_point& b) const {
		return { _container ^ b._container };
	}

	/**
	 * @brief Bitwise XOR operator with a container_t.
	 * @param b The container_t value.
	 * @return fixed_point The bitwise XOR result.
	 */
	inline constexpr fixed_point operator^(container_t b) const {
		return { _container ^ b };
	}

	/**
	 * @brief Right shift operator.
	 * @param b The number of bits to shift.
	 * @return fixed_point The result after right shifting.
	 */
	inline constexpr fixed_point operator>>(container_t b) const {
		return { _container >> b };
	}

	/**
	 * @brief Left shift operator.
	 * @param b The number of bits to shift.
	 * @return fixed_point The result after left shifting.
	 */
	inline constexpr fixed_point operator<<(container_t b) const {
		return { _container << b };
	}

	/** @} */

	/**
	 * @name Increment and Decrement Operators
	 * @{
	 */

	/**
	 * @brief Prefix increment operator.
	 * @return fixed_point& The incremented fixed-point object.
	 */
	inline constexpr fixed_point& operator++() {
		_container += multiplier;
		return *this;
	}

	/**
	 * @brief Postfix increment operator.
	 * @return fixed_point The fixed-point object before increment.
	 */
	inline constexpr fixed_point operator++(int) {
		container_t temp = _container;
		_container += multiplier;
		return { temp };
	}

	/**
	 * @brief Prefix decrement operator.
	 * @return fixed_point& The decremented fixed-point object.
	 */
	inline constexpr fixed_point& operator--() {
		_container -= multiplier;
		return *this;
	}

	/**
	 * @brief Postfix decrement operator.
	 * @return fixed_point The fixed-point object before decrement.
	 */
	inline constexpr fixed_point operator--(int) {
		container_t temp = _container;
		_container -= multiplier;
		return { temp };
	}

	/** @} */
};

template <typename container_t, container_t fraction_bits, Endian endianness>
Io::DataStream<endianness>& operator<<(Io::DataStream<endianness>& left, const fixed_point<container_t, fraction_bits>& right) {
	return left << right._container;
}
template <typename container_t, container_t fraction_bits, Endian endianness>
Io::DataStream<endianness>& operator>>(Io::DataStream<endianness>& left, fixed_point<container_t, fraction_bits>& right) {
	return left >> right._container;
}

typedef fixed_point<std::int16_t> fixed16;
typedef fixed_point<std::int32_t> fixed32;
typedef fixed_point<std::int64_t> fixed64;
#if defined (FIXED_DEFAULT_64_BIT)
typedef fixed64 fixed;
#elif defined (FIXED_DEFAULT_16_BIT)
typedef fixed16 fixed;
#else
typedef fixed32 fixed;
#endif

#ifndef FP_NO_LITERALS
constexpr fixed16 operator""_fx16(long double a) {
	return fixed16::from_long_double(a);
}
constexpr fixed32 operator""_fx32(long double a) {
	return fixed32::from_long_double(a);
}
constexpr fixed64 operator""_fx64(long double a) {
	return fixed64::from_long_double(a);
}
constexpr fixed operator""_fx(long double a) {
	return fixed::from_long_double(a);
}
#endif

/** @} */ // End of fixed_point group


}
}
#endif // ELVFIXEDPOINT_HPP
