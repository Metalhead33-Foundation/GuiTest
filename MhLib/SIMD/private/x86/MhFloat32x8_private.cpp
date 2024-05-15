#ifdef __AVX__
#include "MhFloat32x8_private.hpp"
#include <cstdint>
#include <algorithm>
#include <smmintrin.h>
#define FAST_DIVISION

namespace MH33 {
namespace SIMD {

float storeVectorialSumAs( const __m256& valueA, const __m256& valueB)
{
	__m256 result;
	result = _mm256_mul_ps(valueA,valueB);
	result = _mm256_hadd_ps(result,result);
	result = _mm256_hadd_ps(result,result);
	return *reinterpret_cast<float*>(&result);
}

Float32x8_private::Float32x8_private()
	: _data(_mm256_setzero_ps())
{

}

Float32x8_private::Float32x8_private(Float32x8_private::InitializerList a)
	: _data(_mm256_setzero_ps())
{
	std::copy(a.begin(),a.end(),begin());
}

Float32x8_private::Float32x8_private(const Float32x8_private::ArrayType &a)
	: _data(_mm256_load_ps(a.data()))
{

}

Float32x8_private::Float32x8_private(const __m256 &a)
	: _data(a)
{

}

Float32x8_private::Float32x8_private(float a)
	: _data(_mm256_set1_ps(a))
{

}

Float32x8_private::Float32x8_private(const float *arr)
	: _data(_mm256_load_ps(arr))
{

}

Float32x8_private::Float32x8_private(float e0, float e1, float e2, float e3, float e4, float e5, float e6, float e7)
	: _data(_mm256_set_ps(e7,e6,e5,e4,e3,e2,e1,e0))
{

}

Float32x8_private::Float32x8_private(const Float32x8_private &cpy)
	: _data(cpy._data)
{

}

void Float32x8_private::operator=(const Float32x8_private &cpy)
{
	_data = cpy._data;
}

void Float32x8_private::operator=(Float32x8_private::InitializerList a)
{
	std::copy(a.begin(),a.end(),begin());
}

void Float32x8_private::operator=(float a)
{
	_data = _mm256_set1_ps(a);
}

void Float32x8_private::operator=(const float *arr)
{
	_data = _mm256_load_ps(arr);
}

void Float32x8_private::operator=(const Float32x8_private::ArrayType &arr)
{
	_data = _mm256_load_ps(arr.data());
}

void Float32x8_private::store(float *arr) const
{
	_mm256_store_ps(arr,_data);
}

Float32x8_private Float32x8_private::reciprocal() const
{
	return Float32x8_private(_mm256_rcp_ps(_data));
}

void Float32x8_private::set_to_reciprocal()
{
	_data = _mm256_rcp_ps(_data);
}

Float32x8_private::ArrayType &Float32x8_private::asArray()
{
	return *reinterpret_cast<ArrayType*>(&_data);
}

const Float32x8_private::ArrayType &Float32x8_private::asArray() const
{
	return *reinterpret_cast<const ArrayType*>(&_data);
}

float *Float32x8_private::data()
{
	return reinterpret_cast<float*>(&_data);
}

const float *Float32x8_private::data() const
{
	return reinterpret_cast<const float*>(&_data);
}

size_t Float32x8_private::size() const
{
	return 8;
}
Float32x8_private::iterator Float32x8_private::begin()
{
	return reinterpret_cast<float*>(&_data);
}
Float32x8_private::iterator Float32x8_private::end()
{
	return &reinterpret_cast<float*>(&_data)[8];
}
Float32x8_private::const_iterator Float32x8_private::begin() const
{
	return reinterpret_cast<const float*>(&_data);
}
Float32x8_private::const_iterator Float32x8_private::end() const
{
	return &reinterpret_cast<const float*>(&_data)[8];
}
float& Float32x8_private::at(size_t i)
{
	return reinterpret_cast<float*>(&_data)[i];
}
const float& Float32x8_private::at(size_t i) const
{
	return reinterpret_cast<const float*>(&_data)[i];
}
float& Float32x8_private::operator[](size_t i)
{
	return reinterpret_cast<float*>(&_data)[i];
}
const float& Float32x8_private::operator[](size_t i) const
{
	return reinterpret_cast<const float*>(&_data)[i];
}

void Float32x8_private::operator+=(const Float32x8_private& b)
{
	_data = _mm256_add_ps(_data,b._data);
}
void Float32x8_private::operator-=(const Float32x8_private& b)
{
	_data = _mm256_sub_ps(_data,b._data);
}
void Float32x8_private::operator*=(const Float32x8_private& b)
{
	_data = _mm256_mul_ps(_data,b._data);
}
void Float32x8_private::operator/=(const Float32x8_private& b)
{
#ifdef FAST_DIVISION
	_data = _mm256_mul_ps(_data,_mm256_rcp_ps(b._data));
#else
	_data = _mm256_div_ps(_data,b._data);
#endif
}

Float32x8_private Float32x8_private::operator+(const Float32x8_private& b) const
{
	return Float32x8_private(_mm256_add_ps(_data,b._data));
}
Float32x8_private Float32x8_private::operator-(const Float32x8_private& b) const
{
	return Float32x8_private(_mm256_sub_ps(_data,b._data));
}
Float32x8_private Float32x8_private::operator*(const Float32x8_private& b) const
{
	return Float32x8_private(_mm256_mul_ps(_data,b._data));
}
Float32x8_private Float32x8_private::operator/(const Float32x8_private& b) const
{
#ifdef FAST_DIVISION
	return Float32x8_private(_mm256_mul_ps(_data,_mm256_rcp_ps(b._data)));
#else
	return SimdFvec8(_mm256_div_ps(_data,b._data));
#endif
}
void Float32x8_private::operator<<=(const Float32x8_private& b) // Minimum
{
	_data = _mm256_min_ps(_data,b._data);
}
void Float32x8_private::operator>>=(const Float32x8_private& b) // Maximum
{
	_data = _mm256_max_ps(_data,b._data);
}
Float32x8_private Float32x8_private::operator<<(const Float32x8_private& b) // Minimum
{
	return Float32x8_private(_mm256_min_ps(_data,b._data));
}
Float32x8_private Float32x8_private::operator>>(const Float32x8_private& b) // Maximum
{
	return Float32x8_private(_mm256_max_ps(_data,b._data));
}
Float32x8_private Float32x8_private::min(const Float32x8_private& a, const Float32x8_private& b)
{
	return Float32x8_private(_mm256_min_ps(a._data,b._data));
}
Float32x8_private Float32x8_private::max(const Float32x8_private& a, const Float32x8_private& b)
{
	return Float32x8_private(_mm256_max_ps(a._data,b._data));
}
Float32x8_private Float32x8_private::clamp(const Float32x8_private& a, const Float32x8_private& _min, const Float32x8_private& _max)
{
	return Float32x8_private(_mm256_min_ps(_mm256_max_ps(a._data,_min._data),_max._data));
}

float Float32x8_private::vectorialMul(const Float32x8_private& b) const
{
	return storeVectorialSumAs(_data,b._data);
}
float Float32x8_private::vectorialMul(float b) const
{
	return storeVectorialSumAs(_data,_mm256_set1_ps(b));
}

void Float32x8_private::operator+=(float b)
{
	_data = _mm256_add_ps(_data,_mm256_set1_ps(b));
}
void Float32x8_private::operator-=(float b)
{
	_data = _mm256_sub_ps(_data,_mm256_set1_ps(b));
}
void Float32x8_private::operator*=(float b)
{
	_data = _mm256_mul_ps(_data,_mm256_set1_ps(b));
}
void Float32x8_private::operator/=(float b)
{
#ifdef FAST_DIVISION
	_data = _mm256_mul_ps(_data,_mm256_rcp_ps(_mm256_set1_ps(b)));
#else
	_data = _mm256_div_ps(_data,_mm256_set1_ps(b));
#endif
}

Float32x8_private Float32x8_private::operator+(float b) const
{
	return Float32x8_private(_mm256_add_ps(_data,_mm256_set1_ps(b)));
}
Float32x8_private Float32x8_private::operator-(float b) const
{
	return Float32x8_private(_mm256_sub_ps(_data,_mm256_set1_ps(b)));
}
Float32x8_private Float32x8_private::operator*(float b) const
{
	return Float32x8_private(_mm256_mul_ps(_data,_mm256_set1_ps(b)));
}
Float32x8_private Float32x8_private::operator/(float b) const
{
#ifdef FAST_DIVISION
	return Float32x8_private(_mm256_mul_ps(_data,_mm256_rcp_ps(_mm256_set1_ps(b))));
#else
	return SimdFvec8(_mm256_div_ps(_data,_mm256_set1_ps(b)));
#endif
}
void Float32x8_private::operator<<=(float b) // Minimum
{
	_data = _mm256_min_ps(_data,_mm256_set1_ps(b));
}
void Float32x8_private::operator>>=(float b) // Maximum
{
	_data = _mm256_max_ps(_data,_mm256_set1_ps(b));
}

Float32x8_private Float32x8_private::min(const Float32x8_private &a, float b)
{
	return min(a,Float32x8_private(b));
}

Float32x8_private Float32x8_private::max(const Float32x8_private &a, float b)
{
	return max(a,Float32x8_private(b));
}

Float32x8_private Float32x8_private::clamp(const Float32x8_private &a, float _min, float _max)
{
	return clamp(a,Float32x8_private(_min),Float32x8_private(_max));
}
Float32x8_private Float32x8_private::operator<<(float b) const // Minimum
{
	return Float32x8_private(_mm256_min_ps(_data,_mm256_set1_ps(b)));
}
Float32x8_private Float32x8_private::operator>>(float b) const // Maximum
{
	return Float32x8_private(_mm256_max_ps(_data,_mm256_set1_ps(b)));
}

bool Float32x8_private::operator==(const Float32x8_private& b) const
{
	bool toReturn = false;
	for(int i = 0; i < 8; ++i) {
		toReturn = std::min(toReturn,(*this)[i] == b[i]);
	}
	return toReturn;
}
bool Float32x8_private::operator!=(const Float32x8_private& b) const
{
	bool toReturn = false;
	for(int i = 0; i < 8; ++i) {
		toReturn = std::min(toReturn,(*this)[i] != b[i]);
	}
	return toReturn;
}
bool Float32x8_private::operator>(const Float32x8_private& b) const
{
	bool toReturn = false;
	for(int i = 0; i < 8; ++i) {
		toReturn = std::min(toReturn,(*this)[i] > b[i]);
	}
	return toReturn;
}
bool Float32x8_private::operator>=(const Float32x8_private& b) const
{
	bool toReturn = false;
	for(int i = 0; i < 8; ++i) {
		toReturn = std::min(toReturn,(*this)[i] >= b[i]);
	}
	return toReturn;
}
bool Float32x8_private::operator<(const Float32x8_private& b) const
{
	bool toReturn = false;
	for(int i = 0; i < 8; ++i) {
		toReturn = std::min(toReturn,(*this)[i] < b[i]);
	}
	return toReturn;
}
bool Float32x8_private::operator<=(const Float32x8_private& b) const
{
	bool toReturn = false;
	for(int i = 0; i < 8; ++i) {
		toReturn = std::min(toReturn,(*this)[i] <= b[i]);
	}
	return toReturn;
}
bool Float32x8_private::operator==(float b) const
{
	bool toReturn = false;
	for(int i = 0; i < 8; ++i) {
		toReturn = std::min(toReturn,(*this)[i] == b);
	}
	return toReturn;
}
bool Float32x8_private::operator!=(float b) const
{
	bool toReturn = false;
	for(int i = 0; i < 8; ++i) {
		toReturn = std::min(toReturn,(*this)[i] != b);
	}
	return toReturn;
}
bool Float32x8_private::operator>(float b) const
{
	bool toReturn = false;
	for(int i = 0; i < 8; ++i) {
		toReturn = std::min(toReturn,(*this)[i] > b);
	}
	return toReturn;
}
bool Float32x8_private::operator>=(float b) const
{
	bool toReturn = false;
	for(int i = 0; i < 8; ++i) {
		toReturn = std::min(toReturn,(*this)[i] >= b);
	}
	return toReturn;
}
bool Float32x8_private::operator<(float b) const
{
	bool toReturn = false;
	for(int i = 0; i < 8; ++i) {
		toReturn = std::min(toReturn,(*this)[i] < b);
	}
	return toReturn;
}
bool Float32x8_private::operator<=(float b) const
{
	bool toReturn = false;
	for(int i = 0; i < 8; ++i) {
		toReturn = std::min(toReturn,(*this)[i] <= b);
	}
	return toReturn;
}

Float32x8_private Float32x8_private::sqrt() const
{
	return Float32x8_private(_mm256_sqrt_ps(_data));
}
void Float32x8_private::set_to_sqrt()
{
	_data = _mm256_sqrt_ps(_data);
}
Float32x8_private Float32x8_private::rsqrt() const
{
	return Float32x8_private(_mm256_rsqrt_ps(_data));
}
void Float32x8_private::set_to_rsqrt()
{
	_data = _mm256_rsqrt_ps(_data);
}
Float32x8_private Float32x8_private::normalize() const
{
	float maximum = std::min(at(0),std::min(at(1),std::min(at(2),at(3))));
	return *this / maximum;
}
void Float32x8_private::set_to_norm()
{
	float maximum = std::min(at(0),std::min(at(1),std::min(at(2),at(3))));
	*this /= maximum;
}

float Float32x8_private::sum() const
{
	__m256 tmp = _mm256_hadd_ps(_data,_data);
	tmp = _mm256_hadd_ps(tmp,tmp);
	return reinterpret_cast<float*>(&tmp)[0];
}

}
}
#endif
