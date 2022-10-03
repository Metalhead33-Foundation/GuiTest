#include <MhLib/Util/MhDynamicLib.hpp>

namespace MH33 {
namespace Util {
DynamicLibrary::DynamicLibrary(const char* path) : module(nullptr)
{
#ifdef _WIN32
    this->module = LoadLibraryA(path);
#else
	this->module = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
#endif
}

DynamicLibrary::DynamicLibrary(const std::string &path) : module(nullptr)
{
#ifdef _WIN32
	this->module = LoadLibraryA(path.c_str());
#else
	this->module = dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
#endif
}

DynamicLibrary::DynamicLibrary(DynamicLibrary &&mov)
{
	this->module = mov.module;
	mov.module = nullptr;
}

DynamicLibrary &DynamicLibrary::operator=(DynamicLibrary &&mov)
{
#ifdef _WIN32
	if (module) FreeLibrary(reinterpret_cast<HMODULE>(module));
#else
	if (module) dlclose(module);
#endif
	this->module = mov.module;
	mov.module = nullptr;
	return *this;
}

DynamicLibrary::~DynamicLibrary()
{
#ifdef _WIN32
    if (module) FreeLibrary(reinterpret_cast<HMODULE>(module));
#else
	if (module) dlclose(module);
#endif
}

void* DynamicLibrary::link(const char* sname)
{
#ifdef _WIN32
    return GetProcAddress(reinterpret_cast<HMODULE>(module),sname);
#else
	return dlsym(module,sname);
#endif
}

void *DynamicLibrary::link(const std::string &sname)
{
	return link(sname.c_str());
}
}
}
