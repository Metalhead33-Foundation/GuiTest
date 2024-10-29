#ifndef ELVFIXEDPOINTMATH_HPP
#define ELVFIXEDPOINTMATH_HPP
#include <Elvavena/Util/ElvFixedPoint.hpp>
namespace Elv {
namespace Util {

/**
 * @brief Provides functionality for normalizing and denormalizing integers to/from fixed-point numbers.
 *
 * This struct is templated to work with different container types (e.g., std::int16_t, std::int32_t, std::int64_t)
 * and is designed to facilitate conversions between integer representations of various color formats and their
 * fixed-point equivalents, leveraging the characteristics of the fixed_point class.
 *
 * @tparam container_t The underlying integer type to store the fixed-point number. Defaults to std::int32_t.
 * @tparam fraction_bits The number of bits to use for the fractional part. Defaults to half of the bits in container_t.
 */
FP_TEMPLATE_INTRO struct fixed_norm {
	/**
	 * @brief Type alias for the fixed_point class utilizing the container_t.
	 */
	typedef fixed_point<container_t, fraction_bits> fixed_type;

	/**
	 * @name Precomputed bit gaps for common color format bit depths.
	 * @{
	 */
	/// Bit gap for 8-bit depth (used in RGBA8888, RGB888)
	static constexpr const container_t bitgap_8b = fixed_type::fraction_shifter - 8;
	/// Bit gap for 6-bit depth (used in RGB565)
	static constexpr const container_t bitgap_6b = fixed_type::fraction_shifter - 6;
	/// Bit gap for 5-bit depth (used in RGB565, RGB332)
	static constexpr const container_t bitgap_5b = fixed_type::fraction_shifter - 5;
	/// Bit gap for 3-bit depth (used in RGB332)
	static constexpr const container_t bitgap_3b = fixed_type::fraction_shifter - 3;
	/// Bit gap for 2-bit depth (used in RGB332)
	static constexpr const container_t bitgap_2b = fixed_type::fraction_shifter - 2;
	/// @}

	/**
	 * @brief Nested struct for normalizing and denormalizing integers of type @p integer_type to/from @p fixed_type.
	 *
	 * @tparam integer_type The type of integer to normalize/denormalize.
	 */
	template <typename integer_type> struct norm_denorm {
		/**
		 * @brief The bit size of the @p integer_type.
		 */
		static constexpr const container_t int_bitsize = sizeof(integer_type) * 8;

		/**
		 * @brief The bit gap between the @p fixed_type's fraction shifter and the @p integer_type's bit size.
		 */
		static constexpr const container_t int_bitgap = fixed_type::fraction_shifter - int_bitsize;

		/**
		 * @brief Normalizes an @p integer to a @p fixed_type.
		 *
		 * If the integer is non-zero, shifts it left by @p int_bitgap after adding 1 for positive integers or subtracting 1 for negative integers.
		 *
		 * @param integer The integer to normalize.
		 * @return The normalized fixed-point number.
		 */
		static constexpr fixed_type _normalize(const integer_type& integer) {
			return { integer?
							(integer > 0? ((static_cast<container_t>(integer) + 1)  << int_bitgap) : ((static_cast<container_t>(integer)-1) << int_bitgap))
						  : 0 };
		}

		/**
		 * @brief Denormalizes a @p fixed_type to an @p integer_type.
		 *
		 * If the fixed-point number is non-zero, shifts its container right by @p int_bitgap and then subtracts 1 for positive results or adds 1 for negative results.
		 *
		 * @param fixedpoint The fixed-point number to denormalize.
		 * @return The denormalized integer.
		 */
		static constexpr integer_type _denormalize(const fixed_type& fixedpoint) {
			return static_cast<integer_type>(
				fixedpoint._container?
				(fixedpoint._container > 0? (static_cast<integer_type>((fixedpoint._container - 1) >> int_bitgap))
				: (static_cast<integer_type>((fixedpoint._container + 1) >> int_bitgap)))
				: 0
			);
		}
	};

	/**
	 * @brief Convenience function for normalizing an @p integer_type to a @p fixed_type.
	 *
	 * @tparam integer_type The type of the integer to normalize.
	 * @param integer The integer to normalize.
	 * @return The normalized fixed-point number.
	 */
	template <typename integer_type> static constexpr fixed_type normalize(const integer_type& integer) {
		return norm_denorm<integer_type>::_normalize(integer);
	}

	/**
	 * @brief Convenience function for denormalizing a @p fixed_type to an @p integer_type.
	 *
	 * @tparam integer_type The type of the integer to denormalize to.
	 * @param fixedpoint The fixed-point number to denormalize.
	 * @return The denormalized integer.
	 */
	template <typename integer_type> static constexpr integer_type denormalize(const fixed_type& fixedpoint) {
		return norm_denorm<integer_type>::_denormalize(fixedpoint);
	}

	/**
	 * @name Normalization functions for specific color formats.
	 * @{
	 */

	/**
	 * @brief Normalizes an RGBA8888 color to four @p fixed_type components.
	 *
	 * Each color channel (R, G, B, A) is normalized by shifting left based on the @p bitgap_8b.
	 *
	 * @param rgba The RGBA8888 color as a std::uint32_t.
	 * @param o1 Output for the Red component.
	 * @param o2 Output for the Green component.
	 * @param o3 Output for the Blue component.
	 * @param o4 Output for the Alpha component.
	 */
	static void normalizeRGBA8888(std::uint32_t rgba, fixed_type& o1, fixed_type& o2, fixed_type& o3, fixed_type& o4) {
		const container_t t1 = static_cast<container_t>((rgba & 0xFF000000) >> 24 & 0xFF);
		const container_t t2 = static_cast<container_t>((rgba & 0x00FF0000) >> 16 & 0xFF);
		const container_t t3 = static_cast<container_t>((rgba & 0x0000FF00) >> 8 & 0xFF);
		const container_t t4 = static_cast<container_t>(rgba & 0x000000FF);
		o1._container = t1? (t1+1) << bitgap_8b : 0;
		o2._container = t2? (t2+1) << bitgap_8b : 0;
		o3._container = t3? (t3+1) << bitgap_8b : 0;
		o4._container = t4? (t4+1) << bitgap_8b : 0;
	}

	/**
	 * @brief Normalizes an RGB888 color to three @p fixed_type components.
	 *
	 * Each color channel (R, G, B) is normalized by shifting left based on the @p bitgap_8b.
	 *
	 * @param rgb The RGB888 color as a std::uint32_t.
	 * @param o1 Output for the Red component.
	 * @param o2 Output for the Green component.
	 * @param o3 Output for the Blue component.
	 */
	static void normalizeRGB888(std::uint32_t rgb, fixed_type& o1, fixed_type& o2, fixed_type& o3) {
		const container_t t1 = static_cast<container_t>((rgb & 0xFF0000) >> 16);
		const container_t t2 = static_cast<container_t>((rgb & 0x00FF00) >> 8);
		const container_t t3 = static_cast<container_t>(rgb & 0x0000FF);
		o1._container = t1? (t1+1) << bitgap_8b : 0;
		o2._container = t2? (t2+1) << bitgap_8b : 0;
		o3._container = t3? (t3+1) << bitgap_8b : 0;
	}

	/**
	 * @brief Normalizes an RGB565 color to three @p fixed_type components.
	 *
	 * Each color channel (R, G, B) is normalized based on its respective bit depth (5 for R and B, 6 for G).
	 *
	 * @param rgb The RGB565 color as a std::uint16_t.
	 * @param o1 Output for the Red component.
	 * @param o2 Output for the Green component.
	 * @param o3 Output for the Blue component.
	 */
	static void normalizeRGB565(std::uint16_t rgb, fixed_type& o1, fixed_type& o2, fixed_type& o3) {
		const container_t t1 = static_cast<container_t>((rgb & 0xF800) >> 11);
		const container_t t2 = static_cast<container_t>((rgb & 0x07E0) >> 5);
		const container_t t3 = static_cast<container_t>(rgb & 0x001F);
		o1._container = t1? (t1+1) << bitgap_5b : 0;
		o2._container = t2? (t2+1) << bitgap_6b : 0;
		o3._container = t3? (t3+1) << bitgap_5b : 0;
	}

	/**
	 * @brief Normalizes an RGB332 color to three @p fixed_type components.
	 *
	 * Each color channel (R, G, B) is normalized based on its respective bit depth (3 for R and G, 2 for B).
	 *
	 * @param rgb The RGB332 color as a std::uint8_t.
	 * @param o1 Output for the Red component.
	 * @param o2 Output for the Green component.
	 * @param o3 Output for the Blue component.
	 */
	static void normalizeRGB332(std::uint8_t rgb, fixed_type& o1, fixed_type& o2, fixed_type& o3) {
		const container_t t1 = static_cast<container_t>((rgb & 0xE0) >> 5);
		const container_t t2 = static_cast<container_t>((rgb & 0x1C) >> 2);
		const container_t t3 = static_cast<container_t>(rgb & 0x03);
		o1._container = t1? (t1+1) << bitgap_3b : 0;
		o2._container = t2? (t2+1) << bitgap_3b : 0;
		o3._container = t3? (t3+1) << bitgap_2b : 0;
	}
	/// @}

	/**
	 * @name Denormalization functions for specific color formats.
	 * @{
	 */

	/**
	 * @brief Denormalizes four @p fixed_type components back to an RGBA8888 color.
	 *
	 * Each component is denormalized by shifting right based on the @p bitgap_8b and then adjusting by 1 if non-zero.
	 *
	 * @param i1 The Red component.
	 * @param i2 The Green component.
	 * @param i3 The Blue component.
	 * @param i4 The Alpha component.
	 * @return The denormalized RGBA8888 color as a std::uint32_t.
	 */
	static std::uint32_t denormalizeRGBA8888(const fixed_type& i1, const fixed_type& i2, const fixed_type& i3, const fixed_type& i4) {
		const std::uint32_t t1 = static_cast<std::uint32_t> (i1._container? (i1._container - 1) >> bitgap_8b : 0) & 0xFF;
		const std::uint32_t t2 = static_cast<std::uint32_t> (i2._container? (i2._container - 1) >> bitgap_8b : 0) & 0xFF;
		const std::uint32_t t3 = static_cast<std::uint32_t> (i3._container? (i3._container - 1) >> bitgap_8b : 0) & 0xFF;
		const std::uint32_t t4 = static_cast<std::uint32_t> (i4._container? (i4._container - 1) >> bitgap_8b : 0) & 0xFF;
		return (t1 << 24) | (t2 << 16) | (t3 << 8) | t4;
	}

	/**
	 * @brief Denormalizes three @p fixed_type components back to an RGB888 color.
	 *
	 * Each component is denormalized by shifting right based on the @p bitgap_8b and then adjusting by 1 if non-zero.
	 *
	 * @param i1 The Red component.
	 * @param i2 The Green component.
	 * @param i3 The Blue component.
	 * @return The denormalized RGB888 color as a std::uint32_t.
	 */
	static std::uint32_t denormalizeRGB888(const fixed_type& i1, const fixed_type& i2, const fixed_type& i3) {
		const std::uint32_t t1 = static_cast<std::uint32_t> (i1._container? (i1._container - 1) >> bitgap_8b : 0) & 0xFF;
		const std::uint32_t t2 = static_cast<std::uint32_t> (i2._container? (i2._container - 1) >> bitgap_8b : 0) & 0xFF;
		const std::uint32_t t3 = static_cast<std::uint32_t> (i3._container? (i3._container - 1) >> bitgap_8b : 0) & 0xFF;
		return (t1 << 16) | (t2 << 8) | t3;
	}

	/**
	 * @brief Denormalizes three @p fixed_type components back to an RGB565 color.
	 *
	 * Each component is denormalized by shifting right based on its respective bit gap (@p bitgap_5b for R and B, @p bitgap_6b for G) and then adjusting by 1 if non-zero.
	 *
	 * @param i1 The Red component.
	 * @param i2 The Green component.
	 * @param i3 The Blue component.
	 * @return The denormalized RGB565 color as a std::uint16_t.
	 */
	static std::uint16_t denormalizeRGB565(const fixed_type& i1, const fixed_type& i2, const fixed_type& i3) {
		const std::uint16_t t1 = static_cast<std::uint16_t> (i1._container? (i1._container - 1) >> bitgap_5b : 0) & 0x1F;
		const std::uint16_t t2 = static_cast<std::uint16_t> (i2._container? (i2._container - 1) >> bitgap_6b : 0) & 0x3F;
		const std::uint16_t t3 = static_cast<std::uint16_t> (i3._container? (i3._container - 1) >> bitgap_5b : 0) & 0x1F;
		return (t1 << 11) | (t2 << 5) | t3;
	}

	/**
	 * @brief Denormalizes three @p fixed_type components back to an RGB332 color.
	 *
	 * Each component is denormalized by shifting right based on its respective bit gap (@p bitgap_3b for R and G, @p bitgap_2b for B) and then adjusting by 1 if non-zero.
	 *
	 * @param i1 The Red component.
	 * @param i2 The Green component.
	 * @param i3 The Blue component.
	 * @return The denormalized RGB332 color as a std::uint8_t.
	 */
	static std::uint8_t denormalizeRGB332(const fixed_type& i1, const fixed_type& i2, const fixed_type& i3) {
		const std::uint8_t t1 = static_cast<std::uint8_t> (i1._container? (i1._container - 1) >> bitgap_3b : 0) & 0x07;
		const std::uint8_t t2 = static_cast<std::uint8_t> (i2._container? (i2._container - 1) >> bitgap_3b : 0) & 0x07;
		const std::uint8_t t3 = static_cast<std::uint8_t> (i3._container? (i3._container - 1) >> bitgap_2b : 0) & 0x03;
		return (t1 << 5) | (t2 << 2) | t3;
	}
	/// @}
};

/**
 * @brief Convenience typedefs for common fixed_norm instantiations.
 */
typedef fixed_norm<std::int16_t> fnorm16; /**< fixed_norm for 16-bit containers */
typedef fixed_norm<std::int32_t> fnorm32; /**< fixed_norm for 32-bit containers */
typedef fixed_norm<std::int64_t> fnorm64; /**< fixed_norm for 64-bit containers */

/**
 * @brief Default fixed_norm typedef based on FIXED_DEFAULT_*_BIT definition.
 *
 * If none of the FIXED_DEFAULT_*_BIT macros are defined, fnorm defaults to fnorm32.
 */
#if defined (FIXED_DEFAULT_64_BIT)
	/**
	 * @brief Default fixed_norm set to 64-bit due to FIXED_DEFAULT_64_BIT definition.
	 */
	typedef fnorm64 fnorm;
#elif defined (FIXED_DEFAULT_16_BIT)
	/**
	 * @brief Default fixed_norm set to 16-bit due to FIXED_DEFAULT_16_BIT definition.
	 */
	typedef fnorm16 fnorm;
#else
	/**
	 * @brief Default fixed_norm set to 32-bit (default if no FIXED_DEFAULT_*_BIT is defined).
	 */
	typedef fnorm32 fnorm;
#endif

/**
 * @class fixed_point_math
 * @brief Template class providing various mathematical operations using fixed-point arithmetic.
 *
 * @tparam container_type Underlying integer type for fixed-point representation (default: std::int32_t).
 * @tparam fraction_bits Number of bits allocated for the fractional part (default: sizeof(container_type)*4).
 */
template <typename container_type = std::int32_t, container_type fraction_bits = sizeof(container_type)*4>
struct fixed_point_math {

