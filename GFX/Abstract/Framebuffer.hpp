#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP
#include "Texture.hpp"
namespace MH33 {
namespace GFX {
	DEFINE_CLASS(Framebuffer)
	class Framebuffer : public GfxResource {
		public:
		virtual ~Framebuffer() = default;
		virtual void bind() const = 0;
		virtual void unbind() const = 0;
		virtual uint8_t getAttachmentCount() const = 0;
		// Dimensions
		virtual unsigned getWidth() const = 0;
		virtual float getWidthF() const = 0;
		virtual float getWidthR() const = 0;
		virtual unsigned getHeight() const = 0;
		virtual float getHeightF() const = 0;
		virtual float getHeightR() const = 0;
		// Get attachment
		virtual Texture2D& getTextureAttachment2D(uint8_t unit) = 0;
		virtual const Texture2D& getTextureAttachment2D(uint8_t unit) const = 0;
	};

	DEFINE_CLASS(CubemapFramebuffer)
	class CubemapFramebuffer : public GfxResource {
		public:
		virtual ~CubemapFramebuffer() = default;
		virtual void bind() const = 0;
		virtual void unbind() const = 0;
		virtual uint8_t getAttachmentCount() const = 0;
		// Dimensions
		virtual unsigned getWidth() const = 0;
		virtual float getWidthF() const = 0;
		virtual float getWidthR() const = 0;
		virtual unsigned getHeight() const = 0;
		virtual float getHeightF() const = 0;
		virtual float getHeightR() const = 0;
		// Get attachment
		virtual Cubemap& getCubemapAttachment(uint8_t unit) = 0;
		virtual Cubemap& getCubemapAttachment(uint8_t unit) const = 0;
	};

}
}
#endif // FRAMEBUFFER_HPP
