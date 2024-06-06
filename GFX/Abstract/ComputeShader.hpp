#ifndef COMPUTESHADER_HPP
#define COMPUTESHADER_HPP
#include "Texture.hpp"
#include "UniformBuffer.hpp"
#include <MhLib/Util/MhGlobals.hpp>
namespace MH33 {
namespace GFX {
	DEFINE_CLASS(ComputeShader)
	class ComputeShader : public GfxResource {
		public:
		virtual ~ComputeShader() = default;
		virtual void bind() const = 0;
		virtual unsigned getBindingPoint(const std::string& uniformName) const = 0;
		virtual unsigned getUniformBlockIndex(const std::string& uniformName) const = 0;
		virtual void setUniform(unsigned bindingPoint, StorageBuffer& buffer, uint8_t unit) const = 0;
		virtual void setUniform(unsigned bindingPoint, Texture2D& texture, uint8_t unit, ImageBindingType bindingType) const = 0;
		virtual void setUniform(unsigned bindingPoint, TextureArray2D& texture, uint8_t unit, ImageBindingType bindingType) const = 0;
		virtual void setUniform(unsigned bindingPoint, Texture3D& texture, uint8_t unit, ImageBindingType bindingType) const = 0;
		virtual void setUniform(unsigned bindingPoint, Cubemap& texture, uint8_t unit, ImageBindingType bindingType) const = 0;
		virtual void dispatchCompute(int x, int y, int z) = 0;
		virtual void waitForFinish() const = 0;
	};

}
}
#endif // COMPUTESHADER_HPP
