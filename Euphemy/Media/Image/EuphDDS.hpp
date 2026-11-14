#ifndef EUPHDDS_HPP
#define EUPHDDS_HPP
/**
 * @file EuphDDS.hpp
 * @brief Declares the EuphDDS API in the Euphemy/Media/Image module.
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
namespace DDS {

/**
 * @brief Decodes a DDS image.
 * @param iodev The IO device to read the DDS image from.
 * @param destination Decode target, where the image will be stored at.
 */
void MH_EUPH_API decode(Elv::Io::Device& iodev, DecodeTarget& destination);

}
}
}
}
#endif // EUPHDDS_HPP
