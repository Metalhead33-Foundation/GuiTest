#ifndef MHUNIFORMBUFFER_HPP
#define MHUNIFORMBUFFER_HPP
#include <MhLib/Media/GFX/MhGFXHandle.hpp>
#include <span>
#include <functional>
namespace MH33 {
namespace GFX {

DEFINE_CLASS(UniformBuffer)
class UniformBuffer {
public:
	virtual ~UniformBuffer() = default;
	virtual size_t size() const = 0;
	virtual Handle getNativeHandle() = 0;
	virtual ConstHandle getNativeHandle() const = 0;
	virtual void access(const std::function<void(void*,size_t)>& accessor) = 0;
	virtual void access(const std::function<void(const void*,size_t)>& accessor) const = 0;
};

}
}
#endif // MHUNIFORMBUFFER_HPP
