#include <GL/GlShaderProgram.hpp>

namespace GL {

std::recursive_mutex ShaderEssentials::shadermutex;
typedef std::lock_guard<std::recursive_mutex> RecursiveLock;
GLuint ShaderEssentials::lastBound = 0;
void ShaderEssentials::ensureBound(GLuint id)
{
	if(lastBound != id) {
		glUseProgram(id);
		lastBound = id;
	}
}

GLuint ShaderProgram::getProgramId() const
{
	return programId;
}

ShaderProgram::ShaderProgram()
	: programId(glCreateProgram())
{

}

ShaderProgram::~ShaderProgram()
{
	if(programId) glDeleteProgram(programId);
}

void IShaderProgram::attachShader(const IShader& shader)
{
	glAttachShader(getProgramId(),shader.getShaderId());
}

void IShaderProgram::detachShader(const IShader& shader)
{
	glDetachShader(getProgramId(),shader.getShaderId());
}

void IShaderProgram::link()
{
	glLinkProgram(getProgramId());
}

void IShaderProgram::bind()
{
	//glUseProgram(getProgramId());
	ShaderEssentials::ensureBound(getProgramId());
}

GLint IShaderProgram::getAttribLocation(const GLchar* name) const
{
	return glGetAttribLocation(getProgramId(),name);
}

GLint IShaderProgram::getAttribLocation(const std::string& name) const
{
	return glGetAttribLocation(getProgramId(),name.c_str());
}

GLint IShaderProgram::getUniformLocation(const GLchar* name) const
{
	return glGetUniformLocation(getProgramId(),name);
}

GLint IShaderProgram::getUniformLocation(const std::string& name) const
{
	return glGetUniformLocation(getProgramId(),name.c_str());
}

GLuint IShaderProgram::getUniformBlockIndex(const GLchar* uniformBlockName) const
{
	return glGetUniformBlockIndex(getProgramId(),uniformBlockName);
}

GLuint IShaderProgram::getUniformBlockIndex(const std::string& name) const
{
	return glGetUniformBlockIndex(getProgramId(),name.c_str());
}

void IShaderProgram::uniformBlockBinding(GLuint uniformBlockIndex, GLuint uniformBlockBindingV)
{
	bind();
	glUniformBlockBinding(getProgramId(),uniformBlockIndex,uniformBlockBindingV);
}

void IShaderProgram::bindBufferBase(GLenum target, GLuint index, GLuint buffer)
{
	bind();
	glBindBufferBase(target,index,buffer);
}

void IShaderProgram::bindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)
{
	bind();
	glBindBufferRange(target,index,buffer,offset,size);
}

void IShaderProgram::bindBufferBase(GLuint index, const IBuffer& buffer)
{
	bind();
	glBindBufferBase(buffer.getTarget(),index,buffer.getBuffId());
}

void IShaderProgram::uniform1f(GLint location, GLfloat v0)
{
	bind();
	glUniform1f(location,v0);
}

void IShaderProgram::uniform2f(GLint location, GLfloat v0, GLfloat v1)
{
	bind();
	glUniform2f(location,v0,v1);
}

void IShaderProgram::uniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
	bind();
	glUniform3f(location,v0,v1,v2);
}

void IShaderProgram::uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
	bind();
	glUniform4f(location,v0,v1,v2,v3);
}

void IShaderProgram::uniform1i(GLint location, GLint v0)
{
	bind();
	glUniform1i(location,v0);
}

void IShaderProgram::uniform2i(GLint location, GLint v0, GLint v1)
{
	bind();
	glUniform2i(location,v0,v1);
}

void IShaderProgram::uniform3i(GLint location, GLint v0, GLint v1, GLint v2)
{
	bind();
	glUniform3i(location,v0,v1,v2);
}

void IShaderProgram::uniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
	bind();
	glUniform4i(location,v0,v1,v2,v3);
}

void IShaderProgram::uniform1ui(GLint location, GLuint v0)
{
	bind();
	glUniform1ui(location,v0);
}

void IShaderProgram::uniform2ui(GLint location, GLuint v0, GLuint v1)
{
	bind();
	glUniform2ui(location,v0,v1);
}

