#ifndef MHARRAYTEXTURE2D_HPP
#define MHARRAYTEXTURE2D_HPP
#include <MhLib/Media/GFX/MhGFXHandle.hpp>
#include <MhLib/Media/Image/MhImageType.hpp>

namespace MH33 {
namespace GFX {

class TextureArray2D {
public:
	virtual unsigned getWidth() const = 0;
	virtual unsigned getHeight() const = 0;
	virtual unsigned getImageCount() const = 0;
	virtual Image::Format getFormat() const = 0;
	virtual Handle getNativeHandle() const = 0;

};

}
}
#endif // MHARRAYTEXTURE2D_HPP
