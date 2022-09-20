#include <GL/GlFramebuffer.hpp>
#include <GL/GlRenderbuffer.hpp>
#include <GL/GlTexture.hpp>

namespace GL {


std::recursive_mutex FramebufferEssentials::buffermutex;
typedef std::lock_guard<std::recursive_mutex> RecursiveLock;
GLuint FramebufferEssentials::lastBound = 0;
void FramebufferEssentials::ensureBound(GLuint id)
{
	if(lastBound != id) {
		glBindFramebuffer(GL_FRAMEBUFFER,id);
		lastBound = id;
	}
}
void IFramebuffer::attach(const IRenderbuffer& renderbuffer, GLenum attachment)
{
	RecursiveLock lock(FramebufferEssentials::buffermutex);
	FramebufferEssentials::ensureBound(getBufferId());
	glFramebufferRenderbuffer(GL_FRAMEBUFFER,attachment,GL_RENDERBUFFER,renderbuffer.getBufferId());
}

void IFramebuffer::attach(const ITexture& texture, GLenum attachment, GLint level)
{
	RecursiveLock lock(FramebufferEssentials::buffermutex);
	FramebufferEssentials::ensureBound(getBufferId());
	glFramebufferTexture2D(GL_FRAMEBUFFER,attachment,texture.getTarget(),texture.getTexId(),level);
}

bool IFramebuffer::isComplete()
{
	RecursiveLock lock(FramebufferEssentials::buffermutex);
	FramebufferEssentials::ensureBound(getBufferId());
	return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

GLuint Framebuffer::getBufferId() const
{
	return bufferId;
}

FramebufferRef Framebuffer::toRef() const
{
	return FramebufferRef(bufferId);
}

Framebuffer::Framebuffer()
{
	glGenFramebuffers(1,&bufferId);
}

Framebuffer::~Framebuffer()
{
	if(bufferId) glDeleteFramebuffers(1,&bufferId);
}

Framebuffer::Framebuffer(Framebuffer&& mov)
	: bufferId(mov.bufferId)
{
	mov.bufferId = 0;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& mov)
{
	this->bufferId = mov.bufferId;
	mov.bufferId = 0;
	return *this;
}

FramebufferRef::FramebufferRef(GLuint bufferId)
	: bufferId(bufferId)
{

}

FramebufferRef::~FramebufferRef()
{

}

FramebufferRef::FramebufferRef(const FramebufferRef& cpy)
	: bufferId(cpy.bufferId)
{

}

FramebufferRef& FramebufferRef::operator=(const FramebufferRef& cpy)
{
	this->bufferId = cpy.bufferId;
	return *this;
}

GLuint FramebufferRef::getBufferId() const
{
	return bufferId;
}

void IFramebuffer::doActions(const FramebufferAccessor& fun)
{
	RecursiveLock lock(FramebufferEssentials::buffermutex);
	FramebufferEssentials::ensureBound(getBufferId());
	fun(*this);
}

void IFramebuffer::doActions(const FramebufferConstAccessor& fun) const
{
	RecursiveLock lock(FramebufferEssentials::buffermutex);
	FramebufferEssentials::ensureBound(getBufferId());
	fun(*this);
}

void IFramebuffer::doActions(const FramebufferIdAccessor& fun) const
{
	RecursiveLock lock(FramebufferEssentials::buffermutex);
	FramebufferEssentials::ensureBound(getBufferId());
	fun(getBufferId());
}

void IFramebuffer::bind()
{
	RecursiveLock lock(FramebufferEssentials::buffermutex);
	FramebufferEssentials::ensureBound(getBufferId());
}

}