	/**
	 * @typedef fixed_type
	 * @brief Type alias for the fixed-point type used within this class, based on the template parameters.
	 */
	typedef fixed_point<container_type, fraction_bits> fixed_type;

	// Constants
	/**
	 * @var m_e
	 * @brief Fixed-point representation of Euler's number (e).
	 */
	static constexpr const fixed_type m_e = fixed_type::from_double(M_E);

	/**
	 * @var m_log2e
	 * @brief Fixed-point representation of the natural logarithm of 2.
	 */
	static constexpr const fixed_type m_log2e = fixed_type::from_double(M_LOG2E);

	/**
	 * @var m_log10e
	 * @brief Fixed-point representation of the base-10 logarithm of e.
	 */
	static constexpr const fixed_type m_log10e = fixed_type::from_double(M_LOG10E);

	/**
	 * @var m_ln2
	 * @brief Fixed-point representation of the natural logarithm of 2.
	 */
	static constexpr const fixed_type m_ln2 = fixed_type::from_double(M_LN2);

	/**
	 * @var m_ln10
	 * @brief Fixed-point representation of the natural logarithm of 10.
	 */
	static constexpr const fixed_type m_ln10 = fixed_type::from_double(M_LN10);

	/**
	 * @var m_pi
	 * @brief Fixed-point representation of Pi.
	 */
	static constexpr const fixed_type m_pi = fixed_type::from_double(M_PI);

