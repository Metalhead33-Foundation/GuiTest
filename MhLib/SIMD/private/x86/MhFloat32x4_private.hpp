#ifndef MHFLOAT32X4_PRIVATE_HPP
#define MHFLOAT32X4_PRIVATE_HPP
#ifdef __SSE4_1__
#include <cstdint>
#include <array>
#include <initializer_list>
extern "C" {
#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
}
#include <MhLib/Util/MhGlobals.hpp>

namespace MH33 {
namespace SIMD {

class MH_UTIL_API Float32x4_private {
public:
	typedef float* iterator;
	typedef const float* const_iterator;
	typedef std::array<float,4> ArrayType;
	typedef std::initializer_list<float> InitializerList;
private:
	__m128 _data;
public:
	Float32x4_private();
	Float32x4_private(InitializerList a);
	Float32x4_private(const ArrayType& a);
	Float32x4_private(const __m128& a);
	Float32x4_private(float a);
	Float32x4_private(const float* arr);
	Float32x4_private(float e0, float e1, float e2, float e3);
	Float32x4_private(const Float32x4_private& cpy); // Copy constructor;
	void operator=(const Float32x4_private& cpy); // Copy assignment
	void operator=(InitializerList a);
	void operator=(float a);
	void operator=(const float* arr);
	void operator=(const ArrayType& arr);
	void store(float* arr) const;
	Float32x4_private reciprocal() const;
	void set_to_reciprocal();

	// Compatibility with STL containers
	ArrayType& asArray();
	const ArrayType& asArray() const;
	float* data();
	const float* data() const;
	size_t size() const;
	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;
	float& at(size_t i);
	const float& at(size_t i) const;
	float& operator[](size_t i);
	const float& operator[](size_t i) const;
	// Arithemetics and comparisons with other vectors
	void operator+=(const Float32x4_private& b);
	void operator-=(const Float32x4_private& b);
	void operator*=(const Float32x4_private& b);
	void operator/=(const Float32x4_private& b);
	void operator<<=(const Float32x4_private& b); // Minimum
	void operator>>=(const Float32x4_private& b); // Maximum
	Float32x4_private operator+(const Float32x4_private& b) const;
	Float32x4_private operator-(const Float32x4_private& b) const;
	Float32x4_private operator*(const Float32x4_private& b) const;
	Float32x4_private operator/(const Float32x4_private& b) const;
	bool operator==(const Float32x4_private& b) const;
	bool operator!=(const Float32x4_private& b) const;
	bool operator>(const Float32x4_private& b) const;
	bool operator>=(const Float32x4_private& b) const;
	bool operator<(const Float32x4_private& b) const;
	bool operator<=(const Float32x4_private& b) const;
	Float32x4_private operator<<(const Float32x4_private& b); // Minimum
	Float32x4_private operator>>(const Float32x4_private& b); // Maximum
	static Float32x4_private min(const Float32x4_private& a, const Float32x4_private& b); // Minimum
	static Float32x4_private max(const Float32x4_private& a, const Float32x4_private& b); // Maximum
	static Float32x4_private clamp(const Float32x4_private& a, const Float32x4_private& _min, const Float32x4_private& _max); // Clamp
	float vectorialMul(const Float32x4_private& b) const;
	// Arithemetics with scalars
	void operator+=(float b);
	void operator-=(float b);
	void operator*=(float b);
	void operator/=(float b);
	void operator<<=(float b); // Minimum
	void operator>>=(float b); // Maximum
	Float32x4_private operator+(float b) const;
	Float32x4_private operator-(float b) const;
	Float32x4_private operator*(float b) const;
	Float32x4_private operator/(float b) const;
	Float32x4_private operator<<(float b) const; // Minimum
	Float32x4_private operator>>(float b) const; // Maximum
	static Float32x4_private min(const Float32x4_private& a, float b); // Minimum
	static Float32x4_private max(const Float32x4_private& a, float b); // Maximum
	static Float32x4_private clamp(const Float32x4_private& a, float _min, float _max); // Clamp
	bool operator==(float b) const;
	bool operator!=(float b) const;
	bool operator>(float b) const;
	bool operator>=(float b) const;
	bool operator<(float b) const;
	bool operator<=(float b) const;
	float vectorialMul(float b) const;
	// Other mathematical goodies
	Float32x4_private sqrt() const;
	void set_to_sqrt();
	Float32x4_private rsqrt() const;
	void set_to_rsqrt();
	Float32x4_private normalize() const;
	void set_to_norm();
	float sum() const;
};

}
}

#endif
#else
#warning "SSE4.1 not supported."
#endif // MHFLOAT32X4_PRIVATE_HPP
