#ifndef MHGLOBALS_HPP
#define MHGLOBALS_HPP

/*#ifdef _WIN32
#define MH33_API_EXPORT __declspec(dllexport)
#define MH33_API_IMPORT __declspec(dllimport)
#elif __unix
#define MH33_API_EXPORT __attribute__ ((__visibility__ ("default")))
#define MH33_API_IMPORT __attribute__ ((__visibility__ ("default")))
#else
#define MH33_API_EXPORT
#define MH33_API_IMPORT
#endif*/
#define MH33_API_EXPORT
#define MH33_API_IMPORT

#if defined(MH_UTIL_EXPORT)
#define MH_UTIL_API MH33_API_EXPORT
#else
#define MH_UTIL_API MH33_API_IMPORT
#endif

#endif // MHGLOBALS_HPP
