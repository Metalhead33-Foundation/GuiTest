#include "EuphTempFile.hpp"
#include <system_error>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
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

TempFile::TempFile(TemporaryFileCreationMode creationMode, const char* npath)
	: fileHandle(npath,creationMode)
{

}

TempFile::TempFile(TempFile&& mov)
	: fileHandle(std::move(mov.fileHandle))
{

}

TempFile& TempFile::operator=(TempFile&& mov)
{
	this->fileHandle = std::move(mov.fileHandle);
	return *this;
}

size_t TempFile::read(void* buffer, size_t size, size_t count)
{
	return fileHandle.read(buffer,size,count);
}

size_t TempFile::write(const void* buffer, size_t size, size_t count)
{
	return fileHandle.write(buffer,size,count);
}

int TempFile::seek(long offset, Elv::Io::SeekOrigin whence)
{
	return fileHandle.seek(offset,whence);
}

long TempFile::tell()
{
	return fileHandle.tell();
}

size_t TempFile::size()
{
	return fileHandle.size();
}

bool TempFile::eof()
{
	return fileHandle.eof();
}

Elv::Io::Mode TempFile::getMode() const
{
	return Elv::Io::Mode::READ_WRITE;
}

bool TempFile::flush()
{
	return fileHandle.flush();
}

bool TempFile::isValid() const
{
#ifdef _WIN32
		return fileHandle.fileHandle != INVALID_HANDLE_VALUE;
#else
		return fileHandle.fileDescriptor != -1;
#endif
}

MemoryMappedTempFile::MemoryMappedTempFile(size_t fileSize, TemporaryFileCreationMode creationMode, const char* npath) : MemoryMapped(fileSize), fileHandle(npath,creationMode)
{
	fileHandle.truncate(fileSize);
#ifdef _WIN32
	// Create file mapping
	mappingHandle = CreateFileMappingA(fileHandlefileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (mappingHandle == NULL) {
		throw std::system_error(GetLastError(), std::system_category(), "Failed to create file mapping.");
	}
#endif
	mapFile(false);
}

MemoryMappedTempFile::MemoryMappedTempFile(MemoryMappedTempFile&& mov)
	: MemoryMapped(std::move(mov)), fileHandle(std::move(mov.fileHandle))
{

}

MemoryMappedTempFile& MemoryMappedTempFile::operator=(MemoryMappedTempFile&& mov)
{
	this->fileHandle = std::move(mov.fileHandle);
	MemoryMapped::operator=(std::move(mov));
	return *this;
}

PlatformDependentFileHandleBase& MemoryMappedTempFile::getFileHandle()
{
	return fileHandle;
}

const PlatformDependentFileHandleBase& MemoryMappedTempFile::getFileHandle() const
{
	return fileHandle;
}

const std::string& MemoryMappedTempFile::getFilePath() const
{
	return fileHandle.path;
}

bool MemoryMappedTempFile::readOnly() const
{
	return false;
}

}
}

