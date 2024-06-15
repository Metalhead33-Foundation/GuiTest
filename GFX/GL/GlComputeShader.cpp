#include "GlComputeShader.hpp"
//#include <MhLib/Util/MhStaticStackAllocator.hpp>
#include <MhLib/Util/MhStackAllocator.hpp>
/*typedef MH33::Util::StaticStackAllocator<GL::ShaderModule,16,1> GlShaderModuleAllocator;
template<> size_t GlShaderModuleAllocator::offset = 0;
template<> std::array<GL::ShaderModule, 16> GlShaderModuleAllocator::buffer = {};*/
typedef MH33::Util::StackAllocator<GL::ShaderModule, 16> GlShaderModuleAllocator;

namespace GL {

ComputeShader::ComputeShader(const std::span<const MH33::GFX::ShaderModuleCreateInfo>& createInfo)
{
	if(!createInfo.size()) throw std::runtime_error("No shader source files!");
	if(createInfo.size() > 16) throw std::runtime_error("Too many shader stages!");
	std::vector<ShaderModule, GlShaderModuleAllocator> modules;
	for(const auto& it : createInfo) {
		modules.push_back(ShaderModule(it));
	}
	shaderProgram = glCreateProgram();
	GLint success;
	for(const auto& it : modules) {
		glAttachShader(shaderProgram, it.getId());
	}
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if(!success) {
		throw ShaderError(shaderProgram, true);
	}
}

ComputeShader::ComputeShader(const std::span<const MH33::GFX::ShaderModuleCreateInfoRef>& createInfo)
{
	if(!createInfo.size()) throw std::runtime_error("No shader source files!");
	if(createInfo.size() > 16) throw std::runtime_error("Too many shader stages!");
	std::vector<ShaderModule, GlShaderModuleAllocator> modules;
	for(const auto& it : createInfo) {
		modules.push_back(ShaderModule(it));
	}
	shaderProgram = glCreateProgram();
	GLint success;
	for(const auto& it : modules) {
		glAttachShader(shaderProgram, it.getId());
	}
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if(!success) {
		throw ShaderError(shaderProgram, true);
	}
}

ComputeShader::~ComputeShader()
{
	if(shaderProgram) glDeleteProgram(shaderProgram);
}

MH33::GFX::Handle ComputeShader::getNativeHandle()
{
	return { .oglUint = shaderProgram };
}

MH33::GFX::ConstHandle ComputeShader::getNativeHandle() const
{
	return { .oglUint = shaderProgram };
}

void ComputeShader::bind() const
{
	glUseProgram(shaderProgram);
}

unsigned ComputeShader::getBindingPoint(const std::string& uniformName) const
{
	return glGetUniformLocation(shaderProgram, uniformName.c_str());
}

unsigned ComputeShader::getUniformBlockIndex(const std::string& uniformName) const
{
	return glGetUniformBlockIndex(shaderProgram, uniformName.c_str());
}

void ComputeShader::setUniform(unsigned bindingPoint, MH33::GFX::StorageBuffer& buffer, uint8_t unit) const
{
	buffer.bindBase(unit);
	glUniformBlockBinding(shaderProgram, bindingPoint, unit);
}

void ComputeShader::setUniform(unsigned bindingPoint, MH33::GFX::Texture2D& texture, uint8_t unit, MH33::GFX::ImageBindingType bindingType) const
{
	texture.bindAsImage(unit,bindingType);
	glUniform1i(bindingPoint, unit);
}

void ComputeShader::setUniform(unsigned bindingPoint, MH33::GFX::TextureArray2D& texture, uint8_t unit, MH33::GFX::ImageBindingType bindingType) const
{
	texture.bindAsImage(unit,bindingType);
	glUniform1i(bindingPoint, unit);
}

void ComputeShader::setUniform(unsigned bindingPoint, MH33::GFX::Texture3D& texture, uint8_t unit, MH33::GFX::ImageBindingType bindingType) const
{
	texture.bindAsImage(unit,bindingType);
	glUniform1i(bindingPoint, unit);
}

void ComputeShader::setUniform(unsigned bindingPoint, MH33::GFX::Cubemap& texture, uint8_t unit, MH33::GFX::ImageBindingType bindingType) const
{
	texture.bindAsImage(unit,bindingType);
	glUniform1i(bindingPoint, unit);
}

void ComputeShader::dispatchCompute(int x, int y, int z)
{
	glDispatchCompute(x,y,z);
}

void ComputeShader::waitForFinish() const
{
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
}
