#ifndef EUPHTEMPFILE_HPP
#define EUPHTEMPFILE_HPP
#include <Elvavena/Io/ElvIoDevice.hpp>
namespace Euph {
namespace Io {

class TempFile : public Elv::Io::Device
{
private:
#ifdef _WIN32
	HANDLE fileHandle = INVALID_HANDLE_VALUE;
#else
	int fileDescriptor = -1;
#endif
	std::string filePath;
	TempFile(const TempFile& cpy) = delete;
	TempFile& operator=(const TempFile& cpy) = delete;
	std::string path;
public:
	TempFile();
	~TempFile();
	TempFile(TempFile&& mov);
	TempFile& operator=(TempFile&& mov);
	size_t read(void* buffer, size_t size, size_t count) override;
	size_t write(const void* buffer, size_t size, size_t count) override;
	int seek(long offset, Elv::Io::SeekOrigin whence) override;
	long tell() override;
	size_t size() override;
	bool eof() override;
	Elv::Io::Mode getMode() const override;
	bool flush() override;
	bool isValid() override;
};

}
}

#endif // EUPHTEMPFILE_HPP
