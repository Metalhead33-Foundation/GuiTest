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
	virtual void initializeData(const std::span<const std::byte>& data) = 0;
	virtual void getData(void* data) const = 0;
	virtual size_t getDataSize() const = 0;
	virtual void ensureDataSize(size_t size) = 0;
	virtual void setData(const AccessorFunc& fun, bool needsToRead) = 0;
	virtual void setData(const std::span<const std::byte>& data, size_t offset) = 0;
	virtual void getData(const ConstAccessorFunc& fun) const = 0;
};

}
}
#endif // UNIFORMBUFFER_H
