#ifndef GLPIPELINE_HPP
#define GLPIPELINE_HPP
#include <GFX/Abstract/Pipeline.hpp>
#include <NativeGfxApi/glad/glwrap.h>
#include <vector>
#include <exception>
#include <span>

namespace GL {

struct ShaderModuleCreateInfo {
	GLenum shaderType;
	std::string sourceCode;
};

class ShaderError : public std::exception {
private:
	std::string errStr;
public:
	ShaderError(GLuint shaderId, bool isProgram = false);
	const char* what() const noexcept override;
};

class ShaderModule {
private:
	GLuint shaderModule;
	ShaderModule(const ShaderModule& cpy) = delete;
	ShaderModule& operator=(const ShaderModule& cpy) = delete;
public:
	ShaderModule();
	ShaderModule(ShaderModule&& mov);
	ShaderModule& operator=(ShaderModule&& mov);
	ShaderModule(const ShaderModuleCreateInfo& createInfo);
	~ShaderModule();
	GLuint getId() const;
	operator GLuint() const;
};

class Pipeline : public MH33::GFX::Pipeline
{
private:
	std::vector<GLuint> shaders;
	GLuint shaderProgram;
	Pipeline(const Pipeline& cpy) = delete;
	Pipeline& operator=(const Pipeline& cpy) = delete;
public:
	Pipeline(const std::span<const ShaderModuleCreateInfo>& createInfo);
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
};
}

#endif // GLPIPELINE_HPP
