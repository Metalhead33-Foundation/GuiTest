#ifndef EUPHNULLDEVICE_HPP
#define EUPHNULLDEVICE_HPP
#include <Elvavena/Io/ElvIoDevice.hpp>
namespace Euph {
namespace Io {

class NullDevice : public Elv::Io::Device
{
public:
	NullDevice();

	// Device interface
public:
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
#endif // EUPHNULLDEVICE_HPP
