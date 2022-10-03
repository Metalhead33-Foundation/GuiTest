#ifndef MHCUBEMAP_HPP
#define MHCUBEMAP_HPP
#include <MhLib/Media/GFX/MhGFXHandle.hpp>
#include <MhLib/Media/Image/MhImageType.hpp>

namespace MH33 {
namespace GFX {

class Cubemap {
public:
	virtual unsigned getWidth() const = 0;
	virtual unsigned getHeight() const = 0;
	virtual Image::Format getFormat() const = 0;
	virtual Handle getNativeHandle() const = 0;

};

}
}
#endif // MHCUBEMAP_HPP
