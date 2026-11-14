#include "PhysFS.hpp"
#include <physfs.h>
#include <cassert>

namespace PhysFS {

Device::Device(Device&& mov)
	: handle(mov.handle), mode(mov.mode)
{
	mov.handle = nullptr;
}

Device& Device::operator=(Device&& mov)
{
	if(handle) {
		PHYSFS_close(static_cast<PHYSFS_File*>(handle));
	}
	this->handle = mov.handle;
	this->mode = mov.mode;
	mov.handle = nullptr;
	return *this;
}

Device::Device(const char* path, Elv::Io::Mode mode, int bufferSize)
{
	assert(PHYSFS_isInit());
	switch (mode) {
	case Elv::Io::Mode::READ:
		handle = PHYSFS_openRead(path);
		break;
	case Elv::Io::Mode::WRITE:
		handle = PHYSFS_openWrite(path);
		break;
	case Elv::Io::Mode::APPEND:
		handle = PHYSFS_openAppend(path);
		break;
	default:
		break; // Unsupported mode.
	}
	if(bufferSize) PHYSFS_setBuffer(static_cast<PHYSFS_File*>(handle),bufferSize);
}

Device::~Device()
{
	if(handle) {
		PHYSFS_close(static_cast<PHYSFS_File*>(handle));
	}
}

size_t Device::read(void* buffer, size_t size, size_t count)
{
	assert(handle);
	return PHYSFS_readBytes(static_cast<PHYSFS_File*>(handle),buffer,size * count) / count;
}

size_t Device::write(const void* buffer, size_t size, size_t count)
{
	assert(handle);
	return PHYSFS_writeBytes(static_cast<PHYSFS_File*>(handle),buffer,size * count) / count;
}

int Device::seek(long offset, Elv::Io::SeekOrigin whence)
{
	assert(handle);
	switch (whence) {
	case Elv::Io::SeekOrigin::CUR:
		return !PHYSFS_seek(static_cast<PHYSFS_File*>(handle),PHYSFS_tell(static_cast<PHYSFS_File*>(handle))+offset);
	case Elv::Io::SeekOrigin::END:
		return !PHYSFS_seek(static_cast<PHYSFS_File*>(handle),PHYSFS_fileLength(static_cast<PHYSFS_File*>(handle))+offset);
	case Elv::Io::SeekOrigin::SET:
		return !PHYSFS_seek(static_cast<PHYSFS_File*>(handle),offset);
	}
	return 0;
}

long Device::tell()
{
	assert(handle);
	return PHYSFS_tell(static_cast<PHYSFS_File*>(handle));
}

size_t Device::size()
{
	assert(handle);
	return PHYSFS_fileLength(static_cast<PHYSFS_File*>(handle));
}

bool Device::eof()
{
	assert(handle);
	return PHYSFS_eof(static_cast<PHYSFS_File*>(handle));
}

Elv::Io::Mode Device::getMode() const
{
	return mode;
}

bool Device::flush()
{
	assert(handle);
	return PHYSFS_flush(static_cast<PHYSFS_File*>(handle)) != 0;
}

bool Device::isValid() const
{
	return handle != nullptr;
}

void System::init(const char* argv0)
{
	PHYSFS_init(argv0);
}

void System::deinit()
{
	PHYSFS_deinit();
}

void System::mount(const char* path, const char* mountPount, bool append)
{
	assert(PHYSFS_isInit());
	PHYSFS_mount(path,mountPount,append);
}

void System::unmount(const char* path)
{
	assert(PHYSFS_isInit());
	PHYSFS_unmount(path);
}

void System::enumerateCdDrives(FilenameCallback functor)
{
	assert(PHYSFS_isInit());
	char **cds = PHYSFS_getCdRomDirs();
	char **i;
	for (i = cds; *i != nullptr; i++)
		functor(*i);
	PHYSFS_freeList(cds);
}

const char* System::getBaseDir()
{
	assert(PHYSFS_isInit());
	return PHYSFS_getBaseDir();
}

const char* System::getPrefDir(const char* org, const char* app)
{
	assert(PHYSFS_isInit());
	return PHYSFS_getPrefDir(org,app);
}

const char* System::getWriteDir()
{
	assert(PHYSFS_isInit());
	return PHYSFS_getWriteDir();
}

void System::setWriteDir(const char* dir)
{
	assert(PHYSFS_isInit());
	PHYSFS_setWriteDir(dir);
}

Elv::Io::uDevice System::openUnique(const char* path, Elv::Io::Mode mode, std::pmr::memory_resource* memRes)
{
	assert(PHYSFS_isInit());
	return Elv::Util::pmr_make_unique<Device>(memRes, path, mode);
}

Elv::Io::sDevice System::openShared(const char* path, Elv::Io::Mode mode, std::pmr::memory_resource* memRes)
{
	assert(PHYSFS_isInit());
	return Elv::Util::pmr_make_shared<Device>(memRes, path, mode);
}

Elv::Io::Device* System::open(const char* path, Elv::Io::Mode mode)
{
	assert(PHYSFS_isInit());
	return new Device(path, mode);
}

bool System::exists(const char* path)
{
	assert(PHYSFS_isInit());
	return PHYSFS_exists(path);
}

char System::separator() const
{
	assert(PHYSFS_isInit());
	return PHYSFS_getDirSeparator()[0];
}

void System::enumerate(const char* path, bool withPath, FilenameCallback functor)
{
	assert(PHYSFS_isInit());
	char **rc = PHYSFS_enumerateFiles(path);
	char **i;
	if(withPath) {
		std::string str(path);
		if(!(str.back() == '\\' || str.back() == '/')) str += separator();
		for (i = rc; *i != nullptr; i++) {
			std::string tmpstr = str + *i;
			functor(tmpstr.c_str());
		}
	} else {
		for (i = rc; *i != nullptr; i++) {
			functor(*i);
		}
	}
	PHYSFS_freeList(rc);
}

void System::enumerate(const char* path, FilesystemCallback functor)
{
	assert(PHYSFS_isInit());
	char **rc = PHYSFS_enumerateFiles(path);
	char **i;
	std::string str(path);
	if(!(str.back() == '\\' || str.back() == '/')) str += separator();
	for (i = rc; *i != nullptr; i++) {
		std::string tmpstr = str + *i;
		functor(this,tmpstr.c_str(),*i);
	}
	PHYSFS_freeList(rc);
}

bool System::isDirectory(const char* path)
{
	assert(PHYSFS_isInit());
	PHYSFS_Stat stats;
	PHYSFS_stat(path,&stats);
	return stats.filetype == PHYSFS_FILETYPE_DIRECTORY;
}

bool System::isSymlink(const char* path)
{
	assert(PHYSFS_isInit());
	PHYSFS_Stat stats;
	PHYSFS_stat(path,&stats);
	return stats.filetype == PHYSFS_FILETYPE_SYMLINK;
}

bool System::isFile(const char* path)
{
	assert(PHYSFS_isInit());
	PHYSFS_Stat stats;
	PHYSFS_stat(path,&stats);
	return stats.filetype == PHYSFS_FILETYPE_REGULAR;
}

bool System::mkdir(const char* dir)
{
	assert(PHYSFS_isInit());
	return PHYSFS_mkdir(dir) != 0;
}

bool System::remove(const char* path)
{
	assert(PHYSFS_isInit());
	return PHYSFS_delete(path) != 0;
}

}
