#ifndef FLOAT64X2_PRIVATE_HPP
#define FLOAT64X2_PRIVATE_HPP
#ifdef __AVX512VL__
#include <cstdint>
#include <array>>
#include <initializer_list>
extern "C" {
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
}
#include <MhLib/Util/MhGlobals.hpp>
namespace MH33 {
namespace SIMD {

class MH_UTIL_API Float64x2_private {
public:
	typedef double* iterator;
	typedef const double* const_iterator;
	typedef std::array<double,2> ArrayType;
private:
	__m128d _data;
public:
	Float64x2_private();
	Float64x2_private(InitializerList a);
	Float64x2_private(const ArrayType& a);
	Float64x2_private(const __m128d& a);
	Float64x2_private(double a);
	Float64x2_private(const double* arr);
	Float64x2_private(double e0, double e1);
	Float64x2_private(const Float64x2_private& cpy); // Copy constructor;
	void operator=(const Float64x2_private& cpy); // Copy assignment
	void operator=(InitializerList a);
	void operator=(double a);
	void operator=(const double* arr);
	void operator=(const ArrayType& arr);
	void store(double* arr) const;
	Float64x2_private reciprocal() const;
	void set_to_reciprocal();

	// Compatibility with STL containers
	ArrayType& asArray();
	const ArrayType& asArray() const;
	double* data();
	const double* data() const;
	size_t size() const;
	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;
	double& at(size_t i);
	const double& at(size_t i) const;
	double& operator[](size_t i);
	const double& operator[](size_t i) const;
	// Arithemetics and comparisons with other vectors
	void operator+=(const Float64x2_private& b);
	void operator-=(const Float64x2_private& b);
	void operator*=(const Float64x2_private& b);
	void operator/=(const Float64x2_private& b);
	void operator<<=(const Float64x2_private& b); // Minimum
	void operator>>=(const Float64x2_private& b); // Maximum
	Float64x2_private operator+(const Float64x2_private& b) const;
	Float64x2_private operator-(const Float64x2_private& b) const;
	Float64x2_private operator*(const Float64x2_private& b) const;
	Float64x2_private operator/(const Float64x2_private& b) const;
	bool operator==(const Float64x2_private& b) const;
	bool operator!=(const Float64x2_private& b) const;
	bool operator>(const Float64x2_private& b) const;
	bool operator>=(const Float64x2_private& b) const;
	bool operator<(const Float64x2_private& b) const;
	bool operator<=(const Float64x2_private& b) const;
	Float64x2_private operator<<(const Float64x2_private& b); // Minimum
	Float64x2_private operator>>(const Float64x2_private& b); // Maximum
	static Float64x2_private min(const Float64x2_private& a, const Float64x2_private& b); // Minimum
	static Float64x2_private max(const Float64x2_private& a, const Float64x2_private& b); // Maximum
	static Float64x2_private clamp(const Float64x2_private& a, const Float64x2_private& _min, const Float64x2_private& _max); // Clamp
	double vectorialMul(const Float64x2_private& b) const;
	// Arithemetics with scalars
	void operator+=(double b);
	void operator-=(double b);
	void operator*=(double b);
	void operator/=(double b);
	void operator<<=(double b); // Minimum
	void operator>>=(double b); // Maximum
	Float64x2_private operator+(double b) const;
	Float64x2_private operator-(double b) const;
	Float64x2_private operator*(double b) const;
	Float64x2_private operator/(double b) const;
	Float64x2_private operator<<(double b) const; // Minimum
	Float64x2_private operator>>(double b) const; // Maximum
	static Float64x2_private min(const Float64x2_private& a, float b); // Minimum
	static Float64x2_private max(const Float64x2_private& a, float b); // Maximum
	static Float64x2_private clamp(const Float64x2_private& a, float _min, float _max); // Clamp
	bool operator==(double b) const;
	bool operator!=(double b) const;
	bool operator>(double b) const;
	bool operator>=(double b) const;
	bool operator<(double b) const;
	bool operator<=(double b) const;
	double vectorialMul(double b) const;
	// Other mathematical goodies
	Float64x2_private sqrt() const;
	void set_to_sqrt();
	Float64x2_private rsqrt() const;
	void set_to_rsqrt();
	Float64x2_private normalize() const;
	void set_to_norm();
	double sum() const;
};

}
}
#else
#warning "AVX512VL not supported."
#endif
#endif // FLOAT64X2_PRIVATE_HPP
