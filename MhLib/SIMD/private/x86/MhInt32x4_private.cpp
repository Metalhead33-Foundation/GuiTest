#ifdef __SSE4_1__
#include "MhInt32x4_private.hpp"
#include <cstdint>
#include <algorithm>
#include <smmintrin.h>

namespace MH33 {
namespace SIMD {

Int32x4_private::Int32x4_private()
	: _data(_mm_setzero_si128())
{

}

Int32x4_private::Int32x4_private(Int32x4_private::InitializerList a)
	: _data(_mm_setzero_si128())
{
	std::copy(a.begin(),a.end(),begin());
}

Int32x4_private::Int32x4_private(const Int32x4_private::ArrayType &a)
	: _data(_mm_lddqu_si128(reinterpret_cast<const __m128i*>(a.data())))
{

}

Int32x4_private::Int32x4_private(const __m128i &a)
	: _data(a)
{

}

Int32x4_private::Int32x4_private(int32_t a)
	: _data(_mm_set1_epi32(a))
{

}

Int32x4_private::Int32x4_private(const int32_t *arr)
	: _data(_mm_lddqu_si128(reinterpret_cast<const __m128i*>(arr)))
{

}

Int32x4_private::Int32x4_private(int32_t e0, int32_t e1, int32_t e2, int32_t e3)
	: _data(_mm_set_epi32(e3,e2,e1,e0))
{

}

Int32x4_private::Int32x4_private(const Int32x4_private &cpy)
	: _data(cpy._data)
{

}

void Int32x4_private::operator=(const Int32x4_private &cpy)
{
	_data = cpy._data;
}

void Int32x4_private::operator=(Int32x4_private::InitializerList a)
{
	std::copy(a.begin(),a.end(),begin());
}

void Int32x4_private::operator=(int32_t a)
{
	_data = _mm_set1_epi32(a);
}

void Int32x4_private::operator=(const Int32x4_private::ArrayType &a)
{
	_data = _mm_lddqu_si128(reinterpret_cast<const __m128i*>(a.data()));
}

void Int32x4_private::store(int32_t *arr) const
{
	_mm_store_si128(reinterpret_cast<__m128i*>(arr),_data);
}

Int32x4_private::ArrayType &Int32x4_private::asArray()
{
	return *reinterpret_cast<ArrayType*>(&_data);
}

const Int32x4_private::ArrayType &Int32x4_private::asArray() const
{
	return *reinterpret_cast<const ArrayType*>(&_data);
}
int32_t* Int32x4_private::data() { return reinterpret_cast<int32_t*>(&_data); }
const int32_t* Int32x4_private::data() const { return reinterpret_cast<const int32_t*>(&_data); }
size_t Int32x4_private::size() const { return 4; }
Int32x4_private::iterator Int32x4_private::begin() { return reinterpret_cast<int32_t*>(&_data); }
Int32x4_private::iterator Int32x4_private::end() { return &reinterpret_cast<int32_t*>(&_data)[4]; }
Int32x4_private::const_iterator Int32x4_private::begin() const { return reinterpret_cast<const int32_t*>(&_data); }
Int32x4_private::const_iterator Int32x4_private::end() const { return &reinterpret_cast<const int32_t*>(&_data)[4]; }
int32_t& Int32x4_private::at(size_t i) { return *reinterpret_cast<int32_t*>(&_data); }
const int32_t& Int32x4_private::at(size_t i) const { return *reinterpret_cast<const int32_t*>(&_data); }
int32_t& Int32x4_private::operator[](size_t i) { return *reinterpret_cast<int32_t*>(&_data); }
const int32_t& Int32x4_private::operator[](size_t i) const { return *reinterpret_cast<const int32_t*>(&_data); }

void Int32x4_private::operator+=(const Int32x4_private& b)
{
	_data = _mm_add_epi32(_data,b._data);
}
void Int32x4_private::operator-=(const Int32x4_private& b)
{
	_data = _mm_sub_epi32(_data,b._data);
}
void Int32x4_private::operator*=(const Int32x4_private& b)
{
	// Multiplication is not supported on SIMD!
	at(0) *= b[0];
	at(1) *= b[1];
	at(2) *= b[2];
	at(3) *= b[3];
}
void Int32x4_private::operator/=(const Int32x4_private& b)
{
	// Division is not supported on SIMD!
	at(0) /= b[0];
	at(1) /= b[1];
	at(2) /= b[2];
	at(3) /= b[3];
}
void Int32x4_private::operator<<=(const Int32x4_private& b)
{
	_data = _mm_min_epi32(_data,b._data);
} // Minimum
void Int32x4_private::operator>>=(const Int32x4_private& b)
{
	_data = _mm_max_epi32(_data,b._data);
} // Maximum
Int32x4_private Int32x4_private::operator+(const Int32x4_private& b) const
{
	return Int32x4_private(_mm_add_epi32(_data,b._data));
}
Int32x4_private Int32x4_private::operator-(const Int32x4_private& b) const
{
	return Int32x4_private(_mm_sub_epi32(_data,b._data));
}
Int32x4_private Int32x4_private::operator*(const Int32x4_private& b) const
{
	Int32x4_private tmp(*this);
	tmp *= b;
	return tmp;
}
Int32x4_private Int32x4_private::operator/(const Int32x4_private& b) const
{
	Int32x4_private tmp(*this);
	tmp /= b;
	return tmp;
}
bool Int32x4_private::operator==(const Int32x4_private& b) const
{
	__m128i tmp = _mm_cmpeq_epi32(_data,b._data);
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}
bool Int32x4_private::operator!=(const Int32x4_private& b) const
{
	return !(*this == b);
}
bool Int32x4_private::operator>(const Int32x4_private& b) const
{
	__m128i tmp = _mm_cmpgt_epi32(_data,b._data);
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}
bool Int32x4_private::operator>=(const Int32x4_private& b) const
{
	return (*this == b) || (*this > b);
}
bool Int32x4_private::operator<(const Int32x4_private& b) const
{
	__m128i tmp = _mm_cmplt_epi32(_data,b._data);
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}
bool Int32x4_private::operator<=(const Int32x4_private& b) const
{
	return (*this == b) || (*this < b);
}
Int32x4_private Int32x4_private::operator<<(const Int32x4_private& b) const
{
	return Int32x4_private(_mm_min_epi32(_data,b._data));
} // Minimum
Int32x4_private Int32x4_private::operator>>(const Int32x4_private& b) const
{
	return Int32x4_private(_mm_max_epi32(_data,b._data));
} // Maximum
Int32x4_private Int32x4_private::min(const Int32x4_private& a, const Int32x4_private& b)
{
	return Int32x4_private(_mm_min_epi32(a._data,b._data));
} // Minimum
Int32x4_private Int32x4_private::max(const Int32x4_private& a, const Int32x4_private& b)
{
	return Int32x4_private(_mm_max_epi32(a._data,b._data));
} // Maximum
Int32x4_private Int32x4_private::clamp(const Int32x4_private& a, const Int32x4_private& _min, const Int32x4_private& _max)
{
	return Int32x4_private(_mm_min_epi32(_mm_max_epi32(a._data,_min._data),_max._data));
} // Clamp
// Arithemetics with scalars
void Int32x4_private::operator+=(int32_t b)
{
	_data = _mm_add_epi32(_data,_mm_set1_epi32(b));
}
void Int32x4_private::operator-=(int32_t b)
{
	_data = _mm_sub_epi32(_data,_mm_set1_epi32(b));
}
void Int32x4_private::operator*=(int32_t b)
{
	// Intrinsics don't support multiplication
	at(0) *= b;
	at(1) *= b;
	at(2) *= b;
	at(3) *= b;
}
void Int32x4_private::operator/=(int32_t b)
{
	// Intrinsics don't support division
	at(0) /= b;
	at(1) /= b;
	at(2) /= b;
	at(3) /= b;
}
void Int32x4_private::operator<<=(int32_t b)
{
	_data = _mm_min_epi32(_data,_mm_set1_epi32(b));
} // Minimum
void Int32x4_private::operator>>=(int32_t b)
{
	_data = _mm_max_epi32(_data,_mm_set1_epi32(b));
} // Maximum
Int32x4_private Int32x4_private::operator+(int32_t b) const
{
	return Int32x4_private(_mm_add_epi32(_data,_mm_set1_epi32(b)));
}
Int32x4_private Int32x4_private::operator-(int32_t b) const
{
	return Int32x4_private(_mm_sub_epi32(_data,_mm_set1_epi32(b)));
}
Int32x4_private Int32x4_private::operator*(int32_t b) const
{
	Int32x4_private tmp(*this);
	tmp *= b;
	return tmp;
}
Int32x4_private Int32x4_private::operator/(int32_t b) const
{
	Int32x4_private tmp(*this);
	tmp /= b;
	return tmp;
}
Int32x4_private Int32x4_private::operator<<(int32_t b)
{
	return Int32x4_private(_mm_min_epi32(_data,_mm_set1_epi32(b)));
} // Minimum
Int32x4_private Int32x4_private::operator>>(int32_t b)
{
	return Int32x4_private(_mm_max_epi32(_data,_mm_set1_epi32(b)));
}

Int32x4_private Int32x4_private::min(const Int32x4_private &a, int32_t b)
{
	return min(a,Int32x4_private(b));
}

Int32x4_private Int32x4_private::max(const Int32x4_private &a, int32_t b)
{
	return max(a,Int32x4_private(b));
}

Int32x4_private Int32x4_private::clamp(const Int32x4_private &a, int32_t _min, int32_t _max)
{
	return clamp(a,Int32x4_private(_min),Int32x4_private(_max));
} // Maximum
bool Int32x4_private::operator==(int32_t b) const
{
	__m128i tmp = _mm_cmpeq_epi32(_data,_mm_set1_epi32(b));
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}
bool Int32x4_private::operator!=(int32_t b) const
{
	return !(*this == b);
}
bool Int32x4_private::operator>(int32_t b) const
{
	__m128i tmp = _mm_cmpgt_epi32(_data,_mm_set1_epi32(b));
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}
bool Int32x4_private::operator>=(int32_t b) const
{
	return (*this == b) || (*this > b);
}
bool Int32x4_private::operator<(int32_t b) const
{
	__m128i tmp = _mm_cmplt_epi32(_data,_mm_set1_epi32(b));
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}
bool Int32x4_private::operator<=(int32_t b) const
{
	return (*this == b) || (*this < b);
}
// Other mathematical goodies
int32_t Int32x4_private::sum() const
{
	__m128i result = _mm_hadd_epi32(_data, _data);
	result = _mm_hadd_epi32(result, result);
	return _mm_cvtsi128_si32(result);
}


}
}
#endif
