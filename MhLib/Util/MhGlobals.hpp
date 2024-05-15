#ifndef MHGLOBALS_HPP
#define MHGLOBALS_HPP
#include <memory>

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

#define DEFINE_PTR( a )                                                        \
	typedef a *p##a;                                                           \
	typedef std::shared_ptr< a > s##a;                                         \
	typedef std::weak_ptr< a > w##a;                                           \
	typedef std::unique_ptr< a > u##a;

#define DEFINE_CLASS( klass )                                                  \
	class klass;                                                               \
	DEFINE_PTR( klass )

#define DEFINE_STRUCT( klass )                                                 \
	struct klass;                                                              \
	DEFINE_PTR( klass )

#endif // MHGLOBALS_HPP
