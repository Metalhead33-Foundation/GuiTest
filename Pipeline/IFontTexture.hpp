#ifndef IFONTTEXTURE_HPP
#define IFONTTEXTURE_HPP
#include <Pipeline/ITexture.hpp>
#include <memory>

namespace SYS {

std::unique_ptr<ITexture> createFontTexture(bool accelerated);

}

#endif // IFONTTEXTURE_HPP
