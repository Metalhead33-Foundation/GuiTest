#include "EuphFile.hpp"
#ifdef _WIN32
	#include <windows.h>
#elif defined (__unix)
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/stat.h>
#else
#error "Unsupported operating system type!"
#endif
#include <Elvavena/Util/ElvException.hpp>
namespace Euph {
namespace Io {
typedef Elv::Util::Exception<std::allocator<char>> BasicException;

File::File(const char* path, Elv::Io::Mode mode)
{
#ifdef _WIN32
		DWORD access = 0;
		DWORD creation = OPEN_EXISTING;
		if (static_cast<int>(mode) & static_cast<int>(Mode::READ)) access |= GENERIC_READ;
		if (static_cast<int>(mode) & static_cast<int>(Mode::WRITE)) {
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
#else
		int flags = 0;
		if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::READ) ) flags |= O_RDONLY;
		if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::WRITE) ) flags |= O_WRONLY | O_CREAT | O_TRUNC;
		if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::APPEND) ) flags |= O_WRONLY | O_CREAT | O_APPEND;

		fileDescriptor = open(path, flags, 0644);
		if (fileDescriptor == -1) {
			throw BasicException([](BasicException::StringStream& cerr) {
				cerr << "Failed to open file. Error: " << errno << std::endl;
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

}
}
