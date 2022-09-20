#include "IFontTexture.hpp"
#include <Texture/StandardTexture.hpp>
#include <AGL/GlTexture2D.hpp>

namespace SYS {
std::unique_ptr<ITexture> createFontTexture(bool accelerated) {
	std::unique_ptr<ITexture> tex = nullptr;
	if(accelerated) {
		// GLint internalFormat, GLenum format, GLenum type
		return std::make_unique<GL::AcceleratedTexture>(std::make_unique<SoftwareRenderer::TexGreyscale_U8>(256,256),GLint(GL_ALPHA),GLenum(GL_ALPHA),GLenum(GL_UNSIGNED_BYTE));
	} else {
		return std::make_unique<SoftwareRenderer::TexGreyscale_U8>(256,256);
	}
}
}
