#ifndef EUPHJPEG_HPP
#define EUPHJPEG_HPP
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Euphemy/Config/EuphLib.hpp>
#include <functional>
namespace Euph {
namespace Media {
namespace Image {
namespace JPEG {

bool MH_EUPH_API decode(const std::span<const std::byte>& inbuf, DecodeTarget& destination);
bool MH_EUPH_API decode(Elv::Io::Device& iodev, DecodeTarget& destination);
bool MH_EUPH_API encode(const Frame& frame, Format format,
						const std::function<std::span<std::byte>(unsigned long)>& allocator,
						int jpegSubsamp, float jpegQual, unsigned long &jpegSize);
bool MH_EUPH_API encode(const DecodeTarget& source,
						const std::function<std::span<std::byte>(unsigned long)>& allocator,
						int jpegSubsamp, float jpegQual, unsigned long &jpegSize);
bool MH_EUPH_API encode(Elv::Io::Device& iodev, const Frame& frame, Format format, int jpegSubsamp, float jpegQual);
bool MH_EUPH_API encode(Elv::Io::Device& iodev, const DecodeTarget& source, int jpegSubsamp, float jpegQual);

}
}
}
}
#endif // EUPHJPEG_HPP
