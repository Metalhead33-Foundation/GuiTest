#include "ElvDynamicLib.hpp"
namespace Elv {
namespace Util {

DynamicLibrary::DynamicLibrary(const char* path)
#ifdef _WIN32
	: module(LoadLibraryA(path), freeLibWin)
#elif defined (__unix)
	: module(dlopen(path, RTLD_LAZY | RTLD_GLOBAL), dlclose)
#endif
{
}

DynamicLibrary::DynamicLibrary(const std::string &path)
#ifdef _WIN32
	: module(LoadLibraryA(path.c_str()), freeLibWin)
#elif defined (__unix)
	: module(dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL), dlclose)
#endif
{
}

DynamicLibrary::DynamicLibrary(DynamicLibrary &&mov)
	: module(std::move(mov.module))
{

}

DynamicLibrary &DynamicLibrary::operator=(DynamicLibrary &&mov)
{
	this->module = std::move(mov.module);
	return *this;
}

DynamicLibrary::~DynamicLibrary()
{
}

void* DynamicLibrary::link(const char* sname)
{
#ifdef _WIN32
	return GetProcAddress(reinterpret_cast<HMODULE>(module.get()),sname);
#elif defined (__unix)
	return dlsym(module.get(),sname);
#endif
}

void *DynamicLibrary::link(const std::string &sname)
{
	return link(sname.c_str());
}

}
}
