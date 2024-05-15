#ifndef PIPELINE_H
#define PIPELINE_H
#include "Texture.hpp"
#include "UniformBuffer.hpp"
#include "VertexBuffer.hpp"
namespace MH33 {
namespace GFX {

class Pipeline : public GfxResource {
public:
	virtual ~Pipeline() = default;
	virtual unsigned getBindingPoint(const std::string& uniformName) const = 0;
	virtual void setUniform(unsigned bindingPoint, UniformBuffer& buffer) const = 0;
	virtual void setUniform(unsigned bindingPoint, Texture2D& texture) const = 0;
	virtual void setUniform(unsigned bindingPoint, TextureArray2D& texture) const = 0;
	virtual void setUniform(unsigned bindingPoint, Texture3D& texture) const = 0;
	virtual void setUniform(unsigned bindingPoint, Cubemap& texture) const = 0;
};

}
}
#endif // PIPELINE_H