	/**
	 * @var m_pi_2
	 * @brief Fixed-point representation of Pi divided by 2.
	 */
	static constexpr const fixed_type m_pi_2 = fixed_type::from_double(M_PI_2);

	/**
	 * @var m_pi_4
	 * @brief Fixed-point representation of Pi divided by 4.
	 */
	static constexpr const fixed_type m_pi_4 = fixed_type::from_double(M_PI_4);

	/**
	 * @var m_1_pi
	 * @brief Fixed-point representation of the reciprocal of Pi.
	 */
	static constexpr const fixed_type m_1_pi = fixed_type::from_double(M_1_PI);

	/**
	 * @var m_2_pi
	 * @brief Fixed-point representation of 2 multiplied by Pi.
	 */
	static constexpr const fixed_type m_2_pi = fixed_type::from_double(M_2_PI);

	/**
	 * @var m_2_sqrtpi
	 * @brief Fixed-point representation of 2 multiplied by the square root of Pi.
	 */
	static constexpr const fixed_type m_2_sqrtpi = fixed_type::from_double(M_2_SQRTPI);

	/**
	 * @var m_sqrt2
	 * @brief Fixed-point representation of the square root of 2.
	 */
	static constexpr const fixed_type m_sqrt2 = fixed_type::from_double(M_SQRT2);

