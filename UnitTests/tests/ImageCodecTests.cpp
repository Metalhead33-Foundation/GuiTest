/**
 * @file ImageCodecTests.cpp
 * @brief Round-trip tests for the lossless image codecs.
 *
 * Only the lossless containers are tested here: PNG and TGA must reproduce the exact pixels they
 * were given. JPEG and WebP are lossy (and their encoder entry points take buffers rather than
 * devices), so they stay in the manual sandbox.
 */
#include <catch2/catch_test_macros.hpp>
#include <Euphemy/Io/EuphMemoryDevice.hpp>
#include <Euphemy/Media/Image/EuphPNG.hpp>
#include <Euphemy/Media/Image/EuphPixelFormat.hpp>
#include <Euphemy/Media/Image/EuphTGA.hpp>
#include <cstdint>
#include <vector>

namespace {

/** @brief Width of the generated test image; deliberately odd to catch stride assumptions. */
constexpr unsigned testWidth = 23;
/** @brief Height of the generated test image. */
constexpr unsigned testHeight = 17;

/** @brief Builds an RGBA8 image with a deterministic, alpha-varying pattern. */
Euph::Media::Image::DecodeTarget makeTestImage()
{
	using namespace Euph::Media::Image;
	DecodeTarget target;
	target.setFormat(Format::RGBA8U);
	Frame& frame = target.addFrame(testWidth, testHeight);
	const std::span<PixelRGBA_U8> pixels = frame.asDataSpan<PixelRGBA_U8>();
	for (unsigned y = 0; y < testHeight; ++y) {
		for (unsigned x = 0; x < testWidth; ++x) {
			PixelRGBA_U8& pixel = pixels[y * testWidth + x];
			pixel.r = static_cast<std::uint8_t>((x * 7) & 0xFF);
			pixel.g = static_cast<std::uint8_t>((y * 11) & 0xFF);
			pixel.b = static_cast<std::uint8_t>((x * y) & 0xFF);
			pixel.a = static_cast<std::uint8_t>(x == y ? 255 : 128);
		}
	}
	return target;
}

}

TEST_CASE("PNG survives an encode/decode round-trip", "[image][png][roundtrip]")
{
	using namespace Euph::Media::Image;

	const DecodeTarget source = makeTestImage();
	Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
	PNG::encode(device, source, 0.5f);
	REQUIRE(device.size() > 0);
	REQUIRE(device.seek(0, Elv::Io::SeekOrigin::SET) == 0);

	DecodeTarget decoded;
	PNG::decode(device, decoded);

	REQUIRE(decoded.getFrames().size() == 1);
	CHECK(decoded.getFormat() == Format::RGBA8U);
	const Frame& frame = decoded.getFrame(0);
	REQUIRE(frame.width == testWidth);
	REQUIRE(frame.height == testHeight);
	CHECK(frame.data == source.getFrame(0).data);
}

TEST_CASE("TGA survives an encode/decode round-trip", "[image][tga][roundtrip]")
{
	using namespace Euph::Media::Image;

	// TGA has no RGBA8U writer (it stores BGRA for 32-bit data), so the source is built in the
	// format the encoder actually supports.
	DecodeTarget source;
	source.setFormat(Format::BGRA8U);
	Frame& sourceFrame = source.addFrame(testWidth, testHeight);
	const std::span<PixelBGRA_U8> pixels = sourceFrame.asDataSpan<PixelBGRA_U8>();
	for (unsigned y = 0; y < testHeight; ++y) {
		for (unsigned x = 0; x < testWidth; ++x) {
			PixelBGRA_U8& pixel = pixels[y * testWidth + x];
			pixel.r = static_cast<std::uint8_t>((x * 7) & 0xFF);
			pixel.g = static_cast<std::uint8_t>((y * 11) & 0xFF);
			pixel.b = static_cast<std::uint8_t>((x * y) & 0xFF);
			pixel.a = static_cast<std::uint8_t>(x == y ? 255 : 128);
		}
	}
	const std::vector<std::byte> expected(sourceFrame.data.begin(), sourceFrame.data.end());

	Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
	TGA::encode(device, source);
	REQUIRE(device.size() > 0);
	REQUIRE(device.seek(0, Elv::Io::SeekOrigin::SET) == 0);

	DecodeTarget decoded;
	TGA::decode(device, decoded);

	REQUIRE(decoded.getFrames().size() == 1);
	CHECK(decoded.getFormat() == Format::BGRA8U);
	const Frame& frame = decoded.getFrame(0);
	REQUIRE(frame.width == testWidth);
	REQUIRE(frame.height == testHeight);
	REQUIRE(frame.data.size() == expected.size());
	CHECK(std::vector<std::byte>(frame.data.begin(), frame.data.end()) == expected);
}

TEST_CASE("Encoded images are stable across repeated round-trips", "[image][png][tga][roundtrip]")
{
	using namespace Euph::Media::Image;

	const DecodeTarget first = makeTestImage();
	std::vector<std::byte> firstEncoding;
	{
		Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
		PNG::encode(device, first, 0.5f);
		device.seek(0, Elv::Io::SeekOrigin::SET);
		std::vector<std::byte> buffer(device.size());
		REQUIRE(device.read(buffer.data(), 1, buffer.size()) == buffer.size());
		firstEncoding = std::move(buffer);
	}

	Euph::Io::MemoryDevice firstDevice(Elv::Io::Mode::READ, std::vector<std::byte>(firstEncoding));
	DecodeTarget decoded;
	PNG::decode(firstDevice, decoded);
	REQUIRE(decoded.getFrames().size() == 1);
	CHECK(decoded.getFormat() == Format::RGBA8U);

	Euph::Io::MemoryDevice secondDevice(Elv::Io::Mode::READ_WRITE);
	PNG::encode(secondDevice, decoded, 0.5f);
	REQUIRE(secondDevice.size() == firstEncoding.size());
	secondDevice.seek(0, Elv::Io::SeekOrigin::SET);
	std::vector<std::byte> secondEncoding(secondDevice.size());
	REQUIRE(secondDevice.read(secondEncoding.data(), 1, secondEncoding.size()) == secondEncoding.size());
	CHECK(secondEncoding == firstEncoding);
}
