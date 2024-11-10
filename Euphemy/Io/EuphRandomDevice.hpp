#ifndef RANDOMDEVICE_HPP
#define RANDOMDEVICE_HPP
#include <Elvavena/Io/ElvIoDevice.hpp>
#ifdef _WIN32
#include <Wincrypt.h>
#else
#include <cstdio>
#include <memory>
#endif
namespace Euph {
namespace Io {

enum class RandomSource : uint8_t {
	RANDOM,  // /dev/random or CryptGenRandom (blocking, more secure)
	URANDOM  // /dev/urandom or BCryptGenRandom (non-blocking, suitable for most use cases)
};

class RandomDevice : public Elv::Io::Device
{
private:
	RandomSource source;
#ifdef _WIN32
	HCRYPTPROV hCryptProv_ = nullptr; // For CryptGenRandom
#else
	std::unique_ptr<FILE,decltype(&fclose)> randomFile_;
#endif
	RandomDevice(const RandomDevice& cpy) = delete;
	RandomDevice& operator=(const RandomDevice& cpy) = delete;
public:
	RandomDevice(RandomSource source);
	~RandomDevice();
	RandomDevice(RandomDevice&& mov);
	RandomDevice& operator=(RandomDevice&& mov);
	size_t read(void* buffer, size_t size, size_t count) override;
	size_t write(const void* buffer, size_t size, size_t count) override; // Not applicable for random devices, will fail
	int seek(long offset, Elv::Io::SeekOrigin whence) override; // Not applicable for random devices, will fail
	long tell() override; // Not applicable for random devices, returns 0
	size_t size() override; // Not applicable for random devices, returns 0
	bool eof() override; // Always returns false for random devices
	Elv::Io::Mode getMode() const override; // Returns READ mode
	bool flush() override; // No-op for random devices, returns true
	bool isValid() const override;
};

}
}
#endif // RANDOMDEVICE_HPP