	/**
	 * @var m_sqrt1_2
	 * @brief Fixed-point representation of the square root of 1/2.
	 */
	static constexpr const fixed_type m_sqrt1_2 = fixed_type::from_double(M_SQRT1_2);

	/**
	 * @var logMagic1
	 * @brief First magic constant for logarithmic approximation.
	 */
	static constexpr const fixed_type logMagic1 = fixed_type::from_double(0.411933);

	/**
	 * @var logMagic2
	 * @brief Second magic constant for logarithmic approximation.
	 */
	static constexpr const fixed_type logMagic2 = fixed_type::from_double(0.574785);

	/**
	 * @var logMagic3
	 * @brief Third magic constant for logarithmic approximation.
	 */
	static constexpr const fixed_type logMagic3 = fixed_type::from_double(0.994946);

	/**
	 * @var logMagic4
	 * @brief Fourth magic constant for logarithmic approximation.
	 */
	static constexpr const fixed_type logMagic4 = fixed_type::from_double(0.00227683);

	// Wrappers
	/**
	 * @fn fraction
	 * @brief Returns the fractional part of a fixed-point number.
	 *
	 * @param x Input fixed-point number.
	 * @return Fractional part of x.
	 */
	static constexpr fixed_type fraction(const fixed_type& x) { return x.fraction_part(); }

