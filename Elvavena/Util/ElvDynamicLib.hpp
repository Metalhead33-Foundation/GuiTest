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
typedef void(*funcptr)();
namespace Elv {
namespace Util {

/**
 * @class DynamicLibrary
 * @brief A cross-platform class for loading and managing dynamic libraries.
 * @ingroup Utilities
 *
 * This class provides a simple interface for loading dynamic libraries on both Windows and Unix-like systems.
 * It utilizes smart pointer management to ensure the library is properly unloaded when no longer needed.
 */
class MH_UTIL_API DynamicLibrary
{
public:
	/**
	 * @brief Windows-specific function to free a loaded library.
	 * @param lib Handle to the library to be freed (cast to void*).
	 * @note This function is only defined on Windows platforms (_WIN32).
	 */
#ifdef _WIN32
	static inline void freeLibWin(void* lib) {
		FreeLibrary(static_cast<HMODULE>(lib));
	}

	/**
	 * @typedef LibType
	 * @brief Unique pointer type for managing the loaded library, utilizing freeLibWin as the custom deleter.
	 */
	typedef std::unique_ptr<void,decltype(&freeLibWin)> LibType;
#elif defined (__unix)
	/**
	 * @typedef LibType
	 * @brief Unique pointer type for managing the loaded library, utilizing dlclose as the custom deleter.
	 */
	typedef std::unique_ptr<void,decltype(&dlclose)> LibType;
#endif

private:
	/**
	 * @var module
	 * @brief The loaded library, managed by the LibType unique pointer.
	 */
	LibType module;

	/**
	 * @brief Copy constructor (deleted).
	 * @param cpy DynamicLibrary instance to copy from.
	 * @note Copying is not allowed to prevent unintended library unloading.
	 */
	DynamicLibrary(const DynamicLibrary& cpy) = delete;

	/**
	 * @brief Copy assignment operator (deleted).
	 * @param cpy DynamicLibrary instance to assign from.
	 * @note Copy assignment is not allowed to prevent unintended library unloading.
	 */
	DynamicLibrary& operator=(const DynamicLibrary& cpy) = delete;

public:
	/**
	 * @brief Constructs a DynamicLibrary instance from a library path (C-style string).
	 * @param path Path to the dynamic library.
	 */
	DynamicLibrary(const char* path);

	/**
	 * @brief Constructs a DynamicLibrary instance from a library path (std::string).
	 * @param path Path to the dynamic library.
	 */
	DynamicLibrary(const std::string& path);

	/**
	 * @brief Move constructor.
	 * @param mov DynamicLibrary instance to move from.
	 */
	DynamicLibrary(DynamicLibrary&& mov);

	/**
	 * @brief Move assignment operator.
	 * @param mov DynamicLibrary instance to assign from.
	 * @return Reference to the assigned DynamicLibrary instance.
	 */
	DynamicLibrary& operator=(DynamicLibrary&& mov);

	/**
	 * @brief Destructor, ensuring the library is properly unloaded.
	 */
	~DynamicLibrary();

	/**
	 * @brief Links to a symbol within the loaded library (C-style string).
	 * @param sname Name of the symbol to link.
	 * @return Pointer to the linked symbol, or nullptr if linking fails.
	 */
	void* link(const char* sname);

	/**
	 * @brief Links to a symbol within the loaded library (std::string).
	 * @param sname Name of the symbol to link.
	 * @return Pointer to the linked symbol, or nullptr if linking fails.
	 */
	void* link(const std::string& sname);


	/**
	 * @brief Retrieves a function pointer to a symbol within the loaded library.
	 * @param name Name of the symbol to link.
	 * @return Function pointer to the linked symbol, or nullptr if linking fails.
	 * @note Casting is performed under the assumption that sizeof(void*) == sizeof(funcptr), as per POSIX dlsym documentation.
	 * @warning This function assumes the symbol is a function; use with caution for non-function symbols.
	 */
	funcptr sym_func(const char * name)
	{
		static_assert(sizeof(void*) == sizeof(funcptr));
		return (funcptr)this->link(name);
	}
	/**
	 * @brief Template function for retrieving a symbol (function or pointer) within the loaded library.
	 * @tparam T Type of the symbol to link (function, function pointer, or pointer).
	 * @param name Name of the symbol to link.
	 * @return Linked symbol of type T, or nullptr if linking fails.
	 * @note Performs necessary casting based on the template type parameter T.
	 * @warning Be cautious when using this function, as incorrect type specifications can lead to undefined behavior.
	 */
	template<typename T> auto sym(const char * name)
	{
		static_assert(std::is_function_v<T> || std::is_pointer_v<T>);
		if constexpr (std::is_function_v<T>) return reinterpret_cast<T*>(sym_func(name));
		else if constexpr (std::is_function_v<std::remove_pointer_t<T>>) return reinterpret_cast<T>(sym_func(name));
		else return reinterpret_cast<T>(link(name));
	}
	/**
	 * @brief Template function for retrieving a symbol (function or pointer) within the loaded library, with auto-deduced type.
	 * @tparam T Type of the symbol to link (automatically deduced from the provided template parameter).
	 * @param name Name of the symbol to link.
	 * @return Linked symbol of the deduced type, or nullptr if linking fails.
	 * @note Simply calls sym<T> with the auto-deduced type.
	 */
	template<auto T> auto sym(const char * name)
	{
		return sym<decltype(T)>(name);
	}
};

}
}
#endif // DYNAMICLIB_HPP
