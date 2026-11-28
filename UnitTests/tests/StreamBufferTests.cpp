/**
 * @file StreamBufferTests.cpp
 * @brief Tests for `Elv::Io::DeviceStreamBuffer`, the device-backed `streambuf`.
 *
 * The buffer used to live in `Sandbox/main.cpp` where only a manual run could exercise
 * it. A fixed-size in-memory device makes the interesting parts testable: buffered
 * writes reaching the device on destruction, reads crossing a refill boundary, the
 * putback area surviving a refill and the relative/absolute seek arithmetic.
 */
#include <catch2/catch_test_macros.hpp>
#include <Elvavena/Io/ElvStreamBuffer.hpp>
#include <Euphemy/Io/EuphBufferDevice.hpp>
#include <array>
#include <cstring>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace {

/** @brief A writable in-memory device the stream buffer can read and write. */
struct MemorySpace {
	std::array<std::byte, 256> bytes {};
};

} // namespace

TEST_CASE("DeviceStreamBuffer writes through the device on destruction", "[io][streambuf]")
{
	MemorySpace space;
	{
		Euph::Io::StaticBufferWrapper device(space.bytes.data(), space.bytes.size());
		Elv::Io::DeviceStreamBuffer<char> buffer(device, 16);
		std::ostream stream(&buffer);
		stream << "hello device";
		// No explicit flush: the buffer's destructor commits what is still pending.
	}
	REQUIRE(std::memcmp(space.bytes.data(), "hello device", 12) == 0);
}

TEST_CASE("DeviceStreamBuffer reads across a refill and honours putback", "[io][streambuf]")
{
	const std::string text = "abcdefghijklmnopqrstuvwxyz";
	MemorySpace space;
	std::memcpy(space.bytes.data(), text.data(), text.size());
	Euph::Io::StaticBufferWrapper device(space.bytes.data(), text.size());
	// A tiny buffer forces at least three refills, which is where the putback area and
	// the position arithmetic matter.
	Elv::Io::DeviceStreamBuffer<char> buffer(device, 16);
	std::istream stream(&buffer);

	std::string first(10, '\0');
	stream.read(first.data(), 10);
	REQUIRE(first == text.substr(0, 10));

	// `unget()` has to work even though the read cursor moved across a refill.
	stream.unget();
	REQUIRE(stream.get() == 'j');

	// The putback and the re-read cancel out, so the stream is still at position 10.
	std::string rest(16, '\0');
	stream.read(rest.data(), 16);
	REQUIRE(rest == text.substr(10));

	// The stream is now at the end: no more characters, and the stream reports it.
	stream.get();
	REQUIRE(stream.eof());
}

TEST_CASE("DeviceStreamBuffer seeks and reports absolute positions", "[io][streambuf]")
{
	const std::string text = "0123456789";
	MemorySpace space;
	std::memcpy(space.bytes.data(), text.data(), text.size());
	Euph::Io::StaticBufferWrapper device(space.bytes.data(), text.size());
	Elv::Io::DeviceStreamBuffer<char> buffer(device, 8);
	std::istream stream(&buffer);

	stream.get();
	stream.get();
	// `tellg()` is answered without moving the device cursor, so it stays correct while
	// characters are still buffered.
	REQUIRE(stream.tellg() == std::streampos(2));

	stream.seekg(5, std::ios::beg);
	REQUIRE(stream.tellg() == std::streampos(5));
	REQUIRE(stream.get() == '5');

	stream.seekg(-2, std::ios::cur);
	REQUIRE(stream.get() == '4');

	// Reading past the end leaves the device at its end instead of looping.
	stream.seekg(0, std::ios::end);
	REQUIRE(stream.tellg() == std::streampos(10));
}
