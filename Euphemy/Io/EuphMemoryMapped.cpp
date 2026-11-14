#include "EuphMemoryMapped.hpp"
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
#else
#error "Unsupported operating system type!"
#endif

namespace Euph {
namespace Io {

MemoryMapped::~MemoryMapped()
{
#ifdef _WIN32
		if (mappedView) UnmapViewOfFile(mappedView);
		if (mappingHandle) CloseHandle(mappingHandle);
#else
		if (mappedAddress!= MAP_FAILED) munmap(mappedAddress, fileSize);
#endif
}

MemoryMapped::MemoryMapped(MemoryMapped&& mov)
{
#ifdef _WIN32
		this->mappedView = mov.mappedView;
		mov.mappedView = nullptr;
		this->mappingHandle = mov.mappingHandle;
		mov.mappingHandle = nullptr;
#else
		this->mappedAddress = mov.mappedAddress;
		mov.mappedAddress = MAP_FAILED;
#endif
		this->fileSize = mov.fileSize;
		mov.fileSize = 0;
}

MemoryMapped& MemoryMapped::operator=(MemoryMapped&& mov)
{
	// First, let's clean up
#ifdef _WIN32
		if (mappedView) UnmapViewOfFile(mappedView);
		if (mappingHandle) CloseHandle(mappingHandle);
#else
		if (mappedAddress!= MAP_FAILED) munmap(mappedAddress, fileSize);
#endif
	// Now let's move stuff
#ifdef _WIN32
		this->mappedView = mov.mappedView;
		mov.mappedView = nullptr;
		this->mappingHandle = mov.mappingHandle;
		mov.mappingHandle = nullptr;
#else
		this->mappedAddress = mov.mappedAddress;
		mov.mappedAddress = MAP_FAILED;
#endif
		this->fileSize = mov.fileSize;
		mov.fileSize = 0;
		return *this;
}

/*
#ifdef _WIN32
	HANDLE fileHandle; ///< Handle for the temporary file on Windows.
	HANDLE mappingHandle; ///< Handle for the file mapping on Windows.
	void* mappedView; ///< Mapped view of the file on Windows.
#else
	int fileDescriptor; ///< File descriptor for the temporary file on Unix-like systems.
	void* mappedAddress; ///< Mapped address of the file on Unix-like systems.
#endif
	/// @}

	size_t fileSize; ///< Size of the temporary file.
*/

MemoryMapped::MemoryMapped(size_t fileSize)
	:
	  #ifdef _WIN32
		mappingHandle(nullptr), mappedAddress(MAP_FAILED),
	  #else
		mappedAddress(nullptr),
	  #endif
	  fileSize(fileSize)
{

}

void MemoryMapped::mapFile(bool readOnly = false)
{
	const PlatformDependentFileHandleBase& fileHandle = getFileHandle();
#ifdef _WIN32
	DWORD memoryMapAccess;
	if(readOnly) memoryMapAccess = FILE_MAP_READ;
	else memoryMapAccess = FILE_MAP_ALL_ACCESS;
	// Map view of file
	mappedView = MapViewOfFile(mappingHandle, memoryMapAccess, 0, 0, 0);
	if (mappedView == nullptr) {
		throw std::system_error(GetLastError(), std::system_category(), "Failed to map view of file.");
	}
#else
	// Map file into memory
	int mmapFlags = PROT_READ;
	if (!readOnly) mmapFlags |= PROT_WRITE;

	mappedAddress = mmap(nullptr, fileSize, mmapFlags, MAP_SHARED, fileHandle.fileDescriptor, 0);
	if (mappedAddress == MAP_FAILED || mappedAddress == nullptr) {
		throw std::runtime_error("Failed to map file into memory.");
	};
#endif
}

void MemoryMapped::unmapFile()
{
#ifdef _WIN32
		if (mappedView) UnmapViewOfFile(mappedView);
#else
		if (mappedAddress!= MAP_FAILED) munmap(mappedAddress, fileSize);
#endif
}

size_t MemoryMapped::size() const
{
	return fileSize;
}

void MemoryMapped::resize(size_t newSize)
{
	unmapFile();
	getFileHandle().truncate(newSize);
	mapFile(readOnly());
	fileSize = newSize;
}

std::span<std::byte> MemoryMapped::as_span()
{
#ifdef _WIN32
	return std::span<std::byte>(static_cast<std::byte*>(mappedView), fileSize);
#else
	return std::span<std::byte>(static_cast<std::byte*>(mappedAddress), fileSize);
#endif
}

std::span<const std::byte> MemoryMapped::as_span() const
{
#ifdef _WIN32
	return std::span<const std::byte>(static_cast<const std::byte*>(mappedView), fileSize);
#else
	return std::span<const std::byte>(static_cast<const std::byte*>(mappedAddress), fileSize);
#endif
}

MemoryMapped::operator std::span<std::byte>()
{
	return as_span();
}

MemoryMapped::operator std::span<const std::byte>() const
{
	return as_span();
}

void* MemoryMapped::data()
{
#ifdef _WIN32
	return mappedView;
#else
	return mappedAddress;
#endif
}

const void* MemoryMapped::data() const
{
#ifdef _WIN32
	return mappedView;
#else
	return mappedAddress;
#endif
}

void MemoryMapped::flushAsync(bool invalidate)
{
#ifdef _WIN32
	FlushViewOfFile(mappedView, fileSize);
	if(invalidate) FlushFileBuffers(fileHandle);
#else
	msync(mappedAddress, fileSize, invalidate ? (MS_ASYNC | MS_INVALIDATE) : MS_ASYNC);
#endif
}

void MemoryMapped::flushSync(bool invalidate)
{
#ifdef _WIN32
	FlushViewOfFile(mappedView, fileSize);
	if(invalidate) FlushFileBuffers(fileHandle);
#else
	msync(mappedAddress, fileSize, invalidate ? (MS_SYNC | MS_INVALIDATE) : MS_SYNC);
#endif
}

MemoryMapped::iterator MemoryMapped::begin()
{
	return as_span().begin();
}

MemoryMapped::const_iterator MemoryMapped::begin() const
{
	return as_span().begin();
}

MemoryMapped::reverse_iterator MemoryMapped::rbegin()
{
	return as_span().rbegin();
}

MemoryMapped::reverse_const_iterator MemoryMapped::rbegin() const
{
	return as_span().rbegin();
}

MemoryMapped::iterator MemoryMapped::end()
{
	return as_span().end();
}

MemoryMapped::const_iterator MemoryMapped::end() const
{
	return as_span().end();
}

MemoryMapped::reverse_iterator MemoryMapped::rend()
{
	return as_span().rend();
}

MemoryMapped::reverse_const_iterator MemoryMapped::rend() const
{
	return as_span().rend();
}

MemoryMapped::reference MemoryMapped::operator[](size_t i)
{
	return static_cast<std::byte*>(data())[i];
}

MemoryMapped::const_reference MemoryMapped::operator[](size_t i) const
{
	return static_cast<const std::byte*>(data())[i];
}

}
}
