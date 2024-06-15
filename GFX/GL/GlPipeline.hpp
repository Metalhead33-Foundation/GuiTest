#ifndef GLPIPELINE_HPP
#define GLPIPELINE_HPP
#include <GFX/Abstract/Pipeline.hpp>
#include <NativeGfxApi/glad/glwrap.h>
#include <vector>
#include <span>

namespace GL {

class Pipeline : public MH33::GFX::Pipeline
{
private:
	std::vector<GLuint> shaders;
	GLuint shaderProgram;
	Pipeline(const Pipeline& cpy) = delete;
	Pipeline& operator=(const Pipeline& cpy) = delete;
public:
	Pipeline(const std::span<const MH33::GFX::ShaderModuleCreateInfo>& createInfo);
	Pipeline(const std::span<const MH33::GFX::ShaderModuleCreateInfoRef>& createInfo);
	~Pipeline();
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	void bind() const override;
	unsigned getBindingPoint(const std::string& uniformName) const override;
	unsigned getUniformBlockIndex(const std::string& uniformName) const override;
	void setUniform(unsigned bindingPoint, MH33::GFX::StorageBuffer& buffer, uint8_t unit) const override;
	void setUniform(unsigned bindingPoint, MH33::GFX::Texture2D& texture, uint8_t unit) const override;
	void setUniform(unsigned bindingPoint, MH33::GFX::TextureArray2D& texture, uint8_t unit) const override;
	void setUniform(unsigned bindingPoint, MH33::GFX::Texture3D& texture, uint8_t unit) const override;
	void setUniform(unsigned bindingPoint, MH33::GFX::Cubemap& texture, uint8_t unit) const override;
	void draw(MH33::GFX::UnindexedVertexBuffer& vertices, MH33::GFX::RenderType geometryType, size_t offset, size_t count) override;
	void draw(MH33::GFX::IndexedVertexBuffer& vertices, MH33::GFX::RenderType geometryType, size_t offset, size_t count) override;
	void drawInstanced(uint32_t instances, MH33::GFX::UnindexedVertexBuffer& vertices, MH33::GFX::RenderType geometryType, size_t offset, size_t count) override;
	void drawInstanced(uint32_t instances, MH33::GFX::IndexedVertexBuffer& vertices, MH33::GFX::RenderType geometryType, size_t count) override;
};
}

#endif // GLPIPELINE_HPP
