#ifndef ELVINTEGRALITERATOR_HPP
#define ELVINTEGRALITERATOR_HPP
#include <iterator>
namespace Elv {
namespace Util {

template <typename T> struct IntegralIterator {
	typedef T dataType;
	typedef T& dataTypeRef;
	typedef const T& dataTypeConstRef;
	dataType var;
	IntegralIterator(dataType nvar=0) : var(nvar) {

	}
	IntegralIterator(const IntegralIterator& cpy) : var(cpy.var) {

	}
	// Overloaded operators - assignment
	inline IntegralIterator& operator=(const IntegralIterator& other) {
		this->var = other.var;
		return *this;
	}
	inline IntegralIterator& operator=(const dataType& other) {
		this->var = other;
		return *this;
	}
	inline IntegralIterator& operator+=(const dataType& other) {
		this->var += other;
		return *this;
	}
	inline IntegralIterator& operator-=(const dataType& other) {
		this->var -= other;
		return *this;
	}
	inline IntegralIterator& operator*=(const dataType& other) {
		this->var *= other;
		return *this;
	}
	inline IntegralIterator& operator/=(const dataType& other) {
		this->var /= other;
		return *this;
	}
	inline IntegralIterator& operator%=(const dataType& other) {
		this->var %= other;
		return *this;
	}
	// Overloaded operators - new data
	inline IntegralIterator operator+(const dataType& other) const {
		IntegralIterator a(*this);
		a.var += other;
		return a;
	}
	inline IntegralIterator operator-(const dataType& other) const {
		IntegralIterator a(*this);
		a.var -= other;
		return a;
	}
	inline IntegralIterator operator*(const dataType& other) const {
		IntegralIterator a(*this);
		a.var *= other;
		return a;
	}
	inline IntegralIterator operator/(const dataType& other) const {
		IntegralIterator a(*this);
		a.var /= other;
		return a;
	}
	inline IntegralIterator operator%(const dataType& other) const {
		IntegralIterator a(*this);
		a.var %= other;
		return a;
	}
	// Iterator-like functionality
	inline IntegralIterator& operator++() { ++var; return *this;}
	inline IntegralIterator& operator--() { --var; return *this;}
	inline IntegralIterator operator++(int) { IntegralIterator tmp(*this); ++var; return tmp;}
	inline IntegralIterator operator--(int) { IntegralIterator tmp(*this); --var; return tmp;}
	inline dataType& operator*() { return var; }
	inline const dataType& operator*() const { return var; }
	inline operator dataTypeRef() { return var; }
	inline operator dataTypeConstRef() const { return var; }
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = T;
	using difference_type = T;
	using pointer = T*;
	using reference = T&;
};
// Overloaded operators - Comparisons
template <typename T> inline bool operator==(const IntegralIterator<T>& lhs, const IntegralIterator<T>& rhs) {
	return lhs.var == rhs.var;
}
template <typename T> inline bool operator!=(const IntegralIterator<T>& lhs, const IntegralIterator<T>& rhs) {
	return lhs.var != rhs.var;
}
template <typename T> inline bool operator<(const IntegralIterator<T>& lhs, const IntegralIterator<T>& rhs) {
	return lhs.var < rhs.var;
}
template <typename T> inline bool operator<=(const IntegralIterator<T>& lhs, const IntegralIterator<T>& rhs) {
	return lhs.var <= rhs.var;
}
template <typename T> inline bool operator>(const IntegralIterator<T>& lhs, const IntegralIterator<T>& rhs) {
	return lhs.var > rhs.var;
}
template <typename T> inline bool operator>=(const IntegralIterator<T>& lhs, const IntegralIterator<T>& rhs) {
	return lhs.var >= rhs.var;
}
template <typename T> struct IntegralIterable {
	typedef T dataType;
	typedef T& dataTypeRef;
	typedef const T& dataTypeConstRef;
	dataType var;
	typedef IntegralIterator<T> iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	explicit IntegralIterable(dataType nvar=0) : var(nvar) {

	}
	IntegralIterable(const IntegralIterable& cpy) : var(cpy.var) {

	}
	// Overloaded operators - assignment
	inline IntegralIterable& operator=(const IntegralIterable& other) {
		this->var = other.var;
		return *this;
	}
	inline IntegralIterable& operator=(const dataType& other) {
		this->var = other;
		return *this;
	}
	inline IntegralIterable& operator+=(const dataType& other) {
		this->var += other;
		return *this;
	}
	inline IntegralIterable& operator-=(const dataType& other) {
		this->var -= other;
		return *this;
	}
	inline IntegralIterable& operator*=(const dataType& other) {
		this->var *= other;
		return *this;
	}
	inline IntegralIterable& operator/=(const dataType& other) {
		this->var /= other;
		return *this;
	}
	inline IntegralIterable& operator%=(const dataType& other) {
		this->var %= other;
		return *this;
	}
	// Overloaded operators - new data
	inline IntegralIterable operator+(const dataType& other) const {
		IntegralIterable a(*this);
		a += other;
		return a;
	}
	inline IntegralIterable operator-(const dataType& other) const {
		IntegralIterable a(*this);
		a.var -= other;
		return a;
	}
	inline IntegralIterable operator*(const dataType& other) const {
		IntegralIterable a(*this);
		a.var *= other;
		return a;
	}
	inline IntegralIterable operator/(const dataType& other) const {
		IntegralIterable a(*this);
		a.var /= other;
		return a;
	}
	inline IntegralIterable operator%(const dataType& other) const {
		IntegralIterable a(*this);
		a.var %= other;
		return a;
	}
	inline operator dataTypeRef() { return var; }
	inline operator dataTypeConstRef() const { return var; }
	// Iterators
	inline iterator begin() const { return iterator(0); }
	inline iterator end() const { return iterator(var); }
	inline reverse_iterator rbegin() const { return  reverse_iterator(var); }
	inline reverse_iterator rend() const { return  reverse_iterator(0); }
};
// Overloaded operators - Comparisons
template <typename T> inline bool operator==(const IntegralIterable<T>& lhs, const IntegralIterable<T>& rhs) {
	return lhs.var == rhs.var;
}
template <typename T> inline bool operator!=(const IntegralIterable<T>& lhs, const IntegralIterable<T>& rhs) {
	return lhs.var != rhs.var;
}
template <typename T> inline bool operator<(const IntegralIterable<T>& lhs, const IntegralIterable<T>& rhs) {
	return lhs.var < rhs.var;
}
template <typename T> inline bool operator<=(const IntegralIterable<T>& lhs, const IntegralIterable<T>& rhs) {
	return lhs.var <= rhs.var;
}
template <typename T> inline bool operator>(const IntegralIterable<T>& lhs, const IntegralIterable<T>& rhs) {
	return lhs.var > rhs.var;
}
template <typename T> inline bool operator>=(const IntegralIterable<T>& lhs, const IntegralIterable<T>& rhs) {
	return lhs.var >= rhs.var;
}

}
}
#endif // ELVINTEGRALITERATOR_HPP
