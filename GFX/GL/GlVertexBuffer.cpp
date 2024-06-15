#include "GlVertexBuffer.hpp"
#include "GlUtil.hpp"

namespace GL {

UnindexedVertexBuffer::UnindexedVertexBuffer(GLenum usage, const MH33::GFX::VertexDescriptor* vertexDescriptor, size_t size)
	: usage(usage), size(size), vertexDescriptor(vertexDescriptor)
{
	glGenBuffers(1,&vboIndex);
	glGenVertexArrays(1,&vaoIndex);
	glBindVertexArray(vaoIndex);
	glBindBuffer(GL_ARRAY_BUFFER,vboIndex);
	if(size) glBufferData(GL_ARRAY_BUFFER, size, 0, usage);
	glProcessVertexDescriptor(*vertexDescriptor);
}

UnindexedVertexBuffer::UnindexedVertexBuffer(UnindexedVertexBuffer&& mov)
	: vboIndex(mov.vboIndex), vaoIndex(mov.vaoIndex), usage(mov.usage), size(mov.size), vertexDescriptor(mov.vertexDescriptor)
{
	mov.vboIndex = 0;
	mov.vaoIndex = 0;
	mov.size = 0;
}

UnindexedVertexBuffer& UnindexedVertexBuffer::operator=(UnindexedVertexBuffer&& mov)
{
	this->vboIndex = mov.vboIndex;
	this->vaoIndex = mov.vaoIndex;
	this->size = mov.size;
	this->usage = mov.usage;
	this->vertexDescriptor = mov.vertexDescriptor;
	mov.vboIndex = 0;
	mov.vaoIndex = 0;
	mov.size = 0;
	return *this;
}

UnindexedVertexBuffer::~UnindexedVertexBuffer()
{
	if(vboIndex) glDeleteBuffers(1,&vboIndex);
	if(vaoIndex) glDeleteVertexArrays(1,&vaoIndex);
}

MH33::GFX::Handle UnindexedVertexBuffer::getNativeHandle()
{
	return { .oglUint = vboIndex };
}

MH33::GFX::ConstHandle UnindexedVertexBuffer::getNativeHandle() const
{
	return { .oglUint = vboIndex };
}

const MH33::GFX::VertexDescriptor* UnindexedVertexBuffer::getVertexDescriptor() const
{
	return vertexDescriptor;
}

void UnindexedVertexBuffer::bind() const
{
	glBindVertexArray(vaoIndex);
	glBindBuffer(GL_ARRAY_BUFFER, vboIndex);
}

void UnindexedVertexBuffer::unbind() const
{
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void UnindexedVertexBuffer::initializeData(const std::span<const std::byte>& data)
{
	glBindBuffer(GL_ARRAY_BUFFER, vboIndex);
	glBufferData(GL_ARRAY_BUFFER, data.size(), data.data(), usage);
	this->size = data.size();
}

void UnindexedVertexBuffer::getData(void* data) const
{
	glBindBuffer(GL_ARRAY_BUFFER, vboIndex);
	glGetBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

size_t UnindexedVertexBuffer::getDataSize() const
{
	return size;
}

void UnindexedVertexBuffer::ensureDataSize(size_t size)
{
	if(this->size < size) {
		glBindBuffer(GL_ARRAY_BUFFER, vboIndex);
		glBufferData(GL_ARRAY_BUFFER, size, 0, usage);
		this->size = size;
	}
}

void UnindexedVertexBuffer::setData(const AccessorFunc& fun, bool needsToRead, size_t offset, size_t length)
{
	/*glBindBuffer(GL_ARRAY_BUFFER, vboIndex);
	void* mappedBuffer = glMapBuffer(GL_ARRAY_BUFFER, needsToRead ?  GL_READ_WRITE :  GL_WRITE_ONLY);
	auto dataSpan = std::span<std::byte>( static_cast<std::byte*>(mappedBuffer), size);
	fun(dataSpan);
	glUnmapBuffer(GL_ARRAY_BUFFER);*/
	auto bitfield = generateGlMappedBitfield(needsToRead, !length);
	if(!length) length = size;
	glBindBuffer(GL_ARRAY_BUFFER, vboIndex);
	void* mappedBuffer = glMapBufferRange(GL_ARRAY_BUFFER,offset, length, bitfield);
	auto dataSpan = std::span<std::byte>( static_cast<std::byte*>(mappedBuffer), length);
	fun(dataSpan);
	glFlushMappedBufferRange(GL_ARRAY_BUFFER,offset,length);
	glUnmapBuffer(GL_ARRAY_BUFFER);
}

void UnindexedVertexBuffer::setData(const std::span<const std::byte>& data, size_t offset)
{
	glBindBuffer(GL_ARRAY_BUFFER, vboIndex);
	glBufferSubData(GL_ARRAY_BUFFER, offset, data.size(), data.data());
}

void UnindexedVertexBuffer::getData(const ConstAccessorFunc& fun, size_t offset, size_t length) const
{
	if(!length) length = size;
	glBindBuffer(GL_ARRAY_BUFFER, vboIndex);
	void* mappedBuffer = glMapBufferRange(GL_ARRAY_BUFFER, offset, length, GL_MAP_READ_BIT);
	auto dataSpan = std::span<std::byte>( static_cast<std::byte*>(mappedBuffer), length);
	fun(dataSpan);
	glUnmapBuffer(GL_ARRAY_BUFFER);
}

/*
	GLenum usage;
	size_t size, indexCount;
	const MH33::GFX::VertexDescriptor* vertexDescriptor;
*/

IndexedVertexBuffer::IndexedVertexBuffer(GLenum usage, const MH33::GFX::VertexDescriptor* vertexDescriptor, size_t dataSize, size_t indexSize)
	: usage(usage), size(dataSize), indexCount(indexSize), vertexDescriptor(vertexDescriptor)
{
	glGenBuffers(1,&vboIndex);
	glGenBuffers(1,&eboIndex);
	glGenVertexArrays(1,&vaoIndex);
	glBindBuffer(GL_ARRAY_BUFFER,vboIndex);
	if(size) glBufferData(GL_ARRAY_BUFFER, size, 0, usage);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,eboIndex);
	if(indexCount) glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(uint32_t), 0, usage);
	glBindVertexArray(vaoIndex);
	glProcessVertexDescriptor(*vertexDescriptor);
}

IndexedVertexBuffer::IndexedVertexBuffer(IndexedVertexBuffer&& mov)
	: vboIndex(mov.vboIndex), eboIndex(mov.eboIndex), vaoIndex(mov.vaoIndex), usage(mov.usage),
	  size(mov.size), indexCount(mov.indexCount), vertexDescriptor(mov.vertexDescriptor)
{
	mov.vboIndex = 0;
	mov.eboIndex = 0;
	mov.vaoIndex = 0;
	mov.size = 0;
	mov.indexCount = 0;
}

IndexedVertexBuffer& IndexedVertexBuffer::operator=(IndexedVertexBuffer&& mov)
{
	this->eboIndex = mov.eboIndex;
	this->vboIndex = mov.vboIndex;
	this->vaoIndex = mov.vaoIndex;
	this->size = mov.size;
	this->usage = mov.usage;
	this->vertexDescriptor = mov.vertexDescriptor;
	mov.eboIndex = 0;
	mov.vboIndex = 0;
	mov.vaoIndex = 0;
	mov.size = 0;
	return *this;
}

IndexedVertexBuffer::~IndexedVertexBuffer()
{
	// vboIndex, eboIndex, vaoIndex
	if(vboIndex) glDeleteBuffers(1,&vboIndex);
	if(eboIndex) glDeleteBuffers(1,&eboIndex);
	if(vaoIndex) glDeleteVertexArrays(1,&vaoIndex);
}

MH33::GFX::Handle IndexedVertexBuffer::getNativeHandle()
{
	return { .oglUint = vboIndex };
}

MH33::GFX::ConstHandle IndexedVertexBuffer::getNativeHandle() const
{
	return { .oglUint = vboIndex };
}

const MH33::GFX::VertexDescriptor* IndexedVertexBuffer::getVertexDescriptor() const
{
	return vertexDescriptor;
}

MH33::GFX::Handle IndexedVertexBuffer::getNativeHandleIndices()
{
	return { .oglUint = eboIndex };
}

MH33::GFX::ConstHandle IndexedVertexBuffer::getNativeHandleIndices() const
{
	return { .oglUint = eboIndex };
}

void IndexedVertexBuffer::bindData() const
{
	glBindVertexArray(vaoIndex);
	glBindBuffer(GL_ARRAY_BUFFER, vboIndex);
}

void IndexedVertexBuffer::unbindData() const
{
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void IndexedVertexBuffer::initializeData(const std::span<const std::byte>& data)
{
	glBindBuffer(GL_ARRAY_BUFFER, vboIndex);
	glBufferData(GL_ARRAY_BUFFER, data.size(), data.data(), usage);
	this->size = data.size();
}

void IndexedVertexBuffer::getData(void* data) const
{
	glBindBuffer(GL_ARRAY_BUFFER, vboIndex);
	glGetBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

size_t IndexedVertexBuffer::getDataSize() const
{
	return size;
}

void IndexedVertexBuffer::ensureDataSize(size_t size)
{
	if(this->size < size) {
		glBindBuffer(GL_ARRAY_BUFFER, vboIndex);
		glBufferData(GL_ARRAY_BUFFER, size, 0, usage);
		this->size = size;
	}
}

void IndexedVertexBuffer::setData(const DataAccessorFunc& fun, bool needsToRead, size_t offset, size_t length)
{
	/*glBindBuffer(GL_ARRAY_BUFFER, vboIndex);
	void* mappedBuffer = glMapBuffer(GL_ARRAY_BUFFER, needsToRead ?  GL_READ_WRITE :  GL_WRITE_ONLY);
	auto dataSpan = std::span<std::byte>( static_cast<std::byte*>(mappedBuffer), size);
	fun(dataSpan);
	glUnmapBuffer(GL_ARRAY_BUFFER);*/
	auto bitfield = generateGlMappedBitfield(needsToRead, !length);
	if(!length) length = size;
	glBindBuffer(GL_ARRAY_BUFFER, vboIndex);
	void* mappedBuffer = glMapBufferRange(GL_ARRAY_BUFFER,offset, length, bitfield);
	auto dataSpan = std::span<std::byte>( static_cast<std::byte*>(mappedBuffer), length);
	fun(dataSpan);
	glFlushMappedBufferRange(GL_ARRAY_BUFFER,offset,length);
	glUnmapBuffer(GL_ARRAY_BUFFER);
}

void IndexedVertexBuffer::setData(const std::span<const std::byte>& data, size_t offset)
{
	glBindBuffer(GL_ARRAY_BUFFER, vboIndex);
	glBufferSubData(GL_ARRAY_BUFFER, offset, data.size(), data.data());
}

void IndexedVertexBuffer::getData(const DataConstAccessorFunc& fun, size_t offset, size_t length) const
{
	if(!length) length = size;
	glBindBuffer(GL_ARRAY_BUFFER, vboIndex);
	void* mappedBuffer = glMapBufferRange(GL_ARRAY_BUFFER, offset, length, GL_MAP_READ_BIT);
	auto dataSpan = std::span<std::byte>( static_cast<std::byte*>(mappedBuffer), length);
	fun(dataSpan);
	glUnmapBuffer(GL_ARRAY_BUFFER);
}

void IndexedVertexBuffer::bindIndices() const
{
	glBindVertexArray(vaoIndex);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboIndex);
}

void IndexedVertexBuffer::unbindIndices() const
{
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void IndexedVertexBuffer::initializeIndices(const std::span<const uint32_t>& indices)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboIndex);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size_bytes(), indices.data(), usage);
	this->indexCount = indices.size();
}

