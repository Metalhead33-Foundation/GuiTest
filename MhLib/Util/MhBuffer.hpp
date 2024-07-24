#ifndef MHBUFFER_HPP
#define MHBUFFER_HPP
#include <cstddef>
#include <cstdint>
#include <vector>
#include <array>
#include <span>
namespace MH33 {
namespace Util {
typedef std::vector<std::byte> Buffer;
template <std::size_t size> using StaticBuffer = std::array<std::byte,size>;
typedef std::span<std::byte> ByteSpan;
typedef std::span<const std::byte> CByteSpan;
}
}
#endif // MHBUFFER_HPP
