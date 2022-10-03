#ifndef MHFIXEDBUFFERWRAPPER_HPP
#define MHFIXEDBUFFERWRAPPER_HPP
#include <MhLib/Io/MhIoDevice.hpp>

namespace MH33 {
namespace Io {

class MH_IO_API FixedBufferWrapper : public Device
{
private:
	std::byte* ptr;
	intptr_t cursor;
	intptr_t _size;
public:
	FixedBufferWrapper(void* pointer, intptr_t maxSize);
	// Copy and move constructors and assignments - from other wrappers
	FixedBufferWrapper(const FixedBufferWrapper& cpy);
	FixedBufferWrapper(FixedBufferWrapper&& mov);
	FixedBufferWrapper& operator=(const FixedBufferWrapper& cpy);
	FixedBufferWrapper& operator=(FixedBufferWrapper&& mov);
	// Copy and move constructors - from other buffers
	FixedBufferWrapper(const Util::Buffer& cpy);
	FixedBufferWrapper(Util::Buffer&& mov);
	// Actual IO functions
	bool flush();
	bool seek(SeekOrigin whence, intptr_t offset);
	intptr_t tell();
	intptr_t size();
	size_t write(const void* data, size_t dataSize);
	size_t read(void* destination, size_t dataSize);
	Mode getMode() const;
	bool isValid() const;
	void yield(Util::Buffer& target);
};

}
}
#endif // MHFIXEDBUFFERWRAPPER_HPP
