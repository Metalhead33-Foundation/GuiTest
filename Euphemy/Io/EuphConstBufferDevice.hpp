#ifndef EUPHCONSTBUFFERDEVICE_H
#define EUPHCONSTBUFFERDEVICE_H
#include <span>
#include <Elvavena/Io/ElvIoDevice.hpp>

namespace Euph {
namespace Io {

class ConstBufferDevice : public Elv::Io::Device
{
private:
	std::span<const std::byte> buff;
	uintptr_t ptr;
public:
	template <typename T> static std::span<const std::byte> span_cast(const std::span<const T>& b) {
		return std::span<const std::byte>( reinterpret_cast<const std::byte*>(b.data()), b.size_bytes());
	}
	ConstBufferDevice(const std::span<const std::byte>& buff);
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
#endif // EUPHCONSTBUFFERDEVICE_H
