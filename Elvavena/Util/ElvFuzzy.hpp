#ifndef ELVFUZZY_HPP
#define ELVFUZZY_HPP
#include <Elvavena/Util/ElvInterpolation.hpp>
#include <numeric>
namespace Elv {
namespace Util {

#ifndef FUZZY_DEF_THRESHOLD
#define FUZZY_DEF_THRESHOLD 0.5
#endif

/**
 * @brief Enum class to define different fuzzy logical operator policies.
 */
enum class FuzzyLogicalOperatorPolicy : uint8_t {
	ZADEH,       /**< Zadeh logic operators */
	ALGEBRAIC    /**< Algebraic logic operators */
};

/**
 * @brief Template class representing a fuzzy value.
 *
 * @tparam T Type of the fuzzy value.
 * @tparam policy Fuzzy logical operator policy.
 *
 * @note The class requires the type T to satisfy the InterpolationWeight concept.
 */
template <typename T, FuzzyLogicalOperatorPolicy policy = FuzzyLogicalOperatorPolicy::ZADEH> requires InterpolationWeight<T>
struct fuzzy {
	typedef T fuzzy_type;         /**< Type alias for the fuzzy value type. */
	typedef T& fuzzy_ref;         /**< Type alias for a reference to the fuzzy value type. */
	typedef const T& const_fuzzy_ref; /**< Type alias for a constant reference to the fuzzy value type. */

	fuzzy_type raw_val_;          /**< The raw fuzzy value. */

	/**
	 * @brief Returns the very degree of the fuzzy value.
	 *
	 * @return fuzzy The very degree of the fuzzy value.
	 */
	constexpr fuzzy very() const { return fuzzy(raw_val_*raw_val_); }

	/**
	 * @brief Returns the exceptionally degree of the fuzzy value.
	 *
	 * @return fuzzy The exceptionally degree of the fuzzy value.
	 */
	constexpr fuzzy exceptionally() const { return fuzzy(raw_val_*raw_val_*raw_val_); }

	/**
	 * @brief Returns the more or less degree of the fuzzy value.
	 *
	 * @return fuzzy The more or less degree of the fuzzy value.
	 */
	constexpr fuzzy more_or_less() const { return fuzzy(std::sqrt(raw_val_)); }

	/**
	 * @brief Converts the fuzzy value to a boolean based on a threshold.
	 *
	 * @param threshold The threshold value.
	 * @return bool The boolean value.
	 */
	constexpr bool to_bool(fuzzy_type threshold = static_cast<fuzzy_type>(FUZZY_DEF_THRESHOLD)) const {
		return raw_val_ >= threshold;
	}

	/**
	 * @brief Implicit conversion to boolean.
	 *
	 * @return bool The boolean value.
	 */
	constexpr operator bool() const {
		return raw_val_ >= static_cast<fuzzy_type>(FUZZY_DEF_THRESHOLD);
	}

	/**
	 * @brief Implicit conversion to a reference of the fuzzy value type.
	 *
	 * @return fuzzy_ref The reference to the fuzzy value.
	 */
	constexpr operator fuzzy_ref() {
		return raw_val_;
	}

	/**
	 * @brief Implicit conversion to a constant reference of the fuzzy value type.
	 *
	 * @return const_fuzzy_ref The constant reference to fuzzy value.
	 */
	constexpr operator const_fuzzy_ref() const {
		return raw_val_;
	}

	/**
	 * @brief Implicit conversion to the fuzzy value type.
	 *
	 * @return fuzzy_type The fuzzy value.
	 */
	constexpr operator fuzzy_type() const {
		return raw_val_;
	}

	/**
	 * @brief Copy constructor.
	 *
	 * @param otherfuzzy The other fuzzy value.
	 */
	constexpr fuzzy(const fuzzy& otherfuzzy) : raw_val_(otherfuzzy.raw_val_) {}

	/**
	 * @brief Constructor from a raw value.
	 *
	 * @param fval The raw value.
	 */
	constexpr fuzzy(fuzzy_type fval) : raw_val_(fval) {}

	/**
	 * @brief Default constructor.
	 */
	constexpr fuzzy() : raw_val_(static_cast<fuzzy_type>(FUZZY_DEF_THRESHOLD)) {}