void IShaderProgram::uniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2)
{
	bind();
	glUniform3ui(location,v0,v1,v2);
}

void IShaderProgram::uniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
{
	bind();
	glUniform4ui(location,v0,v1,v2,v3);
}

void IShaderProgram::uniform1fv(GLint location, GLsizei count, const GLfloat* value)
{
	bind();
	glUniform1fv(location,count,value);
}

void IShaderProgram::uniform2fv(GLint location, GLsizei count, const GLfloat* value)
{
	bind();
	glUniform2fv(location,count,value);
}

void IShaderProgram::uniform3fv(GLint location, GLsizei count, const GLfloat* value)
{
	bind();
	glUniform3fv(location,count,value);
}

void IShaderProgram::uniform4fv(GLint location, GLsizei count, const GLfloat* value)
{
	bind();
	glUniform4fv(location,count,value);
}

void IShaderProgram::uniform1iv(GLint location, GLsizei count, const GLint* value)
{
	bind();
	glUniform1iv(location,count,value);
}

void IShaderProgram::uniform2iv(GLint location, GLsizei count, const GLint* value)
{
	bind();
	glUniform2iv(location,count,value);
}

void IShaderProgram::uniform3iv(GLint location, GLsizei count, const GLint* value)
{
	bind();
	glUniform3iv(location,count,value);
}

void IShaderProgram::uniform4iv(GLint location, GLsizei count, const GLint* value)
{
	bind();
	glUniform4iv(location,count,value);
}

void IShaderProgram::uniform1uiv(GLint location, GLsizei count, const GLuint* value)
{
	bind();
	glUniform1uiv(location,count,value);
}

void IShaderProgram::uniform2uiv(GLint location, GLsizei count, const GLuint* value)
{
	bind();
	glUniform2uiv(location,count,value);
}

void IShaderProgram::uniform3uiv(GLint location, GLsizei count, const GLuint* value)
{
	bind();
	glUniform3uiv(location,count,value);
}

void IShaderProgram::uniform4uiv(GLint location, GLsizei count, const GLuint* value)
{
	bind();
	glUniform4uiv(location,count,value);
}

void IShaderProgram::uniformMatrix2fv(GLint location, GLsizei count, bool transpose, const GLfloat* value)
{
	bind();
	glUniformMatrix2fv(location,count,transpose,value);
}

void IShaderProgram::uniformMatrix3fv(GLint location, GLsizei count, bool transpose, const GLfloat* value)
{
	bind();
	glUniformMatrix3fv(location,count,transpose,value);
}

void IShaderProgram::uniformMatrix4fv(GLint location, GLsizei count, bool transpose, const GLfloat* value)
{
	bind();
	glUniformMatrix4fv(location,count,transpose,value);
}

void IShaderProgram::uniformMatrix2x3fv(GLint location, GLsizei count, bool transpose, const GLfloat* value)
{
	bind();
	glUniformMatrix2x3fv(location,count,transpose,value);
}

void IShaderProgram::uniformMatrix3x2fv(GLint location, GLsizei count, bool transpose, const GLfloat* value)
{
	bind();
	glUniformMatrix3x2fv(location,count,transpose,value);
}

void IShaderProgram::uniformMatrix2x4fv(GLint location, GLsizei count, bool transpose, const GLfloat* value)
{
	bind();
	glUniformMatrix2x4fv(location,count,transpose,value);
}

void IShaderProgram::uniformMatrix4x2fv(GLint location, GLsizei count, bool transpose, const GLfloat* value)
{
	bind();
	glUniformMatrix4x2fv(location,count,transpose,value);
}

void IShaderProgram::uniformMatrix3x4fv(GLint location, GLsizei count, bool transpose, const GLfloat* value)
{
	bind();
	glUniformMatrix3x4fv(location,count,transpose,value);
}

void IShaderProgram::uniformMatrix4x3fv(GLint location, GLsizei count, bool transpose, const GLfloat* value)
{
	bind();
	glUniformMatrix4x3fv(location,count,transpose,value);
}

