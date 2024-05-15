#ifndef MHFALLBACKVECTOR_HPP
#define MHFALLBACKVECTOR_HPP
#include <cstdint>
#include <array>
#include <initializer_list>
#include <cmath>
#include <numeric>

namespace MH33 {
namespace SIMD {

template <typename T, size_t C> class FallbackVector {
public:
	typedef std::array<T,4> ArrayType;
	typedef std::initializer_list<T> InitializerList;
	typedef typename ArrayType::iterator iterator;
	typedef typename ArrayType::const_iterator const_iterator;
private:
	ArrayType _data;
public:
	FallbackVector() {
		for(auto& it : _data) {
			it = 0;
		}
	}
	FallbackVector(const ArrayType& a) {
		std::copy(a.begin(),a.end(),_data.begin());
	}
	FallbackVector(T a) {
		for(auto& it : _data) {
			it = a;
		}
	}
	FallbackVector(InitializerList a) {
		for(auto& it : _data) {
			it = 0;
		}
		std::copy(a.begin(),a.end(),_data.begin());
	}
	FallbackVector(const T* arr) {
		for(size_t i = 0; i < _data.size(); ++i) {
			_data[i] = arr[i];
		}
	}
	FallbackVector(const FallbackVector& cpy) {
		std::copy(cpy._data.begin(),cpy._data.end(),_data.begin());
	}
	void operator=(const FallbackVector& cpy) {
		std::copy(cpy._data.begin(),cpy._data.end(),_data.begin());
	}
	void operator=(T a) {
		for(auto& it : _data) {
			it = a;
		}
	}
	void operator=(InitializerList a) {
		std::copy(a.begin(),a.end(),_data.begin());
	}
	void operator=(const T* arr) {
		for(size_t i = 0; i < _data.size(); ++i) {
			_data[i] = arr[i];
		}
	}
	void operator=(const ArrayType& arr) {
		std::copy(arr.begin(),arr.end(),_data.begin());
	}
	void store(T* arr) const {
		for(size_t i = 0; i < _data.size(); ++i) {
			arr[i] = _data[i];
		}
	}
	void set_to_reciprocal() {
		for(size_t i = 0; i < _data.size(); ++i) {
			_data[i] = T(1) / _data[i];
		}
	}
	FallbackVector reciprocal() const {
		FallbackVector cpy(*this);
		cpy.set_to_reciprocal();
		return cpy;
	}

	// Compatibility with STL containers
	ArrayType& asArray() {
		return _data;
	}
	const ArrayType& asArray() const {
		return _data;
	}
	T* data() {
		return _data.data();
	}
	const T* data() const {
		return _data.data();
	}
	size_t size() const {
		return _data.size();
	}
	iterator begin() {
		return _data.begin();
	}
	iterator end() {
		return _data.end();
	}
	const_iterator begin() const {
		return _data.begin();
	}
	const_iterator end() const {
		return _data.end();
	}
	T& at(size_t i) {
		return _data.at(i);
	}
	const T& at(size_t i) const {
		return _data.at(i);
	}
	T& operator[](size_t i) {
		return _data[i];
	}
	const T& operator[](size_t i) const {
		return _data[i];
	}
	// Arithemetics and comparisons with other vectors
	void operator+=(const FallbackVector& b) {
		for(size_t i = 0; i < _data.size(); ++i) {
			_data[i] += b._data[i];
		}
	}
	void operator-=(const FallbackVector& b) {
		for(size_t i = 0; i < _data.size(); ++i) {
			_data[i] -= b._data[i];
		}
	}
	void operator*=(const FallbackVector& b) {
		for(size_t i = 0; i < _data.size(); ++i) {
			_data[i] *= b._data[i];
		}
	}
	void operator/=(const FallbackVector& b) {
		for(size_t i = 0; i < _data.size(); ++i) {
			_data[i] /= b._data[i];
		}
	}
	void operator<<=(const FallbackVector& b) {
		for(size_t i = 0; i < _data.size(); ++i) {
			_data[i] = std::min( b._data[i], _data[i] );
		}
	} // Minimum
	void operator>>=(const FallbackVector& b) {
		for(size_t i = 0; i < _data.size(); ++i) {
			_data[i] = std::max( b._data[i], _data[i] );
		}
	} // Maximum
	FallbackVector operator+(const FallbackVector& b) const {
		FallbackVector tmp;
		for(size_t i = 0; i < _data.size(); ++i) {
			tmp._data[i] = _data[i] + b._data[i];
		}
		return tmp;
	}
	FallbackVector operator-(const FallbackVector& b) const {
		FallbackVector tmp;
		for(size_t i = 0; i < _data.size(); ++i) {
			tmp._data[i] = _data[i] - b._data[i];
		}
		return tmp;
	}
	FallbackVector operator*(const FallbackVector& b) const {
		FallbackVector tmp;
		for(size_t i = 0; i < _data.size(); ++i) {
			tmp._data[i] = _data[i] * b._data[i];
		}
		return tmp;
	}
	FallbackVector operator/(const FallbackVector& b) const {
		FallbackVector tmp;
		for(size_t i = 0; i < _data.size(); ++i) {
			tmp._data[i] = _data[i] / b._data[i];
		}
		return tmp;
	}
	bool operator==(const FallbackVector& b) const {
		bool toReturn = true;
		for(size_t i = 0; i < _data.size(); ++i) {
			toReturn = std::min(toReturn,_data[i] == b._data[i]);
		}
		return toReturn;
	}
	bool operator!=(const FallbackVector& b) const {
		bool toReturn = true;
		for(size_t i = 0; i < _data.size(); ++i) {
			toReturn = std::min(toReturn,_data[i] != b._data[i]);
		}
		return toReturn;
	}
	bool operator>(const FallbackVector& b) const {
		bool toReturn = true;
		for(size_t i = 0; i < _data.size(); ++i) {
			toReturn = std::min(toReturn,_data[i] > b._data[i]);
		}
		return toReturn;
	}
	bool operator>=(const FallbackVector& b) const {
		bool toReturn = true;
		for(size_t i = 0; i < _data.size(); ++i) {
			toReturn = std::min(toReturn,_data[i] >= b._data[i]);
		}
		return toReturn;
	}
	bool operator<(const FallbackVector& b) const {
		bool toReturn = true;
		for(size_t i = 0; i < _data.size(); ++i) {
			toReturn = std::min(toReturn,_data[i] < b._data[i]);
		}
		return toReturn;
	}
	bool operator<=(const FallbackVector& b) const {
		bool toReturn = true;
		for(size_t i = 0; i < _data.size(); ++i) {
			toReturn = std::min(toReturn,_data[i] <= b._data[i]);
		}
		return toReturn;
	}
	FallbackVector operator<<(const FallbackVector& b) const {
		FallbackVector tmp;
		for(size_t i = 0; i < _data.size(); ++i) {
			tmp._data[i] = std::min( b._data[i], _data[i] );
		}
		return tmp;
	} // Minimum
	FallbackVector operator>>(const FallbackVector& b) const {
		FallbackVector tmp;
		for(size_t i = 0; i < _data.size(); ++i) {
			tmp._data[i] = std::max( b._data[i], _data[i] );
		}
		return tmp;
	} // Maximum
	static FallbackVector min(const FallbackVector& a, const FallbackVector& b) {
		return a << b;
	} // Minimum
	static FallbackVector max(const FallbackVector& a, const FallbackVector& b) {
		return a >> b;
	} // Maximum
	static FallbackVector clamp(const FallbackVector& a, const FallbackVector& _min, const FallbackVector& _max) {
		return min(max(a,_min),_max);
	} // Clamp
	T vectorialMul(const FallbackVector& b) const {
		return std::inner_product(_data.begin(),_data.end(),b._data.begin(),b._data.end(),0);
	}
	// Arithemetics with scalars
	void operator+=(T b) {
		for(auto& it : _data) {
			it += b;
		}
	}
	void operator-=(T b) {
		for(auto& it : _data) {
			it -= b;
		}
	}
	void operator*=(T b) {
		for(auto& it : _data) {
			it *= b;
		}
	}
	void operator/=(T b) {
		for(auto& it : _data) {
			it *= b;
		}
	}
	void operator<<=(T b) {
		for(auto& it : _data) {
			it = std::min(it,b);
		}
	} // Minimum
	void operator>>=(T b) {
		for(auto& it : _data) {
			it = std::max(it,b);
		}
	} // Maximum
	FallbackVector operator+(T b) const {
		FallbackVector tmp;
		for(size_t i = 0; i < _data.size(); ++i) {
			tmp._data[i] = _data[i] + b;
		}
		return tmp;
	}
	FallbackVector operator-(T b) const {
		FallbackVector tmp;
		for(size_t i = 0; i < _data.size(); ++i) {
			tmp._data[i] = _data[i] - b;
		}
		return tmp;
	}
	FallbackVector operator*(T b) const {
		FallbackVector tmp;
		for(size_t i = 0; i < _data.size(); ++i) {
			tmp._data[i] = _data[i] * b;
		}
		return tmp;
	}
	FallbackVector operator/(T b) const {
		FallbackVector tmp;
		for(size_t i = 0; i < _data.size(); ++i) {
			tmp._data[i] = _data[i] / b;
		}
		return tmp;
	}
	FallbackVector operator<<(T b) {
		FallbackVector tmp;
		for(size_t i = 0; i < _data.size(); ++i) {
			tmp._data[i] = std::min(_data[i], b);
		}
		return tmp;
	} // Minimum
	FallbackVector operator>>(T b) {
		FallbackVector tmp;
		for(size_t i = 0; i < _data.size(); ++i) {
			tmp._data[i] = std::max(_data[i], b);
		}
		return tmp;
	} // Maximum
	bool operator==(T b) const {
		bool toReturn = true;
		for(size_t i = 0; i < _data.size(); ++i) {
			toReturn = std::min(toReturn,_data[i] == b);
		}
		return toReturn;
	}
	bool operator!=(T b) const {
		bool toReturn = true;
		for(size_t i = 0; i < _data.size(); ++i) {
			toReturn = std::min(toReturn,_data[i] != b);
		}
		return toReturn;
	}
	bool operator>(T b) const {
		bool toReturn = true;
		for(size_t i = 0; i < _data.size(); ++i) {
			toReturn = std::min(toReturn,_data[i] > b);
		}
		return toReturn;
	}
	bool operator>=(T b) const {
		bool toReturn = true;
		for(size_t i = 0; i < _data.size(); ++i) {
			toReturn = std::min(toReturn,_data[i] >= b);
		}
		return toReturn;
	}
	bool operator<(T b) const {
		bool toReturn = true;
		for(size_t i = 0; i < _data.size(); ++i) {
			toReturn = std::min(toReturn,_data[i] < b);
		}
		return toReturn;
	}
	bool operator<=(T b) const {
		bool toReturn = true;
		for(size_t i = 0; i < _data.size(); ++i) {
			toReturn = std::min(toReturn,_data[i] <= b);
		}
		return toReturn;
	}
	static FallbackVector min(const FallbackVector& a, const T b) {
		return a << b;
	} // Minimum
	static FallbackVector max(const FallbackVector& a, const T b) {
		return a >> b;
	} // Maximum
	static FallbackVector clamp(const FallbackVector& a, const T _min, const T _max) {
		return min(max(a,_min),_max);
	} // Clamp
	T vectorialMul(T b) const {
		return vectorialMul(FallbackVector(b));
	}
	// Other mathematical goodies
	void set_to_sqrt() {
		for(auto& it : _data) {
			it = std::sqrt(it);
		}
	}
	void set_to_rsqrt() {
		for(auto& it : _data) {
			it = T(1) / std::sqrt(it);
		}
	}
	void set_to_norm() {
		T largest = T(0);
		for(const auto& it : _data) {
			largest = std::max(largest,it);
		}
		for(auto& it : _data) {
			it /= largest;
		}
	}
	FallbackVector sqrt() const {
		FallbackVector tmp = *this;
		tmp.set_to_sqrt();
		return tmp;
	}
	FallbackVector rsqrt() const {
		FallbackVector tmp = *this;
		tmp.set_to_rsqrt();
		return tmp;
	}
	FallbackVector normalize() const {
		FallbackVector tmp = *this;
		tmp.set_to_norm();
		return tmp;
	}
	T sum() const {
		T tmp = 0;
		for(const auto& it : _data) {
			tmp += it;
		}
		return tmp;
	}
};

}
}

#endif // MHFALLBACKVECTOR_HPP