	/**
	 * @fn whole
	 * @brief Returns the whole part of a fixed-point number.
	 *
	 * @param x Input fixed-point number.
	 * @return Whole part of x.
	 */
	static constexpr fixed_type whole(const fixed_type& x) { return x.whole_part(); }

	/**
	 * @fn abs
	 * @brief Returns the absolute value of a fixed-point number.
	 *
	 * @param x Input fixed-point number.
	 * @return Absolute value of x.
	 */
	static constexpr fixed_type abs(const fixed_type& x) { return x.abs(); }

	/**
	 * @fn floor
	 * @brief Returns the largest integer less than or equal to a fixed-point number.
	 *
	 * @param x Input fixed-point number.
	 * @return Floor of x.
	 */
	static constexpr fixed_type floor(const fixed_type& x) { return x.floor(); }

	/**
	 * @fn ceil
	 * @brief Returns the smallest integer greater than or equal to a fixed-point number.
	 *
	 * @param x Input fixed-point number.
	 * @return Ceiling of x.
	 */
	static constexpr fixed_type ceil(const fixed_type& x) { return x.ceil(); }

	/**
	 * @fn round
	 * @brief Rounds a fixed-point number to the nearest integer.
	 *
	 * @param x Input fixed-point number.
	 * @return Rounded value of x.
	 */
	static constexpr fixed_type round(const fixed_type& x) { return x.round(); }


