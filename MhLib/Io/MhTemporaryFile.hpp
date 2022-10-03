#ifndef MHTEMPFILE_HPP
#define MHTEMPFILE_HPP
#include <MhLib/Io/MhIoDevice.hpp>
#include <cstdio>
namespace MH33 {
namespace Io {
class MH_IO_API TemporaryFile : public Device
{
private:
	FILE* fdev;
	TemporaryFile(const TemporaryFile& cpy) = delete; // No copy constructor
	TemporaryFile& operator=(const TemporaryFile& cpy) = delete; // No copy assignment operator
public:
	TemporaryFile();
	~TemporaryFile();
	TemporaryFile(TemporaryFile&& mov);
	TemporaryFile& operator=(TemporaryFile&& mov);
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
#endif // MHTEMPFILE_HPP
