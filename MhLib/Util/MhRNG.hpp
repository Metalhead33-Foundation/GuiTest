#ifndef MHRNG_HPP
#define MHRNG_HPP
#include <cstdint>
#include <string>
#include <vector>
#include <deque>
#include <array>
#if __cplusplus > 201703L
#include <span>
#endif
#include <MhLib/Util/MhGlobals.hpp>

namespace MH33 {

namespace Util {
DEFINE_CLASS(RNG)
class MH_UTIL_API RNG
{
private:
	uint32_t seed;
public:
	RNG(uint32_t seed=0);
	RNG(const RNG& cpy);
	RNG& operator=(uint32_t seed);
	RNG& operator=(const RNG& cpy);
	uint32_t getSeed() const;
	void setSeed(uint32_t value);

	uint32_t generate();
	float generateFloat();
	double generateDouble();

	uint32_t generate(uint32_t max);
	float generateFloat(float max);
	double generateDouble(double max);

	uint32_t generate(uint32_t min, uint32_t max);
	float generateFloat(float min, float max);
	double generateDouble(double min, double max);
	// Randomly picking elements
	template <typename T, class Allocator = std::allocator<T>> inline T& pick(std::vector<T,Allocator>& data) {
		return data[generate() % data.size()];
	}
	template <typename T, class Allocator = std::allocator<T>> inline const T& pick(const std::vector<T,Allocator>& data) {
		return data[generate() % data.size()];
	}
	template <typename T, size_t size> inline T& pick(std::array<T,size>& data) {
		return data[generate() % data.size()];
	}
	template <typename T, size_t size> inline const T& pick(const std::array<T,size>& data) {
		return data[generate() % data.size()];
	}
	template <typename T, class Traits = std::char_traits<T>, class Allocator = std::allocator<T>> inline T& pick(std::basic_string<T,Traits,Allocator>& data) {
		return data[generate() % data.size()];
	}
	template <typename T, class Traits = std::char_traits<T>, class Allocator = std::allocator<T>> inline const T& pick(const std::basic_string<T,Traits,Allocator>& data) {
		return data[generate() % data.size()];
	}
#if __cplusplus > 201703L
	template <class T,std::size_t Extent = std::dynamic_extent> inline T& pick(std::span<T,Extent>& data) {
		return data[generate() % data.size()];
	}
	template <class T,std::size_t Extent = std::dynamic_extent> inline const T& pick(const std::span<T,Extent>& data) {
		return data[generate() % data.size()];
	}
#endif
	template <typename T, class Allocator = std::allocator<T>> inline T& operator<<(std::deque<T,Allocator>& data) {
		return data[generate() % data.size()];
	}
	template <typename T, class Allocator = std::allocator<T>> inline const T& operator<<(const std::deque<T,Allocator>& data) {
		return data[generate() % data.size()];
	}
};
}

}
#endif // MHRNG_HPP
