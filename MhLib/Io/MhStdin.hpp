#ifndef MHSTDIN_HPP
#define MHSTDIN_HPP
#include <MhLib/Io/MhIoDevice.hpp>
#include <cstdio>

namespace MH33 {
namespace Io {

DEFINE_CLASS(Stdin)
class MH_IO_API Stdin : public Device
{
public:
	Stdin();
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
#endif // MHSTDIN_HPP
