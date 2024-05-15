#ifndef MHPNG_HPP
#define MHPNG_HPP
#include <MhLib/Media/Image/MhImage.hpp>
#include <MhLib/Io/MhIoDevice.hpp>
#include <MhLib/Media/Image/MhDecodeTarget.hpp>
#include <memory>

namespace MH33 {
namespace Image {
namespace PNG
{
	void MH_IMAGE_API decode(Io::Device& iodev, DecodeTarget &destination);
	void MH_IMAGE_API decode(Io::System& iosys, const char* path, DecodeTarget &destination);
	void MH_IMAGE_API decode(Io::System& iosys, const std::string& path, DecodeTarget &destination);
	void MH_IMAGE_API encode(Io::Device& iodev, uint16_t width, uint16_t height, uint8_t color_type, uint8_t bit_depth, Util::Buffer &pixelData, float compressionLevel);
};

}
}

#endif // MHPNG_HPP
