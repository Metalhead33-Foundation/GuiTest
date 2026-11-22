/**
 * @file BitStreamTests.cpp
 * @brief Round-trip and failure tests for the bit-level packet codec.
 *
 * Networking packets mix one-bit flags, narrow integers, varints and quantized
 * floats. These cases pin down the exact bit order, the signed/zig-zag helpers,
 * quantized ranges and the non-throwing failure behaviour that malformed
 * packets rely on.
 */
#include <catch2/catch_test_macros.hpp>
#include <Elvavena/Io/ElvBitStream.hpp>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace {

using Elv::Io::BitReader;
using Elv::Io::BitStreamStatus;
using Elv::Io::BitWriter;

/** @brief Creates a zero-filled byte buffer of the requested size. */
std::vector<std::byte> makeBuffer(std::size_t size) {
	return std::vector<std::byte>(size, std::byte { 0 });
}

} // namespace

TEST_CASE("Bit streams round-trip mixed-width fields", "[net][bitstream]")
{
	std::vector<std::byte> buffer = makeBuffer(128);
	BitWriter writer(buffer);
	REQUIRE(writer.writeBool(true));
	REQUIRE(writer.writeBits(5, 3));
	REQUIRE(writer.writeBits(100, 7));
	REQUIRE(writer.writeBits(0xBEEF, 16));
	REQUIRE(writer.writeBits(0xDEADBEEFu, 32));
	REQUIRE(writer.writeBits(0x0123456789ABCDEFull, 64));
	REQUIRE(writer.writeSignedZigZag(-12345, 20));
	REQUIRE(writer.writeVarUInt(300));
	REQUIRE(writer.writeVarSInt(-50000));
	REQUIRE(writer.writeFloat32(3.25f));
	REQUIRE(writer.writeHalf(half_float::half(1.5f)));
	REQUIRE(writer.writeQuantizedFloat(0.25f, -1.0f, 1.0f, 10));
	REQUIRE(writer.alignToByte());
	REQUIRE(writer.writeBytes("abc", 3));
	REQUIRE(writer.ok());

	BitReader reader(std::span<const std::byte>(buffer.data(), writer.byteSize()));
	bool boolean = false;
	std::uint64_t value = 0;
	std::int64_t signedValue = 0;
	std::uint64_t varUInt = 0;
	std::int64_t varSInt = 0;
	float float32 = 0.0f;
	half_float::half half = half_float::half(0.0f);
	float quantized = 0.0f;
	std::array<char, 3> text {};

	REQUIRE(reader.readBool(boolean));
	CHECK(boolean);
	REQUIRE(reader.readBits(value, 3));
	CHECK(value == 5);
	REQUIRE(reader.readBits(value, 7));
	CHECK(value == 100);
	REQUIRE(reader.readBits(value, 16));
	CHECK(value == 0xBEEF);
	REQUIRE(reader.readBits(value, 32));
	CHECK(value == 0xDEADBEEFu);
	REQUIRE(reader.readBits(value, 64));
	CHECK(value == 0x0123456789ABCDEFull);
	REQUIRE(reader.readSignedZigZag(signedValue, 20));
	CHECK(signedValue == -12345);
	REQUIRE(reader.readVarUInt(varUInt));
	CHECK(varUInt == 300);
	REQUIRE(reader.readVarSInt(varSInt));
	CHECK(varSInt == -50000);
	REQUIRE(reader.readFloat32(float32));
	CHECK(float32 == 3.25f);
	REQUIRE(reader.readHalf(half));
	CHECK(static_cast<float>(half) == 1.5f);
	REQUIRE(reader.readQuantizedFloat(quantized, -1.0f, 1.0f, 10));
	CHECK(std::fabs(quantized - 0.25f) <= (2.0f / 1023.0f));
	REQUIRE(reader.alignToByte());
	REQUIRE(reader.readBytes(text.data(), text.size()));
	CHECK(std::string(text.data(), text.size()) == "abc");
	CHECK(reader.ok());
}

