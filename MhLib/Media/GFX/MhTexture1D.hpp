#ifndef MHTEXTURE1D_HPP
#define MHTEXTURE1D_HPP
#include <MhLib/Media/GFX/MhGFXHandle.hpp>
#include <MhLib/Media/Image/MhImageType.hpp>

namespace MH33 {
namespace GFX {

class Texture1D {
public:
	virtual unsigned getPixelCount() const = 0;
	virtual Image::Format getFormat() const = 0;
	virtual Handle getNativeHandle() const = 0;

};

}
}
#endif // MHTEXTURE1D_HPP