	/**
	 * @fn exp
	 * @brief Calculates the exponential function of a fixed-point number using a Taylor series approximation.
	 *
	 * @param x Input fixed-point number.
	 * @param runs Number of iterations for the Taylor series (default: 5).
	 * @return Exponential of x.
	 */
	static constexpr fixed_type exp(const fixed_type& x, container_type runs = 5) {
		fixed_type sum = fixed_type::from_integer(1);
		for (container_type i = runs - 1; i > 0; --i )
				sum = fixed_type::from_integer(1) + x * sum / i;
		return sum;
	}

	/**
	 * @fn ln
	 * @brief Calculates the natural logarithm of a fixed-point number using an iterative method.
	 *
	 * @param x Input fixed-point number.
	 * @param epsilon Precision threshold (default: fixed_type::from_raw(4096)).
	 * @return Natural logarithm of x.
	 */
	static fixed_type ln(const fixed_type& x, fixed_type epsilon = fixed_type::from_raw(4096)) {
		if (x == m_e) return fixed_type::from_integer(1);
		else {
		fixed_type yn = x - fixed_type::from_integer(1);
		fixed_type yn1 = yn;
		do
		{
			yn = yn1;
			yn1 = yn + fixed_type::from_integer(2) * (x - exp(yn)) / (x + exp(yn));
		} while ((yn - yn1).abs() > epsilon);
		return yn1;
		}
	}

	/**
	 * @fn log
	 * @brief Calculates the logarithm of a fixed-point number with a specified base.
	 *
	 * @param x Input fixed-point number.
	 * @param n Base of the logarithm (default: m_e for natural logarithm).
	 * @param epsilon Precision threshold (default: fixed_type::from_raw(4096)).
	 * @return Logarithm of x with base n.
	 */
	static fixed_type log(const fixed_type& x, const fixed_type& n = m_e, fixed_type epsilon = fixed_type::from_raw(4096)) {
		if(n == m_e) return ln(x, epsilon);
		else if(n.to_integer() == 2) return ln(x, epsilon) / m_ln2;
		else return ln(x, epsilon) / ln(n, epsilon);
	}

	/**
	 * @fn log2
	 * @brief Calculates the base-2 logarithm of a fixed-point number.
	 *
	 * @param x Input fixed-point number.
	 * @param epsilon Precision threshold (default: fixed_type::from_raw(4096)).
	 * @return Base-2 logarithm of x.
	 */
	static fixed_type log2(const fixed_type& x, fixed_type epsilon = fixed_type::from_raw(4096)) {
		return ln(x, epsilon) / m_ln2;
	}

	/**
	 * @fn log10
	 * @brief Calculates the base-10 logarithm of a fixed-point number.
	 *
	 * @param x Input fixed-point number.
	 * @param epsilon Precision threshold (default: fixed_type::from_raw(4096)).
	 * @return Base-10 logarithm of x.
	 */
	static fixed_type log10(const fixed_type& x, fixed_type epsilon = fixed_type::from_raw(4096)) {
		return ln(x, epsilon) / m_ln10;
	}

	/**
	 * @fn pow
	 * @brief Raises a fixed-point number to a power (x^y) using exponential and logarithmic functions.
	 *
	 * @param x Base fixed-point number.
	 * @param y Exponent fixed-point number.
	 * @return x raised to the power of y.
	 */
	static constexpr fixed_type pow(const fixed_type& x, const fixed_type& y) {
		 return exp(ln(x) * y);
	}

	/**
	 * @fn sqrt
	 * @brief Calculates the square root of a fixed-point number using the pow function.
	 *
	 * @param x Input fixed-point number.
	 * @return Square root of x.
	 */
	static constexpr fixed_type sqrt(const fixed_type& x) {
		 return pow(x, fixed_type::from_double(0.5));
	}

