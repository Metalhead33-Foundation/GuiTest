#include "EuphStdfileWrapper.hpp"
#ifdef _WIN32
#include <windows.h>
#include <io.h> // For _fileno and _fstat64
#elif defined(unix) || defined(__unix__) || defined(__unix)
#include <sys/stat.h>
#include <unistd.h>
#else
#error "Unsupported operating system type!"
#endif
namespace Euph {
namespace Io {

StdfileWrapper::StdfileWrapper(FILE* cfile, Elv::Io::Mode mode)
	: cfile(cfile), mode(mode)
{

}

StdfileWrapper::StdfileWrapper(const StdfileWrapper& cpy)
	: cfile(cpy.cfile), mode(cpy.mode)
{

}

StdfileWrapper& StdfileWrapper::operator=(const StdfileWrapper& cpy)
{
	this->cfile = cpy.cfile;
	this->mode = cpy.mode;
	return *this;
}

size_t StdfileWrapper::read(void* buffer, size_t size, size_t count)
{
	if( static_cast<int>(mode) | static_cast<int>(Elv::Io::Mode::READ) ) return fread(buffer,size,count, cfile);
	else return 0;
}

size_t StdfileWrapper::write(const void* buffer, size_t size, size_t count)
{
	if( static_cast<int>(mode) | static_cast<int>(Elv::Io::Mode::WRITE) ) return fwrite(buffer,size,count, cfile);
	else return 0;
}

int StdfileWrapper::seek(long offset, Elv::Io::SeekOrigin whence)
{
	switch (whence) {
		case Elv::Io::SeekOrigin::SET: return fseek(cfile,offset,SEEK_SET);
		case Elv::Io::SeekOrigin::CUR: return fseek(cfile,offset,SEEK_CUR);
		case Elv::Io::SeekOrigin::END: return fseek(cfile,offset,SEEK_END);
	}
}

long StdfileWrapper::tell()
{
	return ftell(cfile);
}

size_t StdfileWrapper::size()
{
#ifdef _WIN32
	int fd = _fileno(cfile);
	if (fd == -1) {
		// Handle error
		return -1;
	}

	struct __stat64 stat_buf;
	if (_fstat64(fd, &stat_buf)!= 0) {
		// Handle error
		return -1;
	}

	return static_cast<size_t>(stat_buf.st_size);
#elif defined(unix) || defined(__unix__) || defined(__unix)
	int fd = fileno(cfile);
	if (fd == -1) {
		// Handle error
		return -1;
	}

	struct stat stat_buf;
	if (fstat(fd, &stat_buf) == -1) {
		// Handle error
		return -1;
	}

	return static_cast<size_t>(stat_buf.st_size);
#else
#error "Unsupported operating system type!"
#endif
}

bool StdfileWrapper::eof()
{
	return feof(cfile);
}

Elv::Io::Mode StdfileWrapper::getMode() const
{
	return mode;
}

bool StdfileWrapper::flush()
{
	return fflush(cfile) == 0;
}

bool StdfileWrapper::isValid() const
{
	return cfile;
}
#ifndef NODECLARE_STANDARD_OUTPUTS
StdfileWrapper STDIN(stdin, Elv::Io::Mode::READ);
StdfileWrapper STDOUT(stdout, Elv::Io::Mode::WRITE);
StdfileWrapper STDERR(stderr, Elv::Io::Mode::WRITE);
#endif

}
}
