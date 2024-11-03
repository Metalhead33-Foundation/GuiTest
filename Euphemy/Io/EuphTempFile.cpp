#include "EuphTempFile.hpp"
#ifdef _WIN32
#include <windows.h>
#elif defined (__unix)
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#else
#error "Unsupported operating system type!"
#endif
namespace Euph {
namespace Io {

TempFile::TempFile()
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
	filePath = tempFileName;
#endif
}
TempFile::~TempFile()
{
#ifdef _WIN32
		if(fileHandle != INVALID_HANDLE_VALUE) CloseHandle(fileHandle);
#else
		if(fileDescriptor) close(fileDescriptor);
#endif
}

TempFile::TempFile(TempFile&& mov)
{
#ifdef _WIN32
	this->fileHandle = mov.fileHandle;
	mov.fileHandle = INVALID_HANDLE_VALUE;
#else
	this->fileDescriptor = mov.fileDescriptor;
	mov.fileDescriptor = 0;
#endif
}

TempFile& TempFile::operator=(TempFile&& mov)
{
#ifdef _WIN32
	this->fileHandle = mov.fileHandle;
	mov.fileHandle = INVALID_HANDLE_VALUE;
#else
	this->fileDescriptor = mov.fileDescriptor;
	mov.fileDescriptor = 0;
#endif
	return *this;
}

size_t TempFile::read(void* buffer, size_t size, size_t count)
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

size_t TempFile::write(const void* buffer, size_t size, size_t count)
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

int TempFile::seek(long offset, Elv::Io::SeekOrigin whence)
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

long TempFile::tell()
{
#ifdef _WIN32
		return SetFilePointer(fileHandle, 0, NULL, FILE_CURRENT);
#else
		return lseek(fileDescriptor, 0, SEEK_CUR);
#endif
}

size_t TempFile::size()
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

bool TempFile::eof()
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

Elv::Io::Mode TempFile::getMode() const
{
	return Elv::Io::Mode::READ_WRITE;
}

bool TempFile::flush()
{
#ifdef _WIN32
		return FlushFileBuffers(fileHandle) != 0;
#else
		return fsync(fileDescriptor) == 0;
#endif
}

bool TempFile::isValid()
{
#ifdef _WIN32
		return fileHandle != INVALID_HANDLE_VALUE;
#else
		return fileDescriptor != -1;
#endif
}

}
}

