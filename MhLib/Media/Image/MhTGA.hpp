#ifndef MHTGAHEADER_HPP
#define MHTGAHEADER_HPP
#include <MhLib/Media/Image/MhImage.hpp>
#include <MhLib/Io/MhDataStream.hpp>
#include <MhLib/Media/Image/MhDecodeTarget.hpp>

namespace MH33 {
namespace Image {
namespace TGA {

void MH_IMAGE_API decode(Io::Device& iodev, DecodeTarget &destination);
void MH_IMAGE_API decode(Io::System& iosys, const char* path, DecodeTarget &destination);
void MH_IMAGE_API decode(Io::System& iosys, const std::string& path, DecodeTarget &destination);

}
}
}
#endif // MHTGAHEADER_HPP