TEST_CASE("Bit streams write most-significant bit first", "[net][bitstream]")
{
	std::array<std::byte, 2> buffer { std::byte { 0 }, std::byte { 0 } };
	BitWriter writer(buffer);
	REQUIRE(writer.writeBits(0b101, 3));
	REQUIRE(writer.writeBits(0b11, 2));
	CHECK(std::to_integer<std::uint8_t>(buffer[0]) == 0b10111000u);

	BitReader reader(buffer);
	std::uint64_t first = 0;
	std::uint64_t second = 0;
	REQUIRE(reader.readBits(first, 3));
	REQUIRE(reader.readBits(second, 2));
	CHECK(first == 0b101);
	CHECK(second == 0b11);
}

TEST_CASE("Signed zig-zag handles the full 64-bit range", "[net][bitstream]")
{
	const std::array<std::int64_t, 6> values {
		std::numeric_limits<std::int64_t>::min(),
		-1,
		0,
		1,
		std::numeric_limits<std::int64_t>::max(),
		-1234567890123ll
	};

	std::vector<std::byte> buffer = makeBuffer(64);
	BitWriter writer(buffer);
	for (const std::int64_t value : values) {
		REQUIRE(writer.writeSignedZigZag(value, 64));
	}
	REQUIRE(writer.ok());

	BitReader reader(std::span<const std::byte>(buffer.data(), writer.byteSize()));
	for (const std::int64_t expected : values) {
		std::int64_t actual = 0;
		REQUIRE(reader.readSignedZigZag(actual, 64));
		CHECK(actual == expected);
	}
	CHECK(reader.ok());
}

TEST_CASE("Quantized floats clamp and round-trip", "[net][bitstream]")
{
	constexpr float minimum = -100.0f;
	constexpr float maximum = 100.0f;
	constexpr unsigned bits = 16;
	const float tolerance = (maximum - minimum) / 65535.0f + 1e-4f;
	const std::array<float, 7> values {
		minimum, -50.0f, -0.25f, 0.0f, 1.0f, 50.0f, maximum
	};

	std::vector<std::byte> buffer = makeBuffer(32);
	BitWriter writer(buffer);
	for (const float value : values) {
		REQUIRE(writer.writeQuantizedFloat(value, minimum, maximum, bits));
	}
	REQUIRE(writer.writeQuantizedFloat(1000.0f, minimum, maximum, bits));
	REQUIRE(writer.writeQuantizedFloat(-1000.0f, minimum, maximum, bits));
	REQUIRE(writer.writeQuantizedFloat(std::numeric_limits<float>::quiet_NaN(),
		minimum, maximum, bits));
	REQUIRE(writer.ok());

	BitReader reader(std::span<const std::byte>(buffer.data(), writer.byteSize()));
	for (const float expected : values) {
		float actual = 0.0f;
		REQUIRE(reader.readQuantizedFloat(actual, minimum, maximum, bits));
		CHECK(std::fabs(actual - expected) <= tolerance);
	}
	float actual = 0.0f;
	REQUIRE(reader.readQuantizedFloat(actual, minimum, maximum, bits));
	CHECK(std::fabs(actual - maximum) <= tolerance);
	REQUIRE(reader.readQuantizedFloat(actual, minimum, maximum, bits));
	CHECK(std::fabs(actual - minimum) <= tolerance);
	REQUIRE(reader.readQuantizedFloat(actual, minimum, maximum, bits));
	CHECK(std::fabs(actual - minimum) <= tolerance);
	CHECK(reader.ok());
}

TEST_CASE("Quantized floats reject invalid widths and degenerate ranges", "[net][bitstream]")
{
	std::vector<std::byte> buffer = makeBuffer(8);
	BitWriter writer(buffer);
	CHECK_FALSE(writer.writeQuantizedFloat(0.0f, -1.0f, 1.0f, 0));
	CHECK(writer.status() == BitStreamStatus::InvalidArgument);

	writer.reset();
	REQUIRE(writer.writeQuantizedFloat(123.0f, 7.0f, 7.0f, 16));
	REQUIRE(writer.ok());
	BitReader reader(std::span<const std::byte>(buffer.data(), writer.byteSize()));
	float actual = 0.0f;
	REQUIRE(reader.readQuantizedFloat(actual, 7.0f, 7.0f, 16));
	CHECK(actual == 7.0f);
}

