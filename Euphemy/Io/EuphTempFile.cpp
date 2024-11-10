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

bool TempFile::isValid() const
{
#ifdef _WIN32
		return fileHandle != INVALID_HANDLE_VALUE;
#else
		return fileDescriptor != -1;
#endif
}

MemoryMappedTempFile::MemoryMappedTempFile(size_t fileSize) : fileSize(fileSize)
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
	// Set file size
	if (SetFileValidData(fileHandle, fileSize) == FALSE) {
		throw std::system_error(GetLastError(), std::system_category(), "Failed to set valid data length.");
	}
	if (SetEndOfFile(fileHandle) == FALSE) {
		throw std::system_error(GetLastError(), std::system_category(), "Failed to set end of file.");
	}

	// Create file mapping
	mappingHandle = CreateFileMappingA(fileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (mappingHandle == NULL) {
		throw std::system_error(GetLastError(), std::system_category(), "Failed to create file mapping.");
	}

	// Map view of file
	mappedView = MapViewOfFile(mappingHandle, FILE_MAP_WRITE, 0, 0, 0);
	if (mappedView == nullptr) {
		throw std::system_error(GetLastError(), std::system_category(), "Failed to map view of file.");
	}
#else
	// Use mkstemp to create a file with a path
	char tempFileName[] = "/tmp/tmpfile.XXXXXX";
	fileDescriptor = mkstemp(tempFileName);
	if (fileDescriptor == -1) {
		throw std::runtime_error("Failed to create temporary file.");
	}
	filePath = tempFileName;
	// Set file size
	if (ftruncate(fileDescriptor, fileSize)!= 0) {
		throw std::runtime_error("Failed to set file size.");
	}
	// Map file into memory
	mappedAddress = mmap(nullptr, fileSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, fileDescriptor, 0);
	if (mappedAddress == MAP_FAILED) {
		throw std::runtime_error("Failed to map file into memory.");
	}
#endif
}

MemoryMappedTempFile::~MemoryMappedTempFile()
{
#ifdef _WIN32
		if (mappedView) UnmapViewOfFile(mappedView);
		if (mappingHandle) CloseHandle(mappingHandle);
		if (fileHandle!= INVALID_HANDLE_VALUE) CloseHandle(fileHandle);
#else
		if (mappedAddress!= MAP_FAILED) munmap(mappedAddress, fileSize);
		if (fileDescriptor!= -1) close(fileDescriptor);
#endif
}

MemoryMappedTempFile::MemoryMappedTempFile(MemoryMappedTempFile&& mov)
{
#ifdef _WIN32
		this->mappedView = mov.mappedView;
		mov.mappedView = nullptr;
		this->mappingHandle = mov.mappingHandle;
		mov.mappingHandle = nullptr;
		this->fileHandle = mov.fileHandle;
		mov.fileHandle = INVALID_HANDLE_VALUE;
#else
		this->mappedAddress = mov.mappedAddress;
		mov.mappedAddress = MAP_FAILED;
		this->fileDescriptor = mov.fileDescriptor;
		mov.fileDescriptor = -1;
#endif
		this->fileSize = mov.fileSize;
		mov.fileSize = 0;
		this->filePath = std::move(mov.filePath);
}

MemoryMappedTempFile& MemoryMappedTempFile::operator=(MemoryMappedTempFile&& mov)
{
	// First, let's clean up
#ifdef _WIN32
		if (mappedView) UnmapViewOfFile(mappedView);
		if (mappingHandle) CloseHandle(mappingHandle);
		if (fileHandle!= INVALID_HANDLE_VALUE) CloseHandle(fileHandle);
#else
		if (mappedAddress!= MAP_FAILED) munmap(mappedAddress, fileSize);
		if (fileDescriptor!= -1) close(fileDescriptor);
#endif
	// Now let's move stuff
#ifdef _WIN32
		this->mappedView = mov.mappedView;
		mov.mappedView = nullptr;
		this->mappingHandle = mov.mappingHandle;
		mov.mappingHandle = nullptr;
		this->fileHandle = mov.fileHandle;
		mov.fileHandle = INVALID_HANDLE_VALUE;
#else
		this->mappedAddress = mov.mappedAddress;
		mov.mappedAddress = MAP_FAILED;
		this->fileDescriptor = mov.fileDescriptor;
		mov.fileDescriptor = -1;
#endif
		this->fileSize = mov.fileSize;
		mov.fileSize = 0;
		this->filePath = std::move(mov.filePath);
	return *this;
}

const std::string& MemoryMappedTempFile::getFilePath() const
{
	return filePath;
}

size_t MemoryMappedTempFile::size() const
{
	return fileSize;
}

std::span<std::byte> MemoryMappedTempFile::as_span()
{
#ifdef _WIN32
	return std::span<std::byte>(static_cast<std::byte*>(mappedView), fileSize);
#else
	return std::span<std::byte>(static_cast<std::byte*>(mappedAddress), fileSize);
#endif
}

std::span<const std::byte> MemoryMappedTempFile::as_span() const
{
#ifdef _WIN32
	return std::span<const std::byte>(static_cast<const std::byte*>(mappedView), fileSize);
#else
	return std::span<const std::byte>(static_cast<const std::byte*>(mappedAddress), fileSize);
#endif
}

MemoryMappedTempFile::operator std::span<std::byte>()
{
	return as_span();
}

MemoryMappedTempFile::operator std::span<const std::byte>() const
{
	return as_span();
}

void* MemoryMappedTempFile::data()
{
#ifdef _WIN32
	return mappedView;
#else
	return mappedAddress;
#endif
}

const void* MemoryMappedTempFile::data() const
{
#ifdef _WIN32
	return mappedView;
#else
	return mappedAddress;
#endif
}

MemoryMappedTempFile::iterator MemoryMappedTempFile::begin()
{
	return as_span().begin();
}

MemoryMappedTempFile::const_iterator MemoryMappedTempFile::begin() const
{
	return as_span().begin();
}

MemoryMappedTempFile::reverse_iterator MemoryMappedTempFile::rbegin()
{
	return as_span().rbegin();
}

MemoryMappedTempFile::reverse_const_iterator MemoryMappedTempFile::rbegin() const
{
	return as_span().rbegin();
}

MemoryMappedTempFile::iterator MemoryMappedTempFile::end()
{
	return as_span().end();
}

MemoryMappedTempFile::const_iterator MemoryMappedTempFile::end() const
{
	return as_span().end();
}

MemoryMappedTempFile::reverse_iterator MemoryMappedTempFile::rend()
{
	return as_span().rend();
}

MemoryMappedTempFile::reverse_const_iterator MemoryMappedTempFile::rend() const
{
	return as_span().rend();
}

MemoryMappedTempFile::reference MemoryMappedTempFile::operator[](size_t i)
{
	return static_cast<std::byte*>(data())[i];
}

MemoryMappedTempFile::const_reference MemoryMappedTempFile::operator[](size_t i) const
{
	return static_cast<const std::byte*>(data())[i];
}

}
}

