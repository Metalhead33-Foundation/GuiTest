#ifndef DYNAMICLIB_HPP
#define DYNAMICLIB_HPP
#ifdef _WIN32
#include <windows.h>
#elif defined (__unix)
#include <dlfcn.h>
#else
#error "Unsupported operating system type!"
#endif
#include <Elvavena/Util/ElvUtilGlobals.hpp>
#include <string>
#include <type_traits>
namespace Elv {
namespace Util {

DEFINE_CLASS(DynamicLibrary)
class MH_UTIL_API DynamicLibrary
{
public:
#ifdef _WIN32
	static inline void freeLibWin(void* lib) {
		FreeLibrary(static_cast<HMODULE>(lib));
	}
	typedef std::unique_ptr<void,decltype(&freeLibWin)> LibType;
#elif defined (__unix)
	typedef std::unique_ptr<void,decltype(&dlclose)> LibType;
#endif
private:
	LibType module;
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
#endif // DYNAMICLIB_HPP
