#ifndef ITEXTURE_HPP
#define ITEXTURE_HPP
#include <MhLib/Media/GFX/MhTexture.hpp>
namespace SYS {
typedef MH33::GFX::WriteableTexture2D ITexture;
typedef std::shared_ptr<ITexture> sTexture;
}
#endif // ITEXTURE_HPP
