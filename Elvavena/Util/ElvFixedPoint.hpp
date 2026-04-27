#ifndef ELVFIXEDPOINT_HPP
#define ELVFIXEDPOINT_HPP
/**
 * @file ElvFixedPoint.hpp
 * @brief Declares the ElvFixedPoint API in the Elvavena/Util module.
 *
 * This header is part of the public declaration surface for Elvavena/Util.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <cstdint>
#include <cmath>
#include <compare>
#include <Elvavena/Io/ElvDataStream.hpp>
#include <concepts>
#include <type_traits>
#if __cplusplus <= 199711L
  #error This library needs at least a C++11 compliant compiler
#endif
/** @brief Documents the FP_TEMPLATE_INTRO constant or macro. */
#define FP_TEMPLATE_INTRO template <typename container_t = std::int32_t, container_t fraction_bits = sizeof(container_t)*4>
/** @brief Documents the FP_TEMPLATE_INST constant or macro. */
#define FP_TEMPLATE_INST fixed_point<container_t,fraction_bits>
namespace Elv {
namespace Util {

/**
 * @defgroup fixed_point Fixed Point Arithmetic Class
 * @brief Provides a class template for fixed-point arithmetic.
 * @ingroup fp
 * @{
 */

// Concept for fixed_point
template <typename T>
concept FixedPoint = requires(T a, T b, typename T::container_type c, long double ld) {
	// Typedefs
	typename T::container_type;

	// Constants and Bit Masks
	/*{ T::bits } -> std::same_as<const std::uint8_t>;
	{ T::fraction_shifter } -> std::same_as<const typename T::container_type>;
	{ T::half_shifter } -> std::same_as<const typename T::container_type>;
	{ T::quarter_shifter } -> std::same_as<const typename T::container_type>;
	{ T::multiplier } -> std::same_as<const typename T::container_type>;
	{ T::fractionmask } -> std::same_as<const typename T::container_type>;
	{ T::wholemask } -> std::same_as<const typename T::container_type>;
	{ T::halfraw } -> std::same_as<const typename T::container_type>;
	{ T::multiplierF } -> std::same_as<const float>;
	{ T::multiplierD } -> std::same_as<const double>;
	{ T::multiplierLD } -> std::same_as<const long double>;
	{ T::dividerF } -> std::same_as<const float>;
	{ T::dividerD } -> std::same_as<const double>;
	{ T::dividerLD } -> std::same_as<const long double>;*/

	// Value Extraction
	{ a.fraction_part() } -> std::same_as<T>;
	{ a.whole_part() } -> std::same_as<T>;
	{ a.abs() } -> std::same_as<T>;
	{ a.floor() } -> std::same_as<T>;
	{ a.ceil() } -> std::same_as<T>;
	{ a.round() } -> std::same_as<T>;

	// Construction from Various Types
	{ T::from_raw(c) } -> std::same_as<T>;
	{ T::from_integer(c) } -> std::same_as<T>;
	{ T::from_float(static_cast<float>(ld)) } -> std::same_as<T>;
	{ T::from_double(static_cast<double>(ld)) } -> std::same_as<T>;
	{ T::from_long_double(ld) } -> std::same_as<T>;

	// Conversions
	{ a.to_raw() } -> std::same_as<typename T::container_type>;
	{ a.to_integer() } -> std::same_as<typename T::container_type>;
	{ a.to_float() } -> std::same_as<float>;
	{ a.to_double() } -> std::same_as<double>;
	{ a.to_long_double() } -> std::same_as<long double>;

	// Implicit Conversions
	{ static_cast<bool>(a) } -> std::same_as<bool>;
	{ static_cast<typename T::container_type>(a) } -> std::same_as<typename T::container_type>;
	{ static_cast<float>(a) } -> std::same_as<float>;
	{ static_cast<double>(a) } -> std::same_as<double>;

	// Arithmetic Operators
	{ a + b } -> std::same_as<T>;
	{ a - b } -> std::same_as<T>;
	{ a * b } -> std::same_as<T>;
	{ a / b } -> std::same_as<T>;
	{ a += b } -> std::same_as<T&>;
	{ a -= b } -> std::same_as<T&>;
	{ a *= b } -> std::same_as<T&>;
	{ a /= b } -> std::same_as<T&>;

	// Comparison Operators
		#if __cplusplus >= 202002L
	{ a <=> b } -> std::same_as<std::strong_ordering>;
		#else
	{ a == b } -> std::same_as<bool>;
	{ a != b } -> std::same_as<bool>;
	{ a > b } -> std::same_as<bool>;
	{ a >= b } -> std::same_as<bool>;
	{ a < b } -> std::same_as<bool>;
	{ a <= b } -> std::same_as<bool>;
		#endif

	// Bitwise Operators
	{ ~a } -> std::same_as<T>;
	{ a & b } -> std::same_as<T>;
	{ a & c } -> std::same_as<T>;
	{ a | b } -> std::same_as<T>;
	{ a | c } -> std::same_as<T>;
	{ a ^ b } -> std::same_as<T>;
	{ a ^ c } -> std::same_as<T>;
	{ a >> c } -> std::same_as<T>;
	{ a << c } -> std::same_as<T>;

	// Increment and Decrement Operators
	{ ++a } -> std::same_as<T&>;
	{ a++ } -> std::same_as<T>;
	{ --a } -> std::same_as<T&>;
	{ a-- } -> std::same_as<T>;
};

/**
 * @class fixed_point
 * @brief Class template for fixed-point numbers.
 *
 * @tparam container_t The underlying integer type to store the fixed-point number. Defaults to std::int32_t.
 * @tparam fraction_bits The number of bits to use for the fractional part. Defaults to half of the bits in container_t.
 */
/** @brief Documents the operator helper. */
FP_TEMPLATE_INTRO
struct fixed_point {
	/** @brief Documents the is_fixed_point declaration. */
	using is_fixed_point = void;
	/** @brief Documents the container_type declaration. */
	typedef container_t container_type;
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
	static constexpr const long double dividerLD = 1.0L / multiplierLD;

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
	 * @brief Default constructor
	 */
	inline constexpr explicit fixed_point() : _container(0) {
	}
	/**
	 * @brief Copy constructor
	 * @param cpy Value to be copied.
	 */
	inline constexpr explicit fixed_point(const fixed_point& cpy) : _container(cpy._container) {
	}
	/**
	 * @brief Copy assignment operator.
	 * @param cpy Value to be copied.
	 */
	inline constexpr fixed_point& operator=(const fixed_point& cpy) {
		this->_container = cpy._container;
		return *this;
	}
	/**
	 * @brief Constructs a fixed_point from an integer value.
	 * @param input The integer value.
	 * @param isRaw Whether the integer should be treated as a raw fixed-point number or not. Default is false.
	 */
	inline constexpr explicit fixed_point(container_t input, bool isRaw=false) : _container(isRaw ? input : (input << fraction_bits) ) {
	}
	/**
	 * @brief Copy assignment operator.
	 * @param input The integer value.
	 */
	inline constexpr fixed_point& operator=(container_t input) {
		this->_container = input << fraction_bits;
		return *this;
	}
	/**
	 * @brief Constructs a fixed_point from a float value.
	 * @param input The float value.
	 */
	inline constexpr explicit fixed_point(float input) : _container(static_cast<container_t>(input * multiplierF)) {
	}
	/**
	 * @brief Copy assignment operator.
	 * @param input The float value.
	 */
	inline constexpr fixed_point& operator=(float input) {
		this->_container = static_cast<container_t>(input * multiplierF);
		return *this;
	}
	/**
	 * @brief Constructs a fixed_point from a double value.
	 * @param input The double value.
	 */
	inline constexpr explicit fixed_point(double input) : _container(static_cast<container_t>(input * multiplierD)) {
	}
	/**
	 * @brief Copy assignment operator.
	 * @param input The double value.
	 */
	inline constexpr fixed_point& operator=(double input) {
		this->_container = static_cast<container_t>(input * multiplierD);
		return *this;
	}
	/**
	 * @brief Constructs a fixed_point from a long double value.
	 * @param input The long double value.
	 */
	inline constexpr explicit fixed_point(long double input) : _container(static_cast<container_t>(input * multiplierLD)) {
	}
	/**
	 * @brief Copy assignment operator.
	 * @param input The long double value.
	 */
	inline constexpr fixed_point& operator=(long double input) {
		this->_container = static_cast<container_t>(input * multiplierLD);
		return *this;
	}

