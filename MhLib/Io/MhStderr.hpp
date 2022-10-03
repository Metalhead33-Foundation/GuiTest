#ifndef MHSTDERR_HPP
#define MHSTDERR_HPP
#include <MhLib/Io/MhIoDevice.hpp>
#include <cstdio>

namespace MH33 {
namespace Io {

class MH_IO_API Stderr : public Device
{
public:
	Stderr();
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
#endif // MHSTDERR_HPP
