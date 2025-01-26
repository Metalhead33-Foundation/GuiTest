#ifndef EUPHDDS_HPP
#define EUPHDDS_HPP
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
