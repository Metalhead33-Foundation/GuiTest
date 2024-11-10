#ifndef FILE_HPP
#define FILE_HPP
#include <Elvavena/Io/ElvIoDevice.hpp>
#ifdef _WIN32
	#include <windows.h>
#endif
namespace Euph {
namespace Io {

class File : public Elv::Io::Device
{
private:
#ifdef _WIN32
	HANDLE fileHandle;
#else
	int fileDescriptor;
#endif
	Elv::Io::Mode mode;
	File(const File& cpy) = delete;
	File& operator=(const File& cpy) = delete;
public:
	File(const char* path, Elv::Io::Mode mode);
	~File();
	File(File&& mov);
	File& operator=(File&& mov);
	size_t read(void* buffer, size_t size, size_t count) override;
	size_t write(const void* buffer, size_t size, size_t count) override;
	int seek(long offset, Elv::Io::SeekOrigin whence) override;
	long tell() override;
	size_t size() override;
	bool eof() override;
	Elv::Io::Mode getMode() const override;
	bool flush() override;
	bool isValid() const override;
};

}
}

#endif // FILE_HPP
