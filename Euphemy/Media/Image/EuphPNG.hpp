#ifndef EUPHPNG_HPP
#define EUPHPNG_HPP
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Euphemy/Config/EuphLib.hpp>
namespace Euph {
namespace Media {
namespace Image {
namespace PNG {

void MH_EUPH_API decode(Elv::Io::Device& iodev, DecodeTarget& destination);
void MH_EUPH_API encode(Elv::Io::Device& iodev, Format format, const Frame& frame, float compressionLevel);
void MH_EUPH_API encode(Elv::Io::Device& iodev, const DecodeTarget& source, float compressionLevel);

}
}
}
}
#endif // EUPHPNG_HPP
