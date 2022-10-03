#ifndef MHSTDOUT_HPP
#define MHSTDOUT_HPP
#include <MhLib/Io/MhIoDevice.hpp>
#include <cstdio>

namespace MH33 {
namespace Io {

class MH_IO_API Stdout : public Device
{
public:
	Stdout();
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
#endif // MHSTDOUT_HPP
