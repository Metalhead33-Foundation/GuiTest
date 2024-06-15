#ifndef VERTEXBUFFER_H
#define VERTEXBUFFER_H
#include <MhLib/Util/MhGlobals.hpp>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <span>
#include "NativeHandle.hpp"
#include "VertexFormat.hpp"
namespace MH33 {
namespace GFX {

enum class VertexBufferUsageClass {
	Static,
	Dynamic
};

DEFINE_CLASS(UnindexedVertexBuffer)
class UnindexedVertexBuffer : public GfxResource {
public:
	typedef std::function<void(const std::span<std::byte>& data)> AccessorFunc;
	typedef std::function<void(const std::span<const std::byte>& data)> ConstAccessorFunc;
	virtual ~UnindexedVertexBuffer() = default;
	virtual const VertexDescriptor* getVertexDescriptor() const = 0;
	virtual void bind() const = 0;
	virtual void unbind() const = 0;
	virtual void initializeData(const std::span<const std::byte>& data) = 0;
	virtual void getData(void* data) const = 0;
	virtual size_t getDataSize() const = 0;
	virtual void ensureDataSize(size_t size) = 0;
	virtual void setData(const AccessorFunc& fun, bool needsToRead, size_t offset = 0, size_t length = 0) = 0;
	virtual void setData(const std::span<const std::byte>& data, size_t offset) = 0;
	virtual void getData(const ConstAccessorFunc& fun, size_t offset = 0, size_t length = 0) const = 0;
	// Typed Data Access Wrappers
	template <typename T> inline void initializeDataT(const std::span<const T>& data) {
		initializeData(std::span<const std::byte>(reinterpret_cast<const std::byte*>(data.data(),data.size_bytes())));
	}
	template <typename T> inline void setDataT(const std::span<const T>& data, size_t offset = 0) {
		setData(std::span<const std::byte>(reinterpret_cast<const std::byte*>(data.data()),data.size_bytes()),offset * sizeof(T));
	}
	template <typename T> inline void setDataT(const std::function<void( std::span<T>& data)>& fun, bool needsToRead, size_t offset = 0, size_t length = 0) {
		setData([&fun](const std::span<std::byte>& data) {
			std::span<T> dataspan(reinterpret_cast<T*>(data.data()),data.size() / sizeof(T));
			fun(dataspan);
		}, needsToRead, offset * sizeof(T), length * sizeof(T));
	}
	template <typename T> inline void getDataT(const std::function<void(const std::span<const T>& data)>& fun, size_t offset = 0, size_t length = 0) {
		getData([&fun](const std::span<const std::byte>& data) {
			const std::span<const T> dataspan(reinterpret_cast<const T*>(data.data()),data.size() / sizeof(T));
			fun(dataspan);
		}, offset, length);
	}
};

DEFINE_CLASS(IndexedVertexBuffer)
class IndexedVertexBuffer : public GfxResource {
public:
	typedef std::function<void(const std::span<std::byte>& data)> DataAccessorFunc;
	typedef std::function<void(const std::span<const std::byte>& data)> DataConstAccessorFunc;
	typedef std::function<void(const std::span<uint32_t>& data)> IndexAccessorFunc;
	typedef std::function<void(const std::span<const uint32_t>& data)> IndexConstAccessorFunc;
	virtual ~IndexedVertexBuffer() = default;
	virtual const VertexDescriptor* getVertexDescriptor() const = 0;
	virtual Handle getNativeHandle() = 0;
	virtual ConstHandle getNativeHandle() const = 0;
	virtual Handle getNativeHandleIndices() = 0;
	virtual ConstHandle getNativeHandleIndices() const = 0;
	// Data
	virtual void bindData() const = 0;
	virtual void unbindData() const = 0;
	virtual void initializeData(const std::span<const std::byte>& data) = 0;
	virtual void getData(void* data) const = 0;
	virtual size_t getDataSize() const = 0;
	virtual void ensureDataSize(size_t size) = 0;
	virtual void setData(const DataAccessorFunc& fun, bool needsToRead, size_t offset = 0, size_t length = 0) = 0;
	virtual void setData(const std::span<const std::byte>& data, size_t offset) = 0;
	virtual void getData(const DataConstAccessorFunc& fun, size_t offset = 0, size_t length = 0) const = 0;
	// Indices
	virtual void bindIndices() const = 0;
	virtual void unbindIndices() const = 0;
	virtual void initializeIndices(const std::span<const uint32_t>& indices) = 0;
	virtual void getIndices(uint32_t* indices) const = 0;
	virtual size_t getIndexCount() const = 0;
	virtual void ensureIndexCount(size_t size) = 0;
	virtual void setIndices(const IndexAccessorFunc& fun, bool needsToRead, size_t offset = 0, size_t length = 0) = 0;
	virtual void setIndices(const std::span<const uint32_t>& indices, size_t offset = 0) = 0;
	virtual void getIndices(const IndexConstAccessorFunc& fun, size_t offset = 0, size_t length = 0) const = 0;
	// Typed Data Access Wrappers
	template <typename T> inline void initializeDataT(const std::span<const T>& data) {
		initializeData(std::span<const std::byte>(reinterpret_cast<const std::byte*>(data.data(),data.size_bytes())));
	}
	template <typename T> inline void setDataT(const std::span<const T>& data, size_t offset = 0) {
		setData(std::span<const std::byte>(reinterpret_cast<const std::byte*>(data.data()),data.size_bytes()),offset * sizeof(T));
	}
	template <typename T> inline void setDataT(const std::function<void( std::span<T>& data)>& fun, bool needsToRead, size_t offset = 0, size_t length = 0) {
		setData([&fun](const std::span<std::byte>& data) {
			std::span<T> dataspan(reinterpret_cast<T*>(data.data()),data.size() / sizeof(T));
			fun(dataspan);
		}, needsToRead, offset * sizeof(T), length * sizeof(T));
	}
	template <typename T> inline void getDataT(const std::function<void(const std::span<const T>& data)>& fun, size_t offset = 0, size_t length = 0) {
		getData([&fun](const std::span<const std::byte>& data) {
			const std::span<const T> dataspan(reinterpret_cast<const T*>(data.data()),data.size() / sizeof(T));
			fun(dataspan);
		}, offset, length);
	}
};

}
}

#endif // VERTEXBUFFER_H
