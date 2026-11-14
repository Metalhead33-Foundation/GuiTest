#ifndef ELVUTILGLOBALS_H
#define ELVUTILGLOBALS_H
/**
 * @file ElvUtilGlobals.hpp
 * @brief Declares the ElvUtilGlobals API in the Elvavena/Util module.
 *
 * This header is part of the public declaration surface for Elvavena/Util.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */

#include <memory>

/**
 * @brief Defines raw/shared/weak/unique pointer aliases for a type.
 * @param a Type name to generate aliases for.
 *
 * Generates:
 * - `pType` as raw pointer
 * - `sType` as `std::shared_ptr<Type>`
 * - `wType` as `std::weak_ptr<Type>`
 * - `uType` as `std::unique_ptr<Type>`
 */
#define DEFINE_PTR(a)                                                           \
	typedef a* p##a;                                                             \
	typedef std::shared_ptr<a> s##a;                                             \
	typedef std::weak_ptr<a> w##a;                                               \
	typedef std::unique_ptr<a> u##a;

/**
 * @brief Forward-declares a class and defines pointer aliases for it.
 * @param klass Class name.
 */
#define DEFINE_CLASS(klass)                                                     \
	class klass;                                                                 \
	DEFINE_PTR(klass)

/**
 * @brief Forward-declares a struct and defines pointer aliases for it.
 * @param klass Struct name.
 */
#define DEFINE_STRUCT(klass)                                                    \
	struct klass;                                                                \
	DEFINE_PTR(klass)

/** @brief Expands a macro argument to a string literal after macro expansion. */
#define XSTRINGIFY(s) STRINGIFY(s)
/** @brief Converts a macro argument to a string literal. */
#define STRINGIFY(s) #s
/** @brief Token-concatenation helper written as plain adjacency. */
#define CONCAT(a, b) a b

/** @brief Export symbol visibility for Windows builds. */
#ifdef _WIN32
#define MH33_API_EXPORT __declspec(dllexport)
/** @brief Import symbol visibility for Windows builds. */
#define MH33_API_IMPORT __declspec(dllimport)
/** @brief Export symbol visibility for Unix-like builds. */
#elif __unix
#define MH33_API_EXPORT __attribute__((__visibility__("default")))
/** @brief Import symbol visibility for Unix-like builds. */
#define MH33_API_IMPORT __attribute__((__visibility__("default")))
#else
/** @brief Fallback export macro for unsupported/other targets. */
#define MH33_API_EXPORT
/** @brief Fallback import macro for unsupported/other targets. */
#define MH33_API_IMPORT
#endif

/**
 * @brief Public API visibility macro for Elvavena utility symbols.
 *
 * When `MH_UTIL_EXPORT` is defined during compilation, symbols are exported.
 * Otherwise, symbols are imported.
 */
#if defined(MH_UTIL_EXPORT)
#define MH_UTIL_API MH33_API_EXPORT
#else
#define MH_UTIL_API MH33_API_IMPORT
#endif

#endif // ELVUTILGLOBALS_H
