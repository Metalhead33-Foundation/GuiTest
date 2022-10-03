#include <MhLib/Io/PhysFSIO.hpp>
#include <physfs.h>
#include <cassert>

namespace PhysFS {

IO::~IO()
{
	if(handle) {
		PHYSFS_close(static_cast<PHYSFS_File*>(handle));
	}
}

IO::IO(IO &&mov) : handle(mov.handle), mode(mov.mode)
{
	mov.handle = nullptr;
}

IO &IO::operator=(IO &&mov)
{
	if(handle) {
		PHYSFS_close(static_cast<PHYSFS_File*>(handle));
	}
	this->handle = mov.handle;
	mov.handle = nullptr;
	this->mode = mov.mode;
	return *this;
}

IO::IO() : handle(nullptr)
{

}

IO::IO(const char *path, MH33::Io::Mode mode) : handle(nullptr), mode(mode)
{
	assert(PHYSFS_isInit());
	switch (mode) {
	case MH33::Io::Mode::READ:
		handle = PHYSFS_openRead(path);
		break;
	case MH33::Io::Mode::WRITE:
		handle = PHYSFS_openWrite(path);
		break;
	case MH33::Io::Mode::APPEND:
		handle = PHYSFS_openAppend(path);
		break;
	default:
		break; // Unsupported mode.
	}
}

IO::IO(const std::string &path, MH33::Io::Mode mode) : handle(nullptr), mode(mode)
{
	assert(PHYSFS_isInit());
	switch (mode) {
	case MH33::Io::Mode::READ:
		handle = PHYSFS_openRead(path.c_str());
		break;
	case MH33::Io::Mode::WRITE:
		handle = PHYSFS_openWrite(path.c_str());
		break;
	case MH33::Io::Mode::APPEND:
		handle = PHYSFS_openAppend(path.c_str());
		break;
	default:
		break; // Unsupported mode.
	}
}

bool IO::flush()
{
	assert(handle);
	return PHYSFS_flush(static_cast<PHYSFS_File*>(handle));
}

bool IO::seek(MH33::Io::SeekOrigin whence, intptr_t offset)
{
	assert(handle);
	switch (whence) {
	case MH33::Io::SeekOrigin::CUR:
		return PHYSFS_seek(static_cast<PHYSFS_File*>(handle),PHYSFS_tell(static_cast<PHYSFS_File*>(handle)-offset));
	case MH33::Io::SeekOrigin::END:
		return PHYSFS_seek(static_cast<PHYSFS_File*>(handle),PHYSFS_fileLength(static_cast<PHYSFS_File*>(handle)-offset));
	case MH33::Io::SeekOrigin::SET:
		return PHYSFS_seek(static_cast<PHYSFS_File*>(handle),offset);
	}
}

intptr_t IO::tell()
{
	assert(handle);
	return PHYSFS_tell(static_cast<PHYSFS_File*>(handle));
}

intptr_t IO::size()
{
	assert(handle);
	return PHYSFS_fileLength(static_cast<PHYSFS_File*>(handle));
}

size_t IO::write(const void *data, size_t dataSize)
{
	assert(handle);
	return PHYSFS_writeBytes(static_cast<PHYSFS_File*>(handle),data,dataSize);
}

size_t IO::read(void *destination, size_t dataSize)
{
	assert(handle);
	return PHYSFS_readBytes(static_cast<PHYSFS_File*>(handle),destination,dataSize);
}

MH33::Io::Mode IO::getMode() const
{
	return mode;
}

bool IO::isValid() const
{
	return handle != nullptr;
}

void IO::setBuffer(size_t newBuffsize)
{
	assert(handle);
	PHYSFS_setBuffer(static_cast<PHYSFS_File*>(handle),newBuffsize);
}

}