void IShaderProgram::uniform1f(GLint location, const glm::vec1& value)
{
	glUniform1f(location,value.x);
}
void IShaderProgram::uniform2f(GLint location, const glm::vec2& value)
{
	glUniform2f(location,value.x,value.y);
}
void IShaderProgram::uniform3f(GLint location, const glm::vec3& value)
{
	glUniform3f(location,value.x,value.y,value.z);
}
void IShaderProgram::uniform4f(GLint location, const glm::vec4& value)
{
	glUniform4f(location,value.x,value.y,value.z,value.w);
}
void IShaderProgram::uniform1i(GLint location, const glm::ivec1& value)
{
	glUniform1i(location,value.x);
}
void IShaderProgram::uniform2i(GLint location, const glm::ivec2& value)
{
	glUniform2i(location,value.x,value.y);
}
void IShaderProgram::uniform3i(GLint location, const glm::ivec3& value)
{
	glUniform3i(location,value.x,value.y,value.z);
}
void IShaderProgram::uniform4i(GLint location, const glm::ivec4& value)
{
	glUniform4i(location,value.x,value.y,value.z,value.w);
}
void IShaderProgram::uniform1ui(GLint location, const glm::uvec1& value)
{
	glUniform1ui(location,value.x);
}
void IShaderProgram::uniform2ui(GLint location, const glm::uvec2& value)
{
	glUniform2ui(location,value.x,value.y);
}
void IShaderProgram::uniform3ui(GLint location, const glm::uvec3& value)
{
	glUniform3ui(location,value.x,value.y,value.z);
}
void IShaderProgram::uniform4ui(GLint location, const glm::uvec4& value)
{
	glUniform4ui(location,value.x,value.y,value.z,value.w);
}
void IShaderProgram::uniformMatrix2f(GLint location, bool transpose, const glm::mat2& value)
{
	uniformMatrix2fv(location,1,transpose, reinterpret_cast<const float*>(&value) );
}
void IShaderProgram::uniformMatrix3f(GLint location, bool transpose, const glm::mat3& value)
{
	uniformMatrix3fv(location,1,transpose, reinterpret_cast<const float*>(&value) );
}
void IShaderProgram::uniformMatrix4f(GLint location, bool transpose, const glm::mat4& value)
{
	uniformMatrix4fv(location,1,transpose, reinterpret_cast<const float*>(&value) );
}
void IShaderProgram::uniformMatrix2x3f(GLint location, bool transpose, const glm::mat2x3& value)
{
	uniformMatrix2x3fv(location,1,transpose, reinterpret_cast<const float*>(&value) );
}
void IShaderProgram::uniformMatrix3x2f(GLint location, bool transpose, const glm::mat3x2& value)
{
	uniformMatrix3x2fv(location,1,transpose, reinterpret_cast<const float*>(&value) );
}
void IShaderProgram::uniformMatrix2x4f(GLint location, bool transpose, const glm::mat2x4& value)
{
	uniformMatrix2x4fv(location,1,transpose, reinterpret_cast<const float*>(&value) );
}
void IShaderProgram::uniformMatrix4x2f(GLint location, bool transpose, const glm::mat4x2& value)
{
	uniformMatrix4x2fv(location,1,transpose, reinterpret_cast<const float*>(&value) );
}
void IShaderProgram::uniformMatrix3x4f(GLint location, bool transpose, const glm::mat3x4& value)
{
	uniformMatrix3x4fv(location,1,transpose, reinterpret_cast<const float*>(&value) );
}
void IShaderProgram::uniformMatrix4x3f(GLint location, bool transpose, const glm::mat4x3& value)
{
	uniformMatrix4x3fv(location,1,transpose, reinterpret_cast<const float*>(&value) );
}
void IShaderProgram::uniform1fv(GLint location, const std::span<const glm::vec1>& values)
{
	uniform1fv(location, values.size(), reinterpret_cast<const float*>(values.data()) );
}
void IShaderProgram::uniform2fv(GLint location, const std::span<const glm::vec2>& values)
{
	uniform2fv(location, values.size(), reinterpret_cast<const float*>(values.data()) );
}
void IShaderProgram::uniform3fv(GLint location, const std::span<const glm::vec3>& values)
{
	uniform3fv(location, values.size(), reinterpret_cast<const float*>(values.data()) );
}
void IShaderProgram::uniform4fv(GLint location, const std::span<const glm::vec4>& values)
{
	uniform4fv(location, values.size(), reinterpret_cast<const float*>(values.data()) );
}
void IShaderProgram::uniform1iv(GLint location, const std::span<const glm::ivec1>& values)
{
	uniform1iv(location, values.size(), reinterpret_cast<const int*>(values.data()) );
}
void IShaderProgram::uniform2iv(GLint location, const std::span<const glm::ivec2>& values)
{
	uniform2iv(location, values.size(), reinterpret_cast<const int*>(values.data()) );
}
void IShaderProgram::uniform3iv(GLint location, const std::span<const glm::ivec3>& values)
{
	uniform3iv(location, values.size(), reinterpret_cast<const int*>(values.data()) );
}
void IShaderProgram::uniform4iv(GLint location, const std::span<const glm::ivec4>& values)
{
	uniform4iv(location, values.size(), reinterpret_cast<const int*>(values.data()) );
}
void IShaderProgram::uniform1uiv(GLint location, const std::span<const glm::uvec1>& values)
{
	uniform1uiv(location, values.size(), reinterpret_cast<const glm::uint*>(values.data()) );
}
void IShaderProgram::uniform2uiv(GLint location, const std::span<const glm::uvec2>& values)
{
	uniform2uiv(location, values.size(),  reinterpret_cast<const glm::uint*>(values.data()) );
}
void IShaderProgram::uniform3uiv(GLint location, const std::span<const glm::uvec3>& values)
{
	uniform3uiv(location, values.size(),  reinterpret_cast<const glm::uint*>(values.data()) );
}
void IShaderProgram::uniform4uiv(GLint location, const std::span<const glm::uvec4>& values)
{
	uniform4uiv(location, values.size(),  reinterpret_cast<const glm::uint*>(values.data()) );
}
void IShaderProgram::uniformMatrix2fv(GLint location, bool transpose, const std::span<const glm::mat2>& values)
{
	uniformMatrix2fv(location, values.size(), transpose, reinterpret_cast<const float*>(values.data()) );
}
void IShaderProgram::uniformMatrix3fv(GLint location, bool transpose, const std::span<const glm::mat3>& values)
{
	uniformMatrix2fv(location, values.size(), transpose, reinterpret_cast<const float*>(values.data()) );
}
void IShaderProgram::uniformMatrix4fv(GLint location, bool transpose, const std::span<const glm::mat4>& values)
{
	uniformMatrix4fv(location, values.size(), transpose, reinterpret_cast<const float*>(values.data()) );
}
void IShaderProgram::uniformMatrix2x3fv(GLint location, bool transpose, const std::span<const glm::mat2x3>& values)
{
	uniformMatrix2x3fv(location, values.size(), transpose, reinterpret_cast<const float*>(values.data()) );
}
void IShaderProgram::uniformMatrix3x2fv(GLint location, bool transpose, const std::span<const glm::mat3x2>& values)
{
	uniformMatrix3x2fv(location, values.size(), transpose, reinterpret_cast<const float*>(values.data()) );
}
void IShaderProgram::uniformMatrix2x4fv(GLint location, bool transpose, const std::span<const glm::mat2x4>& values)
{
	uniformMatrix2x4fv(location, values.size(), transpose, reinterpret_cast<const float*>(values.data()) );
}
void IShaderProgram::uniformMatrix4x2fv(GLint location, bool transpose, const std::span<const glm::mat4x2>& values)
{
	uniformMatrix4x2fv(location, values.size(), transpose, reinterpret_cast<const float*>(values.data()) );
}
void IShaderProgram::uniformMatrix3x4fv(GLint location, bool transpose, const std::span<const glm::mat3x4>& values)
{
	uniformMatrix4x2fv(location, values.size(), transpose, reinterpret_cast<const float*>(values.data()) );
}
void IShaderProgram::uniformMatrix4x3fv(GLint location, bool transpose, const std::span<const glm::mat4x3>& values)
{
	uniformMatrix4x3fv(location, values.size(), transpose, reinterpret_cast<const float*>(values.data()) );
}

}
