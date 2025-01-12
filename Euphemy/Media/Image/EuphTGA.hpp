#ifndef EUPHTGA_HPP
#define EUPHTGA_HPP
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Euphemy/Config/EuphLib.hpp>
namespace Euph {
namespace Media {
namespace Image {
namespace TGA {

void MH_EUPH_API decode(Elv::Io::Device& iodev, DecodeTarget& destination);

}
}
}
}
#endif // EUPHTGA_HPP
