#ifndef UNIFORMBUFFER_HPP
#define UNIFORMBUFFER_HPP
#include <MhLib/Media/GFX/MhUniformBuffer.hpp>
#include <GL/GlBuffer.hpp>
namespace GL {
class UniformBuffer : public MH33::GFX::UniformBuffer
{
	mutable Buffer buffer;
public:
	UniformBuffer();
	size_t size() const override;
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	void access(const std::function<void (void*, size_t)>& accessor) override;
	void access(const std::function<void (const void*, size_t)>& accessor) const override;
	const Buffer& getBuffer() const;
	Buffer& getBuffer();
};
}
#endif // UNIFORMBUFFER_HPP
