#ifndef MHEQUATABLE_HPP
#define MHEQUATABLE_HPP

namespace MH33 {
namespace Interface {

/*! An equateble element. Intended to be inherited by classes that can be equated to T.*/
/*!
  \tparam T The type that elements of this object can be equated to.
*/
template <typename T> class Equatable {
public:
	/*! Destructor.*/
	virtual ~Equatable() = default;
	/*! Compares this object to b*/
	/*!
	  \param b The other value this object is compared to.
	  \return True if this object and B are equals, otherwise false.
	*/
	virtual bool equals(const T& b) const = 0;
	/*! Compares this object to b*/
	/*!
	  \param b The other value this object is compared to.
	  \return True if this object and B are equals, otherwise false.
	*/
	inline bool operator==(const T& b) const { return equals(b); }
	/*! Compares this object to b*/
	/*!
	  \param b The other value this object is compared to.
	  \return False if this object and B are equals, otherwise true.
	*/
	inline bool operator!=(const T& b) const { return !equals(b); }
};

}
}
#endif // MHEQUATABLE_HPP
