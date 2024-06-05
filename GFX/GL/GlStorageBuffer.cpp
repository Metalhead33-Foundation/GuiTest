#include "GlStorageBuffer.hpp"

namespace GL {

ShaderStorageBuffer::ShaderStorageBuffer(size_t size)
	: handle(0), dataSize(0)
{
	glGenBuffers(1,&handle);
	if(size) ensureDataSize(size);
}

ShaderStorageBuffer::~ShaderStorageBuffer()
{
	if(handle) glDeleteBuffers(1,&handle);

}

ShaderStorageBuffer::ShaderStorageBuffer(ShaderStorageBuffer&& mov)
	: handle(mov.handle), dataSize(mov.dataSize)
{
	mov.handle = 0;
	mov.dataSize = 0;
}

ShaderStorageBuffer& ShaderStorageBuffer::operator=(ShaderStorageBuffer&& mov)
{
	this->handle = mov.handle;
	this->dataSize = mov.dataSize;
	mov.handle = 0;
	mov.dataSize = 0;
	return *this;
}

MH33::GFX::Handle  ShaderStorageBuffer::getNativeHandle()
{
	return { .oglUint = handle };
}

MH33::GFX::ConstHandle  ShaderStorageBuffer::getNativeHandle() const
{
	return { .oglUint = handle };
}

MH33::GFX::StorageBufferType  ShaderStorageBuffer::getType() const
{
	return MH33::GFX::StorageBufferType::SHADER_STORAGE_BUFFER;
}

void  ShaderStorageBuffer::bind() const
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
}

void  ShaderStorageBuffer::unbind() const
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void  ShaderStorageBuffer::bindBase(uint8_t bindingPoint) const
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, handle);
}

void ShaderStorageBuffer::bindRange(uint8_t bindingPoint, size_t offset, size_t length) const
{
	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bindingPoint, handle, offset, length);
}

void  ShaderStorageBuffer::initializeData(const std::span<const std::byte>& data)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
	glBufferData(GL_SHADER_STORAGE_BUFFER, data.size(), data.data(), GL_DYNAMIC_DRAW);
	dataSize = data.size();
}

void  ShaderStorageBuffer::getData(void* data, size_t offset, size_t length) const
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
	if(length) glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, length, data);
	else glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, dataSize, data);
}

size_t  ShaderStorageBuffer::getDataSize() const
{
	return dataSize;
}

void  ShaderStorageBuffer::ensureDataSize(size_t size)
{
	if(dataSize > size) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
		glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
		dataSize = size;
	}
}

void  ShaderStorageBuffer::setData(const AccessorFunc& fun, bool needsToRead, size_t offset, size_t length)
{
/*	glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
	void* mappedBuffer = glMapBuffer(GL_SHADER_STORAGE_BUFFER, needsToRead ?  GL_READ_WRITE :  GL_WRITE_ONLY);
	auto dataSpan = std::span<std::byte>( static_cast<std::byte*>(mappedBuffer), dataSize);
	fun(dataSpan);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);*/
	auto bitfield = generateGlMappedBitfield(needsToRead, !length);
	if(!length) length = dataSize;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
	void* mappedBuffer = glMapBufferRange(GL_SHADER_STORAGE_BUFFER,offset, length, bitfield);
	auto dataSpan = std::span<std::byte>( static_cast<std::byte*>(mappedBuffer), length);
	fun(dataSpan);
	glFlushMappedBufferRange(GL_SHADER_STORAGE_BUFFER,offset,length);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void  ShaderStorageBuffer::setData(const std::span<const std::byte>& data, size_t offset)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, data.size(), data.data());
}

void  ShaderStorageBuffer::getData(const ConstAccessorFunc& fun, size_t offset, size_t length) const
{
	if(!length) length = dataSize;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
	void* mappedBuffer = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, offset, length, GL_MAP_READ_BIT);
	auto dataSpan = std::span<std::byte>( static_cast<std::byte*>(mappedBuffer), length);
	fun(dataSpan);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

