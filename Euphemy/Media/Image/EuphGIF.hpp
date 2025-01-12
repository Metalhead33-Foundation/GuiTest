#ifndef EUPHGIF_HPP
#define EUPHGIF_HPP
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Euphemy/Config/EuphLib.hpp>
namespace Euph {
namespace Media {
namespace Image {
namespace GIF {

void MH_EUPH_API decode(Elv::Io::Device& iodev, DecodeTarget& destination);

}
}
}
}
#endif // EUPHGIF_HPP
