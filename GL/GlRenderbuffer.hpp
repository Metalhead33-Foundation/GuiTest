#ifndef GLRENDERBUFFER_HPP
#define GLRENDERBUFFER_HPP
#include <GL/gles2.h>
#include <string>
#include <span>
#include <vector>
#include <functional>
#include <GL/gles2.h>
#include <string>
#include <span>
#include <vector>
#include <mutex>
#include <functional>
namespace GL {

struct RenderbufferEssentials {
	static std::recursive_mutex buffermutex;
	static GLuint lastBound;
	static void ensureBound(GLuint id);
};

class IRenderbuffer
{
public:
	typedef std::function<void(IRenderbuffer&)> RenderbufferAccessor;
	typedef std::function<void(const IRenderbuffer&)> RenderbufferConstAccessor;
	typedef std::function<void(GLuint)> RenderbufferIdAccessor;
	virtual ~IRenderbuffer() = default;
	virtual GLuint getBufferId() const = 0;
	void doActions(const RenderbufferAccessor& fun);
	void doActions(const RenderbufferConstAccessor& fun) const;
	void doActions(const RenderbufferIdAccessor& fun) const;
	void bind();
	void storage(GLenum internalformat, GLsizei width, GLsizei height);
};

class RenderbufferRef : public IRenderbuffer
{
private:
	GLuint bufferId;
public:
	RenderbufferRef(GLuint bufferId);
	~RenderbufferRef() override;
	RenderbufferRef(const RenderbufferRef& cpy);
	RenderbufferRef& operator=(const RenderbufferRef& cpy);
	GLuint getBufferId() const override;
};

class Renderbuffer : public IRenderbuffer
{
private:
	GLuint bufferId;
	Renderbuffer(const Renderbuffer& cpy) = delete;
	Renderbuffer& operator=(const Renderbuffer& cpy) = delete;
public:
	Renderbuffer();
	~Renderbuffer() override;
	Renderbuffer(Renderbuffer&& mov);
	Renderbuffer& operator=(Renderbuffer&& mov);
	GLuint getBufferId() const override;
	RenderbufferRef toRef() const;
};

}
#endif // GLRENDERBUFFER_HPP
