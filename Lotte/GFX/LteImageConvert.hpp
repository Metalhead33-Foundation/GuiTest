#ifndef LTEIMAGECONVERT_HPP
#define LTEIMAGECONVERT_HPP
/**
 * @file LteImageConvert.hpp
 * @brief CPU-side conversion of Euphemy decoded images into the RGBA8 layout GUI work needs.
 *
 * Textures that the GUI mutates (cursor images, glyph atlas pages) all share one
 * CPU layout: tightly packed, 8-bit, straight-alpha RGBA. Euphemy decodes into
 * whatever the container stores (PNG usually yields `RGBA8U`, TGA `BGRA8U`), so
 * the bridge belongs in Lotte, next to the rest of the Euphemy/Kaldi glue.
 *
 * The helpers are header-only and throw `std::invalid_argument` for unsupported
 * formats rather than silently producing a wrong-looking image: a caller that
 * feeds a compressed or floating-point image here has a bug, not a fallback case.
 */
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Euphemy/Media/Image/EuphImageType.hpp>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace Lotte {
namespace Gfx {

/** @brief Tightly packed RGBA8 image produced by @ref toRgba8(). */
struct Rgba8Image {
	/** @brief Width in pixels. */
	std::uint16_t width = 0;
	/** @brief Height in pixels. */
	std::uint16_t height = 0;
	/** @brief `width * height * 4` bytes of straight-alpha RGBA data. */
	std::vector<std::uint8_t> pixels;

	/** @brief Bytes in one row of @ref pixels. */
	std::size_t rowBytes() const noexcept { return std::size_t(width) * 4u; }
	/** @brief True when the image has dimensions and matching storage. */
	bool isValid() const noexcept
	{
		return width != 0 && height != 0 &&
			pixels.size() == std::size_t(width) * std::size_t(height) * 4u;
	}
};

/** @brief Bytes per pixel of the Kaldi/Euphemy format @p format. */
inline std::size_t bytesPerPixel(Euph::Media::Image::Format format) noexcept
{
	return Euph::Media::Image::pixelByteSize(format);
}

/**
 * @brief True when @ref toRgba8() understands @p format.
 *
 * Only the 8-bit unsigned layouts the GUI actually consumes are accepted.
 * Block-compressed and high-precision formats are reported as unsupported so a
 * caller can pick a different upload path instead of getting a wrong image.
 */
inline bool isRgba8Convertible(Euph::Media::Image::Format format) noexcept
{
	using Euph::Media::Image::Format;
	switch (format) {
		case Format::RGBA8U:
		case Format::BGRA8U:
		case Format::RGB8U:
		case Format::BGR8U:
		case Format::RG8U:
		case Format::R8U:
			return true;
		default:
			return false;
	}
}

/**
 * @brief Converts the first frame of @p source into tightly packed RGBA8.
 *
 * Channels the source does not have are filled with an opaque value: greyscale
 * and two-channel images get `255` in every missing colour channel, and formats
 * without alpha get `255` in alpha.
 *
 * @param source Decoded image; only frame 0 is read.
 * @return The converted image, never empty on success.
 * @throws std::invalid_argument When the image has no frames, uses an
 *         unsupported format, has zero/oversized extents, or the frame storage
 *         is shorter than its declared stride and height.
 */
inline Rgba8Image toRgba8(const Euph::Media::Image::DecodeTarget& source)
{
	using namespace Euph::Media::Image;
	if (source.getFrames().empty()) {
		throw std::invalid_argument("toRgba8: the decode target has no frames");
	}
	const Format format = source.getFormat();
	if (!isRgba8Convertible(format)) {
		throw std::invalid_argument("toRgba8: unsupported source format " +
			std::to_string(static_cast<unsigned>(format)));
	}
	const Frame& frame = source.getFrame(0);
	if (frame.width == 0 || frame.height == 0) {
		throw std::invalid_argument("toRgba8: the frame has zero extents");
	}
	if (frame.width > 0xFFFFu || frame.height > 0xFFFFu) {
		throw std::invalid_argument("toRgba8: the frame exceeds the 16-bit extent limit");
	}
	const std::size_t sourceStride = frame.stride != 0
		? std::size_t(frame.stride)
		: std::size_t(frame.width) * bytesPerPixel(format);
	if (frame.data.size() < sourceStride * std::size_t(frame.height)) {
		throw std::invalid_argument("toRgba8: the frame is shorter than its declared stride");
	}

	Rgba8Image result;
	result.width = static_cast<std::uint16_t>(frame.width);
	result.height = static_cast<std::uint16_t>(frame.height);
	result.pixels.resize(std::size_t(frame.width) * std::size_t(frame.height) * 4u);

	const auto* sourceBytes = reinterpret_cast<const std::uint8_t*>(frame.data.data());
	std::uint8_t* destination = result.pixels.data();
	for (unsigned y = 0; y < frame.height; ++y) {
		const std::uint8_t* row = sourceBytes + std::size_t(y) * sourceStride;
		for (unsigned x = 0; x < frame.width; ++x) {
			std::uint8_t r = 0;
			std::uint8_t g = 0;
			std::uint8_t b = 0;
			std::uint8_t a = 255;
			switch (format) {
				case Format::RGBA8U:
					r = row[0]; g = row[1]; b = row[2]; a = row[3];
					break;
				case Format::BGRA8U:
					b = row[0]; g = row[1]; r = row[2]; a = row[3];
					break;
				case Format::RGB8U:
					r = row[0]; g = row[1]; b = row[2];
					break;
				case Format::BGR8U:
					b = row[0]; g = row[1]; r = row[2];
					break;
				case Format::RG8U:
					r = row[0]; g = row[1];
					break;
				case Format::R8U:
					r = g = b = row[0];
					break;
				default:
					// isRgba8Convertible() already rejected everything else.
					break;
			}
			destination[0] = r;
			destination[1] = g;
			destination[2] = b;
			destination[3] = a;
			row += bytesPerPixel(format);
			destination += 4;
		}
	}
	return result;
}

} // namespace Gfx
} // namespace Lotte

#endif // LTEIMAGECONVERT_HPP
