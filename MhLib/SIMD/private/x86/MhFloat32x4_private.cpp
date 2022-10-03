#ifdef __SSE4_1__
#include "MhFloat32x4_private.hpp"
#include <cstdint>
#include <algorithm>
#include <smmintrin.h>
#define FAST_DIVISION

namespace MH33 {
namespace SIMD {

float storeVectorialSumAs( const __m128& valueA, const __m128& valueB)
{
	__m128 result;
	result = _mm_mul_ps(valueA,valueB);
	result = _mm_hadd_ps(result,result);
	result = _mm_hadd_ps(result,result);
	return *reinterpret_cast<float*>(&result);
}

Float32x4_private::Float32x4_private()
	: _data(_mm_setzero_ps())
{

}

Float32x4_private::Float32x4_private(Float32x4_private::InitializerList a)
	: _data(_mm_setzero_ps())
{
	std::copy(a.begin(),a.end(),begin());
}

Float32x4_private::Float32x4_private(const Float32x4_private::ArrayType &a)
	: _data(_mm_load_ps(a.data()))
{

}

Float32x4_private::Float32x4_private(const __m128 &a)
	: _data(a)
{

}

Float32x4_private::Float32x4_private(float a)
	: _data(_mm_set1_ps(a))
{

}

Float32x4_private::Float32x4_private(const float *arr)
	: _data(_mm_load_ps(arr))
{

}

Float32x4_private::Float32x4_private(float e0, float e1, float e2, float e3)
	: _data(_mm_set_ps(e3,e2,e1,e0))
{

}

Float32x4_private::Float32x4_private(const Float32x4_private &cpy)
	: _data(cpy._data)
{

}

void Float32x4_private::operator=(const Float32x4_private &cpy)
{
	_data = cpy._data;
}

void Float32x4_private::operator=(Float32x4_private::InitializerList a)
{
	std::copy(a.begin(),a.end(),begin());
}

void Float32x4_private::operator=(float a)
{
	_data = _mm_set1_ps(a);
}

void Float32x4_private::operator=(const float *arr)
{
	_data = _mm_load_ps(arr);
}

void Float32x4_private::operator=(const Float32x4_private::ArrayType &arr)
{
	_data = _mm_load_ps(arr.data());
}

void Float32x4_private::store(float *arr) const
{
	_mm_store_ps(arr,_data);
}

Float32x4_private Float32x4_private::reciprocal() const
{
	return Float32x4_private(_mm_rcp_ps(_data));
}

void Float32x4_private::set_to_reciprocal()
{
	_data = _mm_rcp_ps(_data);
}

Float32x4_private::ArrayType &Float32x4_private::asArray()
{
	return *reinterpret_cast<ArrayType*>(&_data);
}

const Float32x4_private::ArrayType &Float32x4_private::asArray() const
{
	return *reinterpret_cast<const ArrayType*>(&_data);
}

float *Float32x4_private::data()
{
	return reinterpret_cast<float*>(&_data);
}

const float *Float32x4_private::data() const
{
	return reinterpret_cast<const float*>(&_data);
}

size_t Float32x4_private::size() const
{
	return 4;
}
Float32x4_private::iterator Float32x4_private::begin()
{
	return reinterpret_cast<float*>(&_data);
}
Float32x4_private::iterator Float32x4_private::end()
{
	return &reinterpret_cast<float*>(&_data)[4];
}
Float32x4_private::const_iterator Float32x4_private::begin() const
{
	return reinterpret_cast<const float*>(&_data);
}
Float32x4_private::const_iterator Float32x4_private::end() const
{
	return &reinterpret_cast<const float*>(&_data)[4];
}
float& Float32x4_private::at(size_t i)
{
	return reinterpret_cast<float*>(&_data)[i];
}
const float& Float32x4_private::at(size_t i) const
{
	return reinterpret_cast<const float*>(&_data)[i];
}
float& Float32x4_private::operator[](size_t i)
{
	return reinterpret_cast<float*>(&_data)[i];
}
const float& Float32x4_private::operator[](size_t i) const
{
	return reinterpret_cast<const float*>(&_data)[i];
}

void Float32x4_private::operator+=(const Float32x4_private& b)
{
	_data = _mm_add_ps(_data,b._data);
}
void Float32x4_private::operator-=(const Float32x4_private& b)
{
	_data = _mm_sub_ps(_data,b._data);
}
void Float32x4_private::operator*=(const Float32x4_private& b)
{
	_data = _mm_mul_ps(_data,b._data);
}
void Float32x4_private::operator/=(const Float32x4_private& b)
{
#ifdef FAST_DIVISION
	_data = _mm_mul_ps(_data,_mm_rcp_ps(b._data));
#else
	_data = _mm_div_ps(_data,b._data);
#endif
}

Float32x4_private Float32x4_private::operator+(const Float32x4_private& b) const
{
	return Float32x4_private(_mm_add_ps(_data,b._data));
}
Float32x4_private Float32x4_private::operator-(const Float32x4_private& b) const
{
	return Float32x4_private(_mm_sub_ps(_data,b._data));
}
Float32x4_private Float32x4_private::operator*(const Float32x4_private& b) const
{
	return Float32x4_private(_mm_mul_ps(_data,b._data));
}
Float32x4_private Float32x4_private::operator/(const Float32x4_private& b) const
{
#ifdef FAST_DIVISION
	return Float32x4_private(_mm_mul_ps(_data,_mm_rcp_ps(b._data)));
#else
	return SimdFvec4(_mm_div_ps(_data,b._data));
#endif
}
void Float32x4_private::operator<<=(const Float32x4_private& b) // Minimum
{
	_data = _mm_min_ps(_data,b._data);
}
void Float32x4_private::operator>>=(const Float32x4_private& b) // Maximum
{
	_data = _mm_max_ss(_data,b._data);
}
Float32x4_private Float32x4_private::operator<<(const Float32x4_private& b) // Minimum
{
	return Float32x4_private(_mm_min_ps(_data,b._data));
}
Float32x4_private Float32x4_private::operator>>(const Float32x4_private& b) // Maximum
{
	return Float32x4_private(_mm_max_ps(_data,b._data));
}
Float32x4_private Float32x4_private::min(const Float32x4_private& a, const Float32x4_private& b)
{
	return Float32x4_private(_mm_min_ps(a._data,b._data));
}
Float32x4_private Float32x4_private::max(const Float32x4_private& a, const Float32x4_private& b)
{
	return Float32x4_private(_mm_max_ps(a._data,b._data));
}
Float32x4_private Float32x4_private::clamp(const Float32x4_private& a, const Float32x4_private& _min, const Float32x4_private& _max)
{
	return Float32x4_private(_mm_min_ps(_mm_max_ps(a._data,_min._data),_max._data));
}

float Float32x4_private::vectorialMul(const Float32x4_private& b) const
{
	return storeVectorialSumAs(_data,b._data);
}
float Float32x4_private::vectorialMul(float b) const
{
	return storeVectorialSumAs(_data,_mm_set1_ps(b));
}

void Float32x4_private::operator+=(float b)
{
	_data = _mm_add_ps(_data,_mm_set1_ps(b));
}
void Float32x4_private::operator-=(float b)
{
	_data = _mm_sub_ps(_data,_mm_set1_ps(b));
}
void Float32x4_private::operator*=(float b)
{
	_data = _mm_mul_ps(_data,_mm_set1_ps(b));
}
void Float32x4_private::operator/=(float b)
{
#ifdef FAST_DIVISION
	_data = _mm_mul_ps(_data,_mm_rcp_ps(_mm_set1_ps(b)));
#else
	_data = _mm_div_ps(_data,_mm_set1_ps(b));
#endif
}

Float32x4_private Float32x4_private::operator+(float b) const
{
	return Float32x4_private(_mm_add_ps(_data,_mm_set1_ps(b)));
}
Float32x4_private Float32x4_private::operator-(float b) const
{
	return Float32x4_private(_mm_sub_ps(_data,_mm_set1_ps(b)));
}
Float32x4_private Float32x4_private::operator*(float b) const
{
	return Float32x4_private(_mm_mul_ps(_data,_mm_set1_ps(b)));
}
Float32x4_private Float32x4_private::operator/(float b) const
{
#ifdef FAST_DIVISION
	return Float32x4_private(_mm_mul_ps(_data,_mm_rcp_ps(_mm_set1_ps(b))));
#else
	return SimdFvec4(_mm_div_ps(_data,_mm_set1_ps(b)));
#endif
}
void Float32x4_private::operator<<=(float b) // Minimum
{
	_data = _mm_min_ps(_data,_mm_set1_ps(b));
}
void Float32x4_private::operator>>=(float b) // Maximum
{
	_data = _mm_max_ps(_data,_mm_set1_ps(b));
}
Float32x4_private Float32x4_private::operator<<(float b) const // Minimum
{
	return Float32x4_private(_mm_min_ps(_data,_mm_set1_ps(b)));
}
Float32x4_private Float32x4_private::operator>>(float b) const // Maximum
{
	return Float32x4_private(_mm_max_ps(_data,_mm_set1_ps(b)));
}

Float32x4_private Float32x4_private::min(const Float32x4_private &a, float b)
{
	return min(a,Float32x4_private(b));
}

Float32x4_private Float32x4_private::max(const Float32x4_private &a, float b)
{
	return max(a,Float32x4_private(b));
}

Float32x4_private Float32x4_private::clamp(const Float32x4_private &a, float _min, float _max)
{
	return clamp(a,Float32x4_private(_min),Float32x4_private(_max));
}

bool Float32x4_private::operator==(const Float32x4_private& b) const
{
	__m128 tmp = _mm_cmpeq_ps(_data,b._data);
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}
bool Float32x4_private::operator!=(const Float32x4_private& b) const
{
	__m128 tmp = _mm_cmpneq_ps(_data,b._data);
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}
bool Float32x4_private::operator>(const Float32x4_private& b) const
{
	__m128 tmp = _mm_cmpgt_ps(_data,b._data);
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}
bool Float32x4_private::operator>=(const Float32x4_private& b) const
{
	__m128 tmp = _mm_cmpge_ps(_data,b._data);
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}
bool Float32x4_private::operator<(const Float32x4_private& b) const
{
	__m128 tmp = _mm_cmplt_ps(_data,b._data);
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}
bool Float32x4_private::operator<=(const Float32x4_private& b) const
{
	__m128 tmp = _mm_cmple_ps(_data,b._data);
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}
bool Float32x4_private::operator==(float b) const
{
	__m128 tmp = _mm_cmpeq_ps(_data,_mm_set1_ps(b));
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}
bool Float32x4_private::operator!=(float b) const
{
	__m128 tmp = _mm_cmpneq_ps(_data,_mm_set1_ps(b));
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}
bool Float32x4_private::operator>(float b) const
{
	__m128 tmp = _mm_cmpgt_ps(_data,_mm_set1_ps(b));
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}
bool Float32x4_private::operator>=(float b) const
{
	__m128 tmp = _mm_cmpge_ps(_data,_mm_set1_ps(b));
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}
bool Float32x4_private::operator<(float b) const
{
	__m128 tmp = _mm_cmplt_ps(_data,_mm_set1_ps(b));
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}
bool Float32x4_private::operator<=(float b) const
{
	__m128 tmp = _mm_cmple_ps(_data,_mm_set1_ps(b));
	return reinterpret_cast<uint32_t*>(&tmp)[0] && reinterpret_cast<uint32_t*>(&tmp)[1] &&
		   reinterpret_cast<uint32_t*>(&tmp)[2] && reinterpret_cast<uint32_t*>(&tmp)[3];
}

Float32x4_private Float32x4_private::sqrt() const
{
	return Float32x4_private(_mm_sqrt_ps(_data));
}
void Float32x4_private::set_to_sqrt()
{
	_data = _mm_sqrt_ps(_data);
}
Float32x4_private Float32x4_private::rsqrt() const
{
	return Float32x4_private(_mm_rsqrt_ps(_data));
}
void Float32x4_private::set_to_rsqrt()
{
	_data = _mm_rsqrt_ps(_data);
}
Float32x4_private Float32x4_private::normalize() const
{
	float maximum = std::min(at(0),std::min(at(1),std::min(at(2),at(3))));
	return *this / maximum;
}
void Float32x4_private::set_to_norm()
{
	float maximum = std::min(at(0),std::min(at(1),std::min(at(2),at(3))));
	*this /= maximum;
}

float Float32x4_private::sum() const
{
	__m128 tmp = _mm_hadd_ps(_data,_data);
	tmp = _mm_hadd_ps(tmp,tmp);
	return reinterpret_cast<float*>(&tmp)[0];
}

}
}
#endif
