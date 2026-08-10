#ifndef EUPHWEBP_HPP
#define EUPHWEBP_HPP
/**
 * @file EuphWEBP.hpp
 * @brief Declares the EuphWEBP API in the Euphemy/Media/Image module.
 *
 * This header is part of the public declaration surface for Euphemy/Media/Image.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Euphemy/Config/EuphLib.hpp>
namespace Euph {
namespace Media {
namespace Image {
namespace WEBP {

/**
 * @brief Decodes data from a source buffer into a DecodeTarget.
 *
 * This function decodes the data contained in the provided source buffer
 * and stores the result in the given DecodeTarget object.
 *
 * @param srcBuffer The source buffer containing the encoded data.
 * @param destination The DecodeTarget object where the decoded data will be stored.
 * @return True if the decoding was successful, false otherwise.
 */
bool MH_EUPH_API decode(const std::span<const std::byte>& srcBuffer, DecodeTarget& destination);

/**
 * @brief Decodes data from an input/output device into a DecodeTarget.
 *
 * This function reads encoded data from the provided input/output device
 * and stores the result in the given DecodeTarget object.
 *
 * @param iodev The input/output device containing the encoded data.
 * @param destination The DecodeTarget object where the decoded data will be stored.
 * @return True if the decoding was successful, false otherwise.
 */
bool MH_EUPH_API decode(Elv::Io::Device& iodev, DecodeTarget& destination);

/**
 * @brief Encodes a frame into a buffer with specified format and compression level.
 *
 * This function encodes the provided frame using the specified format and compression level.
 * The resulting encoded data is stored in a dynamically allocated buffer pointed to by `ptr`.
 * The caller is responsible for freeing this buffer after use.
 *
 * @param frame The frame to be encoded.
 * @param format The format to use for encoding.
 * @param compressionLevel The level of compression to apply (0.0 for no compression, 1.0 for maximum compression).
 * @param ptr Pointer to the dynamically allocated buffer where the encoded data will be stored.
 * @return The size of the encoded data in bytes.
 */
size_t MH_EUPH_API encode(const Frame& frame, Format format, float compressionLevel, uint8_t** ptr);

/**
 * @brief Encodes a frame into an output/input device with specified format and compression level.
 *
 * This function encodes the provided frame using the specified format and compression level
 * and writes the resulting encoded data to the provided output/input device.
 *
 * @param iodev The output/input device where the encoded data will be written.
 * @param frame The frame to be encoded.
 * @param format The format to use for encoding.
 * @param compressionLevel The level of compression to apply (0.0 for no compression, 1.0 for maximum compression).
 * @return True if the encoding was successful, false otherwise.
 */
bool MH_EUPH_API encode(Elv::Io::Device& iodev, const Frame& frame, Format format, float compressionLevel);

/**
 * @brief Encodes a DecodeTarget into an output/input device with specified compression level.
 *
 * This function encodes the data contained in the provided DecodeTarget
 * using the specified compression level and writes the resulting encoded data
 * to the provided output/input device.
 *
 * @param iodev The output/input device where the encoded data will be written.
 * @param source The DecodeTarget containing the data to be encoded.
 * @param compressionLevel The level of compression to apply (0.0 for no compression, 1.0 for maximum compression).
 * @return True if the encoding was successful, false otherwise.
 */
bool MH_EUPH_API encode(Elv::Io::Device& iodev, const DecodeTarget& source, float compressionLevel);

/**
 * @brief Demultiplexes data from a source buffer into a DecodeTarget.
 *
 * This function demultiplexes the data contained in the provided source buffer
 * and stores the result in the given DecodeTarget object.
 *
 * @param srcBuffer The source buffer containing the multiplexed data.
 * @param destination The DecodeTarget object where the demultiplexed data will be stored.
 * @return True if the demultiplexing was successful, false otherwise.
 */
bool MH_EUPH_API demux(const std::span<const std::byte>& srcBuffer, DecodeTarget& destination);

/**
 * @brief Demultiplexes data from an input/output device into a DecodeTarget.
 *
 * This function reads multiplexed data from the provided input/output device
 * and stores the result in the given DecodeTarget object.
 *
 * @param iodev The input/output device containing the multiplexed data.
 * @param destination The DecodeTarget object where the demultiplexed data will be stored.
 * @return True if the demultiplexing was successful, false otherwise.
 */
bool MH_EUPH_API demux(Elv::Io::Device& iodev, DecodeTarget& destination);

}
}
}
}
#endif // EUPHWEBP_HPP
