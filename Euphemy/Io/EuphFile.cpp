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
	: fileHandle(path, mode), mode(mode)
{
}

File::File(File&& mov)
	: fileHandle(std::move(mov.fileHandle)), mode(mov.mode)
{
}

File& File::operator=(File&& mov)
{
	this->fileHandle = std::move(mov.fileHandle);
	this->mode = mov.mode;
	return *this;
}

size_t File::read(void* buffer, size_t size, size_t count)
{
	return fileHandle.read(buffer,size,count);
}

size_t File::write(const void* buffer, size_t size, size_t count)
{
	return fileHandle.write(buffer,size,count);
}

int File::seek(long offset, Elv::Io::SeekOrigin whence)
{
	return fileHandle.seek(offset,whence);
}

long File::tell()
{
	return fileHandle.tell();
}

size_t File::size()
{
	return fileHandle.size();
}

bool File::eof()
{
	return fileHandle.eof();
}

Elv::Io::Mode File::getMode() const
{
	return mode;
}

bool File::flush()
{
	return fileHandle.flush();
}

bool File::isValid() const
{
#ifdef _WIN32
		return fileHandle.fileHandle != INVALID_HANDLE_VALUE;
#else
		return fileHandle.fileDescriptor != -1;
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

MemoryMappedFile::MemoryMappedFile(const char* path, Elv::Io::Mode mode, size_t minSize) : fileHandle(path,mode), mode(mode)
{
	// Set file size
	fileSize = fileHandle.size();
	if(fileSize < minSize && (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::WRITE)) ) {
		fileHandle.truncate(minSize);
		fileSize = minSize;
	}
#ifdef _WIN32
	// Create file mapping
	DWORD pageAccess;
	if (static_cast<int>(mode) & ( static_cast<int>(Elv::Io::Mode::READ) | static_cast<int>(Elv::Io::Mode::WRITE) )) pageAccess = PAGE_EXECUTE_READWRITE;
	else if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::READ)) pageAccess = PAGE_EXECUTE_READ;
	else if (static_cast<int>(mode) & static_cast<int>(Elv::Io::Mode::WRITE)) pageAccess = PAGE_EXECUTE_READWRITE;
	mappingHandle = CreateFileMappingA(fileHandle.fileHandle, NULL, pageAccess, 0, 0, NULL);
	if (mappingHandle == NULL) {
		throw std::system_error(GetLastError(), std::system_category(), "Failed to create file mapping.");
	}
#endif
	mapFile(mode == Elv::Io::Mode::READ);
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

PlatformDependentFileHandleBase& MemoryMappedFile::getFileHandle()
{
	return fileHandle;
}

const PlatformDependentFileHandleBase& MemoryMappedFile::getFileHandle() const
{
	return fileHandle;
}

Elv::Io::Mode MemoryMappedFile::getMode() const
{
	return mode;
}

bool MemoryMappedFile::readOnly() const
{
	return mode == Elv::Io::Mode::READ;
}

}
}

