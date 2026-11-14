/**
 * @file CompressionTests.cpp
 * @brief Round-trip tests for the streaming compression devices.
 *
 * Every test writes a payload through a compressing proxy device into an in-memory device, rewinds
 * that device, and reads it back through the matching decompressing proxy. The assertion is always
 * "the bytes that come out are exactly the bytes that went in", which is the property the asset
 * pipeline depends on.
 */
#include <catch2/catch_test_macros.hpp>
#include <Euphemy/Io/EuphMemoryDevice.hpp>
#include <Euphemy/Io/EuphSnappy.hpp>
#include <Euphemy/Io/EuphZstd.hpp>
#include "support/TestSupport.hpp"
#include <cstddef>
#include <vector>

using namespace UnitTests;

namespace {

/** @brief Payload sizes covering empty input, chunk edges and multi-chunk streams. */
const std::vector<size_t> roundTripSizes { 0, 1, 63, 64, 65, 4095, 4096, 65535, 65536, 65537, 200000 };

}

TEST_CASE("Snappy proxy devices round-trip arbitrary payloads", "[compression][snappy]")
{
	for (const size_t size : roundTripSizes) {
		CAPTURE(size);
		const std::vector<std::byte> original = makePattern(size);
		Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
		{
			Euph::Io::SnappyCompressor compressor(&device);
			if (!original.empty())
				REQUIRE(compressor.write(original.data(), 1, original.size()) == original.size());
		}
		REQUIRE(device.seek(0, Elv::Io::SeekOrigin::SET) == 0);

		Euph::Io::SnappyDecompressor decompressor(&device);
		REQUIRE(readToEnd(decompressor) == original);
	}
}

TEST_CASE("Snappy proxy devices round-trip chunk sizes other than the default", "[compression][snappy]")
{
	for (const size_t chunkSize : { size_t(1), size_t(7), size_t(1024) }) {
		CAPTURE(chunkSize);
		const std::vector<std::byte> original = makeCompressible(3000);
		Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
		{
			Euph::Io::SnappyCompressor compressor(&device, chunkSize);
			REQUIRE(compressor.write(original.data(), original.size(), 1) == 1);
		}
		REQUIRE(device.seek(0, Elv::Io::SeekOrigin::SET) == 0);

		Euph::Io::SnappyDecompressor decompressor(&device, 65536);
		REQUIRE(readToEnd(decompressor, 33) == original);
	}
}

TEST_CASE("Snappy compressor writes a readable empty stream", "[compression][snappy]")
{
	Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
	{
		Euph::Io::SnappyCompressor compressor(&device);
		REQUIRE(compressor.write(nullptr, 0, 0) == 0);
	}
	REQUIRE(device.size() == 5); // magic plus version, no chunks
	REQUIRE(device.seek(0, Elv::Io::SeekOrigin::SET) == 0);

	Euph::Io::SnappyDecompressor decompressor(&device);
	REQUIRE(readToEnd(decompressor).empty());
}

TEST_CASE("Snappy decompressor rejects data it did not produce", "[compression][snappy]")
{
	const std::vector<std::byte> garbage = makePattern(64);
	Euph::Io::MemoryDevice device(Elv::Io::Mode::READ, std::vector<std::byte>(garbage));

	Euph::Io::SnappyDecompressor decompressor(&device);
	REQUIRE_THROWS(readToEnd(decompressor));
}

TEST_CASE("Snappy decompressor rejects a truncated chunk", "[compression][snappy]")
{
	const std::vector<std::byte> original = makeCompressible(4096);
	Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
	{
		Euph::Io::SnappyCompressor compressor(&device, 4096);
		REQUIRE(compressor.write(original.data(), original.size(), 1) == 1);
	}

	REQUIRE(device.seek(0, Elv::Io::SeekOrigin::SET) == 0);
	std::vector<std::byte> stored = readToEnd(device);
	REQUIRE(stored.size() > 12);
	stored.resize(stored.size() - 8); // cut the tail off the single chunk

	Euph::Io::MemoryDevice truncated(Elv::Io::Mode::READ, std::move(stored));
	Euph::Io::SnappyDecompressor decompressor(&truncated);
	REQUIRE_THROWS(readToEnd(decompressor));
}

TEST_CASE("Zstd proxy devices round-trip arbitrary payloads", "[compression][zstd]")
{
	for (const size_t size : roundTripSizes) {
		CAPTURE(size);
		const std::vector<std::byte> original = makePattern(size, 0x12345678u);
		Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
		{
			Euph::Io::ZstdCompressor compressor(&device);
			if (!original.empty())
				REQUIRE(compressor.write(original.data(), 1, original.size()) == original.size());
		}
		REQUIRE(device.seek(0, Elv::Io::SeekOrigin::SET) == 0);

		Euph::Io::ZstdDecompressor decompressor(&device);
		REQUIRE(readToEnd(decompressor, 1024) == original);
	}
}

TEST_CASE("Zstd checksum and level settings do not change the payload", "[compression][zstd]")
{
	const std::vector<std::byte> original = makeCompressible(150000);
	Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
	{
		Euph::Io::ZstdCompressor compressor(&device);
		compressor.setCompressionLevel(1.0f);
		compressor.setChecksum(true);
		REQUIRE(compressor.write(original.data(), original.size(), 1) == 1);
	}
	REQUIRE(device.seek(0, Elv::Io::SeekOrigin::SET) == 0);

	Euph::Io::ZstdDecompressor decompressor(&device);
	REQUIRE(readToEnd(decompressor) == original);
}

TEST_CASE("Compression layers nest without losing bytes", "[compression][zstd][snappy]")
{
	// Snappy-compress first, then Zstd-compress the result, then unwind in the same order.
	const std::vector<std::byte> original = makePattern(70000, 0xABCDEF01u);

	Euph::Io::MemoryDevice snappyStage(Elv::Io::Mode::READ_WRITE);
	{
		Euph::Io::SnappyCompressor compressor(&snappyStage);
		REQUIRE(compressor.write(original.data(), original.size(), 1) == 1);
	}
	REQUIRE(snappyStage.seek(0, Elv::Io::SeekOrigin::SET) == 0);
	const std::vector<std::byte> snappyBytes = readToEnd(snappyStage);

	Euph::Io::MemoryDevice zstdStage(Elv::Io::Mode::READ_WRITE);
	{
		Euph::Io::ZstdCompressor compressor(&zstdStage);
		REQUIRE(compressor.write(snappyBytes.data(), snappyBytes.size(), 1) == 1);
	}
	REQUIRE(zstdStage.seek(0, Elv::Io::SeekOrigin::SET) == 0);

	Euph::Io::ZstdDecompressor zstdDecompressor(&zstdStage);
	const std::vector<std::byte> unwrappedSnappy = readToEnd(zstdDecompressor);
	REQUIRE(unwrappedSnappy == snappyBytes);

	Euph::Io::MemoryDevice snappyRead(Elv::Io::Mode::READ, std::vector<std::byte>(unwrappedSnappy));
	Euph::Io::SnappyDecompressor snappyDecompressor(&snappyRead);
	REQUIRE(readToEnd(snappyDecompressor) == original);
}
