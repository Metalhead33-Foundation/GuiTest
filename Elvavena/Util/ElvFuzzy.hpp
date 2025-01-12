#ifndef ELVFUZZY_HPP
#define ELVFUZZY_HPP
#include <Elvavena/Util/ElvInterpolation.hpp>
#include <numeric>
namespace Elv {
namespace Util {

#ifndef FUZZY_DEF_THRESHOLD
#define FUZZY_DEF_THRESHOLD 0.5
#endif

enum class FuzzyLogicalOperatorPolicy : uint8_t {
	ZADEH,
	ALGEBRAIC
};
template <typename T, FuzzyLogicalOperatorPolicy policy = FuzzyLogicalOperatorPolicy::ZADEH> requires InterpolationWeight<T> struct fuzzy {
	typedef T fuzzy_type;
	typedef T& fuzzy_ref;
	typedef const T& const_fuzzy_ref;

	fuzzy_type raw_val_;
	constexpr fuzzy very() const { return fuzzy(raw_val_*raw_val_); }
	constexpr fuzzy exceptionally() const { return fuzzy(raw_val_*raw_val_*raw_val_); }
	constexpr fuzzy more_or_less() const { return fuzzy(std::sqrt(raw_val_)); }

	constexpr bool to_bool(fuzzy_type threshold = static_cast<fuzzy_type>(FUZZY_DEF_THRESHOLD)) const {
		return raw_val_ >= threshold;
	}

	constexpr operator bool() const {
		return raw_val_ >= static_cast<fuzzy_type>(FUZZY_DEF_THRESHOLD);
	}

	constexpr operator fuzzy_ref() {
		return raw_val_;
	}

	constexpr operator const_fuzzy_ref() const {
		return raw_val_;
	}

	constexpr operator fuzzy_type() const {
		return raw_val_;
	}

	constexpr fuzzy(const fuzzy& otherfuzzy) : raw_val_(otherfuzzy.raw_val_) {

	}

	constexpr fuzzy(fuzzy_type fval) : raw_val_(fval) {

	}

	constexpr fuzzy() : raw_val_(static_cast<fuzzy_type>(0)) {

	}

	constexpr fuzzy& operator=(const fuzzy& otherfuzzy) {
		this->raw_val_ = otherfuzzy.raw_val_;
		return *this;
	}

	constexpr fuzzy& operator=(fuzzy_type fval) {
		this->raw_val_ = fval;
		return *this;
	}
	template <typename T2> constexpr static fuzzy fuzzifyMinMax(const T2& crisp, const T2& min, const T2& max)
	{
		return (crisp <= min) ? fuzzy(static_cast<fuzzy_type>(0))
							  : ( (crisp >= max) ? fuzzy(static_cast<fuzzy_type>(1))
												 : fuzzy( static_cast<fuzzy_type>(crisp-min) / static_cast<fuzzy_type>(max-min) ));
	}
	template <typename T2> constexpr static fuzzy fuzzifyTriangle(const T2& crisp, const T2& left_min, const T2& centre, const T2& right_min)
	{
		return (crisp <= left_min || crisp >= right_min) ? fuzzy(static_cast<fuzzy_type>(0)) :( (crisp <= centre) ?
																				fuzzy(static_cast<fuzzy_type>(crisp-left_min)/static_cast<fuzzy_type>(centre-left_min))
																			  : fuzzy(static_cast<fuzzy_type>(right_min-crisp)/static_cast<fuzzy_type>(right_min-centre)));
	}
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
	constexpr fuzzy complement() const { return fuzzy(static_cast<fuzzy_type>(1)-raw_val_); }
	constexpr fuzzy tNormZadeh(const fuzzy& b) const { return fuzzy(std::min(raw_val_,b.raw_val_)); }
	constexpr fuzzy tNormAlgebraic(const fuzzy& b) const { return fuzzy(raw_val_ * b.raw_val_); }
	constexpr fuzzy sNormZadeh(const fuzzy& b) const { return fuzzy(std::max(raw_val_,b.raw_val_)); }
	constexpr fuzzy sNormAlgebraic(const fuzzy& b) const { return fuzzy((raw_val_+b.raw_val_)-(raw_val_ * b.raw_val_)); }
	constexpr fuzzy operator!() const { return complement(); }
	constexpr fuzzy operator&&(const fuzzy& right) const {
		switch (policy) {
			case FuzzyLogicalOperatorPolicy::ALGEBRAIC: return tNormAlgebraic(right);
			case FuzzyLogicalOperatorPolicy::ZADEH: return tNormZadeh(right);
			default: return tNormZadeh(right);
		}
	}
	constexpr fuzzy operator||(const fuzzy& right) const {
		switch (policy) {
			case FuzzyLogicalOperatorPolicy::ALGEBRAIC: return sNormAlgebraic(right);
			case FuzzyLogicalOperatorPolicy::ZADEH: return sNormZadeh(right);
			default: return sNormZadeh(right);
		}
	}
};
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
template <typename T>
constexpr T defuzzifyCenterOfGravity(std::span<const T> elements, auto membershipFunc, auto valueFunc) {
	return defuzzifyHelper(elements.begin(), elements.end(), membershipFunc, valueFunc);
}

template <typename T, typename MembershipFunction>
constexpr T defuzzifyCenterOfGravity(T min, T max, size_t steps, MembershipFunction membershipFn) {
	auto stepSize = (max - min) / static_cast<T>(steps);
	return defuzzifyHelper(
		size_t{0},
		steps + 1,
		[&](size_t i) { return membershipFn(min + static_cast<T>(i) * stepSize); },
		[&](size_t i) { return min + static_cast<T>(i) * stepSize; }
	);
}


}
}
#endif // ELVFUZZY_HPP
