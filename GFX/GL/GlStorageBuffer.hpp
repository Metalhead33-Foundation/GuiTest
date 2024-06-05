#ifndef STORAGEBUFFER_HPP
#define STORAGEBUFFER_HPP
#include <GFX/Abstract/UniformBuffer.hpp>
#include <NativeGfxApi/glad/glwrap.h>

namespace GL {
class UniformBuffer : public MH33::GFX::StorageBuffer
{
private:
	GLuint handle;
	size_t dataSize;
	UniformBuffer(const UniformBuffer& cpy) = delete;
	UniformBuffer& operator=(const UniformBuffer& cpy) = delete;
public:
	UniformBuffer(size_t size = 0);
	~UniformBuffer();
	UniformBuffer(UniformBuffer&& mov);
	UniformBuffer& operator=(UniformBuffer&& mov);
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	MH33::GFX::StorageBufferType getType() const override;
	void bind() const override;
	void unbind() const override;
	void bindBase(uint8_t bindingPoint) const override;
	void bindRange(uint8_t bindingPoint, size_t offset, size_t length) const override;
	void initializeData(const std::span<const std::byte>& data) override;
	void getData(void* data, size_t offset = 0, size_t length = 0) const override;
	size_t getDataSize() const override;
	void ensureDataSize(size_t size) override;
	void setData(const AccessorFunc& fun, bool needsToRead, size_t offset = 0, size_t length = 0) override;
	void setData(const std::span<const std::byte>& data, size_t offset) override;
	void getData(const ConstAccessorFunc& fun, size_t offset = 0, size_t length = 0) const override;
};
class ShaderStorageBuffer : public MH33::GFX::StorageBuffer
{
private:
	GLuint handle;
	size_t dataSize;
	ShaderStorageBuffer(const ShaderStorageBuffer& cpy) = delete;
	ShaderStorageBuffer& operator=(const ShaderStorageBuffer& cpy) = delete;
public:
	ShaderStorageBuffer(size_t size = 0);
	~ShaderStorageBuffer();
	ShaderStorageBuffer(ShaderStorageBuffer&& mov);
	ShaderStorageBuffer& operator=(ShaderStorageBuffer&& mov);
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	MH33::GFX::StorageBufferType getType() const override;
	void bind() const override;
	void unbind() const override;
	void bindBase(uint8_t bindingPoint) const override;
	void bindRange(uint8_t bindingPoint, size_t offset, size_t length) const override;
	void initializeData(const std::span<const std::byte>& data) override;
	void getData(void* data, size_t offset = 0, size_t length = 0) const override;
	size_t getDataSize() const override;
	void ensureDataSize(size_t size) override;
	void setData(const AccessorFunc& fun, bool needsToRead, size_t offset = 0, size_t length = 0) override;
	void setData(const std::span<const std::byte>& data, size_t offset) override;
	void getData(const ConstAccessorFunc& fun, size_t offset = 0, size_t length = 0) const override;
};
}
#endif // STORAGEBUFFER_HPP
