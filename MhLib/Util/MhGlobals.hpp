#ifndef MHGLOBALS_HPP
#define MHGLOBALS_HPP
#include <memory>
#include <span>
#include <cassert>
#include <functional>

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

#define XSTRINGIFY(s) STRINGIFY(s)
#define STRINGIFY(s) #s
#define CONCAT(a,b) a b

namespace MH33 {
namespace Util {

template <typename T> struct span_wrappers {
	static std::span<std::byte> as_byte_span(T& thingie) {
		return std::span<std::byte>( reinterpret_cast<std::byte*>(&thingie), sizeof(T) );
	}
	static const std::span<const std::byte> as_const_byte_span(const T& thingie) {
		return std::span<const std::byte>( reinterpret_cast<const std::byte*>(&thingie), sizeof(T) );
	}
	static std::span<std::byte> as_byte_span(T* thingies, size_t size) {
		return std::span<std::byte>( reinterpret_cast<std::byte*>(thingies), sizeof(T) * size );
	}
	static const std::span<const std::byte> as_const_byte_span(const T* thingies, size_t size) {
		return std::span<const std::byte>( reinterpret_cast<const std::byte*>(&thingies), sizeof(T) * size );
	}
	static std::span<std::byte> as_byte_span(std::span<T>& thingies) {
		return std::span<std::byte>( reinterpret_cast<std::byte*>(thingies.data()), thingies.size_bytes() );
	}
	static const std::span<const std::byte> as_const_byte_span(const std::span<const T> thingies) {
		return std::span<const std::byte>( reinterpret_cast<const std::byte*>(thingies.data()), thingies.size_bytes() );
	}
};
template <> struct span_wrappers<void> {
	static std::span<std::byte> as_byte_span(void* thingies, size_t size) {
		return std::span<std::byte>( reinterpret_cast<std::byte*>(thingies), size );
	}
	static const std::span<const std::byte> as_const_byte_span(const void* thingies, size_t size) {
		return std::span<const std::byte>( reinterpret_cast<const std::byte*>(&thingies), size );
	}
};
template <typename T> std::span<std::byte> as_byte_span(std::span<T>& thingies) {
	return span_wrappers<T>::as_byte_span(thingies);
}
template <typename T> const std::span<const std::byte> as_const_byte_span(const std::span<const T> thingies) {
	return span_wrappers<T>::as_const_byte_span(thingies);
}
template <typename T> std::span<std::byte> as_byte_span(T& thingie) {
	return span_wrappers<T>::as_byte_span(thingie);
}
template <typename T> const std::span<const std::byte> as_const_byte_span(const T& thingie) {
	return span_wrappers<T>::as_const_byte_span(thingie);
}
template <typename T> std::span<std::byte> as_byte_span(T* thingies, size_t size) {
	return span_wrappers<T>::as_byte_span(thingies,size);
}
template <typename T> const std::span<const std::byte> as_const_byte_span(const T* thingies, size_t size) {
	return span_wrappers<T>::as_const_byte_span(thingies,size);
}

template <typename TIn, typename TOut> struct transform_wrappers {
	void transform(const std::span<const TIn>& input, const std::span<TOut>& output)
	{
		assert(input.size() == output.size());
		for(size_t i = 0; i < input.size(); ++i) {
			output[i] = static_cast<TOut>(input[i]);
		}
	}
	typedef std::function<TOut(const TIn&)> tf0_t;
	void transform(const std::span<const TIn>& input, const std::span<TOut>& output, const tf0_t& transformFun)
	{
		assert(input.size() == output.size());
		for(size_t i = 0; i < input.size(); ++i) {
			output[i] = transformFun(input[i]);
		}
	}
	typedef std::function<void(const TIn&,TOut&)> tf1_t;
	void transform(const std::span<const TIn>& input, const std::span<TOut>& output, const tf1_t& transformFun)
	{
		assert(input.size() == output.size());
		for(size_t i = 0; i < input.size(); ++i) {
			transformFun(input[i],output[i]);
		}
	}
};
template <typename TIn, typename TOut> void transform(const std::span<const TIn>& input, const std::span<TOut>& output) {
	transform_wrappers<TIn,TOut>::transform(input,output);
}
template <typename TIn, typename TOut> void transform(const std::span<const TIn>& input, const std::span<TOut>& output, const std::function<TOut(const TIn&)>& transFormFun) {
	transform_wrappers<TIn,TOut>::transform(input,output,transFormFun);
}
template <typename TIn, typename TOut> void transform(const std::span<const TIn>& input, const std::span<TOut>& output, const std::function<void(const TIn&,TOut&)>& transFormFun) {
	transform_wrappers<TIn,TOut>::transform(input,output,transFormFun);
}


}
}

#endif // MHGLOBALS_HPP
