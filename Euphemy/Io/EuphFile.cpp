#include "EuphFile.hpp"
#include <filesystem>
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

File::File(const char* path, Elv::Io::Mode mode)
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

		fileHandle = CreateFile(path, access, 0, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
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
		fileDescriptor = open(path, flags, 0644);
		if (fileDescriptor == -1) {
			throw BasicException([](BasicException::StringStream& cerr) {
				cerr << "Failed to open file. Error: " << errno << std::endl;
				cerr << strerror(errno) << std::endl;
			} );
		}
#endif
}

File::~File()
{
#ifdef _WIN32
		if(fileHandle != INVALID_HANDLE_VALUE) CloseHandle(fileHandle);
#else
		if(fileDescriptor) close(fileDescriptor);
#endif
}

File::File(File&& mov)
{
#ifdef _WIN32
	this->fileHandle = mov.fileHandle;
	mov.fileHandle = INVALID_HANDLE_VALUE;
#else
	this->fileDescriptor = mov.fileDescriptor;
	mov.fileDescriptor = 0;
#endif
	this->mode = mov.mode;
}

File& File::operator=(File&& mov)
{
#ifdef _WIN32
	this->fileHandle = mov.fileHandle;
	mov.fileHandle = INVALID_HANDLE_VALUE;
#else
	this->fileDescriptor = mov.fileDescriptor;
	mov.fileDescriptor = 0;
#endif
	this->mode = mov.mode;
	return *this;
}

size_t File::read(void* buffer, size_t size, size_t count)
{
#ifdef _WIN32
		DWORD bytesRead;
		if (!ReadFile(fileHandle, buffer, size * count, &bytesRead, NULL)) {
			return 0;
		}
		return bytesRead / size;
#else
		ssize_t bytesRead = ::read(fileDescriptor, buffer, size * count);
		return (bytesRead >= 0) ? bytesRead / size : 0;
#endif
}

size_t File::write(const void* buffer, size_t size, size_t count)
{
#ifdef _WIN32
		DWORD bytesWritten;
		if (!WriteFile(fileHandle, buffer, size * count, &bytesWritten, NULL)) {
			return 0;
		}
		return bytesWritten / size;
#else
		ssize_t bytesWritten = ::write(fileDescriptor, buffer, size * count);
		return (bytesWritten >= 0) ? bytesWritten / size : 0;
#endif
}

int File::seek(long offset, Elv::Io::SeekOrigin whence)
{
#ifdef _WIN32
		DWORD moveMethod;
		switch (whence) {
			case Elv::Io::SeekOrigin::SET: moveMethod = FILE_BEGIN; break;
			case Elv::Io::SeekOrigin::CUR: moveMethod = FILE_CURRENT; break;
			case Elv::Io::SeekOrigin::END: moveMethod = FILE_END; break;
			default: return -1;
		}
		return SetFilePointer(fileHandle, offset, NULL, moveMethod) == INVALID_SET_FILE_POINTER ? -1 : 0;
#else
		int origin;
		switch (whence) {
			case Elv::Io::SeekOrigin::SET: origin = SEEK_SET; break;
			case Elv::Io::SeekOrigin::CUR: origin = SEEK_CUR; break;
			case Elv::Io::SeekOrigin::END: origin = SEEK_END; break;
			default: return -1;
		}
		return lseek(fileDescriptor, offset, origin) == -1 ? -1 : 0;
#endif
}

long File::tell()
{
#ifdef _WIN32
		return SetFilePointer(fileHandle, 0, NULL, FILE_CURRENT);
#else
		return lseek(fileDescriptor, 0, SEEK_CUR);
#endif
}

size_t File::size()
{
#ifdef _WIN32
		LARGE_INTEGER fileSize;
		if (GetFileSizeEx(fileHandle, &fileSize)) {
			return static_cast<size_t>(fileSize.QuadPart);
		}
		return 0;
#else
		struct stat st;
		if (fstat(fileDescriptor, &st) == 0) {
			return static_cast<size_t>(st.st_size);
		}
		return 0;
#endif
}

