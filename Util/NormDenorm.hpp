#ifndef NORMDENORM_HPP
#define NORMDENORM_HPP
#include <limits>
#include <cmath>

template <typename T> struct norm_denorm {
	static constexpr const float fmin = static_cast<float>(std::numeric_limits<T>::min());
	static constexpr const float fmax = static_cast<float>(std::numeric_limits<T>::max());
	static constexpr const float fmin_recip = ((std::numeric_limits<T>::min())) ? 1.0f / fmin : 0.0f;
	static constexpr const float fmax_recip = 1.0f / fmax;

	inline static constexpr float normalize(const T& org)
	{
		return org < 0
			? static_cast<float>(org) * fmin_recip
			:  static_cast<float>(org) * fmax_recip;
	}
	inline static constexpr T denormalize(const float& org)
	{
		return org < 0.0f
			? static_cast<T>(std::round(org * fmin))
			: static_cast<T>(std::round(org * fmax));
	}
};

template <> struct norm_denorm<float> {
	inline static constexpr float normalize(const float& org)
	{
		return org;
	}
	inline static constexpr float denormalize(const float& org)
	{
		return org;
	}
};
template <> struct norm_denorm<double> {
	inline static constexpr float normalize(const double& org)
	{
		return static_cast<float>(org);
	}
	inline static constexpr double denormalize(const float& org)
	{
		return static_cast<double>(org);
	}
};

template <typename T> float normalize(const T& org)
{
	return norm_denorm<T>::normalize(org);
}
template <typename T> T denormalize(const float& org)
{
	return norm_denorm<T>::denormalize(org);
}
template <typename INPUT, typename OUTPUT> OUTPUT scale(const INPUT& in)
{
	return denormalize<OUTPUT>(normalize(in));
}


#endif // NORMDENORM_HPP