void IndexedVertexBuffer::getIndices(uint32_t* indices) const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboIndex);
	glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexCount * sizeof(uint32_t), indices);
}

size_t IndexedVertexBuffer::getIndexCount() const
{
	return indexCount;
}

void IndexedVertexBuffer::ensureIndexCount(size_t size)
{
	if(this->indexCount < size) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboIndex);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * sizeof(uint32_t), 0, usage);
		this->indexCount = size;
	}
}

void IndexedVertexBuffer::setIndices(const IndexAccessorFunc& fun, bool needsToRead, size_t offset, size_t length)
{
	/*glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboIndex);
	void* mappedBuffer = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, needsToRead ?  GL_READ_WRITE :  GL_WRITE_ONLY);
	auto dataSpan = std::span<uint32_t>( static_cast<uint32_t*>(mappedBuffer), indexCount);
	fun(dataSpan);
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);*/
	auto bitfield = generateGlMappedBitfield(needsToRead, !length);
	if(!length) length = indexCount;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndex);
	void* mappedBuffer = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER,offset * sizeof(uint32_t), length * sizeof(uint32_t), bitfield);
	auto dataSpan = std::span<uint32_t>( static_cast<uint32_t*>(mappedBuffer), length);
	fun(dataSpan);
	glFlushMappedBufferRange(GL_ELEMENT_ARRAY_BUFFER,offset * sizeof(uint32_t), length * sizeof(uint32_t));
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}

void IndexedVertexBuffer::setIndices(const std::span<const uint32_t>& indices, size_t offset)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboIndex);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset * sizeof(uint32_t), indices.size_bytes(), indices.data());
}

void IndexedVertexBuffer::getIndices(const IndexConstAccessorFunc& fun, size_t offset, size_t length) const
{
	if(!length) length = indexCount;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboIndex);
	void* mappedBuffer = glMapBufferRange(GL_ARRAY_BUFFER, offset * sizeof(uint32_t), length * sizeof(uint32_t), GL_MAP_READ_BIT);
	auto dataSpan = std::span<uint32_t>( static_cast<uint32_t*>(mappedBuffer), length);
	fun(dataSpan);
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}


}