bool File::eof()
{
#ifdef _WIN32
		return tell() >= size();
#else
		off_t currPos = lseek(fileDescriptor, 0, SEEK_CUR);
		off_t fileSize = lseek(fileDescriptor, 0, SEEK_END);
		lseek(fileDescriptor, currPos, SEEK_SET); // restore original position
		return currPos >= fileSize;
#endif
}

Elv::Io::Mode File::getMode() const
{
	return mode;
}

bool File::flush()
{
#ifdef _WIN32
		return FlushFileBuffers(fileHandle) != 0;
#else
		return fsync(fileDescriptor) == 0;
#endif
}

bool File::isValid() const
{
#ifdef _WIN32
		return fileHandle != INVALID_HANDLE_VALUE;
#else
		return fileDescriptor != -1;
#endif
}

Elv::Io::Device* Filesystem::open(const char* path, Elv::Io::Mode mode)
{
	return new File(path,mode);
}

MemoryMappedFile* Filesystem::openMemoryMapped(const char* path, Elv::Io::Mode mode)
{
	return new MemoryMappedFile(path, mode);
}

bool Filesystem::exists(const char* path)
{
	return std::filesystem::exists(path);
}

char Filesystem::separator() const
{
#if defined(_WIN32)
	return '\\';
#elif defined(__unix)
	return '/';
#else
#error "Unknown operating system, we don't know what the OS separator is!"
#endif
}

void Filesystem::enumerate(const char* path, bool withPath, FilenameCallback functor)
{
	auto tmpath = std::filesystem::directory_iterator(path);
	if(withPath) {
		for(const auto& p : tmpath ) {
			const auto tmp = p.path();
			functor(tmp.c_str());
		}
	} else {
		for(const auto& p : tmpath ) {
			const auto tmp = p.path().filename();
			functor(tmp.c_str());
		}
	}
}

void Filesystem::enumerate(const char* path, FilesystemCallback functor)
{
	auto tmpath = std::filesystem::directory_iterator(path);
	for(const auto& p : tmpath ) {
		const auto tmpA = p.path();
		const auto tmpB = tmpA.filename();
		functor(this,tmpA.c_str(),tmpB.c_str());
	}
}

bool Filesystem::isDirectory(const char* path)
{
	return std::filesystem::is_directory(path);
}

bool Filesystem::isSymlink(const char* path)
{
	return std::filesystem::is_symlink(path);
}

bool Filesystem::isFile(const char* path)
{
	return std::filesystem::is_regular_file(path);
}

bool Filesystem::mkdir(const char* dir)
{
	return std::filesystem::create_directory(dir);
}

bool Filesystem::remove(const char* path)
{
	return std::filesystem::remove_all(path);
}

