#include "EuphRandomDevice.hpp"
#include <stdexcept>
namespace Euph {
namespace Io {

RandomDevice::RandomDevice(RandomSource source) : source(source)
#ifndef _WIN32
  , randomFile_(fopen((source == RandomSource::RANDOM)? "/dev/random" : "/dev/urandom", "rb"),fclose)
#endif
{
#ifdef _WIN32
if (source == RandomSource::RANDOM) {
	// CryptAcquireContext is not strictly necessary for CryptGenRandom,
	// but it's a good practice to acquire a context for more advanced crypto operations.
	if (!CryptAcquireContext(&hCryptProv_, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
		throw std::runtime_error("Could not acquire random device!");
	}
} else { /* URANDOM (using BCryptGenRandom, available from Windows 10) */
	OSVersionInfo osvi;
	ZeroMemory(&osvi, sizeof(OSVersionInfo));
	osvi.dwOSVersionInfoSize = sizeof(OSVersionInfo);
	GetVersionEx(&osvi);
	if (osvi.dwMajorVersion >= 10) { /* Windows 10 or later */
		// BCryptGenRandom doesn't require explicit initialization
	} else {
		throw std::runtime_error("Could not acquire random device!");
	}
}
#endif
}

RandomDevice::~RandomDevice()
{
#ifdef _WIN32
	if(hCryptProv_) CryptReleaseContext(hCryptProv_,0);
#endif
}

RandomDevice::RandomDevice(RandomDevice&& mov) : source(mov.source)
  #ifndef _WIN32
	, randomFile_(std::move(mov.randomFile_))
  #endif
{
#ifdef _WIN32
	this->hCryptProv_ = mov.hCryptProv_;
	mov.hCryptProv_ = nullptr;
#endif
}

RandomDevice& RandomDevice::operator=(RandomDevice&& mov)
{
#ifdef _WIN32
	this->hCryptProv_ = mov.hCryptProv_;
	mov.hCryptProv_ = nullptr;
#else
	this->randomFile_ = std::move(mov.randomFile_);
#endif
	this->source = mov.source;
	return *this;
}

size_t RandomDevice::read(void* buffer, size_t size, size_t count)
{
#ifdef _WIN32
if (source_ == RandomSource::RANDOM) {
	if (hCryptProv_) {
		if (CryptGenRandom(hCryptProv_, size * count, static_cast<BYTE*>(buffer))) {
			return count;
		} else {
			return 0;
		}
	}
} else { /* URANDOM (BCryptGenRandom) */
	if (BCryptGenRandom(nullptr, static_cast<PUCHAR>(buffer), size * count, BCRYPT_USE_SYSTEM_PREFERRED_RNG)) {
		return count;
	} else {
		return 0;
	}
}
return count;
#else
return fread(buffer, size, count, randomFile_.get());
#endif
}

size_t RandomDevice::write(const void* buffer, size_t size, size_t count)
{
	return 0;
}

int RandomDevice::seek(long offset, Elv::Io::SeekOrigin whence)
{
	return -1;
}

long RandomDevice::tell()
{
	return 0;
}

size_t RandomDevice::size()
{
	return 0;
}

bool RandomDevice::eof()
{
	return false;
}

Elv::Io::Mode RandomDevice::getMode() const
{
	return Elv::Io::Mode::READ;
}

bool RandomDevice::flush()
{
	return true;
}

bool RandomDevice::isValid() const
{
#ifdef _WIN32
return hCryptProv_!= nullptr || (source_ == RandomSource::URANDOM && OSVersionInfo().dwMajorVersion >= 10);
#else
return randomFile_!= nullptr;
#endif
}

}
}
