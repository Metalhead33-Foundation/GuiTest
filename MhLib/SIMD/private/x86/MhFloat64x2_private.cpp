#ifdef __AVX512VL__
#include "MhFloat64x2_private.hpp"
#include <cstdint>
#include <algorithm>
#include <smmintrin.h>
#include <emmintrin.h>
#include <immintrin.h>

namespace MH33 {
namespace SIMD {

double storeVectorialSumAs( const __m128d& valueA, const __m128d& valueB)
{
	__m128d result;
	result = _mm_mul_pd(valueA,valueB);
	result = _mm_hadd_pd(result,result);
	result = _mm_hadd_pd(result,result);
	return *reinterpret_cast<double*>(&result);
}

Float64x2_private::Float64x2_private()
	: _data(_mm_setzero_pd())
{

}

Float64x2_private::Float64x2_private(const Float64x2_private::ArrayType &a)
	: _data(_mm_load_pd(a.data()))
{

}

Float64x2_private::Float64x2_private(const __m128d &a)
	: _data(a)
{

}

Float64x2_private::Float64x2_private(double a)
	: _data(_mm_set1_pd(a))
{

}

Float64x2_private::Float64x2_private(const double *arr)
	: _data(_mm_load_pd(arr))
{

}

Float64x2_private::Float64x2_private(double e0, double e1)
	: _data(_mm_set_pd(e1,e0))
{

}

Float64x2_private::Float64x2_private(const Float64x2_private &cpy)
	: _data(cpy._data)
{

}

void Float64x2_private::operator=(const Float64x2_private &cpy)
{
	_data = cpy._data;
}

void Float64x2_private::operator=(double a)
{
	_data = _mm_set1_pd(a);
}

void Float64x2_private::operator=(const double *arr)
{
	_data = _mm_load_pd(arr);
}

void Float64x2_private::operator=(Float64x2_private::InitializerList a)
{
	std::copy(a.begin(),a.end(),begin());
}

Float64x2_private::Float64x2_private(Float64x2_private::InitializerList a)
	: _data(_mm_setzero_pd())
{
	std::copy(a.begin(),a.end(),begin());
}

void Float64x2_private::operator=(const Float64x2_private::ArrayType &arr)
{
	_data = _mm_load_pd(arr.data());
}

void Float64x2_private::store(double *arr) const
{
	_mm_store_pd(arr,_data);
}

Float64x2_private Float64x2_private::reciprocal() const
{
	Float64x2_private tmp = *this;
	tmp[0] = 1.0 / tmp[0];
	tmp[1] = 1.0 / tmp[1];
	return tmp;
}

void Float64x2_private::set_to_reciprocal()
{
	(*this)[0] = 1.0 / (*this)[0];
	(*this)[1] = 1.0 / (*this)[1];
}

Float64x2_private::ArrayType &Float64x2_private::asArray()
{
	return *reinterpret_cast<ArrayType*>(&_data);
}

const Float64x2_private::ArrayType &Float64x2_private::asArray() const
{
	return *reinterpret_cast<const ArrayType*>(&_data);
}

double *Float64x2_private::data()
{
	return reinterpret_cast<double*>(&_data);
}

const double *Float64x2_private::data() const
{
	return reinterpret_cast<const double*>(&_data);
}

size_t Float64x2_private::size() const
{
	return 4;
}
Float64x2_private::iterator Float64x2_private::begin()
{
	return reinterpret_cast<double*>(&_data);
}
Float64x2_private::iterator Float64x2_private::end()
{
	return &reinterpret_cast<double*>(&_data)[4];
}
Float64x2_private::const_iterator Float64x2_private::begin() const
{
	return reinterpret_cast<const double*>(&_data);
}
Float64x2_private::const_iterator Float64x2_private::end() const
{
	return &reinterpret_cast<const double*>(&_data)[4];
}
double& Float64x2_private::at(size_t i)
{
	return reinterpret_cast<double*>(&_data)[i];
}
const double& Float64x2_private::at(size_t i) const
{
	return reinterpret_cast<const double*>(&_data)[i];
}
double& Float64x2_private::operator[](size_t i)
{
	return reinterpret_cast<double*>(&_data)[i];
}
const double& Float64x2_private::operator[](size_t i) const
{
	return reinterpret_cast<const double*>(&_data)[i];
}

void Float64x2_private::operator+=(const Float64x2_private& b)
{
	_data = _mm_add_pd(_data,b._data);
}
void Float64x2_private::operator-=(const Float64x2_private& b)
{
	_data = _mm_sub_pd(_data,b._data);
}
void Float64x2_private::operator*=(const Float64x2_private& b)
{
	_data = _mm_mul_pd(_data,b._data);
}
void Float64x2_private::operator/=(const Float64x2_private& b)
{
	_data = _mm_div_pd(_data,b._data);
}

Float64x2_private Float64x2_private::operator+(const Float64x2_private& b) const
{
	return Float64x2_private(_mm_add_pd(_data,b._data));
}
Float64x2_private Float64x2_private::operator-(const Float64x2_private& b) const
{
	return Float64x2_private(_mm_sub_pd(_data,b._data));
}
Float64x2_private Float64x2_private::operator*(const Float64x2_private& b) const
{
	return Float64x2_private(_mm_mul_pd(_data,b._data));
}
Float64x2_private Float64x2_private::operator/(const Float64x2_private& b) const
{
	return Float64x2_private(_mm_div_pd(_data,b._data));
}
void Float64x2_private::operator<<=(const Float64x2_private& b) // Minimum
{
	_data = _mm_min_pd(_data,b._data);
}
void Float64x2_private::operator>>=(const Float64x2_private& b) // Maximum
{
	_data = _mm_max_sd(_data,b._data);
}
Float64x2_private Float64x2_private::operator<<(const Float64x2_private& b) const // Minimum
{
	return Float64x2_private(_mm_min_pd(_data,b._data));
}
Float64x2_private Float64x2_private::operator>>(const Float64x2_private& b) const // Maximum
{
	return Float64x2_private(_mm_max_pd(_data,b._data));
}
Float64x2_private Float64x2_private::min(const Float64x2_private& a, const Float64x2_private& b)
{
	return Float64x2_private(_mm_min_pd(a._data,b._data));
}
Float64x2_private Float64x2_private::max(const Float64x2_private& a, const Float64x2_private& b)
{
	return Float64x2_private(_mm_max_pd(a._data,b._data));
}
Float64x2_private Float64x2_private::clamp(const Float64x2_private& a, const Float64x2_private& _min, const Float64x2_private& _max)
{
	return Float64x2_private(_mm_min_pd(_mm_max_pd(a._data,_min._data),_max._data));
}

double Float64x2_private::vectorialMul(const Float64x2_private& b) const
{
	return storeVectorialSumAs(_data,b._data);
}
double Float64x2_private::vectorialMul(double b) const
{
	return storeVectorialSumAs(_data,_mm_set1_pd(b));
}
Float64x2_private Float32x4_private::min(const Float64x2_private &a, float b)
{
	return min(a,Float64x2_private(b));
}

Float64x2_private Float64x2_private::max(const Float64x2_private &a, float b)
{
	return max(a,Float64x2_private(b));
}

Float64x2_private Float64x2_private::clamp(const Float64x2_private &a, float _min, float _max)
{
	return clamp(a,Float64x2_private(_min),Float64x2_private(_max));
}
void Float64x2_private::operator+=(double b)
{
	_data = _mm_add_pd(_data,_mm_set1_pd(b));
}
void Float64x2_private::operator-=(double b)
{
	_data = _mm_sub_pd(_data,_mm_set1_pd(b));
}
void Float64x2_private::operator*=(double b)
{
	_data = _mm_mul_pd(_data,_mm_set1_pd(b));
}
void Float64x2_private::operator/=(double b)
{
	_data = _mm_div_pd(_data,_mm_set1_pd(b));
}

Float64x2_private Float64x2_private::operator+(double b) const
{
	return Float64x2_private(_mm_add_pd(_data,_mm_set1_pd(b)));
}
Float64x2_private Float64x2_private::operator-(double b) const
{
	return Float64x2_private(_mm_sub_pd(_data,_mm_set1_pd(b)));
}
Float64x2_private Float64x2_private::operator*(double b) const
{
	return Float64x2_private(_mm_mul_pd(_data,_mm_set1_pd(b)));
}
Float64x2_private Float64x2_private::operator/(double b) const
{
	return Float64x2_private(_mm_div_pd(_data,_mm_set1_pd(b)));
}
void Float64x2_private::operator<<=(double b) // Minimum
{
	_data = _mm_min_pd(_data,_mm_set1_pd(b));
}
void Float64x2_private::operator>>=(double b) // Maximum
{
	_data = _mm_max_pd(_data,_mm_set1_pd(b));
}
Float64x2_private Float64x2_private::operator<<(double b) // Minimum
{
	return Float64x2_private(_mm_min_pd(_data,_mm_set1_pd(b)));
}
Float64x2_private Float64x2_private::operator>>(double b) // Maximum
{
	return Float64x2_private(_mm_max_pd(_data,_mm_set1_pd(b)));
}

bool Float64x2_private::operator==(const Float64x2_private& b) const
{
	__m128d tmp = _mm_cmpeq_pd(_data,b._data);
	return reinterpret_cast<uint64_t*>(&tmp)[0] && reinterpret_cast<uint64_t*>(&tmp)[1];
}
bool Float64x2_private::operator!=(const Float64x2_private& b) const
{
	__m128d tmp = _mm_cmpneq_pd(_data,b._data);
	return reinterpret_cast<uint64_t*>(&tmp)[0] && reinterpret_cast<uint64_t*>(&tmp)[1];
}
bool Float64x2_private::operator>(const Float64x2_private& b) const
{
	__m128d tmp = _mm_cmpgt_pd(_data,b._data);
	return reinterpret_cast<uint64_t*>(&tmp)[0] && reinterpret_cast<uint64_t*>(&tmp)[1];
}
bool Float64x2_private::operator>=(const Float64x2_private& b) const
{
	__m128d tmp = _mm_cmpge_pd(_data,b._data);
	return reinterpret_cast<uint64_t*>(&tmp)[0] && reinterpret_cast<uint64_t*>(&tmp)[1];
}
bool Float64x2_private::operator<(const Float64x2_private& b) const
{
	__m128d tmp = _mm_cmplt_pd(_data,b._data);
	return reinterpret_cast<uint64_t*>(&tmp)[0] && reinterpret_cast<uint64_t*>(&tmp)[1];
}
bool Float64x2_private::operator<=(const Float64x2_private& b) const
{
	__m128d tmp = _mm_cmple_pd(_data,b._data);
	return reinterpret_cast<uint64_t*>(&tmp)[0] && reinterpret_cast<uint64_t*>(&tmp)[1];
}
bool Float64x2_private::operator==(double b) const
{
	__m128d tmp = _mm_cmpeq_pd(_data,_mm_set1_pd(b));
	return reinterpret_cast<uint64_t*>(&tmp)[0] && reinterpret_cast<uint64_t*>(&tmp)[1];
}
bool Float64x2_private::operator!=(double b) const
{
	__m128d tmp = _mm_cmpneq_pd(_data,_mm_set1_pd(b));
	return reinterpret_cast<uint64_t*>(&tmp)[0] && reinterpret_cast<uint64_t*>(&tmp)[1];
}
bool Float64x2_private::operator>(double b) const
{
	__m128d tmp = _mm_cmpgt_pd(_data,_mm_set1_pd(b));
	return reinterpret_cast<uint64_t*>(&tmp)[0] && reinterpret_cast<uint64_t*>(&tmp)[1];
}
bool Float64x2_private::operator>=(double b) const
{
	__m128d tmp = _mm_cmpge_pd(_data,_mm_set1_pd(b));
	return reinterpret_cast<uint64_t*>(&tmp)[0] && reinterpret_cast<uint64_t*>(&tmp)[1];
}
bool Float64x2_private::operator<(double b) const
{
	__m128d tmp = _mm_cmplt_pd(_data,_mm_set1_pd(b));
	return reinterpret_cast<uint64_t*>(&tmp)[0] && reinterpret_cast<uint64_t*>(&tmp)[1];
}
bool Float64x2_private::operator<=(double b) const
{
	__m128d tmp = _mm_cmple_pd(_data,_mm_set1_pd(b));
	return reinterpret_cast<uint64_t*>(&tmp)[0] && reinterpret_cast<uint64_t*>(&tmp)[1];
}

Float64x2_private Float64x2_private::sqrt() const
{
	return Float64x2_private(_mm_sqrt_pd(_data));
}
void Float64x2_private::set_to_sqrt()
{
	_data = _mm_sqrt_pd(_data);
}
Float64x2_private Float64x2_private::rsqrt() const
{
	return Float64x2_private(_mm_mask_rsqrt14_pd(_data,0xFF,_data));
}
void Float64x2_private::set_to_rsqrt()
{
	_data = _mm_mask_rsqrt14_pd(_data,0xFF,_data);
}
Float64x2_private Float64x2_private::normalize() const
{
	double maximum = std::min(at(0),std::min(at(1),std::min(at(2),at(3))));
	return *this / maximum;
}
void Float64x2_private::set_to_norm()
{
	double maximum = std::min(at(0),std::min(at(1),std::min(at(2),at(3))));
	*this /= maximum;
}

double Float64x2_private::sum() const
{
	__m128d tmp = _mm_hadd_pd(_data,_data);
	tmp = _mm_hadd_pd(tmp,tmp);
	return reinterpret_cast<double*>(&tmp)[0];
}

}
}
#endif
