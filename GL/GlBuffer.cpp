#include <GL/GlBuffer.hpp>
namespace GL {

std::recursive_mutex BufferEssentials::buffermutex;
typedef std::lock_guard<std::recursive_mutex> RecursiveLock;
GLuint BufferEssentials::lastBound = 0;
void BufferEssentials::ensureBound(GLuint id, GLenum target)
{
	if(lastBound != id) {
		glBindBuffer(target,id);
		lastBound = id;
	}
}

GLuint Buffer::getBuffId() const
{
	return buffId;
}

GLenum Buffer::getTarget() const
{
	return target;
}

GLenum Buffer::getUsage() const
{
	return usage;
}

BufferRef Buffer::toRef()
{
	return BufferRef(buffId,target,usage);
}

Buffer::Buffer(GLenum target, GLenum usage) : target(target), usage(usage)
{
	glGenBuffers(1,&buffId);
}

Buffer::~Buffer()
{
	if(!buffId) glDeleteBuffers(1,&buffId);
}

Buffer::Buffer(const Buffer& cpy)
	: target(cpy.target), usage(cpy.usage)
{
	RecursiveLock lock(BufferEssentials::buffermutex);
	glGenBuffers(1,&buffId);
	glBindBuffer(GL_COPY_READ_BUFFER,cpy.buffId);
	glBindBuffer(GL_COPY_WRITE_BUFFER,buffId);
	GLint buffsize;
	glGetBufferParameteriv(GL_COPY_READ_BUFFER,GL_BUFFER_SIZE,&buffsize);
	glBufferData(GL_COPY_WRITE_BUFFER,buffsize,nullptr,cpy.usage);
	glCopyBufferSubData(GL_COPY_READ_BUFFER,GL_COPY_WRITE_BUFFER,0,0,buffsize);
}
Buffer& Buffer::operator=(const Buffer& cpy) {
	this->target = cpy.target;
	this->usage = cpy.usage;
	RecursiveLock lock(BufferEssentials::buffermutex);
	GLint buffsizeR, buffsizeW;
	glBindBuffer(GL_COPY_READ_BUFFER,cpy.buffId);
	glBindBuffer(GL_COPY_WRITE_BUFFER,buffId);
	glGetBufferParameteriv(GL_COPY_READ_BUFFER,GL_BUFFER_SIZE,&buffsizeR);
	glGetBufferParameteriv(GL_COPY_WRITE_BUFFER,GL_BUFFER_SIZE,&buffsizeW);
	if(buffsizeR != buffsizeW) glBufferData(GL_COPY_WRITE_BUFFER,buffsizeR,nullptr,cpy.usage);
	glCopyBufferSubData(GL_COPY_READ_BUFFER,GL_COPY_WRITE_BUFFER,0,0,buffsizeR);
	return *this;
}

void IBuffer::bufferData(const void* data, size_t size)
{
	RecursiveLock lock(BufferEssentials::buffermutex);
	BufferEssentials::ensureBound(getBuffId(),getTarget());
	glBufferData(getTarget(),size,data,getUsage());
}

void IBuffer::bufferSubData(size_t offset, const void* data, size_t size)
{
	RecursiveLock lock(BufferEssentials::buffermutex);
	BufferEssentials::ensureBound(getBuffId(),getTarget());
	glBufferSubData(getTarget(),offset,size,data);
}

void IBuffer::copy(IBuffer& target, size_t readOffset, size_t writeOffset, size_t size) const
{
	RecursiveLock lock(BufferEssentials::buffermutex);
	glBindBuffer(GL_COPY_READ_BUFFER,getBuffId());
	glBindBuffer(GL_COPY_WRITE_BUFFER,target.getBuffId());
	glCopyBufferSubData(GL_COPY_READ_BUFFER,GL_COPY_WRITE_BUFFER,readOffset,writeOffset,size);
}

BufferMappedRange IBuffer::map(GLbitfield access, size_t offset, size_t size)
{
	return BufferMappedRange(this,size,offset,access);
}

size_t IBuffer::getSize() const
{
	GLint toReturn;
	RecursiveLock lock(BufferEssentials::buffermutex);
	BufferEssentials::ensureBound(getBuffId(),getTarget());
	glGetBufferParameteriv(getTarget(),GL_BUFFER_SIZE,&toReturn);
	return toReturn;
}

void IBuffer::bind()
{
	RecursiveLock lock(BufferEssentials::buffermutex);
	BufferEssentials::ensureBound(getBuffId(),getTarget());
}

void IBuffer::bind(GLenum target)
{
	RecursiveLock lock(BufferEssentials::buffermutex);
	BufferEssentials::ensureBound(getBuffId(),target);
}

void IBuffer::doActions(const BufferAccessor& fun)
{
	RecursiveLock lock(BufferEssentials::buffermutex);
	BufferEssentials::ensureBound(getBuffId(),getTarget());
	fun(*this);
}

void IBuffer::doActions(const BufferConstAccessor& fun) const
{
	RecursiveLock lock(BufferEssentials::buffermutex);
	BufferEssentials::ensureBound(getBuffId(),getTarget());
	fun(*this);
}

void GL::IBuffer::doActions(const BufferIdAccessor1& fun) const
{
	RecursiveLock lock(BufferEssentials::buffermutex);
	BufferEssentials::ensureBound(getBuffId(),getTarget());
	fun(getBuffId());
}

void GL::IBuffer::doActions(const BufferIdAccessor2& fun) const
{
	RecursiveLock lock(BufferEssentials::buffermutex);
	BufferEssentials::ensureBound(getBuffId(),getTarget());
	fun(getBuffId(),getTarget());
}

void GL::IBuffer::doActions(const BufferIdAccessor3& fun) const
{
	RecursiveLock lock(BufferEssentials::buffermutex);
	BufferEssentials::ensureBound(getBuffId(),getTarget());
	fun(getBuffId(),getTarget(),getUsage());
}

void GL::IBuffer::draw(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
	RecursiveLock lock(BufferEssentials::buffermutex);
	BufferEssentials::ensureBound(getBuffId(),getTarget());
	glDrawElements(mode,count,type,indices);
}

void GL::IBuffer::draw(GLenum mode, GLsizei count, GLenum type, uintptr_t indicesOffset)
{
	RecursiveLock lock(BufferEssentials::buffermutex);
	BufferEssentials::ensureBound(getBuffId(),getTarget());
	glDrawElements(mode,count,type,reinterpret_cast<void*>(indicesOffset));
}

Buffer::Buffer(Buffer&& mov)
	: buffId(mov.buffId), target(mov.target), usage(mov.usage)
{
	mov.buffId = 0;
	mov.target = 0;
	mov.usage = 0;
}

Buffer& Buffer::operator=(Buffer&& mov)
{
	if(!buffId) glDeleteBuffers(1,&buffId);
	this->buffId = mov.buffId;
	this->target = mov.target;
	this->usage = mov.usage;
	mov.buffId = 0;
	mov.target = 0;
	mov.usage = 0;
	return *this;
}

BufferMappedRange::BufferMappedRange(IBuffer* buffer, size_t size, size_t offset, GLbitfield access)
	: buffer(buffer), _data(nullptr), _size(size), _offset(offset)
{
	RecursiveLock lock(BufferEssentials::buffermutex);
	BufferEssentials::ensureBound(buffer->getBuffId(),buffer->getTarget());
	_data = glMapBufferRange(buffer->getTarget(),offset,size,access);
}

BufferMappedRange::~BufferMappedRange()
{
	if(buffer && _data) {
		RecursiveLock lock(BufferEssentials::buffermutex);
		BufferEssentials::ensureBound(buffer->getBuffId(),buffer->getTarget());
		glUnmapBuffer(buffer->getTarget());
	}
}

/*
	IBuffer* buffer;
	void* _data;
	size_t _size, _offset;
*/

size_t BufferMappedRange::size() const
{
	return _size;
}

size_t BufferMappedRange::offset() const
{
	return _offset;
}

void* BufferMappedRange::data()
{
	return _data;
}

const void* BufferMappedRange::data() const
{
	return _data;
}

std::byte& BufferMappedRange::get(size_t i)
{
	return static_cast<std::byte*>(_data)[i];
}

const std::byte& BufferMappedRange::get(size_t i) const
{
	return static_cast<std::byte*>(_data)[i];
}

std::byte& BufferMappedRange::operator[](size_t i)
{
	return static_cast<std::byte*>(_data)[i];
}

std::span<std::byte> BufferMappedRange::toSpan()
{
	return std::span<std::byte>(static_cast<std::byte*>(_data),_size);
}

std::span<const std::byte> BufferMappedRange::toSpan() const
{
	return std::span<const std::byte>(static_cast<const std::byte*>(_data),_size);
}

void BufferMappedRange::flush()
{
	RecursiveLock lock(BufferEssentials::buffermutex);
	BufferEssentials::ensureBound(buffer->getBuffId(),buffer->getTarget());
	glFlushMappedBufferRange(buffer->getTarget(),_offset,_size);
}

const std::byte& BufferMappedRange::operator[](size_t i) const
{
	return static_cast<std::byte*>(_data)[i];
}

BufferMappedRange::BufferMappedRange(BufferMappedRange&& mov)
	: buffer(mov.buffer), _data(mov._data), _size(mov._size), _offset(mov._offset)
{
	mov.buffer = nullptr;
	mov._data = nullptr;
	mov._size = 0;
	mov._offset = 0;
}

BufferMappedRange& BufferMappedRange::operator=(BufferMappedRange&& mov)
{
	if(buffer && _data) {
		RecursiveLock lock(BufferEssentials::buffermutex);
		BufferEssentials::ensureBound(buffer->getBuffId(),buffer->getTarget());
		glUnmapBuffer(buffer->getTarget());
	}
	this->buffer = mov.buffer;
	this->_data = mov._data;
	this->_size = mov._size;
	this->_offset = mov._offset;
	mov.buffer = nullptr;
	mov._data = nullptr;
	mov._size = 0;
	mov._offset = 0;
	return *this;
}

BufferRef::BufferRef(const GLuint buffId, const GLenum target, const GLenum usage)
	: buffId(buffId), target(target), usage(usage)
{

}

BufferRef::~BufferRef()
{

}

BufferRef::BufferRef(const BufferRef& cpy)
	: buffId(cpy.buffId), target(cpy.target), usage(cpy.usage)
{

}

BufferRef& BufferRef::operator=(const BufferRef& cpy)
{
	this->buffId = cpy.buffId;
	this->target = cpy.target;
	this->usage = cpy.usage;
	return *this;
}

GLuint BufferRef::getBuffId() const
{
	return buffId;
}

GLenum BufferRef::getTarget() const
{
	return target;
}

GLenum BufferRef::getUsage() const
{
	return usage;
}

}