MemoryMappedFile::MemoryMappedFile(const char* path, Elv::Io::Mode mode, size_t minSize) : mode(mode)
{
#ifdef _WIN32

	DWORD access = 0;
	DWORD creation = OPEN_EXISTING;
	if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::READ)) access |= GENERIC_READ;
	if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::WRITE)) {
		access |= GENERIC_WRITE;
		creation = OPEN_ALWAYS;
	}
	if (static_cast<int>(mode) & static_cast<int>(Mode::APPEND)) creation = OPEN_ALWAYS;

	fileHandle = CreateFile(path, access, 0, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		throw BasicException([](BasicException::StringStream& cerr) {
			cerr << "Failed to open file. Error: " << GetLastError() << std::endl;
		} );
	}
	// Get file size
	LARGE_INTEGER tmpFileSize;
	if (GetFileSizeEx(fileHandle, &fileSize)) {
		fileSize = tmpFileSize;
	}
	// Set file size
	if(fileSize < minSize && (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::WRITE)) ) {
		fileSize = minSize;
		if (SetFileValidData(fileHandle, minSize) == FALSE) {
			throw std::system_error(GetLastError(), std::system_category(), "Failed to set valid data length.");
		}
		if (SetEndOfFile(fileHandle) == FALSE) {
			throw std::system_error(GetLastError(), std::system_category(), "Failed to set end of file.");
		}
	}

	// Create file mapping
	DWORD pageAccess;
	if (static_cast<int>(mode) & ( static_cast<int>(Elv::Io::Mode::READ) | static_cast<int>(Elv::Io::Mode::WRITE) )) pageAccess = PAGE_EXECUTE_READWRITE;
	else if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::READ)) pageAccess = PAGE_EXECUTE_READ;
	else if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::WRITE)) pageAccess = PAGE_EXECUTE_READWRITE;
	mappingHandle = CreateFileMappingA(fileHandle, NULL, pageAccess, 0, 0, NULL);
	if (mappingHandle == NULL) {
		throw std::system_error(GetLastError(), std::system_category(), "Failed to create file mapping.");
	}
	DWORD memoryMapAccess;
	if (static_cast<int>(mode) & ( static_cast<int>(Elv::Io::Mode::READ) | static_cast<int>(Elv::Io::Mode::WRITE) )) memoryMapAccess = FILE_MAP_ALL_ACCESS;
	else if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::READ)) memoryMapAccess = FILE_MAP_READ;
	else if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::WRITE)) memoryMapAccess = FILE_MAP_WRITE;
	// Map view of file
	mappedView = MapViewOfFile(mappingHandle, memoryMapAccess, 0, 0, 0);
	if (mappedView == nullptr) {
		throw std::system_error(GetLastError(), std::system_category(), "Failed to map view of file.");
	}
#else
	int flags = 0;
	if ((static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::READ)) &&
		(static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::WRITE))) flags |= O_RDWR | O_CREAT;
	else if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::READ) ) flags |= O_RDONLY;
	else if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::WRITE) ) flags |= O_WRONLY | O_CREAT | O_TRUNC;
	else if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::APPEND) ) flags |= O_WRONLY | O_CREAT | O_APPEND;
	flags |= O_SYNC; // For synchronous I/O (optional, adjust as needed)

	fileDescriptor = open(path, flags, 0644);
	if (fileDescriptor == -1) {
		throw BasicException([](BasicException::StringStream& cerr) {
			cerr << "Failed to open file. Error: " << errno << std::endl;
		} );
	}

	// Get file size
	struct stat statBuf;
	if (fstat(fileDescriptor, &statBuf)!= 0) {
		throw std::runtime_error("Failed to get file size.");
	}
	fileSize = statBuf.st_size;
	// Set file size
	if(fileSize < minSize && (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::WRITE)) ) {
		if (ftruncate(fileDescriptor, fileSize)!= 0) {
			throw std::runtime_error("Failed to set file size.");
		}
		fileSize = minSize;
	}
	// Map file into memory
	int mmapFlags = 0;
	if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::READ) ) mmapFlags |= PROT_READ;
	if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::WRITE) ) mmapFlags |= PROT_WRITE;

	mappedAddress = mmap(nullptr, fileSize, mmapFlags, MAP_SHARED, fileDescriptor, 0);
	if (mappedAddress == MAP_FAILED || mappedAddress == nullptr) {
		throw std::runtime_error("Failed to map file into memory.");
	}
#endif
}


MemoryMappedFile::MemoryMappedFile(MemoryMappedFile&& mov)
	: MemoryMapped(std::move(mov))
{
	this->mode = mov.mode;
}

MemoryMappedFile& MemoryMappedFile::operator=(MemoryMappedFile&& mov)
{
	this->mode = mov.mode;
	MemoryMapped::operator=(std::move(mov));
	return *this;
}

Elv::Io::Mode MemoryMappedFile::getMode() const
{
	return mode;
}

}
}

