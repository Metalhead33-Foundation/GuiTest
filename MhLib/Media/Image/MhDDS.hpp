#ifndef MHDDSHEADER_HPP
#define MHDDSHEADER_HPP
#include <MhLib/Media/Image/MhImage.hpp>
#include <MhLib/Io/MhDataStream.hpp>
#include <MhLib/Media/Image/MhDecodeTarget.hpp>

namespace MH33 {
namespace Image {
namespace DDS {

void MH_IMAGE_API decode(Io::Device& iodev, DecodeTarget &destination);
void MH_IMAGE_API decode(Io::System& iosys, const char* path, DecodeTarget &destination);
void MH_IMAGE_API decode(Io::System& iosys, const std::string& path, DecodeTarget &destination);

}
}
}
#endif // MHDDSHEADER_HPP
