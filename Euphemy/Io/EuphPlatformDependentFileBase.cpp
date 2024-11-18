#include "EuphPlatformDependentFileBase.hpp"
#include <system_error>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#elif defined (__unix)
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cerrno>
#else
#error "Unsupported operating system type!"
#endif
#include <cstring>
#include <Elvavena/Util/ElvException.hpp>

namespace Euph {
namespace Io {

typedef Elv::Util::Exception<std::allocator<char>> BasicException;
PlatformDependentFileHandleBase::PlatformDependentFileHandleBase()
	:
#ifdef _WIN32
	fileHandle(INVALID_HANDLE_VALUE)
#else
	fileDescriptor(-1)
#endif
{

}
#ifdef _WIN32
PlatformDependentFileBase::PlatformDependentFileBase(HANDLE fileHandle)
	: fileHandle(fileHandle)
{

}
#else
PlatformDependentFileHandleBase::PlatformDependentFileHandleBase(int fileDescriptor)
	: fileDescriptor(fileDescriptor)
{

}
#endif
PlatformDependentFileHandleBase::PlatformDependentFileHandleBase(PlatformDependentFileHandleBase&& mov)
{
#ifdef _WIN32
	fileHandle = mov.fileHandle;
	mov.fileHandle = INVALID_HANDLE_VALUE;
#else
	fileDescriptor = mov.fileDescriptor;
	mov.fileDescriptor = -1;
#endif
}

PlatformDependentFileHandleBase::~PlatformDependentFileHandleBase()
{
#ifdef _WIN32
		if(fileHandle != INVALID_HANDLE_VALUE) CloseHandle(fileHandle);
#else
		if(fileDescriptor) close(fileDescriptor);
#endif
}

PlatformDependentFileHandleBase& PlatformDependentFileHandleBase::operator=(PlatformDependentFileHandleBase&& mov)
{
	if (this != &mov) {
#ifdef _WIN32
	if(fileHandle != INVALID_HANDLE_VALUE) CloseHandle(fileHandle);
	fileHandle = mov.fileHandle;
	mov.fileHandle = INVALID_HANDLE_VALUE;
#else
	if(fileDescriptor) close(fileDescriptor);
	fileDescriptor = mov.fileDescriptor;
	mov.fileDescriptor = -1;
#endif
	}
	return *this;
}

PlatformDependentFileBase::PlatformDependentFileBase(PlatformDependentFileBase&& mov)
	: PlatformDependentFileHandleBase(std::move(mov)), path(std::move(mov.path)), deleter(std::move(mov.deleter))
{
	mov.deleter = nullptr;
}

PlatformDependentFileBase& PlatformDependentFileBase::operator=(PlatformDependentFileBase&& mov)
{
	if (this != &mov) {
		if(deleter) deleter(path.c_str());
		this->path = std::move(mov.path);
		this->deleter = std::move(mov.deleter);
		mov.deleter = nullptr;
		PlatformDependentFileHandleBase::operator=(std::move(mov));
	}
	return *this;
}

PlatformDependentFileBase::PlatformDependentFileBase()
	: PlatformDependentFileHandleBase(), deleter(nullptr)
{

}

void PlatformDependentFileBase::initializeViaRegularLoad(const char* cpath, Elv::Io::Mode mode)
{
	#ifdef _WIN32
			DWORD access = 0;
			DWORD creation = OPEN_EXISTING;
			if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::READ)) access |= GENERIC_READ;
			if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::WRITE)) {
				access |= GENERIC_WRITE;
				creation = OPEN_ALWAYS;
			}
			if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::APPEND)) creation = OPEN_ALWAYS;

			fileHandle = CreateFile(cpath, access, 0, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
			if (fileHandle == INVALID_HANDLE_VALUE) {
				throw BasicException([](BasicException::StringStream& cerr) {
					cerr << "Failed to open file. Error: " << GetLastError() << std::endl;
				} );
			}
	#else
		int flags = 0;
		if ((static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::READ)) &&
			(static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::WRITE))) {
			// Both READ and WRITE are set
			flags |= O_RDWR | O_CREAT;
		} else if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::READ)) {
			flags |= O_RDONLY;
		} else if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::WRITE)) {
			flags |= O_WRONLY | O_CREAT | O_TRUNC;
		} else if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::APPEND)) {
			flags |= O_WRONLY | O_CREAT | O_APPEND;
		}

			// Add O_SYNC only if not in read-only mode
			if (flags & (O_WRONLY | O_RDWR)) {
				flags |= O_SYNC;
			}
			fileDescriptor = open(cpath, flags, 0644);
			if (fileDescriptor == -1) {
				throw BasicException([](BasicException::StringStream& cerr) {
					cerr << "Failed to open file. Error: " << errno << std::endl;
					cerr << strerror(errno) << std::endl;
				} );
			}
#endif
}

