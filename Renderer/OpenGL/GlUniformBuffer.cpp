#include "GlUniformBuffer.hpp"
namespace GL {

const Buffer& UniformBuffer::getBuffer() const
{
	return buffer;
}

Buffer& UniformBuffer::getBuffer()
{
	return buffer;
}

UniformBuffer::UniformBuffer()
	: buffer(GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW)
{

}

size_t UniformBuffer::size() const
{
	return buffer.getSize();
}

MH33::GFX::Handle UniformBuffer::getNativeHandle()
{
	return { .oglUint = buffer.getBuffId() };
}

MH33::GFX::ConstHandle UniformBuffer::getNativeHandle() const
{
	return { .oglUint = buffer.getBuffId() };
}

void UniformBuffer::access(const std::function<void (void*, size_t)>& accessor)
{
	buffer.bind();
	auto range = buffer.map(GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT,0,buffer.getSize());
	accessor(range.data(),range.size());
	range.flush();
}

void UniformBuffer::access(const std::function<void (const void*, size_t)>& accessor) const
{
	buffer.bind();
	auto range = buffer.map(GL_MAP_READ_BIT | GL_MAP_UNSYNCHRONIZED_BIT,0,buffer.getSize());
	accessor(range.data(),range.size());
}

}
