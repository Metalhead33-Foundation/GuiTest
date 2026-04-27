#ifndef EUPHPNG_HPP
#define EUPHPNG_HPP
/**
 * @file EuphPNG.hpp
 * @brief Declares the EuphPNG API in the Euphemy/Media/Image module.
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
namespace PNG {

/**
 * @brief Decodes a PNG image.
 * @param iodev The IO device to read the PNG image from.
 * @param destination Decode target, where the image will be stored at.
 */
void MH_EUPH_API decode(Elv::Io::Device& iodev, DecodeTarget& destination);
/**
 * @brief Encodes a PNG image.
 * @param iodev The IO device used to save the PNG image.
 * @param format Format of the image. Only a limited number of formats are accepted for PNG.
 * @param frame The image frame to encode into as a PNG file.
 * @param compressionLevel Image compression level. Higher compression levels won't affect quality (as PNG is lossless), but will have an impact on performance.
 */
void MH_EUPH_API encode(Elv::Io::Device& iodev, Format format, const Frame& frame, float compressionLevel);
/**
 * @brief Encodes a PNG image.
 * @param iodev The IO device used to save the PNG image.
 * @param source The image to encode into as a PNG file.
 * @param compressionLevel Image compression level. Higher compression levels won't affect quality (as PNG is lossless), but will have an impact on performance.
 */
void MH_EUPH_API encode(Elv::Io::Device& iodev, const DecodeTarget& source, float compressionLevel);

}
}
}
}
#endif // EUPHPNG_HPP
