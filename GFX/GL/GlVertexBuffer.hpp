#ifndef GLVERTEXBUFFER_HPP
#define GLVERTEXBUFFER_HPP
#include <GFX/Abstract/VertexBuffer.hpp>
#include <NativeGfxApi/glad/glwrap.h>

namespace GL {

void glProcessVertexDescriptor(const MH33::GFX::VertexDescriptor& vertexDescriptor);

class UnindexedVertexBuffer : public MH33::GFX::UnindexedVertexBuffer {
private:
	GLuint vboIndex, vaoIndex;
	GLenum usage;
	size_t size;
	const MH33::GFX::VertexDescriptor* vertexDescriptor;
	UnindexedVertexBuffer(const UnindexedVertexBuffer& cpy) = delete;
	UnindexedVertexBuffer& operator=(const UnindexedVertexBuffer& cpy) = delete;
public:
	UnindexedVertexBuffer(GLenum usage, const MH33::GFX::VertexDescriptor* vertexDescriptor, size_t size = 0);
	UnindexedVertexBuffer(UnindexedVertexBuffer&& mov);
	UnindexedVertexBuffer& operator=(UnindexedVertexBuffer&& mov);
	~UnindexedVertexBuffer();
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	const MH33::GFX::VertexDescriptor* getVertexDescriptor() const override;
	void bind() const override;
	void unbind() const override;
	void initializeData(const std::span<const std::byte>& data) override;
	void getData(void* data) const override;
	size_t getDataSize() const override;
	void ensureDataSize(size_t size) override;
	void setData(const AccessorFunc& fun, bool needsToRead, size_t offset = 0, size_t length = 0) override;
	void setData(const std::span<const std::byte>& data, size_t offset) override;
	void getData(const ConstAccessorFunc& fun, size_t offset = 0, size_t length = 0) const override;
};

class IndexedVertexBuffer : public MH33::GFX::IndexedVertexBuffer {
private:
	GLuint vboIndex, eboIndex, vaoIndex;
	GLenum usage;
	size_t size, indexCount;
	const MH33::GFX::VertexDescriptor* vertexDescriptor;
	IndexedVertexBuffer(const IndexedVertexBuffer& cpy) = delete;
	IndexedVertexBuffer& operator=(const IndexedVertexBuffer& cpy) = delete;
	// GfxResource interface
public:
	IndexedVertexBuffer(GLenum usage, const MH33::GFX::VertexDescriptor* vertexDescriptor, size_t dataSize = 0, size_t indexSize = 0);
	IndexedVertexBuffer(IndexedVertexBuffer&& mov);
	IndexedVertexBuffer& operator=(IndexedVertexBuffer&& mov);
	~IndexedVertexBuffer();
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	const MH33::GFX::VertexDescriptor* getVertexDescriptor() const override;
	MH33::GFX::Handle getNativeHandleIndices() override;
	MH33::GFX::ConstHandle getNativeHandleIndices() const override;
	void bindData() const override;
	void unbindData() const override;
	void initializeData(const std::span<const std::byte>& data) override;
	void getData(void* data) const override;
	size_t getDataSize() const override;
	void ensureDataSize(size_t size) override;
	void setData(const DataAccessorFunc& fun, bool needsToRead, size_t offset = 0, size_t length = 0) override;
	void setData(const std::span<const std::byte>& data, size_t offset) override;
	void getData(const DataConstAccessorFunc& fun, size_t offset = 0, size_t length = 0) const override;
	void bindIndices() const override;
	void unbindIndices() const override;
	void initializeIndices(const std::span<const uint32_t>& indices) override;
	void getIndices(uint32_t* indices) const override;
	size_t getIndexCount() const override;
	void ensureIndexCount(size_t size) override;
	void setIndices(const IndexAccessorFunc& fun, bool needsToRead, size_t offset = 0, size_t length = 0) override;
	void setIndices(const std::span<const uint32_t>& indices, size_t offset) override;
	void getIndices(const IndexConstAccessorFunc& fun, size_t offset = 0, size_t length = 0) const override;
};

}

#endif // GLVERTEXBUFFER_HPP