	/**
	 * @brief Constructs a fixed_point from a raw container value.
	 * @param input The raw container value.
	 * @return fixed_point The constructed fixed-point object.
	 */
	inline static constexpr fixed_point from_raw(container_t input) {
		return fixed_point(input, true);
	}

	/**
	 * @brief Constructs a fixed_point from an integer value.
	 * @param input The integer value.
	 * @return fixed_point The constructed fixed-point object.
	 */
	inline static constexpr fixed_point from_integer(container_t input) {
		return fixed_point(input, false);
	}

	/**
	 * @brief Constructs a fixed_point from a float value.
	 * @param input The float value.
	 * @return fixed_point The constructed fixed-point object.
	 */
	inline static constexpr fixed_point from_float(float input) {
		return fixed_point(input);
	}

	/**
	 * @brief Constructs a fixed_point from a double value.
	 * @param input The double value.
	 * @return fixed_point The constructed fixed-point object.
	 */
	inline static constexpr fixed_point from_double(double input) {
		return fixed_point(input);
	}

	/**
	 * @brief Constructs a fixed_point from a long double value.
	 * @param input The long double value.
	 * @return fixed_point The constructed fixed-point object.
	 */
	inline static constexpr fixed_point from_long_double(long double input) {
		return fixed_point(input);
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

	/**
	 * @brief Addition operator
	 * @param b The other fixed_point object.
	 * @return Addition result.
	 */
	inline constexpr fixed_point& operator+=(const fixed_point& b) {
		_container += b._container;
		return *this;
	}
	/**
	 * @brief Addition operator
	 * @param b The other fixed_point object.
	 * @return Addition result.
	 */
	inline constexpr fixed_point operator+(const fixed_point& b) const {
		return from_raw(_container + b._container);
	}
	/**
	 * @brief Subtraction operator
	 * @param b The other fixed_point object.
	 * @return Subtraction result.
	 */
	inline constexpr fixed_point& operator-=(const fixed_point& b) {
		_container -= b._container;
		return *this;
	}
	/**
	 * @brief Subtraction operator
	 * @param b The other fixed_point object.
	 * @return Subtraction result.
	 */
	inline constexpr fixed_point operator-(const fixed_point& b) const {
		return from_raw(_container - b._container);
	}
	/**
	 * @brief Modulo operator
	 * @param b The other fixed_point object.
	 * @return Modulo result.
	 */
	inline constexpr fixed_point& operator%=(const fixed_point& b) {
		_container %= b._container;
		return *this;
	}
	/**
	 * @brief Modulo operator
	 * @param b The other fixed_point object.
	 * @return Modulo result.
	 */
	inline constexpr fixed_point operator%(const fixed_point& b) const {
		return from_raw(_container % b._container);
	}
	/**
	 * @brief Multiplication operator
	 * @param b The other fixed_point object.
	 * @return Multiplication result.
	 */
	inline constexpr fixed_point& operator*=(const fixed_point& b) {
		this->_container = (_container >> quarter_shifter) * (b._container >> quarter_shifter);
		return *this;
	}
	/**
	 * @brief Multiplication operator
	 * @param b The other fixed_point object.
	 * @return Multiplication result.
	 */
	inline constexpr fixed_point operator*(const fixed_point& b) const {
		return from_raw((_container >> quarter_shifter) * (b._container >> quarter_shifter));
	}
	/**
	 * @brief Division operator
	 * @param b The other fixed_point object.
	 * @return Division result.
	 */
	inline constexpr fixed_point& operator/=(const fixed_point& b) {
		this->_container = ((_container << quarter_shifter) / b._container) << quarter_shifter;
		return *this;
	}
	/**
	 * @brief Division operator
	 * @param b The other fixed_point object.
	 * @return Division result.
	 */
	inline constexpr fixed_point operator/(const fixed_point& b) const {
		return from_raw((_container >> ((_container << quarter_shifter) / b._container) << quarter_shifter));
	}

	/**
	 * @brief Addition operator
	 * @param b The other container_t object.
	 * @return Addition result.
	 */
	inline constexpr fixed_point& operator+=(container_t b) {
		*this += from_integer(b);
		return *this;
	}
	/**
	 * @brief Addition operator
	 * @param b The other container_t object.
	 * @return Addition result.
	 */
	inline constexpr fixed_point operator+(container_t b) const {
		return *this + from_integer(b);
	}
	/**
	 * @brief Subtraction operator
	 * @param b The other container_t object.
	 * @return Subtraction result.
	 */
	inline constexpr fixed_point& operator-=(container_t b) {
		*this -= from_integer(b);
		return *this;
	}
	/**
	 * @brief Subtraction operator
	 * @param b The other container_t object.
	 * @return Subtraction result.
	 */
	inline constexpr fixed_point operator-(container_t b) const {
		return *this - from_integer(b);
	}
	/**
	 * @brief Modulo operator
	 * @param b The other container_t object.
	 * @return Modulo result.
	 */
	inline constexpr fixed_point& operator%=(container_t b) {
		*this %= from_integer(b);
		return *this;
	}
	/**
	 * @brief Modulo operator
	 * @param b The other container_t object.
	 * @return Modulo result.
	 */
	inline constexpr fixed_point operator%(container_t b) const {
		return *this % from_integer(b);
	}
	/**
	 * @brief Multiplication operator
	 * @param b The other container_t object.
	 * @return Multiplication result.
	 */
	inline constexpr fixed_point& operator*=(container_t b) {
		*this *= from_integer(b);
		return *this;
	}
	/**
	 * @brief Multiplication operator
	 * @param b The other container_t object.
	 * @return Multiplication result.
	 */
	inline constexpr fixed_point operator*(container_t b) const {
		return *this * from_integer(b);
	}
	/**
	 * @brief Division operator
	 * @param b The other container_t object.
	 * @return Division result.
	 */
	inline constexpr fixed_point& operator/=(container_t b) {
		*this /= from_integer(b);
		return *this;
	}
	/**
	 * @brief Division operator
	 * @param b The other container_t object.
	 * @return Division result.
	 */
	inline constexpr fixed_point operator/(container_t b) const {
		return *this / from_integer(b);
	}

	/**
	 * @brief Addition operator
	 * @param b The other float object.
	 * @return Addition result.
	 */
	inline constexpr fixed_point& operator+=(float b) {
		*this += from_float(b);
		return *this;
	}
	/**
	 * @brief Addition operator
	 * @param b The other float object.
	 * @return Addition result.
	 */
	inline constexpr fixed_point operator+(float b) const {
		return *this + from_float(b);
	}
	/**
	 * @brief Subtraction operator
	 * @param b The other float object.
	 * @return Subtraction result.
	 */
	inline constexpr fixed_point& operator-=(float b) {
		*this -= from_float(b);
		return *this;
	}
	/**
	 * @brief Subtraction operator
	 * @param b The other float object.
	 * @return Subtraction result.
	 */
	inline constexpr fixed_point operator-(float b) const {
		return *this - from_float(b);
	}
	/**
	 * @brief Modulo operator
	 * @param b The other float object.
	 * @return Modulo result.
	 */
	inline constexpr fixed_point& operator%=(float b) {
		*this %= from_float(b);
		return *this;
	}
	/**
	 * @brief Modulo operator
	 * @param b The other float object.
	 * @return Modulo result.
	 */
	inline constexpr fixed_point operator%(float b) const {
		return *this % from_float(b);
	}
	/**
	 * @brief Multiplication operator
	 * @param b The other float object.
	 * @return Multiplication result.
	 */
	inline constexpr fixed_point& operator*=(float b) {
		*this *= from_float(b);
		return *this;
	}
	/**
	 * @brief Multiplication operator
	 * @param b The other float object.
	 * @return Multiplication result.
	 */
	inline constexpr fixed_point operator*(float b) const {
		return *this * from_float(b);
	}
	/**
	 * @brief Division operator
	 * @param b The other float object.
	 * @return Division result.
	 */
	inline constexpr fixed_point& operator/=(float b) {
		*this /= from_float(b);
		return *this;
	}
	/**
	 * @brief Division operator
	 * @param b The other float object.
	 * @return Division result.
	 */
	inline constexpr fixed_point operator/(float b) const {
		return *this / from_float(b);
	}

	/**
	 * @brief Addition operator
	 * @param b The other double object.
	 * @return Addition result.
	 */
	inline constexpr fixed_point& operator+=(double b) {
		*this += from_double(b);
		return *this;
	}
	/**
	 * @brief Addition operator
	 * @param b The other double object.
	 * @return Addition result.
	 */
	inline constexpr fixed_point operator+(double b) const {
		return *this + from_double(b);
	}
	/**
	 * @brief Subtraction operator
	 * @param b The other double object.
	 * @return Subtraction result.
	 */
	inline constexpr fixed_point& operator-=(double b) {
		*this -= from_double(b);
		return *this;
	}
	/**
	 * @brief Subtraction operator
	 * @param b The other double object.
	 * @return Subtraction result.
	 */
	inline constexpr fixed_point operator-(double b) const {
		return *this - from_double(b);
	}
	/**
	 * @brief Modulo operator
	 * @param b The other double object.
	 * @return Modulo result.
	 */
	inline constexpr fixed_point& operator%=(double b) {
		*this %= from_double(b);
		return *this;
	}
	/**
	 * @brief Modulo operator
	 * @param b The other double object.
	 * @return Modulo result.
	 */
	inline constexpr fixed_point operator%(double b) const {
		return *this % from_double(b);
	}
	/**
	 * @brief Multiplication operator
	 * @param b The other double object.
	 * @return Multiplication result.
	 */
	inline constexpr fixed_point& operator*=(double b) {
		*this *= from_double(b);
		return *this;
	}
	/**
	 * @brief Multiplication operator
	 * @param b The other double object.
	 * @return Multiplication result.
	 */
	inline constexpr fixed_point operator*(double b) const {
		return *this * from_double(b);
	}
	/**
	 * @brief Division operator
	 * @param b The other double object.
	 * @return Division result.
	 */
	inline constexpr fixed_point& operator/=(double b) {
		*this /= from_double(b);
		return *this;
	}
	/**
	 * @brief Division operator
	 * @param b The other double object.
	 * @return Division result.
	 */
	inline constexpr fixed_point operator/(double b) const {
		return *this / from_double(b);
	}

	/**
	 * @brief Addition operator
	 * @param b The other long double object.
	 * @return Addition result.
	 */
	inline constexpr fixed_point& operator+=(long double b) {
		*this += from_long_double(b);
		return *this;
	}
	/**
	 * @brief Addition operator
	 * @param b The other long double object.
	 * @return Addition result.
	 */
	inline constexpr fixed_point operator+(long double b) const {
		return *this + from_long_double(b);
	}
	/**
	 * @brief Subtraction operator
	 * @param b The other long double object.
	 * @return Subtraction result.
	 */
	inline constexpr fixed_point& operator-=(long double b) {
		*this -= from_long_double(b);
		return *this;
	}
	/**
	 * @brief Subtraction operator
	 * @param b The other long double object.
	 * @return Subtraction result.
	 */
	inline constexpr fixed_point operator-(long double b) const {
		return *this - from_long_double(b);
	}
	/**
	 * @brief Modulo operator
	 * @param b The other long double object.
	 * @return Modulo result.
	 */
	inline constexpr fixed_point& operator%=(long double b) {
		*this %= from_long_double(b);
		return *this;
	}
	/**
	 * @brief Modulo operator
	 * @param b The other long double object.
	 * @return Modulo result.
	 */
	inline constexpr fixed_point operator%(long double b) const {
		return *this % from_long_double(b);
	}
	/**
	 * @brief Multiplication operator
	 * @param b The other long double object.
	 * @return Multiplication result.
	 */
	inline constexpr fixed_point& operator*=(long double b) {
		*this *= from_long_double(b);
		return *this;
	}
	/**
	 * @brief Multiplication operator
	 * @param b The other long double object.
	 * @return Multiplication result.
	 */
	inline constexpr fixed_point operator*(long double b) const {
		return *this * from_long_double(b);
	}
	/**
	 * @brief Division operator
	 * @param b The other long double object.
	 * @return Division result.
	 */
	inline constexpr fixed_point& operator/=(long double b) {
		*this /= from_long_double(b);
		return *this;
	}
	/**
	 * @brief Division operator
	 * @param b The other long double object.
	 * @return Division result.
	 */
	inline constexpr fixed_point operator/(long double b) const {
		return *this / from_long_double(b);
	}

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
	inline constexpr fixed_point operator~() const {
		return from_raw(~_container);
	}

	/**
	 * @brief Bitwise AND operator with another fixed_point.
	 * @param b The other fixed_point object.
	 * @return fixed_point The bitwise AND result.
	 */
	inline constexpr fixed_point operator&(const fixed_point& b) const {
		return from_raw(_container & b._container);
	}

	/**
	 * @brief Bitwise AND operator with another fixed_point.
	 * @param b The other fixed_point object.
	 * @return fixed_point The bitwise AND result.
	 */
	inline constexpr fixed_point& operator&=(const fixed_point& b) {
		_container = _container & b._container;
		return *this;
	}

	/**
	 * @brief Bitwise AND operator with a container_t.
	 * @param b The container_t value.
	 * @return fixed_point The bitwise AND result.
	 */
	inline constexpr fixed_point operator&(container_t b) const {
		return from_raw(_container & b );
	}

	/**
	 * @brief Bitwise AND operator with a container_t.
	 * @param b The container_t value.
	 * @return fixed_point The bitwise AND result.
	 */
	inline constexpr fixed_point& operator&=(container_t b) {
		_container = _container & b;
		return *this;
	}

	/**
	 * @brief Bitwise OR operator with another fixed_point.
	 * @param b The other fixed_point object.
	 * @return fixed_point The bitwise OR result.
	 */
	inline constexpr fixed_point operator|(const fixed_point& b) const {
		return from_raw(_container | b._container);
	}

	/**
	 * @brief Bitwise OR operator with another fixed_point.
	 * @param b The other fixed_point object.
	 * @return fixed_point The bitwise OR result.
	 */
	inline constexpr fixed_point& operator|=(const fixed_point& b) {
		_container = _container | b._container;
		return *this;
	}

	/**
	 * @brief Bitwise OR operator with a container_t.
	 * @param b The container_t value.
	 * @return fixed_point The bitwise OR result.
	 */
	inline constexpr fixed_point operator|(container_t b) const {
		return from_raw(_container | b );
	}

	/**
	 * @brief Bitwise OR operator with a container_t.
	 * @param b The container_t value.
	 * @return fixed_point The bitwise OR result.
	 */
	inline constexpr fixed_point& operator|=(container_t b) {
		_container = _container | b;
		return *this;
	}

	/**
	 * @brief Bitwise XOR operator with another fixed_point.
	 * @param b The other fixed_point object.
	 * @return fixed_point The bitwise XOR result.
	 */
	inline constexpr fixed_point operator^(const fixed_point& b) const {
		return from_raw(_container ^ b._container);
	}
	/**
	 * @brief Bitwise XOR operator with another fixed_point.
	 * @param b The other fixed_point object.
	 * @return fixed_point The bitwise XOR result.
	 */
	inline constexpr fixed_point& operator^=(const fixed_point& b) {
		_container = _container ^ b._container;
		return *this;
	}

	/**
	 * @brief Bitwise XOR operator with a container_t.
	 * @param b The container_t value.
	 * @return fixed_point The bitwise XOR result.
	 */
	inline constexpr fixed_point operator^(container_t b) const {
		return from_raw(_container ^ b );
	}


	/**
	 * @brief Bitwise XOR operator with a container_t.
	 * @param b The container_t value.
	 * @return fixed_point The bitwise XOR result.
	 */
	inline constexpr fixed_point& operator^=(container_t b) {
		_container = _container ^ b;
		return *this;
	}

	/**
	 * @brief Right shift operator.
	 * @param b The number of bits to shift.
	 * @return fixed_point The result after right shifting.
	 */
	inline constexpr fixed_point operator>>(container_t b) const {
		return from_raw(_container >> b);
	}

	/**
	 * @brief Right shift operator.
	 * @param b The number of bits to shift.
	 * @return fixed_point The result after right shifting.
	 */
	inline constexpr fixed_point& operator>>=(container_t b) {
		_container = _container >> b;
		return *this;
	}

	/**
	 * @brief Left shift operator.
	 * @param b The number of bits to shift.
	 * @return fixed_point The result after left shifting.
	 */
	inline constexpr fixed_point operator<<(container_t b) const {
		return from_raw(_container << b );
	}

	/**
	 * @brief Left shift operator.
	 * @param b The number of bits to shift.
	 * @return fixed_point The result after left shifting.
	 */
	inline constexpr fixed_point& operator<<=(container_t b) {
		_container = _container << b;
		return *this;
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
		return from_raw(temp);
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
		return from_raw(temp);
	}

	/** @} */
};

template <typename container_t, container_t fraction_bits, class CharT, class Traits = std::char_traits<CharT>>
std::basic_ostream<CharT,Traits>& operator<<(std::basic_ostream<CharT,Traits>& left, const fixed_point<container_t, fraction_bits>& right) {
	return left << right.to_double();
}
template <typename container_t, container_t fraction_bits, class CharT, class Traits = std::char_traits<CharT>>
/** @brief Documents the operator helper. */
std::basic_istream<CharT,Traits>& operator>>(std::basic_istream<CharT,Traits>& left, const fixed_point<container_t, fraction_bits>& right) {
	double tmp;
	left >> tmp;
	right = fixed_point<container_t, fraction_bits>::from_double(tmp);
	return left;
}
template <typename container_t, container_t fraction_bits, Endian endianness>
Io::DataStream<endianness>& operator<<(Io::DataStream<endianness>& left, const fixed_point<container_t, fraction_bits>& right) {
	return left << right._container;
}
template <typename container_t, container_t fraction_bits, Endian endianness>
/** @brief Documents the operator helper. */
Io::DataStream<endianness>& operator>>(Io::DataStream<endianness>& left, fixed_point<container_t, fraction_bits>& right) {
	return left >> right._container;
}

/** @brief Documents the fixed16 declaration. */
typedef fixed_point<std::int16_t> fixed16;
/** @brief Documents the fixed32 declaration. */
typedef fixed_point<std::int32_t> fixed32;
/** @brief Documents the fixed64 declaration. */
typedef fixed_point<std::int64_t> fixed64;
#if defined (FIXED_DEFAULT_64_BIT)
typedef fixed64 fixed;
#elif defined (FIXED_DEFAULT_16_BIT)
typedef fixed16 fixed;
#else
/** @brief Documents the fixed declaration. */
typedef fixed32 fixed;
#endif

#ifndef FP_NO_LITERALS
/** @brief Documents the operator""_fx16 helper. */
constexpr fixed16 operator""_fx16(long double a) {
	return fixed16::from_long_double(a);
}
/** @brief Documents the operator""_fx32 helper. */
constexpr fixed32 operator""_fx32(long double a) {
	return fixed32::from_long_double(a);
}
/** @brief Documents the operator""_fx64 helper. */
constexpr fixed64 operator""_fx64(long double a) {
	return fixed64::from_long_double(a);
}
/** @brief Documents the operator""_fx helper. */
constexpr fixed operator""_fx(long double a) {
	return fixed::from_long_double(a);
}
#endif

/** @} */ // End of fixed_point group


}
}
#endif // ELVFIXEDPOINT_HPP
