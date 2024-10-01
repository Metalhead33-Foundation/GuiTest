#ifndef ELVCONTINUOUSITERATOR_HPP
#define ELVCONTINUOUSITERATOR_HPP
#include <cstddef>
#include <iterator>
namespace Elv {
namespace Util {

template <typename T> struct continuous_iterator {
public:
	using iterator_category = std::random_access_iterator_tag;
	using difference_type = ptrdiff_t;
	using value_type = T;
	using pointer = T*;
	using reference = T&;

private:
	pointer m_ptr;

public:
	// Constructor
	continuous_iterator(pointer ptr) : m_ptr(ptr) {}
	// Dereference operator
	reference operator*() const { return *m_ptr; }
	// Member access operator
	pointer operator->() { return m_ptr; }
	// Subscript operator
	reference operator[](difference_type i) { return m_ptr[i]; }
	// Prefix increment
	continuous_iterator& operator++() { ++m_ptr; return *this; }
	// Postfix increment
	continuous_iterator operator++(int) { continuous_iterator tmp = *this; ++(*this); return tmp; }
	// Prefix decrement
	continuous_iterator& operator--() { --m_ptr; return *this; }
	// Postfix decrement
	continuous_iterator operator--(int) { continuous_iterator tmp = *this; --(*this); return tmp; }
	// Addition operator
	continuous_iterator operator+(difference_type i) const { return iterator(m_ptr + i); }
	// Subtraction operator
	continuous_iterator operator-(difference_type i) const { return iterator(m_ptr - i); }
	// Difference between two iterators
	difference_type operator-(const continuous_iterator& other) const { return m_ptr - other.m_ptr; }
	// Compound assignment for addition
	continuous_iterator& operator+=(difference_type i) { m_ptr += i; return *this; }
	// Compound assignment for subtraction
	continuous_iterator& operator-=(difference_type i) { m_ptr -= i; return *this; }
	// Comparison operators
	friend bool operator==(const continuous_iterator& a, const continuous_iterator& b) { return a.m_ptr == b.m_ptr; }
	friend bool operator!=(const continuous_iterator& a, const continuous_iterator& b) { return a.m_ptr != b.m_ptr; }
	friend bool operator<(const continuous_iterator& a, const continuous_iterator& b) { return a.m_ptr < b.m_ptr; }
	friend bool operator>(const continuous_iterator& a, const continuous_iterator& b) { return a.m_ptr > b.m_ptr; }
	friend bool operator<=(const continuous_iterator& a, const continuous_iterator& b) { return a.m_ptr <= b.m_ptr; }
	friend bool operator>=(const continuous_iterator& a, const continuous_iterator& b) { return a.m_ptr >= b.m_ptr; }
	// Compatibility with pointers (for contiguous iterator)
	pointer get_pointer() const { return m_ptr; }
	// Implicit conversion to a raw pointer (for std::contiguous_iterator concept)
	operator pointer() const { return m_ptr; }
};

}
}
#endif // ELVCONTINUOUSITERATOR_HPP
