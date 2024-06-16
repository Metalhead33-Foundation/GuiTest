#include "GlShaderModule.hpp"
namespace GL {

ShaderModule::ShaderModule()
	: shaderModule(0)
{

}

ShaderModule::ShaderModule(ShaderModule&& mov)
	: shaderModule(mov.shaderModule)
{
	mov.shaderModule = 0;
}

ShaderModule& ShaderModule::operator=(ShaderModule&& mov)
{
	this->shaderModule = mov.shaderModule;
	mov.shaderModule = 0;
	return *this;
}

ShaderModule::ShaderModule(const MH33::GFX::ShaderModuleCreateInfo& createInfo)
	: shaderModule(0)
{
	const GLchar* sourceCode = reinterpret_cast<const GLchar*>(createInfo.source.data());
	GLenum shaderType;
	switch (createInfo.shaderType) {
		case MH33::GFX::ShaderModuleType::VERTEX_SHADER:
			shaderType = GL_VERTEX_SHADER; break;
		case MH33::GFX::ShaderModuleType::GEOMETRY_SHADER:
			shaderType = GL_GEOMETRY_SHADER; break;
		case MH33::GFX::ShaderModuleType::TESSELLATION_CONTROL_SHADER:
			shaderType = GL_TESS_CONTROL_SHADER; break;
		case MH33::GFX::ShaderModuleType::TESSELLATION_EVALUATION_SHADER:
			shaderType = GL_TESS_EVALUATION_SHADER; break;
		case MH33::GFX::ShaderModuleType::PIXEL_SHADER:
			shaderType = GL_FRAGMENT_SHADER; break;
		case MH33::GFX::ShaderModuleType::COMPUTE_SHADER:
			shaderType = GL_COMPUTE_SHADER; break;
		default: shaderType = GL_INVALID_ENUM;
	}
	shaderModule = glCreateShader(shaderType);
	if(createInfo.isBinary) {
		const GLint sourceCodeLength = createInfo.source.size(); // Exclude null terminator.
		glShaderBinary(1,&shaderModule,GL_SHADER_BINARY_FORMAT_SPIR_V_ARB,sourceCode,sourceCodeLength);
		glSpecializeShaderARB(shaderModule, "main", 0, 0, 0);
	} else {
		const GLint sourceCodeLength = createInfo.source.size() - 1; // Exclude null terminator.
		glShaderSource(shaderModule,1,&sourceCode,&sourceCodeLength);
		glCompileShader(shaderModule);
	}
	GLint success;
	glGetShaderiv(shaderModule, GL_COMPILE_STATUS, &success);
	if(!success) {
		throw ShaderError(shaderModule);
	}
}

ShaderModule::ShaderModule(const MH33::GFX::ShaderModuleCreateInfoRef& createInfo)
{
	const GLchar* sourceCode = reinterpret_cast<const GLchar*>(createInfo.source.data());
	const GLint sourceCodeLength = createInfo.source.size() - 1; // Exclude null terminator.
	GLenum shaderType;
	switch (createInfo.shaderType) {
		case MH33::GFX::ShaderModuleType::VERTEX_SHADER:
			shaderType = GL_VERTEX_SHADER; break;
		case MH33::GFX::ShaderModuleType::GEOMETRY_SHADER:
			shaderType = GL_GEOMETRY_SHADER; break;
		case MH33::GFX::ShaderModuleType::TESSELLATION_CONTROL_SHADER:
			shaderType = GL_TESS_CONTROL_SHADER; break;
		case MH33::GFX::ShaderModuleType::TESSELLATION_EVALUATION_SHADER:
			shaderType = GL_TESS_EVALUATION_SHADER; break;
		case MH33::GFX::ShaderModuleType::PIXEL_SHADER:
			shaderType = GL_FRAGMENT_SHADER; break;
		default: shaderType = GL_INVALID_ENUM;
	}
	shaderModule = glCreateShader(shaderType);
	glShaderSource(shaderModule,1,&sourceCode,&sourceCodeLength);
	glCompileShader(shaderModule);
	GLint success;
	glGetShaderiv(shaderModule, GL_COMPILE_STATUS, &success);
	if(!success) {
		throw ShaderError(shaderModule);
	}
}

ShaderModule::~ShaderModule()
{
	if(shaderModule) glDeleteShader(shaderModule);
}

GLuint ShaderModule::getId() const
{
	return shaderModule;
}

ShaderModule::operator GLuint() const
{
	return shaderModule;
}

ShaderError::ShaderError(GLuint shaderId, bool isProgram)
{
	GLint totalLenght=0;
	if(isProgram) {
		glGetProgramiv(shaderId,GL_INFO_LOG_LENGTH,&totalLenght);
		errStr.resize(totalLenght);
		glGetProgramInfoLog(shaderId, totalLenght, nullptr, errStr.data());
		errStr = "SHADER_PROGRAM_LINKING_ERROR: " + errStr;
	} else {
		glGetShaderiv(shaderId,GL_INFO_LOG_LENGTH,&totalLenght);
		errStr.resize(totalLenght);
		glGetShaderInfoLog(shaderId, totalLenght, nullptr, errStr.data());
		errStr = "SHADER_COMPILATION_ERROR: " + errStr;
	}
}

const char* ShaderError::what() const noexcept
{
	return errStr.c_str();
}

}
