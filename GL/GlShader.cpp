#include <GL/GlShader.hpp>
#include <stdexcept>

namespace GL {

class ShaderCompilationError : public std::exception {
private:
	std::string infoLog;
public:
	ShaderCompilationError(GLuint shader)
	{
		GLint infoLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		infoLog.resize(infoLen+1,0);
		glGetShaderInfoLog(shader, infoLen, nullptr, infoLog.data());
	}
	const char* what() const noexcept override;
};

const char* ShaderCompilationError::what() const noexcept
{
	return infoLog.c_str();
}

GLuint Shader::getShaderId() const
{
	return shaderId;
}

GLenum Shader::getType() const
{
	return type;
}

ShaderRef Shader::toRef() const
{
	return ShaderRef(shaderId,type);
}

Shader::Shader(GLenum type)
	: shaderId(glCreateShader(type)), type(type)
{

}

Shader::Shader(Shader&& mov)
	: shaderId(mov.shaderId), type(mov.type)
{
	mov.shaderId = 0;
	mov.type = 0;
}

Shader& Shader::operator=(Shader&& mov)
{
	this->shaderId = mov.shaderId;
	mov.shaderId = 0;
	this->type = mov.type;
	mov.type = 0;
	return *this;
}

Shader::~Shader()
{
	if(shaderId) glDeleteShader(shaderId);
}

void IShader::doActions(const ShaderAccessor& fun)
{
	fun(*this);
}

void IShader::doActions(const ShaderConstAccessor& fun) const
{
	fun(*this);
}

void IShader::doActions(const ShaderIdAccessor& fun) const
{
	fun(getShaderId());
}

void IShader::source(GLsizei count, const GLchar** string, const GLint* length)
{
	glShaderSource(getShaderId(),count,string,length);
}

void IShader::source(const std::span<const GLchar*>& strings, const std::span<const GLint>& lengths)
{
	if(strings.size() != lengths.size()) throw std::runtime_error("\"strings\" and \"lengths\" must have the same amount of elements!");
	source(strings.size(),strings.data(),lengths.data());
}

void IShader::source(const std::span<const std::string>& strings)
{
	//std::vector<const GLchar*> strs(strings.size());
	//std::vector<GLint> lengths(strings.size());
	const GLchar** strs = static_cast<const GLchar**>(alloca(sizeof(const GLchar*) * strings.size()));
	GLint* lengths = static_cast<GLint*>(alloca(sizeof(GLint) * strings.size()));
	for(size_t i = 0; i < strings.size(); ++i)
	{
		strs[i] = strings[i].c_str();
		lengths[i] = strings[i].length();
	}
	source(strings.size(),strs,lengths);
	//source(strs,lengths);
}

void IShader::source(const GLchar* string, GLint length)
{
	glShaderSource(getShaderId(),1,&string,&length);
}

void IShader::source(const std::string& string)
{
	source(string.c_str(),string.length());
}

void IShader::compile()
{
	GLint compiled;
	glCompileShader(getShaderId());
	glGetShaderiv(getShaderId(), GL_COMPILE_STATUS, &compiled);
	if(!compiled) {
		throw ShaderCompilationError(getShaderId());
	}
}

ShaderRef::ShaderRef(GLuint shaderId, GLenum type)
	: shaderId(shaderId), type(type)
{

}

ShaderRef::~ShaderRef()
{

}

ShaderRef::ShaderRef(const ShaderRef& cpy)
	: shaderId(cpy.shaderId), type(cpy.type)
{

}

ShaderRef& ShaderRef::operator=(const ShaderRef& cpy)
{
	this->shaderId = cpy.shaderId;
	this->type = cpy.type;
}

GLuint ShaderRef::getShaderId() const
{
	return shaderId;
}

GLenum ShaderRef::getType() const
{
	return type;
}

}
