#include <MhLib/IoSys/PhysFSIoSystem.hpp>
#include  <MhLib/Io/PhysFSIO.hpp>
#include <physfs.h>
#include <cassert>

namespace PhysFS {

IoSystem::IoSystem()
{

}

MH33::Io::Device *IoSystem::open(const char *path, MH33::Io::Mode mode)
{
	assert(PHYSFS_isInit());
	return new IO(path,mode);
}

bool IoSystem::exists(const char *path)
{
	assert(PHYSFS_isInit());
	return PHYSFS_exists(path);
}

char IoSystem::separator() const
{
	assert(PHYSFS_isInit());
	return PHYSFS_getDirSeparator()[0];
}

void IoSystem::enumerate(const char *path, bool withPath, std::vector<std::string> &output)
{
	assert(PHYSFS_isInit());
	char **rc = PHYSFS_enumerateFiles(path);
	char **i;
	if(withPath) {
		std::string str(path);
		if(!(str.back() == '\\' || str.back() == '/')) str += separator();
		for (i = rc; *i != nullptr; i++) {
			std::string tmpstr = str + *i;
			output.push_back(tmpstr);
		}
	} else {
		for (i = rc; *i != nullptr; i++) {
			output.emplace_back(*i);
		}
	}
	PHYSFS_freeList(rc);
}

void IoSystem::enumerate(const char *path, bool withPath, IoSystem::FilenameCallback functor)
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

void IoSystem::enumerate(const char *path, bool withPath, MH33::Io::System::FilenameCallbackPP functor)
{
	assert(PHYSFS_isInit());
	char **rc = PHYSFS_enumerateFiles(path);
	char **i;
	if(withPath) {
		std::string str(path);
		if(!(str.back() == '\\' || str.back() == '/')) str += separator();
		for (i = rc; *i != nullptr; i++) {
			std::string tmpstr = str + *i;
			functor(tmpstr);
		}
	} else {
		for (i = rc; *i != nullptr; i++) {
			functor(std::string(*i));
		}
	}
	PHYSFS_freeList(rc);
}

void IoSystem::enumerate(const char *path, MH33::Io::System::FilesystemCallback functor)
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

void IoSystem::enumerate(const char *path, MH33::Io::System::FilesystemCallbackPP functor)
{
	assert(PHYSFS_isInit());
	char **rc = PHYSFS_enumerateFiles(path);
	char **i;
	std::string str(path);
	if(!(str.back() == '\\' || str.back() == '/')) str += separator();
	for (i = rc; *i != nullptr; i++) {
		std::string tmpstr = str + *i;
		functor(this,tmpstr,std::string(*i));
	}
	PHYSFS_freeList(rc);
}

bool IoSystem::isDirectory(const char *path)
{
	assert(PHYSFS_isInit());
	PHYSFS_Stat stats;
	PHYSFS_stat(path,&stats);
	return stats.filetype == PHYSFS_FILETYPE_DIRECTORY;
}

bool IoSystem::isSymlink(const char *path)
{
	assert(PHYSFS_isInit());
	PHYSFS_Stat stats;
	PHYSFS_stat(path,&stats);
	return stats.filetype == PHYSFS_FILETYPE_SYMLINK;
}

bool IoSystem::isFile(const char *path)
{
	assert(PHYSFS_isInit());
	PHYSFS_Stat stats;
	PHYSFS_stat(path,&stats);
	return stats.filetype == PHYSFS_FILETYPE_REGULAR;
}

void IoSystem::mount(const char *path, const char *mountPount, bool append)
{
	assert(PHYSFS_isInit());
	PHYSFS_mount(path,mountPount,append);
}

void IoSystem::enumerateCdDrives(std::vector<std::string> &output)
{
	assert(PHYSFS_isInit());
	char **cds = PHYSFS_getCdRomDirs();
	char **i;
	for (i = cds; *i != nullptr; i++)
		output.emplace_back(*i);
	PHYSFS_freeList(cds);
}

void IoSystem::enumerateCdDrives(MH33::Io::System::FilenameCallback functor)
{
	assert(PHYSFS_isInit());
	char **cds = PHYSFS_getCdRomDirs();
	char **i;
	for (i = cds; *i != nullptr; i++)
		functor(*i);
	PHYSFS_freeList(cds);
}

std::string IoSystem::getBaseDir()
{
	assert(PHYSFS_isInit());
	return std::string(PHYSFS_getBaseDir());
}

std::string IoSystem::getPrefDir(const char *org, const char *app)
{
	assert(PHYSFS_isInit());
	return std::string(PHYSFS_getPrefDir(org,app));
}

std::string IoSystem::getWriteDir()
{
	assert(PHYSFS_isInit());
	return std::string(PHYSFS_getWriteDir());
}

void IoSystem::setWriteDir(const char *dir)
{
	assert(PHYSFS_isInit());
	PHYSFS_setWriteDir(dir);
}

bool IoSystem::mkdir(const char *dir)
{
	assert(PHYSFS_isInit());
	return PHYSFS_mkdir(dir) != 0;
}

bool IoSystem::remove(const char *path)
{
	assert(PHYSFS_isInit());
	return PHYSFS_delete(path) != 0;
}

void IoSystem::initialize(const char *argv0)
{
	PHYSFS_init(argv0);
}

void IoSystem::deinitialize()
{
	PHYSFS_deinit();
}

}
