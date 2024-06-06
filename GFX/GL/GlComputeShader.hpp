#ifndef GLCOMPUTESHADER_HPP
#define GLCOMPUTESHADER_HPP
#include <GFX/Abstract/ComputeShader.hpp>
#include <GFX/GL/GlShaderModule.hpp>

namespace GL {
class ComputeShader : public MH33::GFX::ComputeShader
{
private:
	GLuint shaderProgram;
	ComputeShader(const ComputeShader& cpy) = delete;
	ComputeShader& operator=(const ComputeShader& cpy) = delete;
public:
	ComputeShader(const std::span<const MH33::GFX::ShaderModuleCreateInfo>& createInfo);
	ComputeShader(const std::span<const MH33::GFX::ShaderModuleCreateInfoRef>& createInfo);
	~ComputeShader();
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	void bind() const override;
	unsigned getBindingPoint(const std::string& uniformName) const override;
	unsigned getUniformBlockIndex(const std::string& uniformName) const override;
	void setUniform(unsigned bindingPoint, MH33::GFX::StorageBuffer& buffer, uint8_t unit) const override;
	void setUniform(unsigned bindingPoint, MH33::GFX::Texture2D& texture, uint8_t unit, MH33::GFX::ImageBindingType bindingType) const override;
	void setUniform(unsigned bindingPoint, MH33::GFX::TextureArray2D& texture, uint8_t unit, MH33::GFX::ImageBindingType bindingType) const override;
	void setUniform(unsigned bindingPoint, MH33::GFX::Texture3D& texture, uint8_t unit, MH33::GFX::ImageBindingType bindingType) const override;
	void setUniform(unsigned bindingPoint, MH33::GFX::Cubemap& texture, uint8_t unit, MH33::GFX::ImageBindingType bindingType) const override;
	void dispatchCompute(int x, int y, int z) override;
	void waitForFinish() const override;
};
}

#endif // GLCOMPUTESHADER_HPP
