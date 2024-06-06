#ifndef GLSHADERMODULE_HPP
#define GLSHADERMODULE_HPP
#include <NativeGfxApi/glad/glwrap.h>
#include <exception>
#include <string>
#include <GFX/Abstract/Pipeline.hpp>

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
	ShaderModule(const MH33::GFX::ShaderModuleCreateInfo& createInfo);
	ShaderModule(const MH33::GFX::ShaderModuleCreateInfoRef& createInfo);
	~ShaderModule();
	GLuint getId() const;
	operator GLuint() const;
};

}

#endif // GLSHADERMODULE_HPP
