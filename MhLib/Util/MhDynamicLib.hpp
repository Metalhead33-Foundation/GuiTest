#ifndef MHDYNAMICLIB_HPP
#define MHDYNAMICLIB_HPP
#ifdef _WIN32
#include <windows.h>
#elif defined (__unix)
#include <dlfcn.h>
#else
#error "Unsupported operating system type!"
#endif
#include <MhLib/Util/MhGlobals.hpp>
#include <string>

namespace MH33 {
namespace Util {

class MH_UTIL_API DynamicLibrary
{
private:
	void* module;
	DynamicLibrary(const DynamicLibrary& cpy) = delete;
	DynamicLibrary& operator=(const DynamicLibrary& cpy) = delete;
public:
	DynamicLibrary(const char* path);
	DynamicLibrary(const std::string& path);
	DynamicLibrary(DynamicLibrary&& mov);
	DynamicLibrary& operator=(DynamicLibrary&& mov);
	~DynamicLibrary();
	void* link(const char* sname);
	void* link(const std::string& sname);
};

}
}
#endif // MHDYNAMICLIB_HPP
