#ifndef MHCOMPARABLE_HPP
#define MHCOMPARABLE_HPP

namespace MH33 {
namespace Interface {

enum ComparisonResult {
	EQUAL,
	GREATER,
	LESSER
};
template <typename T> class Comparable {
public:
	virtual ~Comparable() = default;
	virtual ComparisonResult compare(const T& b) const = 0;
	inline bool operator==(const T& b) const { return compare(b) == EQUAL; }
	inline bool operator!=(const T& b) const { return compare(b) != EQUAL; }
	inline bool operator<(const T& b) const { return compare(b) == LESSER; }
	inline bool operator<=(const T& b) const { return compare(b) != GREATER; }
	inline bool operator>(const T& b) const { return compare(b) == GREATER; }
	inline bool operator>=(const T& b) const { return compare(b) != LESSER; }

};

}
}

#endif // MHCOMPARABLE_HPP
