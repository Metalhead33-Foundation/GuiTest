#include <GL/GlRenderbuffer.hpp>

namespace GL {

std::recursive_mutex RenderbufferEssentials::buffermutex;
typedef std::lock_guard<std::recursive_mutex> RecursiveLock;
GLuint RenderbufferEssentials::lastBound = 0;
void RenderbufferEssentials::ensureBound(GLuint id)
{
	if(lastBound != id) {
		glBindRenderbuffer(GL_RENDERBUFFER,id);
		lastBound = id;
	}
}

GLuint Renderbuffer::getBufferId() const
{
	return bufferId;
}

RenderbufferRef Renderbuffer::toRef() const
{
	return RenderbufferRef(bufferId);
}

Renderbuffer::Renderbuffer()
{
	glGenRenderbuffers(1,&bufferId);
}

Renderbuffer::~Renderbuffer()
{
	if(bufferId) glDeleteRenderbuffers(1,&bufferId);
}

Renderbuffer::Renderbuffer(Renderbuffer&& mov)
	: bufferId(mov.bufferId)
{
	mov.bufferId = 0;
}

Renderbuffer& Renderbuffer::operator=(Renderbuffer&& mov)
{
	this->bufferId = mov.bufferId;
	mov.bufferId = 0;
	return *this;
}

RenderbufferRef::RenderbufferRef(GLuint bufferId)
	: bufferId(bufferId)
{

}

RenderbufferRef::~RenderbufferRef()
{

}

RenderbufferRef::RenderbufferRef(const RenderbufferRef& cpy)
	: bufferId(cpy.bufferId)
{

}

RenderbufferRef& RenderbufferRef::operator=(const RenderbufferRef& cpy)
{
	this->bufferId = cpy.bufferId;
	return *this;
}

GLuint RenderbufferRef::getBufferId() const
{
	return bufferId;
}

void IRenderbuffer::doActions(const RenderbufferAccessor& fun)
{
	RecursiveLock lock(RenderbufferEssentials::buffermutex);
	RenderbufferEssentials::ensureBound(getBufferId());
	fun(*this);
}

void IRenderbuffer::doActions(const RenderbufferConstAccessor& fun) const
{
	RecursiveLock lock(RenderbufferEssentials::buffermutex);
	RenderbufferEssentials::ensureBound(getBufferId());
	fun(*this);
}

void IRenderbuffer::doActions(const RenderbufferIdAccessor& fun) const
{
	RecursiveLock lock(RenderbufferEssentials::buffermutex);
	RenderbufferEssentials::ensureBound(getBufferId());
	fun(getBufferId());
}

void IRenderbuffer::bind()
{
	RecursiveLock lock(RenderbufferEssentials::buffermutex);
	RenderbufferEssentials::ensureBound(getBufferId());
}

void IRenderbuffer::storage(GLenum internalformat, GLsizei width, GLsizei height)
{
	RecursiveLock lock(RenderbufferEssentials::buffermutex);
	RenderbufferEssentials::ensureBound(getBufferId());
	glRenderbufferStorage(GL_RENDERBUFFER,internalformat,width,height);
}

}
