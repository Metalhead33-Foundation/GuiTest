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
/**
 * @brief Template struct representing an Integral Iterable.
 *
 * This class provides an iterable interface for integral types (e.g., int, long, etc.),
 * enabling iteration over a range defined by the stored integral value, along with
 * basic arithmetic operations and comparisons.
 *
 * @tparam T Integral type (e.g., int, long, etc.)
 */
template <typename T> struct IntegralIterable {
	/**
	 * @brief Type definition for the data type stored in the iterable.
	 */
	typedef T dataType;

	/**
	 * @brief Type definition for a reference to the data type.
	 */
	typedef T& dataTypeRef;

	/**
	 * @brief Type definition for a constant reference to the data type.
	 */
	typedef const T& dataTypeConstRef;

	/**
	 * @brief The integral value defining the iteration range.
	 */
	dataType var;

	/**
	 * @brief Type definition for the iterator used by this iterable.
	 */
	typedef IntegralIterator<T> iterator;

	/**
	 * @brief Type definition for the reverse iterator used by this iterable.
	 */
	typedef std::reverse_iterator<iterator> reverse_iterator;

	/**
	 * @brief Explicit constructor initializing the iterable with an optional value.
	 *
	 * @param nvar Initial value for the iterable (defaults to 0 if not provided).
	 */
	explicit IntegralIterable(dataType nvar = 0) : var(nvar) {}

	/**
	 * @brief Copy constructor for creating a new iterable from an existing one.
	 *
	 * @param cpy The IntegralIterable instance to copy.
	 */
	IntegralIterable(const IntegralIterable& cpy) : var(cpy.var) {}

	// Overloaded operators - assignment
	/**
	 * @brief Assignment operator for copying values from another IntegralIterable.
	 *
	 * @param other The IntegralIterable instance to copy values from.
	 * @return Reference to this iterable after assignment.
	 */
	inline IntegralIterable& operator=(const IntegralIterable& other) {
		this->var = other.var;
		return *this;
	}

	/**
	 * @brief Assignment operator for assigning a value of the dataType.
	 *
	 * @param other The value to assign to this iterable.
	 * @return Reference to this iterable after assignment.
	 */
	inline IntegralIterable& operator=(const dataType& other) {
		this->var = other;
		return *this;
	}

	/**
	 * @brief Addition assignment operator.
	 *
	 * @param other The value to add to this iterable's current value.
	 * @return Reference to this iterable after addition.
	 */
	inline IntegralIterable& operator+=(const dataType& other) {
		this->var += other;
		return *this;
	}

	/**
	 * @brief Subtraction assignment operator.
	 *
	 * @param other The value to subtract from this iterable's current value.
	 * @return Reference to this iterable after subtraction.
	 */
	inline IntegralIterable& operator-=(const dataType& other) {
		this->var -= other;
		return *this;
	}

	/**
	 * @brief Multiplication assignment operator.
	 *
	 * @param other The value to multiply with this iterable's current value.
	 * @return Reference to this iterable after multiplication.
	 */
	inline IntegralIterable& operator*=(const dataType& other) {
		this->var *= other;
		return *this;
	}

	/**
	 * @brief Division assignment operator.
	 *
	 * @param other The value to divide this iterable's current value by.
	 * @return Reference to this iterable after division.
	 */
	inline IntegralIterable& operator/=(const dataType& other) {
		this->var /= other;
		return *this;
	}

	/**
	 * @brief Modulus assignment operator.
	 *
	 * @param other The value to compute the modulus with this iterable's current value.
	 * @return Reference to this iterable after modulus operation.
	 */
	inline IntegralIterable& operator%=(const dataType& other) {
		this->var %= other;
		return *this;
	}

	// Overloaded operators - new data
	/**
	 * @brief Addition operator creating a new iterable with the sum.
	 *
	 * @param other The value to add to this iterable's current value.
	 * @return A new IntegralIterable with the result of the addition.
	 */
	inline IntegralIterable operator+(const dataType& other) const {
		IntegralIterable a(*this);
		a += other;
		return a;
	}

	/**
	 * @brief Subtraction operator creating a new iterable with the difference.
	 *
	 * @param other The value to subtract from this iterable's current value.
	 * @return A new IntegralIterable with the result of the subtraction.
	 */
	inline IntegralIterable operator-(const dataType& other) const {
		IntegralIterable a(*this);
		a.var -= other;
		return a;
	}

	/**
	 * @brief Multiplication operator creating a new iterable with the product.
	 *
	 * @param other The value to multiply with this iterable's current value.
	 * @return A new IntegralIterable with the result of the multiplication.
	 */
	inline IntegralIterable operator*(const dataType& other) const {
		IntegralIterable a(*this);
		a.var *= other;
		return a;
	}

	/**
	 * @brief Division operator creating a new iterable with the quotient.
	 *
	 * @param other The value to divide this iterable's current value by.
	 * @return A new IntegralIterable with the result of the division.
	 */
	inline IntegralIterable operator/(const dataType& other) const {
		IntegralIterable a(*this);
		a.var /= other;
		return a;
	}

	/**
	 * @brief Modulus operator creating a new iterable with the remainder.
	 *
	 * @param other The value to compute the modulus with this iterable's current value.
	 * @return A new IntegralIterable with the result of the modulus operation.
	 */
	inline IntegralIterable operator%(const dataType& other) const {
		IntegralIterable a(*this);
		a.var %= other;
		return a;
	}

	/**
	 * @brief Implicit conversion operator to dataTypeRef.
	 *
	 * @return Reference to the stored value.
	 */
	inline operator dataTypeRef() { return var; }

	/**
	 * @brief Implicit conversion operator to dataTypeConstRef for const instances.
	 *
	 * @return Constant reference to the stored value.
	 */
	inline operator dataTypeConstRef() const { return var; }

	// Iterators
	/**
	 * @brief Returns an iterator pointing to the beginning of the iteration range.
	 *
	 * @return Iterator pointing to the start (0).
	 */
	inline iterator begin() const { return iterator(0); }

	/**
	 * @brief Returns an iterator pointing to the end of the iteration range.
	 *
	 * @return Iterator pointing to the end (var).
	 */
	inline iterator end() const { return iterator(var); }

	/**
	 * @brief Returns a reverse iterator pointing to the beginning of the reversed iteration range.
	 *
	 * @return Reverse iterator pointing to the end (var) in reverse iteration.
	 */
	inline reverse_iterator rbegin() const { return reverse_iterator(var); }

	/**
	 * @brief Returns a reverse iterator pointing to the end of the reversed iteration range.
	 *
	 * @return Reverse iterator pointing to the start (0) in reverse iteration.
	 */
	inline reverse_iterator rend() const { return reverse_iterator(0); }
};

