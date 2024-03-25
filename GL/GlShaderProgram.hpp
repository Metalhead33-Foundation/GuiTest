#ifndef GLSHADERPROGRAM_HPP
#define GLSHADERPROGRAM_HPP
#include <GL/GlShader.hpp>
#include <GL/GlBuffer.hpp>
#include <mutex>
#include <glm/glm.hpp>
#include <glm/matrix.hpp>

namespace GL {

struct ShaderEssentials {
	static std::recursive_mutex shadermutex;
	static GLuint lastBound;
	static void ensureBound(GLuint id);
};

class IShaderProgram
{
public:
	virtual ~IShaderProgram() = default;
	virtual GLuint getProgramId() const = 0;
	void attachShader(const IShader& shader);
	void detachShader(const IShader& shader);
	void link();
	void bind();
	// Normal getters and setters
	GLint getAttribLocation(const GLchar *name) const;
	GLint getAttribLocation(const std::string& name) const;
	GLint getUniformLocation(const GLchar *name) const;
	GLint getUniformLocation(const std::string& name) const;
	GLuint getUniformBlockIndex(const GLchar *uniformBlockName) const;
	GLuint getUniformBlockIndex(const std::string& name) const;
	void uniformBlockBinding(GLuint uniformBlockIndex, GLuint uniformBlockBindingV);
	// Raw setters
	void bindBufferBase(GLenum target, GLuint index, GLuint buffer);
	void bindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
	void bindBufferBase(GLuint index, const IBuffer& buffer);
	void uniform1f(GLint location, GLfloat v0);
	void uniform2f(GLint location, GLfloat v0, GLfloat v1);
	void uniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
	void uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
	void uniform1i(GLint location, GLint v0);
	void uniform2i(GLint location, GLint v0, GLint v1);
	void uniform3i(GLint location, GLint v0, GLint v1, GLint v2);
	void uniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
	void uniform1ui(GLint location, GLuint v0);
	void uniform2ui(GLint location, GLuint v0, GLuint v1);
	void uniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2);
	void uniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
	void uniform1fv(GLint location, GLsizei count, const GLfloat *value);
	void uniform2fv(GLint location, GLsizei count, const GLfloat *value);
	void uniform3fv(GLint location, GLsizei count, const GLfloat *value);
	void uniform4fv(GLint location, GLsizei count, const GLfloat *value);
	void uniform1iv(GLint location, GLsizei count, const GLint *value);
	void uniform2iv(GLint location, GLsizei count, const GLint *value);
	void uniform3iv(GLint location, GLsizei count, const GLint *value);
	void uniform4iv(GLint location, GLsizei count, const GLint *value);
	void uniform1uiv(GLint location, GLsizei count, const GLuint *value);
	void uniform2uiv(GLint location, GLsizei count, const GLuint *value);
	void uniform3uiv(GLint location, GLsizei count, const GLuint *value);
	void uniform4uiv(GLint location, GLsizei count, const GLuint *value);
	void uniformMatrix2fv(GLint location, GLsizei count, bool transpose, const GLfloat *value);
	void uniformMatrix3fv(GLint location, GLsizei count, bool transpose, const GLfloat *value);
	void uniformMatrix4fv(GLint location, GLsizei count, bool transpose, const GLfloat *value);
	void uniformMatrix2x3fv(GLint location, GLsizei count, bool transpose, const GLfloat *value);
	void uniformMatrix3x2fv(GLint location, GLsizei count, bool transpose, const GLfloat *value);
	void uniformMatrix2x4fv(GLint location, GLsizei count, bool transpose, const GLfloat *value);
	void uniformMatrix4x2fv(GLint location, GLsizei count, bool transpose, const GLfloat *value);
	void uniformMatrix3x4fv(GLint location, GLsizei count, bool transpose, const GLfloat *value);
	void uniformMatrix4x3fv(GLint location, GLsizei count, bool transpose, const GLfloat *value);
	// GLM matrix setters - singular
	void uniform1f(GLint location, const glm::vec1& value);
	void uniform2f(GLint location, const glm::vec2& value);
	void uniform3f(GLint location, const glm::vec3& value);
	void uniform4f(GLint location, const glm::vec4& value);
	void uniform1i(GLint location, const glm::ivec1& value);
	void uniform2i(GLint location, const glm::ivec2& value);
	void uniform3i(GLint location, const glm::ivec3& value);
	void uniform4i(GLint location, const glm::ivec4& value);
	void uniform1ui(GLint location, const glm::uvec1& value);
	void uniform2ui(GLint location, const glm::uvec2& value);
	void uniform3ui(GLint location, const glm::uvec3& value);
	void uniform4ui(GLint location, const glm::uvec4& value);
	void uniformMatrix2f(GLint location, bool transpose, const glm::mat2& value);
	void uniformMatrix3f(GLint location, bool transpose, const glm::mat3& value);
	void uniformMatrix4f(GLint location, bool transpose, const glm::mat4& value);
	void uniformMatrix2x3f(GLint location, bool transpose, const glm::mat2x3& value);
	void uniformMatrix3x2f(GLint location, bool transpose, const glm::mat3x2& value);
	void uniformMatrix2x4f(GLint location, bool transpose, const glm::mat2x4& value);
	void uniformMatrix4x2f(GLint location, bool transpose, const glm::mat4x2& value);
	void uniformMatrix3x4f(GLint location, bool transpose, const glm::mat3x4& value);
	void uniformMatrix4x3f(GLint location, bool transpose, const glm::mat4x3& value);
	// GLM setters - plural
	void uniform1fv(GLint location, const std::span<const glm::vec1>& values);
	void uniform2fv(GLint location, const std::span<const glm::vec2>& values);
	void uniform3fv(GLint location, const std::span<const glm::vec3>& values);
	void uniform4fv(GLint location, const std::span<const glm::vec4>& values);
	void uniform1iv(GLint location, const std::span<const glm::ivec1>& values);
	void uniform2iv(GLint location, const std::span<const glm::ivec2>& values);
	void uniform3iv(GLint location, const std::span<const glm::ivec3>& values);
	void uniform4iv(GLint location, const std::span<const glm::ivec4>& values);
	void uniform1uiv(GLint location, const std::span<const glm::uvec1>& values);
	void uniform2uiv(GLint location, const std::span<const glm::uvec2>& values);
	void uniform3uiv(GLint location, const std::span<const glm::uvec3>& values);
	void uniform4uiv(GLint location, const std::span<const glm::uvec4>& values);
	void uniformMatrix2fv(GLint location, bool transpose, const std::span<const glm::mat2>& values);
	void uniformMatrix3fv(GLint location, bool transpose, const std::span<const glm::mat3>& values);
	void uniformMatrix4fv(GLint location, bool transpose, const std::span<const glm::mat4>& values);
	void uniformMatrix2x3fv(GLint location, bool transpose, const std::span<const glm::mat2x3>& values);
	void uniformMatrix3x2fv(GLint location, bool transpose, const std::span<const glm::mat3x2>& values);
	void uniformMatrix2x4fv(GLint location, bool transpose, const std::span<const glm::mat2x4>& values);
	void uniformMatrix4x2fv(GLint location, bool transpose, const std::span<const glm::mat4x2>& values);
	void uniformMatrix3x4fv(GLint location, bool transpose, const std::span<const glm::mat3x4>& values);
	void uniformMatrix4x3fv(GLint location, bool transpose, const std::span<const glm::mat4x3>& values);
};

class ShaderProgram : public IShaderProgram
{
private:
	GLuint programId;
public:
	ShaderProgram();
	~ShaderProgram() override;
	GLuint getProgramId() const override;
};
}

#endif // GLSHADERPROGRAM_HPP
