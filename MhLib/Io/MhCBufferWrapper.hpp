#ifndef MHCBUFFERWRAPPER_HPP
#define MHCBUFFERWRAPPER_HPP
#include <MhLib/Io/MhIoDevice.hpp>

namespace MH33 {
namespace Io {

DEFINE_CLASS(CBufferWrapper)
class MH_IO_API CBufferWrapper : public Device
{
private:
	const Util::CByteSpan buffer;
	intptr_t cursor;
public:
	CBufferWrapper(const Util::CByteSpan& data);
	CBufferWrapper(const void* data, size_t size);
	// Direct buffer access, in case we need it.
	const Util::CByteSpan& getBuffer() const;
	// Actual IO functions
	bool flush();
	bool seek(SeekOrigin whence, intptr_t offset);
	intptr_t tell();
	intptr_t size();
	size_t write(const void* data, size_t dataSize);
	size_t read(void* destination, size_t dataSize);
	Mode getMode() const;
	bool isValid() const;
};

}
}
#endif // MHCBUFFERWRAPPER_HPP
