/**
 * @file ElvDynamicLib.cpp
 * @brief Implementation of cross-platform dynamic library loading helpers.
 */

#include "ElvDynamicLib.hpp"

namespace Elv {
namespace Util {

/**
 * @brief Loads a dynamic library from a C-string path.
 * @param path Filesystem path to the dynamic library module.
 */
DynamicLibrary::DynamicLibrary(const char* path)
#ifdef _WIN32
	: module(LoadLibraryA(path), freeLibWin)
#elif defined(__unix)
	: module(dlopen(path, RTLD_LAZY | RTLD_GLOBAL), dlclose)
#endif
{
}

/**
 * @brief Loads a dynamic library from a string path.
 * @param path Filesystem path to the dynamic library module.
 */
DynamicLibrary::DynamicLibrary(const std::string& path)
#ifdef _WIN32
	: module(LoadLibraryA(path.c_str()), freeLibWin)
#elif defined(__unix)
	: module(dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL), dlclose)
#endif
{
}

/**
 * @brief Move-constructs a dynamic library wrapper.
 * @param mov Source instance.
 */
DynamicLibrary::DynamicLibrary(DynamicLibrary&& mov)
	: module(std::move(mov.module))
{
}

/**
 * @brief Move-assigns a dynamic library wrapper.
 * @param mov Source instance.
 * @return Reference to `*this`.
 */
DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& mov)
{
	this->module = std::move(mov.module);
	return *this;
}

/** @brief Destructor. */
DynamicLibrary::~DynamicLibrary()
{
}

/**
 * @brief Resolves a symbol by name.
 * @param sname Symbol name.
 * @return Raw symbol address or `nullptr` on failure.
 */
void* DynamicLibrary::link(const char* sname)
{
#ifdef _WIN32
	return GetProcAddress(reinterpret_cast<HMODULE>(module.get()), sname);
#elif defined(__unix)
	return dlsym(module.get(), sname);
#endif
}

/**
 * @brief Resolves a symbol by name.
 * @param sname Symbol name.
 * @return Raw symbol address or `nullptr` on failure.
 */
void* DynamicLibrary::link(const std::string& sname)
{
	return link(sname.c_str());
}

} // namespace Util
} // namespace Elv
