#ifndef UNIFORMBUFFER_H
#define UNIFORMBUFFER_H
#include <MhLib/Util/MhGlobals.hpp>
#include <functional>
#include <span>
#include "NativeHandle.hpp"
namespace MH33 {
namespace GFX {

enum StorageBufferType {
	UNIFORM_BUFFER,
	SHADER_STORAGE_BUFFER
};

DEFINE_CLASS(StorageBuffer)
class StorageBuffer : public GfxResource {
public:
	typedef std::function<void(const std::span<std::byte>& data)> AccessorFunc;
	typedef std::function<void(const std::span<const std::byte>& data)> ConstAccessorFunc;
	virtual ~StorageBuffer() = default;
	virtual StorageBufferType getType() const = 0;
	virtual void bind() const = 0;
	virtual void unbind() const = 0;
	virtual void bindBase(uint8_t bindingPoint) const = 0;
	virtual void bindRange(uint8_t bindingPoint, size_t offset, size_t length) const = 0;
	virtual void initializeData(const std::span<const std::byte>& data) = 0;
	virtual void getData(void* data, size_t offset = 0, size_t length = 0) const = 0;
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

}
}
#endif // UNIFORMBUFFER_H
