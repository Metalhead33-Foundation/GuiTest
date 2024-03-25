#ifndef MHCOMPARABLE_HPP
#define MHCOMPARABLE_HPP

namespace MH33 {
namespace Interface {

/*! Comparison result.*/
enum ComparisonResult {
	/*! Equal.*/
	EQUAL,
	/*! Greater.*/
	GREATER,
	/*! Lesser.*/
	LESSER
};
/*! A comparable element. Intended to be inherited by classes that can be compared to T.*/
/*!
  \tparam T The type that elements of this object can be compared to.
*/
template <typename T> class Comparable {
public:
	/*! Destructor.*/
	virtual ~Comparable() = default;
	/*! Compares this object to b*/
	/*!
	 \param b The other value this object is compared to.
	  \return The comparison result.
	*/
	virtual ComparisonResult compare(const T& b) const = 0;
	/*! Compares this object to b*/
	/*!
	  \param b The other value this object is compared to.
	   \return True if compare() returns EQUAL, otherwise false
	   \sa compare
	*/
	inline bool operator==(const T& b) const { return compare(b) == EQUAL; }
	/*! Compares this object to b*/
	/*!
	 \param b The other value this object is compared to.
	   \return False if compare() returns EQUAL, otherwise true
	   \sa compare
	*/
	inline bool operator!=(const T& b) const { return compare(b) != EQUAL; }
	/*! Compares this object to b*/
	/*!
	 \param b The other value this object is compared to.
	   \return True if compare() returns LESSER, otherwise false
	   \sa compare
	*/
	inline bool operator<(const T& b) const { return compare(b) == LESSER; }
	/*! Compares this object to b*/
	/*!
	 \param b The other value this object is compared to.
	   \return False if compare() returns LESSER, otherwise true
	   \sa compare
	*/
	inline bool operator<=(const T& b) const { return compare(b) != GREATER; }
	/*! Compares this object to b*/
	/*!
	 \param b The other value this object is compared to.
	   \return True if compare() returns GREATER, otherwise false
	   \sa compare
	*/
	inline bool operator>(const T& b) const { return compare(b) == GREATER; }
	/*! Compares this object to b*/
	/*!
	 \param b The other value this object is compared to.
	   \return False if compare() returns GREATER, otherwise true
	   \sa compare
	*/
	inline bool operator>=(const T& b) const { return compare(b) != LESSER; }

};

}
}

#endif // MHCOMPARABLE_HPP
