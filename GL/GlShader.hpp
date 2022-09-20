#ifndef GLSHADER_HPP
#define GLSHADER_HPP
#include <GL/gles2.h>
#include <string>
#include <span>
#include <vector>
#include <functional>
namespace GL {

class IShader
{
private:
	GLint getValueI(GLenum pname) const;
	void setValueI(GLenum pname, GLint value);
	float getValueF(GLenum pname) const;
	void setValueF(GLenum pname, float value);
	GLint getLevelValueI(GLenum pname, GLint level) const;
	float getLevelValueF(GLenum pname, GLint level) const;
public:
	typedef std::function<void(IShader&)> ShaderAccessor;
	typedef std::function<void(const IShader&)> ShaderConstAccessor;
	typedef std::function<void(GLuint)> ShaderIdAccessor;
	virtual ~IShader() = default;
	virtual GLuint getShaderId() const = 0;
	virtual GLenum getType() const = 0;
	// Bind
	void doActions(const ShaderAccessor& fun);
	void doActions(const ShaderConstAccessor& fun) const;
	void doActions(const ShaderIdAccessor& fun) const;
	// Getters, setters
	void source(GLsizei count, const GLchar **string, const GLint *length);
	void source(const std::span<const GLchar*>& strings, const std::span<const GLint>& lengths);
	void source(const std::span<const std::string>& strings);
	void source(const GLchar* string, GLint length);
	void source(const std::string& string);
	void compile();
};

class ShaderRef : public IShader
{
private:
	GLuint shaderId;
	GLenum type;
public:
	ShaderRef(GLuint shaderId, GLenum type);
	~ShaderRef() override;
	ShaderRef(const ShaderRef& cpy);
	ShaderRef& operator=(const ShaderRef& cpy);
	GLuint getShaderId() const override;
	GLenum getType() const override;
};
class Shader : public IShader
{
private:
	GLuint shaderId;
	GLenum type;
	Shader(const Shader& cpy) = delete;
	Shader& operator=(const Shader& cpy) = delete;
public:
	Shader(GLenum type);
	Shader(Shader&& mov);
	Shader& operator=(Shader&& mov);
	~Shader() override;
	GLuint getShaderId() const override;
	GLenum getType() const override;
	ShaderRef toRef() const;
};
}
#endif // GLSHADER_HPP