	/**
	 * @brief Copy assignment operator.
	 *
	 * @param otherfuzzy The other fuzzy value.
	 * @return fuzzy& The assigned fuzzy value.
	 */
	constexpr fuzzy& operator=(const fuzzy& otherfuzzy) {
		this->raw_val_ = otherfuzzy.raw_val_;
		return *this;
	}

	/**
	 * @brief Assignment operator from a raw value.
	 *
	 * @param fval The raw value.
	 * @return fuzzy& The assigned fuzzy value.
	 */
	constexpr fuzzy& operator=(fuzzy_type fval) {
		this->raw_val_ = fval;
		return *this;
	}

	/**
	 * @brief Fuzzify a crisp value using the min-max method.
	 *
	 * @tparam T2 Type of the crisp value.
	 * @param crisp The crisp value.
	 * @param min The minimum value.
	 * @param max The maximum value.
	 * @return fuzzy The fuzzified fuzzy value.
	 */
	template <typename T2> constexpr static fuzzy fuzzifyMinMax(const T2& crisp, const T2& min, const T2& max)
	{
		return (crisp <= min) ? fuzzy(static_cast<fuzzy_type>(0))
							  : ( (crisp >= max) ? fuzzy(static_cast<fuzzy_type>(1))
												 : fuzzy( static_cast<fuzzy_type>(crisp-min) / static_cast<fuzzy_type>(max-min) ));
	}

	/**
	 * @brief Fuzzify a crisp value using the triangle method.
	 *
	 * @tparam T2 Type of the crisp value.
	 * @param crisp The crisp value.
	 * @param left_min The left minimum value.
	 * @param centre The centre value.
	 * @param right_min The right minimum value.
	 * @return fuzzy The fuzzified fuzzy value.
	 */
	template <typename T2> constexpr static fuzzy fuzzifyTriangle(const T2& crisp, const T2& left_min, const T2& centre, const T2& right_min)
	{
		return (crisp <= left_min || crisp >= right_min) ? fuzzy(static_cast<fuzzy_type>(0)) :
			   ( (crisp <= centre) ?
				 fuzzy(static_cast<fuzzy_type>(crisp-left_min)/static_cast<fuzzy_type>(centre-left_min))
			   : fuzzy(static_cast<fuzzy_type>(right_min-crisp)/static_cast<fuzzy_type>(right_min-centre)));
	}

	/**
	 * @brief Fuzzify a crisp value using the trapezoid method.
	 *
	 * @tparam T2 Type of the crisp value.
	 * @param crisp The crisp value.
	 * @param left_zero The left zero value.
	 * @param left_peak The left peak value.
	 * @param right_peak The right peak value.
	 * @param right_zero The right zero value.
	 * @return fuzzy The fuzzified fuzzy value.
	 */
	template <typename T2> constexpr static fuzzy fuzzifyTrapezoid(const T2& crisp, const T2& left_zero,
																	 const T2& left_peak, const T2& right_peak,
																	 const T2& right_zero) {
		return (crisp <= left_zero || crisp >= right_zero) ? fuzzy(static_cast<fuzzy_type>(0)) : (
			(crisp >= left_peak && crisp <= right_peak) ? fuzzy(static_cast<fuzzy_type>(1)) : (
			(crisp <= left_peak) ? fuzzy(static_cast<fuzzy_type>(crisp-left_zero)/static_cast<fuzzy_type>(left_peak-left_zero)) :
								   fuzzy(static_cast<fuzzy_type>(right_zero-crisp)/static_cast<fuzzy_type>(right_zero-right_peak))
			));
	}

	// Defuzzification

	// Logical operators

	/**
	 * @brief Complement the fuzzy value.
	 *
	 * @return fuzzy The complemented fuzzy value.
	 */
	constexpr fuzzy complement() const { return fuzzy(static_cast<fuzzy_type>(1)-raw_val_); }

	/**
	 * @brief T-Norm using Zadeh logic.
	 *
	 * @param b The other fuzzy value.
	 * @return fuzzy The T-Norm result.
	 */
	constexpr fuzzy tNormZadeh(const fuzzy& b) const { return fuzzy(std::min(raw_val_,b.raw_val_)); }

	/**
	 * @brief T-Norm using algebraic logic.
	 *
	 * @param b The other fuzzy value.
	 * @return fuzzy The T-Norm result.
	 */
	constexpr fuzzy tNormAlgebraic(const fuzzy& b) const { return fuzzy(raw_val_ * b.raw_val_); }

