#ifndef MHSEQUENTIALIDGENERATOR_HPP
#define MHSEQUENTIALIDGENERATOR_HPP
#include <atomic>
#include <concepts>
namespace MH33 {
namespace Util {

// A thread-safe unique ID generator
template <std::integral T> class SequentialIdGenerator {
private:
	std::atomic<T> currentID;
public:
	SequentialIdGenerator() : currentID(0) {}

	T generateID(std::memory_order order = std::memory_order_relaxed) {
		return currentID.fetch_add(1, order);
	}
};


}
}
#endif // MHSEQUENTIALIDGENERATOR_HPP
