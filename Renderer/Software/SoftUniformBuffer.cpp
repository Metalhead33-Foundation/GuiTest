#include "SoftUniformBuffer.hpp"

namespace Renderer {
namespace Software {

UniformBuffer::UniformBuffer(size_t size)
	: buffer(size)
{

}

size_t UniformBuffer::size() const
{
	return buffer.size();
}

MH33::GFX::Handle UniformBuffer::getNativeHandle()
{
	return { .ptr = buffer.data() };
}

MH33::GFX::ConstHandle UniformBuffer::getNativeHandle() const
{
	return { .ptr = buffer.data() };
}

void UniformBuffer::access(const std::function<void (void*, size_t)>& accessor)
{
	std::lock_guard<std::mutex> lock(mutex);
	accessor(buffer.data(),buffer.size());
}

void UniformBuffer::access(const std::function<void (const void*, size_t)>& accessor) const
{
	std::lock_guard<std::mutex> lock(mutex);
	accessor(buffer.data(),buffer.size());
}

}
}