	/**
	 * @brief S-Norm using Zadeh logic.
	 *
	 * @param b The other fuzzy value.
	 * @return fuzzy The S-Norm result.
	 */
	constexpr fuzzy sNormZadeh(const fuzzy& b) const { return fuzzy(std::max(raw_val_,b.raw_val_)); }

	/**
	 * @brief S-Norm using algebraic logic.
	 *
	 * @param b The other fuzzy value.
	 * @return fuzzy The S-Norm result.
	 */
	constexpr fuzzy sNormAlgebraic(const fuzzy& b) const { return fuzzy((raw_val_+b.raw_val_)-(raw_val_ * b.raw_val_)); }

	/**
	 * @brief Logical NOT operator.
	 *
	 * @return fuzzy The complemented fuzzy value.
	 */
	constexpr fuzzy operator!() const { return complement(); }

	/**
	 * @brief Logical AND operator.
	 *
	 * @param right The other fuzzy value.
	 * @return fuzzy The logical AND result.
	 */
	constexpr fuzzy operator&&(const fuzzy& right) const {
		switch (policy) {
			case FuzzyLogicalOperatorPolicy::ALGEBRAIC: return tNormAlgebraic(right);
			case FuzzyLogicalOperatorPolicy::ZADEH: return tNormZadeh(right);
			default: return tNormZadeh(right);
		}
	}

	/**
	 * @brief Logical OR operator.
	 *
	 * @param right The other fuzzy value.
	 * @return fuzzy The logical OR result.
	 */
	constexpr fuzzy operator||(const fuzzy& right) const {
		switch (policy) {
			case FuzzyLogicalOperatorPolicy::ALGEBRAIC: return sNormAlgebraic(right);
			case FuzzyLogicalOperatorPolicy::ZADEH: return sNormZadeh(right);
			default: return sNormZadeh(right);
		}
	}
};

/**
 * @brief Helper function for defuzzification.
 *
 * @tparam Iter Iterator type.
 * @tparam MembershipFunc Membership function type.
 * @tparam ValueFunc Value function type.
 * @param begin Begin iterator.
 * @param end End iterator.
 * @param membershipFunc Membership function.
 * @param valueFunc Value function.
 * @return auto The defuzzified value.
 */
template <typename Iter, typename MembershipFunc, typename ValueFunc>
constexpr auto defuzzifyHelper(Iter begin, Iter end, MembershipFunc membershipFunc, ValueFunc valueFunc) {
	using FuzzyType = decltype(membershipFunc(*begin));
	using ValueType = decltype(valueFunc(*begin));

	FuzzyType numerator = static_cast<FuzzyType>(0);
	FuzzyType denominator = static_cast<FuzzyType>(0);

	for (auto it = begin; it != end; ++it) {
		FuzzyType membership = membershipFunc(*it);
		ValueType value = valueFunc(*it);

		numerator += value * membership;
		denominator += membership;
	}

	return (denominator > static_cast<FuzzyType>(0)) ? (numerator / denominator) : static_cast<ValueType>(0);
}

/**
 * @brief Defuzzify using the center of gravity method.
 *
 * @tparam T Type of the elements.
 * @param elements The elements.
 * @param membershipFunc Membership function.
 * @param valueFunc Value function.
 * @return T The defuzzified value.
 */
template <typename T>
constexpr T defuzzifyCenterOfGravity(std::span<const T> elements, auto membershipFunc, auto valueFunc) {
	return defuzzifyHelper(elements.begin(), elements.end(), membershipFunc, valueFunc);
}

/**
 * @brief Defuzzify using the center of gravity method with a specified range and step count.
 *
 * @tparam T Type of the elements.
 * @tparam MembershipFunction Membership function type.
 * @param min The minimum value.
 * @param max The maximum value.
 * @param steps The number of steps.
 * @param membershipFn Membership function.
 * @return T The defuzzified value.
 */
template <typename T, typename MembershipFunction>
constexpr T defuzzifyCenterOfGravity(T min, T max, size_t steps, MembershipFunction membershipFn) {
	auto stepSize = (max - min) / static_cast<T>(steps);
	return defuzzifyHelper(size_t(0),
						   steps + 1,
						   [&](size_t i) { return membershipFn(min + static_cast<T>(i) * stepSize); },
						   [&](size_t i) { return min + static_cast<T>(i) * stepSize; }
	);
}


}
}
#endif // ELVFUZZY_HPP
