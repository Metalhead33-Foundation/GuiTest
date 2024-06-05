#ifndef MHSTANDARDIMAGE2D_HPP
#define MHSTANDARDIMAGE2D_HPP
#include <MhLib/Media/Image/MhImage.hpp>
#include <MhLib/Media/Image/MhImage2D.hpp>
#include <MhLib/Util/MhPixelFormat.hpp>
#include <MhLib/Media/Image/MhDecodeTarget.hpp>
namespace MH33 {
namespace Image {

#ifndef MH33_IMG_LEAN_AND_MEAN
#include "MhStandardImage2D.ipp"
#endif

Image2D* MH_IMAGE_API createImage2D(Format format, unsigned width, unsigned height);
Image2D* MH_IMAGE_API createImage2D(const Frame& source, Format format);
Image2D* MH_IMAGE_API createImageReference2D(void* pixels, Format format, unsigned width, unsigned height);

}
}
#endif // MHSTANDARDIMAGE2D_HPP
