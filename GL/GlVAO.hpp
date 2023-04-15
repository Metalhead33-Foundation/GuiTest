#ifndef GLVAO_HPP
#define GLVAO_HPP
#include <GL/gles2.h>
#include <mutex>
#include <span>
#include <vector>
#include <functional>
#include <cstdint>
#include <MhLib/Media/GFX/MhVertexFormat.hpp>

namespace GL {

struct VaoEssentials {
	static std::recursive_mutex vaomutex;
	static GLuint lastBound;
	static void ensureBound(GLuint id);
};

class IVao
{
public:
	typedef std::function<void(IVao&)> VaoAccessor;
	typedef std::function<void(const IVao&)> VaoConstAccessor;
	typedef std::function<void(GLuint)> VaoIdAccessor;
	virtual ~IVao() = default;
	virtual GLuint getVaoId() const = 0;
	void bind();
	void enableVertexAttribArray(GLuint index);
	void vertexAttribPointer(GLuint index, GLint size, GLenum type, bool normalized, GLsizei stride, uintptr_t offset);
	void vertexAttribPointer(GLuint index, MH33::GFX::PrimitiveType type, GLsizei stride, uintptr_t offset);
	void enableAttributes(const MH33::GFX::VertexDescriptor& descriptor);
	void doActions(const VaoAccessor& fun);
	void doActions(const VaoConstAccessor& fun) const;
	void doActions(const VaoIdAccessor& fun) const;
	void draw(GLenum mode, GLint first, GLsizei count);
};

class VaoRef : public IVao
{
private:
	GLuint vaoId;
public:
	VaoRef(GLuint vaoId);
	~VaoRef() override;
	VaoRef(const VaoRef& cpy);
	VaoRef& operator=(const VaoRef& cpy);
	GLuint getVaoId() const override;
};

class Vao : public IVao
{
private:
	GLuint vaoId;
	Vao(const Vao& cpy) = delete;
	Vao& operator=(const Vao& cpy) = delete;
public:
	Vao();
	~Vao() override;
	Vao(Vao&& mov);
	Vao& operator=(Vao&& mov);
	GLuint getVaoId() const override;
	VaoRef toRef() const;
};

}
#endif // GLVAO_HPP
