#ifndef ITEXTUREATLAS_HPP
#define ITEXTUREATLAS_HPP
#include <Pipeline/ITexture.hpp>

namespace SYS {
class ITextureAtlas {
public:
	virtual ~ITextureAtlas() = default;
	virtual ITexture* getTexture() = 0;
	virtual const ITexture* getTexture() const = 0;
};
}

#endif // ITEXTUREATLAS_HPP