UniformBuffer::UniformBuffer(size_t size)
	: handle(0), dataSize(0)
{
	glGenBuffers(1,&handle);
	if(size) ensureDataSize(size);
}

UniformBuffer::~UniformBuffer()
{
	if(handle) glDeleteBuffers(1,&handle);
}

UniformBuffer::UniformBuffer(UniformBuffer&& mov)
	: handle(mov.handle), dataSize(mov.dataSize)
{
	mov.handle = 0;
	mov.dataSize = 0;
}

UniformBuffer& UniformBuffer::operator=(UniformBuffer&& mov)
{
	this->handle = mov.handle;
	this->dataSize = mov.dataSize;
	mov.handle = 0;
	mov.dataSize = 0;
	return *this;
}

MH33::GFX::Handle  UniformBuffer::getNativeHandle()
{
	return { .oglUint = handle };
}

MH33::GFX::ConstHandle  UniformBuffer::getNativeHandle() const
{
	return { .oglUint = handle };
}

MH33::GFX::StorageBufferType  UniformBuffer::getType() const
{
	return MH33::GFX::StorageBufferType::UNIFORM_BUFFER;
}

void  UniformBuffer::bind() const
{
	glBindBuffer(GL_UNIFORM_BUFFER, handle);
}

void  UniformBuffer::unbind() const
{
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void  UniformBuffer::bindBase(uint8_t bindingPoint) const
{
	glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, handle);
}

void UniformBuffer::bindRange(uint8_t bindingPoint, size_t offset, size_t length) const
{
	glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, handle, offset, length);
}

void  UniformBuffer::initializeData(const std::span<const std::byte>& data)
{
	glBindBuffer(GL_UNIFORM_BUFFER, handle);
	glBufferData(GL_UNIFORM_BUFFER, data.size(), data.data(), GL_DYNAMIC_DRAW);
	dataSize = data.size();
}

void  UniformBuffer::getData(void* data, size_t offset, size_t length) const
{
	if(length) glGetBufferSubData(GL_UNIFORM_BUFFER, offset, length, data);
	else glGetBufferSubData(GL_UNIFORM_BUFFER, offset, dataSize, data);
}

size_t  UniformBuffer::getDataSize() const
{
	return dataSize;
}

void  UniformBuffer::ensureDataSize(size_t size)
{
	if(dataSize > size) {
		glBindBuffer(GL_UNIFORM_BUFFER, handle);
		glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
		dataSize = size;
	}
}

void  UniformBuffer::setData(const AccessorFunc& fun, bool needsToRead, size_t offset, size_t length)
{
	auto bitfield = generateGlMappedBitfield(needsToRead, !length);
	if(!length) length = dataSize;
	glBindBuffer(GL_UNIFORM_BUFFER, handle);
	void* mappedBuffer = glMapBufferRange(GL_UNIFORM_BUFFER,offset, length, bitfield);
	auto dataSpan = std::span<std::byte>( static_cast<std::byte*>(mappedBuffer), length);
	fun(dataSpan);
	glFlushMappedBufferRange(GL_UNIFORM_BUFFER,offset,length);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
}

void  UniformBuffer::setData(const std::span<const std::byte>& data, size_t offset)
{
	glBindBuffer(GL_UNIFORM_BUFFER, handle);
	glBufferSubData(GL_UNIFORM_BUFFER, offset, data.size(), data.data());
}

void  UniformBuffer::getData(const ConstAccessorFunc& fun, size_t offset, size_t length) const
{
	if(!length) length = dataSize;
	glBindBuffer(GL_UNIFORM_BUFFER, handle);
	void* mappedBuffer = glMapBufferRange(GL_UNIFORM_BUFFER, offset, length, GL_MAP_READ_BIT);
	auto dataSpan = std::span<std::byte>( static_cast<std::byte*>(mappedBuffer), length);
	fun(dataSpan);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
}

}
