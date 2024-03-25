#include "GlMesh.hpp"
namespace GL {
const Buffer& UnindexedMesh::getBuffer() const
{
	return buffer;
}

Buffer& UnindexedMesh::getBuffer()
{
	return buffer;
}

const Vao& UnindexedMesh::getVao() const
{
	return vao;
}

Vao& UnindexedMesh::getVao()
{
	return vao;
}

UnindexedMesh::UnindexedMesh(const MH33::GFX::VertexDescriptor* descriptor, bool streaming)
	: buffer(GL_ARRAY_BUFFER,streaming ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW), vao(), descriptor(descriptor)
{
	buffer.bind();
	buffer.bufferData(nullptr,1);
	vao.enableAttributes(*descriptor);
}

size_t UnindexedMesh::getVertexCount() const
{
	return buffer.getSize() / descriptor->stride;
}

MH33::GFX::Handle UnindexedMesh::getNativeHandle()
{
	return { .oglUint = buffer.getBuffId() };
}

MH33::GFX::ConstHandle UnindexedMesh::getNativeHandle() const
{
	return { .oglUint = buffer.getBuffId() };
}

const MH33::GFX::VertexDescriptor* UnindexedMesh::getVertexDescriptor() const
{
	return descriptor;
}

void UnindexedMesh::access(const std::function<void (void*, size_t)>& accessor)
{
	buffer.bind();
	auto range = buffer.map(GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT,0,buffer.getSize());
	accessor(range.data(),range.size());
	range.flush();
}

void UnindexedMesh::access(const std::function<void (const void*, size_t)>& accessor) const
{
	buffer.bind();
	auto range = buffer.map(GL_MAP_READ_BIT | GL_MAP_UNSYNCHRONIZED_BIT,0,buffer.getSize());
	accessor(range.data(),range.size());
}

void UnindexedMesh::ensureSize(size_t count)
{
	if(getVertexCount() < count) {
		buffer.bind();
		buffer.bufferData(nullptr,count * descriptor->stride);
	}
}

const Buffer& IndexedMesh::getVertexBuffer() const
{
	return vertexBuffer;
}

const Buffer& IndexedMesh::getIndexBuffer() const
{
	return indexBuffer;
}

const Vao& IndexedMesh::getVao() const
{
	return vao;
}

Buffer& IndexedMesh::getVertexBuffer()
{
	return vertexBuffer;
}

Buffer& IndexedMesh::getIndexBuffer()
{
	return indexBuffer;
}

Vao& IndexedMesh::getVao()
{
	return vao;
}

IndexedMesh::IndexedMesh(const MH33::GFX::VertexDescriptor* descriptor, bool streaming)
	: vertexBuffer(GL_ARRAY_BUFFER,streaming ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW),
	  indexBuffer(GL_ELEMENT_ARRAY_BUFFER,streaming ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW),
	  vao(), descriptor(descriptor)
{
	vertexBuffer.bind();
	vertexBuffer.bufferData(nullptr,1);
	indexBuffer.bind();
	indexBuffer.bufferData(nullptr,1);
	vao.enableAttributes(*descriptor);
}

size_t IndexedMesh::getVertexCount() const
{
	return vertexBuffer.getSize() / descriptor->stride;
}

MH33::GFX::Handle IndexedMesh::getVertexNativeHandle()
{
	return { .oglUint = vertexBuffer.getBuffId() };
}

MH33::GFX::ConstHandle IndexedMesh::getVertexNativeHandle() const
{
	return { .oglUint = vertexBuffer.getBuffId() };
}

size_t IndexedMesh::getIndexCount() const
{
	return indexBuffer.getSize() / sizeof(uint32_t);
}

MH33::GFX::Handle IndexedMesh::getIndexNativeHandle()
{
	return { .oglUint = indexBuffer.getBuffId() };
}

MH33::GFX::ConstHandle IndexedMesh::getIndexNativeHandle() const
{
	return { .oglUint = indexBuffer.getBuffId() };
}

const MH33::GFX::VertexDescriptor* IndexedMesh::getVertexDescriptor() const
{
	return descriptor;
}

void IndexedMesh::accessVertices(const std::function<void (void*, size_t)>& accessor)
{
	vertexBuffer.bind();
	auto range = vertexBuffer.map(GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT,0,vertexBuffer.getSize());
	accessor(range.data(),range.size());
	range.flush();
}

void IndexedMesh::accessVertices(const std::function<void (const void*, size_t)>& accessor) const
{
	vertexBuffer.bind();
	auto range = vertexBuffer.map(GL_MAP_READ_BIT | GL_MAP_UNSYNCHRONIZED_BIT,0,vertexBuffer.getSize());
	accessor(range.data(),range.size());
}

void IndexedMesh::accessIndices(const std::function<void (std::span<uint32_t>&)>& accessor)
{
	indexBuffer.bind();
	auto range = indexBuffer.map(GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT,0,indexBuffer.getSize());
	auto span = std::span<uint32_t>(static_cast<uint32_t*>(range.data()),getIndexCount());
	accessor(span);
	range.flush();
}

void IndexedMesh::accessIndices(const std::function<void (const std::span<const uint32_t>&)>& accessor) const
{
	indexBuffer.bind();
	auto range = indexBuffer.map(GL_MAP_READ_BIT | GL_MAP_UNSYNCHRONIZED_BIT,0,indexBuffer.getSize());
	auto span = std::span<uint32_t>(static_cast<uint32_t*>(range.data()),getIndexCount());
	accessor(span);
}

void IndexedMesh::ensureVertexCount(size_t count)
{
	if(getVertexCount() < count) {
		vertexBuffer.bind();
		vertexBuffer.bufferData(nullptr,count * descriptor->stride);
	}
}

void IndexedMesh::ensureIndexCount(size_t count)
{
	if(getIndexCount() < count)
	{
		indexBuffer.bind();
		indexBuffer.bufferData(nullptr,count * sizeof(uint32_t));
	}
}

}