// Overloaded operators - Comparisons
/**
 * @brief Equality comparison operator.
 *
 * @tparam T Integral type.
 * @param lhs Left-hand side IntegralIterable.
 * @param rhs Right-hand side IntegralIterable.
 * @return True if both iterables hold the same value, false otherwise.
 */
template <typename T> inline bool operator==(const IntegralIterable<T>& lhs, const IntegralIterable<T>& rhs) {
	return lhs.var == rhs.var;
}

/**
 * @brief Inequality comparison operator.
 *
 * @tparam T Integral type.
 * @param lhs Left-hand side IntegralIterable.
 * @param rhs Right-hand side IntegralIterable.
 * @return True if the iterables hold different values, false otherwise.
 */
template <typename T> inline bool operator!=(const IntegralIterable<T>& lhs, const IntegralIterable<T>& rhs) {
	return lhs.var!= rhs.var;
}

/**
 * @brief Less-than comparison operator.
 *
 * @tparam T Integral type.
 * @param lhs Left-hand side IntegralIterable.
 * @param rhs Right-hand side IntegralIterable.
 * @return True if the left iterable's value is less than the right's, false otherwise.
 */
template <typename T> inline bool operator<(const IntegralIterable<T>& lhs, const IntegralIterable<T>& rhs) {
	return lhs.var < rhs.var;
}

/**
 * @brief Less-than-or-equal comparison operator.
 *
 * @tparam T Integral type.
 * @param lhs Left-hand side IntegralIterable.
 * @param rhs Right-hand side IntegralIterable.
 * @return True if the left iterable's value is less than or equal to the right's, false otherwise.
 */
template <typename T> inline bool operator<=(const IntegralIterable<T>& lhs, const IntegralIterable<T>& rhs) {
	return lhs.var <= rhs.var;
}

/**
 * @brief Greater-than comparison operator.
 *
 * @tparam T Integral type.
 * @param lhs Left-hand side IntegralIterable.
 * @param rhs Right-hand side IntegralIterable.
 * @return True if the left iterable's value is greater than the right's, false otherwise.
 */
template <typename T> inline bool operator>(const IntegralIterable<T>& lhs, const IntegralIterable<T>& rhs) {
	return lhs.var > rhs.var;
}

/**
 * @brief Greater-than-or-equal comparison operator.
 *
 * @tparam T Integral type.
 * @param lhs Left-hand side IntegralIterable.
 * @param rhs Right-hand side IntegralIterable.
 * @return True if the left iterable's value is greater than or equal to the right's, false otherwise.
 */
template <typename T> inline bool operator>=(const IntegralIterable<T>& lhs, const IntegralIterable<T>& rhs) {
	return lhs.var >= rhs.var;
}

}
}
#endif // ELVINTEGRALITERATOR_HPP
