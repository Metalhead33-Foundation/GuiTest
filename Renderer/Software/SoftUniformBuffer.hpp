#ifndef SOFTUNIFORMBUFFER_HPP
#define SOFTUNIFORMBUFFER_HPP
#include <MhLib/Media/GFX/MhUniformBuffer.hpp>
#include <vector>
#include <mutex>

namespace Renderer {
namespace Software {
DEFINE_CLASS(UniformBuffer)
class UniformBuffer : public MH33::GFX::UniformBuffer
{
private:
	std::vector<std::byte> buffer;
	mutable std::mutex mutex;
public:
	UniformBuffer(size_t size);
	size_t size() const override;
	MH33::GFX::Handle getNativeHandle() override;
	MH33::GFX::ConstHandle getNativeHandle() const override;
	void access(const std::function<void (void*, size_t)>& accessor) override;
	void access(const std::function<void (const void*, size_t)>& accessor) const override;
};
}
}
#endif // SOFTUNIFORMBUFFER_HPP