TEST_CASE("Writer reports overflow and reader reports truncation", "[net][bitstream]")
{
	std::array<std::byte, 1> buffer { std::byte { 0 } };
	BitWriter writer(buffer);
	CHECK_FALSE(writer.writeBits(0x1FF, 9));
	CHECK(writer.status() == BitStreamStatus::BufferFull);
	CHECK_FALSE(writer.writeBits(0, 1));

	BitReader reader(buffer);
	std::uint64_t value = 0;
	CHECK_FALSE(reader.readBits(value, 9));
	CHECK(reader.status() == BitStreamStatus::BufferExhausted);
	CHECK_FALSE(reader.readBits(value, 1));
}

TEST_CASE("Byte writes work across bit boundaries", "[net][bitstream]")
{
	const std::vector<std::byte> payload {
		std::byte { 0x00 }, std::byte { 0x11 }, std::byte { 0x22 },
		std::byte { 0x33 }, std::byte { 0x44 }, std::byte { 0x55 }
	};

	std::vector<std::byte> buffer = makeBuffer(64);
	BitWriter writer(buffer);
	REQUIRE(writer.writeBits(0b101, 3));
	REQUIRE(writer.writeBytes(payload));
	REQUIRE(writer.ok());

	BitReader reader(std::span<const std::byte>(buffer.data(), writer.byteSize()));
	std::uint64_t prefix = 0;
	REQUIRE(reader.readBits(prefix, 3));
	CHECK(prefix == 0b101);
	std::vector<std::byte> decoded(payload.size());
	REQUIRE(reader.readBytes(decoded));
	CHECK(decoded == payload);
	CHECK(reader.ok());
}

TEST_CASE("Float encodings preserve their exact bit patterns", "[net][bitstream]")
{
	const std::array<float, 5> floats {
		0.0f,
		-0.0f,
		1.5f,
		std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity()
	};
	const std::array<double, 3> doubles {
		0.0,
		-2.25,
		std::numeric_limits<double>::max()
	};

	std::vector<std::byte> buffer = makeBuffer(128);
	BitWriter writer(buffer);
	for (const float value : floats) REQUIRE(writer.writeFloat32(value));
	for (const double value : doubles) REQUIRE(writer.writeFloat64(value));
	for (const float value : floats) {
		REQUIRE(writer.writeHalf(half_float::half(value)));
	}
	REQUIRE(writer.ok());

	BitReader reader(std::span<const std::byte>(buffer.data(), writer.byteSize()));
	for (const float expected : floats) {
		float actual = 0.0f;
		REQUIRE(reader.readFloat32(actual));
		CHECK(std::bit_cast<std::uint32_t>(actual) == std::bit_cast<std::uint32_t>(expected));
	}
	for (const double expected : doubles) {
		double actual = 0.0;
		REQUIRE(reader.readFloat64(actual));
		CHECK(std::bit_cast<std::uint64_t>(actual) == std::bit_cast<std::uint64_t>(expected));
	}
	for (const float expected : floats) {
		half_float::half actual = half_float::half(0.0f);
		REQUIRE(reader.readHalf(actual));
		CHECK(std::bit_cast<std::uint16_t>(actual)
			== std::bit_cast<std::uint16_t>(half_float::half(expected)));
	}
}

TEST_CASE("Varints are compact and round-trip", "[net][bitstream]")
{
	const std::array<std::uint64_t, 7> values {
		0ull, 1ull, 127ull, 128ull, 300ull, 16384ull,
		std::numeric_limits<std::uint64_t>::max()
	};

	std::vector<std::byte> buffer = makeBuffer(128);
	BitWriter writer(buffer);
	for (const std::uint64_t value : values) {
		REQUIRE(writer.writeVarUInt(value));
	}
	REQUIRE(writer.ok());

	BitReader reader(std::span<const std::byte>(buffer.data(), writer.byteSize()));
	for (const std::uint64_t expected : values) {
		std::uint64_t actual = 0;
		REQUIRE(reader.readVarUInt(actual));
		CHECK(actual == expected);
	}
	CHECK(reader.ok());
}

TEST_CASE("Reader rejects an overlong varint", "[net][bitstream]")
{
	std::array<std::byte, 10> buffer {};
	for (std::byte& value : buffer) value = std::byte { 0x80 };
	BitReader reader(buffer);
	std::uint64_t value = 0;
	CHECK_FALSE(reader.readVarUInt(value));
	CHECK(reader.status() == BitStreamStatus::InvalidArgument);
}