	// Trigonometrics
	/**
	 * @fn sin
	 * @brief Approximates the sine of a fixed-point angle (in radians) using a Taylor series.
	 *
	 * @param x Input fixed-point angle in radians.
	 * @return Sine of x.
	 */
	static constexpr fixed_type sin(const fixed_type& x) {
		 fixed_type a = x % m_pi;
		 return a
			 - ((a * a * a) / 6)
			 + ((a * a * a * a * a) / 120)
			 - ((a * a * a * a * a * a * a) / 5040);
	}

	/**
	 * @fn cos
	 * @brief Approximates the cosine of a fixed-point angle (in radians) using a Taylor series.
	 *
	 * @param x Input fixed-point angle in radians.
	 * @return Cosine of x.
	 */
	static constexpr fixed_type cos(const fixed_type& x) {
		 fixed_type a = x % m_pi;
		 return fixed_type::from_integer(1)
			 - ((a * a) / 2)
			 + ((a * a * a * a) / 24)
			 - ((a * a * a * a * a * a) / 720);
	}

	/**
	 * @fn tan
	 * @brief Approximates the tangent of a fixed-point angle (in radians) using a Taylor series.
	 *
	 * @param x Input fixed-point angle in radians.
	 * @return Tangent of x.
	 */
	static constexpr fixed_type tan(const fixed_type& x) {
		 fixed_type a = x % m_pi;
		 return a
			 + ((a * a * a) / 3)
			 + (((a * a * a * a * a) * 2) / 15)
			 + (((a * a * a * a * a * a * a) * 17) / 315);
	}

	/**
	 * @fn sinh
	 * @brief Approximates the hyperbolic sine of a fixed-point value using a Taylor series.
	 *
	 * @param x Input fixed-point value.
	 * @return Hyperbolic sine of x.
	 */
	static constexpr fixed_type sinh(const fixed_type& x) {
		 fixed_type a = x % m_pi;
		 return a
			 + ((a * a * a) / 6)
			 + ((a * a * a * a * a) / 120)
			 + ((a * a * a * a * a * a * a) / 5040);
	}

	/**
	 * @fn cosh
	 * @brief Approximates the hyperbolic cosine of a fixed-point value using a Taylor series.
	 *
	 * @param x Input fixed-point value.
	 * @return Hyperbolic cosine of x.
	 */
	static constexpr fixed_type cosh(const fixed_type& x) {
		fixed_type a = x;
		return fixed_type::from_integer(1) + a
			+ ((a * a) / 2)
			+ ((a * a * a * a) / 24)
			+ ((a * a * a * a * a * a) / 720);
	}

	/**
	 * @fn tanh
	 * @brief Approximates the hyperbolic tangent of a fixed-point value using a Taylor series.
	 *
	 * @param x Input fixed-point value.
	 * @return Hyperbolic tangent of x.
	 */
	static constexpr fixed_type tanh(const fixed_type& x) {
		fixed_type a = x;
		return a
		   - ((a * a * a) / 3)
		   + (((a * a * a * a * a) * 2) / 15)
		   - (((a * a * a * a * a * a * a) * 17) / 315);
	}
};

/**
 * @typedef fmath16
 * @brief Convenience alias for fixed_point_math with std::int16_t as the container type.
 */
typedef fixed_point_math<std::int16_t> fmath16;

/**
 * @typedef fmath32
 * @brief Convenience alias for fixed_point_math with std::int32_t as the container type (default).
 */
typedef fixed_point_math<std::int32_t> fmath32;

/**
 * @typedef fmath64
 * @brief Convenience alias for fixed_point_math with std::int64_t as the container type.
 */
typedef fixed_point_math<std::int64_t> fmath64;

/**
 * @typedef fmath
 * @brief Default fixed_point_math type, selected based on FIXED_DEFAULT_64_BIT, FIXED_DEFAULT_16_BIT, or defaulting to fmath32.
 */
#if defined (FIXED_DEFAULT_64_BIT)
typedef fmath64 fmath;
#elif defined (FIXED_DEFAULT_16_BIT)
typedef fmath16 fmath;
#else
typedef fmath32 fmath;
#endif

}
}
#endif // ELVFIXEDPOINTMATH_HPP
