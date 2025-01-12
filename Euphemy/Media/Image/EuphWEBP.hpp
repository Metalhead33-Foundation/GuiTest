#ifndef EUPHWEBP_HPP
#define EUPHWEBP_HPP
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Euphemy/Config/EuphLib.hpp>
namespace Euph {
namespace Media {
namespace Image {
namespace WEBP {

bool MH_EUPH_API decode(const std::span<const std::byte>& srcBuffer, DecodeTarget& destination);
bool MH_EUPH_API decode(Elv::Io::Device& iodev, DecodeTarget& destination);
size_t MH_EUPH_API encode(const Frame& frame, Format format, float compressionLevel, uint8_t** ptr);
bool MH_EUPH_API encode(Elv::Io::Device& iodev, const Frame& frame, Format format, float compressionLevel);
bool MH_EUPH_API encode(Elv::Io::Device& iodev, const DecodeTarget& source, float compressionLevel);

bool MH_EUPH_API demux(const std::span<const std::byte>& srcBuffer, DecodeTarget& destination);
bool MH_EUPH_API demux(Elv::Io::Device& iodev, DecodeTarget& destination);

}
}
}
}
#endif // EUPHWEBP_HPP
