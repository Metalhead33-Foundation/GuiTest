#ifndef GLFRAMEBUFFER_HPP
#define GLFRAMEBUFFER_HPP
#include <GL/gles2.h>
#include <string>
#include <span>
#include <vector>
#include <mutex>
#include <functional>
namespace GL {

struct FramebufferEssentials {
	static std::recursive_mutex buffermutex;
	static GLuint lastBound;
	static void ensureBound(GLuint id);
};

class IRenderbuffer;
class ITexture;
class IFramebuffer
{
public:
	typedef std::function<void(IFramebuffer&)> FramebufferAccessor;
	typedef std::function<void(const IFramebuffer&)> FramebufferConstAccessor;
	typedef std::function<void(GLuint)> FramebufferIdAccessor;
	virtual ~IFramebuffer() = default;
	virtual GLuint getBufferId() const = 0;
	void doActions(const FramebufferAccessor& fun);
	void doActions(const FramebufferConstAccessor& fun) const;
	void doActions(const FramebufferIdAccessor& fun) const;
	void bind();
	void attach(const IRenderbuffer& renderbuffer, GLenum attachment);
	void attach(const ITexture& texture, GLenum attachment, GLint level = 0);
	bool isComplete();
};

class FramebufferRef : public IFramebuffer
{
private:
	GLuint bufferId;
public:
	FramebufferRef(GLuint bufferId);
	~FramebufferRef() override;
	FramebufferRef(const FramebufferRef& cpy);
	FramebufferRef& operator=(const FramebufferRef& cpy);
	GLuint getBufferId() const override;
};

class Framebuffer : public IFramebuffer
{
private:
	GLuint bufferId;
	Framebuffer(const Framebuffer& cpy) = delete;
	Framebuffer& operator=(const Framebuffer& cpy) = delete;
public:
	Framebuffer();
	~Framebuffer() override;
	Framebuffer(Framebuffer&& mov);
	Framebuffer& operator=(Framebuffer&& mov);
	GLuint getBufferId() const override;
	FramebufferRef toRef() const;
};

}
#endif // GLFRAMEBUFFER_HPP
