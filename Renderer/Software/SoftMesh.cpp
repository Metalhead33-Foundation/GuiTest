#include "SoftMesh.hpp"
#include <cstring>

namespace Renderer {
namespace Software {

UnindexedMesh::UnindexedMesh(const MH33::GFX::VertexDescriptor* vertexDescriptor)
	: vertexDescriptor(vertexDescriptor), vertexCount(0)
{

}

UnindexedMesh::UnindexedMesh(const MH33::GFX::VertexDescriptor* vertexDescriptor, const std::span<const std::byte>& bytes, size_t vertexCount)
	: vertexDescriptor(vertexDescriptor), vertexCount(vertexCount)
{
	buffer.resize(vertexCount * vertexDescriptor->stride);
	std::memcpy(buffer.data(),bytes.data(),buffer.size());
}

size_t UnindexedMesh::getVertexCount() const
{
	return vertexCount;
}

MH33::GFX::Handle UnindexedMesh::getNativeHandle()
{
	return { .ptr = buffer.data() };
}

MH33::GFX::ConstHandle UnindexedMesh::getNativeHandle() const
{
	return { .ptr = buffer.data() };
}

const MH33::GFX::VertexDescriptor* UnindexedMesh::getVertexDescriptor() const
{
	return vertexDescriptor;
}

void UnindexedMesh::access(const std::function<void (void*, size_t)>& accessor)
{
	std::lock_guard<std::mutex> lock(mutex);
	accessor(buffer.data(),vertexCount);
}

void UnindexedMesh::access(const std::function<void (const void*, size_t)>& accessor) const
{
	std::lock_guard<std::mutex> lock(mutex);
	accessor(buffer.data(),vertexCount);
}

void UnindexedMesh::ensureSize(size_t count)
{
	const size_t nsize = count * vertexDescriptor->stride;
	if(buffer.size() < nsize) {
		buffer.resize(nsize);
	}
	vertexCount = count;
}

IndexedMesh::IndexedMesh(const MH33::GFX::VertexDescriptor* vertexDescriptor)
	: vertexDescriptor(vertexDescriptor), vertexCount(0), indexCount(0)
{

}

IndexedMesh::IndexedMesh(const MH33::GFX::VertexDescriptor* vertexDescriptor, const std::span<const std::byte>& bytes, size_t vertexCount, const std::span<const uint32_t>& indices)
	: vertexDescriptor(vertexDescriptor), vertexCount(vertexCount), indexCount(indices.size())
{
	vertexBuffer.resize(vertexCount * vertexDescriptor->stride);
	std::memcpy(vertexBuffer.data(),bytes.data(),vertexBuffer.size());
	indexBuffer.resize(indices.size());
	std::copy(indices.begin(),indices.end(),indexBuffer.begin());
}

size_t  IndexedMesh::getVertexCount() const
{
	return vertexCount;
}

MH33::GFX::Handle  IndexedMesh::getVertexNativeHandle()
{
	return { .ptr = vertexBuffer.data() };
}

MH33::GFX::ConstHandle  IndexedMesh::getVertexNativeHandle() const
{
	return { .ptr = vertexBuffer.data() };
}

size_t  IndexedMesh::getIndexCount() const
{
	return indexCount;
}

MH33::GFX::Handle  IndexedMesh::getIndexNativeHandle()
{
	return { .ptr = indexBuffer.data() };
}

MH33::GFX::ConstHandle  IndexedMesh::getIndexNativeHandle() const
{
	return { .ptr = indexBuffer.data() };
}

const MH33::GFX::VertexDescriptor*  IndexedMesh::getVertexDescriptor() const
{
	return vertexDescriptor;
}

void  IndexedMesh::accessVertices(const std::function<void (void*, size_t)>& accessor)
{
	std::lock_guard<std::mutex> lock(mutex);
	accessor(vertexBuffer.data(),vertexCount);
}

void  IndexedMesh::accessVertices(const std::function<void (const void*, size_t)>& accessor) const
{
	std::lock_guard<std::mutex> lock(mutex);
	accessor(vertexBuffer.data(),vertexCount);
}

void  IndexedMesh::accessIndices(const std::function<void (std::span<uint32_t>&)>& accessor)
{
	std::lock_guard<std::mutex> lock(mutex);
	auto indexSpan = std::span<uint32_t>(indexBuffer.data(),indexBuffer.size());
	accessor(indexSpan);
}

void  IndexedMesh::accessIndices(const std::function<void (const std::span<const uint32_t>&)>& accessor) const
{
	auto indexSpan = std::span<const uint32_t>(indexBuffer.data(),indexBuffer.size());
	accessor(indexSpan);
}

void  IndexedMesh::ensureVertexCount(size_t count)
{
	const size_t nsize = count * vertexDescriptor->stride;
	if(vertexBuffer.size() < nsize) {
		vertexBuffer.resize(nsize);
	}
	vertexCount = count;
}

void  IndexedMesh::ensureIndexCount(size_t count)
{
	if(indexBuffer.size() < count) {
		indexBuffer.resize(count);
	}
	indexCount = count;
}

}
}