void PlatformDependentFileBase::initializeViaMkstemp()
{
#ifdef _WIN32
	// Generate temporary filename
	char tempPath[MAX_PATH];
	if (GetTempPathA(MAX_PATH, tempPath) == 0) {
		throw std::runtime_error("Failed to get temporary path.");
	}
	char tempFileName[MAX_PATH];
	if (GetTempFileNameA(tempPath, "tmp", 0, tempFileName) == 0) {
		throw std::runtime_error("Failed to create temporary file name.");
	}
	filePath = std::string(tempFileName);

	fileHandle = CreateFile(filePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		throw std::system_error(GetLastError(), std::system_category(), "Failed to create temporary file.");
	}
#else
	// Use mkstemp to create a file with a path
	char tempFileName[] = "/tmp/tmpfile.XXXXXX";
	fileDescriptor = mkstemp(tempFileName);
	if (fileDescriptor == -1) {
		throw std::runtime_error("Failed to create temporary file.");
	}
	path = tempFileName;
	deleter = unlink;
#endif
}

void PlatformDependentFileBase::initialiteViaShmOpen(const char* cpath)
{
#ifdef _WIN32
	(void)cpath;
	initializeViaMkstemp();
#else
	fileDescriptor = shm_open(cpath, O_RDWR | O_CREAT | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
	if (fileDescriptor == -1) {
		throw std::runtime_error("Failed to create temporary file.");
	}
	deleter = shm_unlink;
#endif
}

void PlatformDependentFileBase::initializeViaMemfdCreate(const char* cpath)
{
#ifdef _WIN32
	(void)cpath;
	initializeViaMkstemp();
#else
#ifdef __linux__
	fileDescriptor = memfd_create(cpath, MFD_ALLOW_SEALING);
	if (fileDescriptor == -1) {
		throw std::runtime_error("Failed to create temporary file.");
	}
	char tmpath[256];
	sprintf(tmpath,"/proc/%jd/fd/%d",static_cast<intmax_t>(getpid()),fileDescriptor);
	path = tmpath;
#else
	initialiteViaShmOpen(cpath);
#endif
#endif
}
#ifdef _WIN32
PlatformDependentFileBase::PlatformDependentFileBase(HANDLE fileHandle, std::string&& path, Deleter&& deleter)
	: PlatformDependentFileHandleBase(fileHandle), path(std::move(path)), deleter(std::move(deleter))
{

}

PlatformDependentFileBase::PlatformDependentFileBase(HANDLE fileHandle, std::string&& path, const Deleter& deleter)
	: PlatformDependentFileHandleBase(fileHandle), path(std::move(path)), deleter(deleter)
{

}

PlatformDependentFileBase::PlatformDependentFileBase(HANDLE fileHandle, const std::string& path, Deleter&& deleter)
	: PlatformDependentFileHandleBase(fileHandle), path(path), deleter(std::move(deleter))
{

}

PlatformDependentFileBase::PlatformDependentFileBase(HANDLE fileHandle, const std::string& path, const Deleter& deleter)
	: PlatformDependentFileHandleBase(fileHandle), path(path), deleter(deleter)
{

}
#else
PlatformDependentFileBase::PlatformDependentFileBase(int fileDescriptor, std::string&& path, Deleter&& deleter)
	: PlatformDependentFileHandleBase(fileDescriptor), path(std::move(path)), deleter(std::move(deleter))
{

}

PlatformDependentFileBase::PlatformDependentFileBase(int fileDescriptor, std::string&& path, const Deleter& deleter)
	: PlatformDependentFileHandleBase(fileDescriptor), path(std::move(path)), deleter(deleter)
{

}

PlatformDependentFileBase::PlatformDependentFileBase(int fileDescriptor, const std::string& path, Deleter&& deleter)
	: PlatformDependentFileHandleBase(fileDescriptor), path(path), deleter(std::move(deleter))
{

}

PlatformDependentFileBase::PlatformDependentFileBase(int fileDescriptor, const std::string& path, const Deleter& deleter)
	: PlatformDependentFileHandleBase(fileDescriptor), path(path), deleter(deleter)
{

}
#endif

PlatformDependentFileBase::PlatformDependentFileBase(const std::string& path, Elv::Io::Mode mode)
	: PlatformDependentFileHandleBase(), path(path)
{
	initializeViaRegularLoad(this->path.c_str(), mode);
}

PlatformDependentFileBase::PlatformDependentFileBase(std::string&& path, Elv::Io::Mode mode)
	: PlatformDependentFileHandleBase(), path(std::move(path))
{
	initializeViaRegularLoad(this->path.c_str(), mode);
}

PlatformDependentFileBase::PlatformDependentFileBase(const char* cpath, TemporaryFileCreationMode mode)
	: PlatformDependentFileHandleBase()
{
	switch (mode) {
		case TemporaryFileCreationMode::MKSTEMP:
			initializeViaMkstemp();
			break;
		case TemporaryFileCreationMode::SHM_OPEN:
			initialiteViaShmOpen(cpath);
			break;
		case TemporaryFileCreationMode::MEMFD_CREATE:
			initializeViaMemfdCreate(cpath);
			break;
	}
}


PlatformDependentFileBase::~PlatformDependentFileBase()
{
	if(deleter) deleter(path.c_str());
}

}
}
