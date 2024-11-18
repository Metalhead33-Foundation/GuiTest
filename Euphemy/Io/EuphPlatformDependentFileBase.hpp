#ifndef EUPHPLATFORMDEPENDENTFILEBASE_HPP
#define EUPHPLATFORMDEPENDENTFILEBASE_HPP
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <cstddef>
#include <span>
#ifdef _WIN32
	#include <windows.h>
#endif
#include <string>
#include <functional>
namespace Euph {
namespace Io {

struct PlatformDependentFileHandleBase {
public:
#ifdef _WIN32
	HANDLE fileHandle; ///< Handle for the temporary file on Windows.
#else
	int fileDescriptor; ///< File descriptor for the temporary file on Unix-like systems.
#endif
private:
	PlatformDependentFileHandleBase(const PlatformDependentFileHandleBase& cpy) = delete;
	PlatformDependentFileHandleBase& operator=(const PlatformDependentFileHandleBase& cpy) = delete;
public:
	PlatformDependentFileHandleBase();
#ifdef _WIN32
	PlatformDependentFileBase(HANDLE fileHandle);
#else
	PlatformDependentFileHandleBase(int fileDescriptor);
#endif
	~PlatformDependentFileHandleBase();
	PlatformDependentFileHandleBase(PlatformDependentFileHandleBase&& mov);
	PlatformDependentFileHandleBase& operator=(PlatformDependentFileHandleBase&& mov);
};

// Only relevant for Unix-like systems: ignored for Windows.
enum class TemporaryFileCreationMode : uint8_t {
	MKSTEMP, // Regular POSIX mkstemp()
	SHM_OPEN, // POSIX smh_open()
	MEMFD_CREATE // Linux-specific memfd_create()
};

struct PlatformDependentFileBase : public PlatformDependentFileHandleBase {
public:
	typedef std::function<void(const char*)> Deleter;
	std::string path;
	Deleter deleter;
private:
	PlatformDependentFileBase(const PlatformDependentFileHandleBase& cpy) = delete;
	PlatformDependentFileBase& operator=(const PlatformDependentFileHandleBase& cpy) = delete;
	void initializeViaRegularLoad(const char* cpath, Elv::Io::Mode mode);
	void initializeViaMkstemp();
	void initialiteViaShmOpen(const char* cpath);
	void initializeViaMemfdCreate(const char* cpath);
public:
	~PlatformDependentFileBase();
	PlatformDependentFileBase(PlatformDependentFileBase&& mov);
	PlatformDependentFileBase& operator=(PlatformDependentFileBase&& mov);
	PlatformDependentFileBase();
#ifdef _WIN32
	PlatformDependentFileBase(HANDLE fileHandle, std::string&& path, Deleter&& deleter);
	PlatformDependentFileBase(HANDLE fileHandle, std::string&& path, const Deleter& deleter);
	PlatformDependentFileBase(HANDLE fileHandle, const std::string& path, Deleter&& deleter);
	PlatformDependentFileBase(HANDLE fileHandle, const std::string& path, const Deleter& deleter);
#else
	PlatformDependentFileBase(int fileDescriptor, std::string&& path, Deleter&& deleter);
	PlatformDependentFileBase(int fileDescriptor, std::string&& path, const Deleter& deleter);
	PlatformDependentFileBase(int fileDescriptor, const std::string& path, Deleter&& deleter);
	PlatformDependentFileBase(int fileDescriptor, const std::string& path, const Deleter& deleter);
#endif
	PlatformDependentFileBase(const std::string& path, Elv::Io::Mode mode);
	PlatformDependentFileBase(std::string&& path, Elv::Io::Mode mode);
	PlatformDependentFileBase(const char* cpath, TemporaryFileCreationMode mode);
};

}
}

#endif // EUPHPLATFORMDEPENDENTFILEBASE_HPP
