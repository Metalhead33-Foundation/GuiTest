#ifndef VERTEXBUFFERADAPTER_H
#define VERTEXBUFFERADAPTER_H
#include "VertexBuffer.hpp"
#include <memory>

namespace MH33 {
namespace GFX {

template <typename T> class TypedVertexBuffer {
private:
	std::unique_ptr<UnindexedVertexBuffer> _buffer;
	size_t elementCount;
	TypedVertexBuffer(const TypedVertexBuffer& cpy) = delete;
	TypedVertexBuffer& operator=(const TypedVertexBuffer& cpy) = delete;
public:
	typedef std::function<UnindexedVertexBuffer*(const VertexDescriptor*,size_t)> BufferCreator;
	TypedVertexBuffer(const BufferCreator& creator, const VertexDescriptor* descriptor, size_t elementCount)
		: _buffer(creator(descriptor, elementCount * sizeof(T))), elementCount(elementCount) {

	}
	TypedVertexBuffer(TypedVertexBuffer&& mov) : _buffer(std::move(mov._buffer)), elementCount(mov.elementCount) {
		mov.elementCount = 0;
	}
	const VertexDescriptor* getVertexDescriptor() const { _buffer->getVertexDescriptor(); }
	TypedVertexBuffer& operator=(TypedVertexBuffer&& mov) {
		_buffer = std::move(mov._buffer);
		elementCount = mov.elementCount;
		mov.elementCount = 0;
		return *this;
	}
	// The actual wrapper stuff
	typedef std::function<void(const std::span<T>& data)> AccessorFunc;
	typedef std::function<void(const std::span<const T>& data)> ConstAccessorFunc;
	inline void bind() const { _buffer->bind(); }
	inline void unbind() const { _buffer->unbind(); }
	inline void initializeData(const std::span<const T>& data) { _buffer->initializeData(std::span<const std::byte>(
																	reinterpret_cast<const std::byte*>(data.data()), data.size_bytes() ));
																}
	inline void getData(T* data) { _buffer->getData(data); }
	inline size_t getElementCount() const { return elementCount; }
	inline void ensureElementCount(size_t size) { elementCount = size; _buffer->ensureDataSize(size * sizeof(T)); }
	inline void setData(const AccessorFunc& fun, bool needsToRead) {
		_buffer->setData([&fun](const std::span<std::byte>& data) {
			std::span<T> spanAsT(std::reinterpret_pointer_cast<T*>(data.data()),data.size() / sizeof(T));
			fun(spanAsT);
		}, needsToRead);
	}
	inline void setData(const std::span<const T>& data, size_t offset) {
		std::span<const std::byte> asByteSpan(reinterpret_cast<const std::byte*>(data.data()), data.size_bytes());
		_buffer->setData(asByteSpan, offset * sizeof(T));
	}
	inline void getData(const ConstAccessorFunc& fun) {
		_buffer->getData([&fun](const std::span<const std::byte>& data) {
			std::span<const T> spanAsT(reinterpret_cast<const T*>(data.data()),data.size() / sizeof(T));
			fun(spanAsT);
		});
	}
	Handle getNativeHandle() {
		return _buffer->getNativeHandle();
	}
	ConstHandle getNativeHandle() const  {
		return static_cast<const UnindexedVertexBuffer*>( _buffer.get() )->getNativeHandle();
	}
};
template <typename T> class IndexedTypedVertexBuffer {
private:
	std::unique_ptr<IndexedVertexBuffer> _buffer;
	size_t elementCount;
	IndexedTypedVertexBuffer(const IndexedTypedVertexBuffer& cpy) = delete;
	IndexedTypedVertexBuffer& operator=(const IndexedTypedVertexBuffer& cpy) = delete;
public:
	typedef std::function<IndexedTypedVertexBuffer*(const VertexDescriptor*,size_t, size_t)> BufferCreator;
	IndexedTypedVertexBuffer(const BufferCreator& creator, const VertexDescriptor* descriptor, size_t elementCount, size_t indexCount) :
		_buffer(creator(descriptor, elementCount * sizeof(T),indexCount)), elementCount(elementCount) {

	}
	IndexedTypedVertexBuffer(IndexedTypedVertexBuffer&& mov) : _buffer(std::move(mov._buffer)), elementCount(mov.elementCount) {
		mov.elementCount = 0;
	}
	IndexedTypedVertexBuffer& operator=(IndexedTypedVertexBuffer&& mov) {
		_buffer = std::move(mov._buffer);
		elementCount = mov.elementCount;
		mov.elementCount = 0;
		return *this;
	}
	Handle getNativeHandle() {
		return _buffer->getNativeHandle();
	}
	ConstHandle getNativeHandle() const  {
		return static_cast<const IndexedVertexBuffer*>( _buffer.get() )->getNativeHandle();
	}
	Handle getNativeHandleIndices() { return _buffer->getNativeHandleIndices(); }
	ConstHandle getNativeHandleIndices() const { return static_cast<const IndexedVertexBuffer*>( _buffer.get() )->getNativeHandleIndices(); }
	const VertexDescriptor* getVertexDescriptor() const { _buffer->getVertexDescriptor(); }
	// Let's wrap around the index stuff
	inline void bindIndices() const { _buffer->bindIndices(); }
	inline void unbindIndices() const { _buffer->unbindIndices(); }
	inline void initializeIndices(const std::span<const uint32_t>& indices) { _buffer->initializeIndices(indices); }
	inline void getIndices(uint32_t* indices) const { _buffer->getIndices(indices); }
	inline size_t getIndexCount() { return _buffer->getIndexCount(); }
	inline void ensureIndexCount(size_t size) { _buffer->ensureIndexCount(size); }
	inline void setIndices(const IndexedVertexBuffer::IndexAccessorFunc& fun, bool needsToRead) { _buffer->setIndices(fun,needsToRead); }
	inline void setIndices(const std::span<const uint32_t>& indices, size_t offset) { _buffer->setIndices(indices,offset); }
	inline void getIndices(const IndexedVertexBuffer::IndexConstAccessorFunc& fun) const { _buffer->getIndices(fun); }
	// The actual wrapper stuff
	typedef std::function<void(const std::span<T>& data)> AccessorFunc;
	typedef std::function<void(const std::span<const T>& data)> ConstAccessorFunc;
	inline void bind() const { _buffer->bindData(); }
	inline void unbind() const { _buffer->unbindData(); }
	inline void initializeData(const std::span<const T>& data) { _buffer->initializeData(std::span<const std::byte>(
																	reinterpret_cast<const std::byte*>(data.data()), data.size_bytes() ));
																}
	inline void getData(T* data) { _buffer->getData(data); }
	inline size_t getElementCount() const { return elementCount; }
	inline void ensureElementCount(size_t size) { elementCount = size; _buffer->ensureDataSize(size * sizeof(T)); }
	inline void setData(const AccessorFunc& fun, bool needsToRead) {
		_buffer->setData([&fun](const std::span<std::byte>& data) {
			std::span<T> spanAsT(reinterpret_cast<T*>(data.data()),data.size() / sizeof(T));
			fun(spanAsT);
		}, needsToRead);
	}
	inline void setData(const std::span<const T>& data, size_t offset) {
		std::span<const std::byte> asByteSpan(reinterpret_cast<const std::byte*>(data.data()), data.size_bytes());
		_buffer->setData(asByteSpan, offset * sizeof(T));
	}
	inline void getData(const ConstAccessorFunc& fun) {
		_buffer->getData([&fun](const std::span<const std::byte>& data) {
			std::span<const T> spanAsT(std::reinterpret_pointer_cast<const T*>(data.data()),data.size() / sizeof(T));
			fun(spanAsT);
		});
	}
};

}
}

#endif // VERTEXBUFFERADAPTER_H
