#ifndef MHINT32X4_PRIVATE_HPP
#define MHINT32X4_PRIVATE_HPP
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
class MH_UTIL_API Int32x4_private {
public:
	typedef int32_t* iterator;
	typedef const int32_t* const_iterator;
	typedef std::array<int32_t,4> ArrayType;
	typedef std::initializer_list<float> InitializerList;
private:
	__m128i _data;
public:
	Int32x4_private();
	Int32x4_private(InitializerList a);
	Int32x4_private(const ArrayType& a);
	Int32x4_private(const __m128i& a);
	Int32x4_private(int32_t a);
	Int32x4_private(const int32_t* arr);
	Int32x4_private(int32_t e0, int32_t e1, int32_t e2, int32_t e3);
	Int32x4_private(const Int32x4_private& cpy); // Copy constructor;
	void operator=(const Int32x4_private& cpy); // Copy assignment
	void operator=(InitializerList a);
	void operator=(int32_t a);
	void operator=(const ArrayType& a);
	void store(int32_t* arr) const;
	// Compatibility with STL containers	ArrayType& asArray();
	ArrayType& asArray();
	const ArrayType& asArray() const;
	int32_t* data();
	const int32_t* data() const;
	size_t size() const;
	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;
	int32_t& at(size_t i);
	const int32_t& at(size_t i) const;
	int32_t& operator[](size_t i);
	const int32_t& operator[](size_t i) const;
	// Arithemetics and comparisons with other vectors
	void operator+=(const Int32x4_private& b);
	void operator-=(const Int32x4_private& b);
	void operator*=(const Int32x4_private& b);
	void operator/=(const Int32x4_private& b);
	void operator<<=(const Int32x4_private& b); // Minimum
	void operator>>=(const Int32x4_private& b); // Maximum
	Int32x4_private operator+(const Int32x4_private& b) const;
	Int32x4_private operator-(const Int32x4_private& b) const;
	Int32x4_private operator*(const Int32x4_private& b) const;
	Int32x4_private operator/(const Int32x4_private& b) const;
	bool operator==(const Int32x4_private& b) const;
	bool operator!=(const Int32x4_private& b) const;
	bool operator>(const Int32x4_private& b) const;
	bool operator>=(const Int32x4_private& b) const;
	bool operator<(const Int32x4_private& b) const;
	bool operator<=(const Int32x4_private& b) const;
	Int32x4_private operator<<(const Int32x4_private& b) const; // Minimum
	Int32x4_private operator>>(const Int32x4_private& b) const; // Maximum
	static Int32x4_private min(const Int32x4_private& a, const Int32x4_private& b); // Minimum
	static Int32x4_private max(const Int32x4_private& a, const Int32x4_private& b); // Maximum
	static Int32x4_private clamp(const Int32x4_private& a, const Int32x4_private& _min, const Int32x4_private& _max); // Clamp
	// Arithemetics with scalars
	void operator+=(int32_t b);
	void operator-=(int32_t b);
	void operator*=(int32_t b);
	void operator/=(int32_t b);
	void operator<<=(int32_t b); // Minimum
	void operator>>=(int32_t b); // Maximum
	Int32x4_private operator+(int32_t b) const;
	Int32x4_private operator-(int32_t b) const;
	Int32x4_private operator*(int32_t b) const;
	Int32x4_private operator/(int32_t b) const;
	Int32x4_private operator<<(int32_t b); // Minimum
	Int32x4_private operator>>(int32_t b); // Maximum
	static Int32x4_private min(const Int32x4_private& a, int32_t b); // Minimum
	static Int32x4_private max(const Int32x4_private& a, int32_t b); // Maximum
	static Int32x4_private clamp(const Int32x4_private& a, int32_t _min, int32_t _max); // Clamp
	bool operator==(int32_t b) const;
	bool operator!=(int32_t b) const;
	bool operator>(int32_t b) const;
	bool operator>=(int32_t b) const;
	bool operator<(int32_t b) const;
	bool operator<=(int32_t b) const;
	// Other mathematical goodies
	int32_t sum() const;
};
}
}
#else
#warning "SSE4.1 not supported."
#endif
#endif // MHINT32X4_PRIVATE_HPP
