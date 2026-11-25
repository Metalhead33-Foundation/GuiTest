#ifndef JSUTIL_MH33_HPP
#define JSUTIL_MH33_HPP
#include <jsapi.h>
#include <mozjs-153/js/Object.h>
#include <mozjs-153/js/Array.h>
#include <mozjs-153/js/MapAndSet.h>
#include <mozjs-153/js/Id.h>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <functional>
#include <memory>
#include <Elvavena/Util/ElvIntegralIterator.hpp>
#include <Elvavena/Util/ElvSignal.hpp>
#include <Elvavena/Util/ElvContainerBasic.hpp>

namespace JS {

std::string JsToNativeString(JSContext* cx, JSString* jsstring);

struct ClassCreator {
	JSClassOps ops;
	JSClass protoClass;
	std::string name;
	JSNative constructor;
	unsigned nargs;
	std::unique_ptr<JS::RootedObject> prototype;
	std::vector<JSPropertySpec> ps;
	std::vector<JSFunctionSpec> fs;
	std::vector<JSPropertySpec> static_ps;
	std::vector<JSFunctionSpec> static_fs;
	JSObject* registerToJS(JSContext* cx, JS::HandleObject obj);
	JSObject* registerToJS(JSContext* cx, JS::HandleObject obj, JS::HandleObject prototype);
};

/*
using JSNative = bool (*)(JSContext* cx, unsigned argc, JS::Value* vp);
typedef bool (*JSAddPropertyOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleValue v);
typedef bool (*JSDeletePropertyOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::ObjectOpResult& result);
typedef bool (*JSNewEnumerateOp)(JSContext* cx, JS::HandleObject obj, JS::MutableHandleIdVector properties, bool enumerableOnly);
typedef bool (*JSEnumerateOp)(JSContext* cx, JS::HandleObject obj);
typedef JSString* (*JSFunToStringOp)(JSContext* cx, JS::HandleObject obj, bool isToSource);
typedef bool (*JSResolveOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool* resolvedp);
typedef bool (*JSMayResolveOp)(const JSAtomState& names, jsid id, JSObject* maybeObj);
typedef void (*JSFinalizeOp)(JS::GCContext* gcx, JSObject* obj);
typedef void (*JSTraceOp)(JSTracer* trc, JSObject* obj);
typedef size_t (*JSObjectMovedOp)(JSObject* obj, JSObject* old);
typedef bool (*LookupPropertyOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleObject objp, PropertyResult* propp);
typedef bool (*DefinePropertyOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::Handle<JS::PropertyDescriptor> desc, JS::ObjectOpResult& result);
typedef bool (*HasPropertyOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool* foundp);
typedef bool (*GetPropertyOp)(JSContext* cx, JS::HandleObject obj, JS::HandleValue receiver, JS::HandleId id, JS::MutableHandleValue vp);
typedef bool (*SetPropertyOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleValue v, JS::HandleValue receiver, JS::ObjectOpResult& result);
typedef bool (*GetOwnPropertyOp)( JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc);
typedef bool (*DeletePropertyOp)(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::ObjectOpResult& result);
*/
typedef std::function<bool(JSContext* cx, unsigned argc, JS::Value* vp)> JSNativeFunctionWrapper;
typedef std::function<bool(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleValue v)> JSAddPropertyOpWrapper;
typedef std::function<bool(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::ObjectOpResult& result)> JSDeletePropertyOpWrapper;
typedef std::function<bool(JSContext* cx, JS::HandleObject obj, JS::MutableHandleIdVector properties, bool enumerableOnly)> JSNewEnumerateOpWrapper;
typedef std::function<bool(JSContext* cx, JS::HandleObject obj)> JSEnumerateOpWrapper;
typedef std::function<JSString*(JSContext* cx, JS::HandleObject obj, bool isToSource)> JSFunToStringOpWrapper;
typedef std::function<bool(JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool* resolvedp)> JSResolveOpWrapper;
typedef std::function<bool(const JSAtomState& names, jsid id, JSObject* maybeObj)> JSMayResolveOpWrapper;
typedef std::function<void(JS::GCContext* gcx, JSObject* obj)> JSFinalizeOpWrapper;
typedef std::function<void(JSTracer* trc, JSObject* obj)> JSTraceOpWrapper;
typedef std::function<size_t(JSObject* obj, JSObject* old)> JSObjectMovedOpWrapper;
typedef std::function<bool(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleObject objp, js::PropertyResult* propp)> LookupPropertyOpWrapper;
typedef std::function<bool(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::Handle<JS::PropertyDescriptor> desc, JS::ObjectOpResult& result)> DefinePropertyOpWrapper;
typedef std::function<bool(JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool* foundp)> HasPropertyOpWrapper;
typedef std::function<bool(JSContext* cx, JS::HandleObject obj, JS::HandleValue receiver, JS::HandleId id, JS::MutableHandleValue vp)> GetPropertyOpWrapper;
typedef std::function<bool(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleValue v, JS::HandleValue receiver, JS::ObjectOpResult& result)> SetPropertyOpWrapper;
typedef std::function<bool(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc)> GetOwnPropertyOpWrapper;
typedef std::function<bool(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::ObjectOpResult& result)> DeletePropertyOpWrapper;

bool executeJSNative(const JSNativeFunctionWrapper& fun, JSContext* cx, unsigned argc, JS::Value* vp);
bool executeJSAddPropertyOp(const JSAddPropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleValue v);
bool executeJSDeletePropertyOp(const JSDeletePropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::ObjectOpResult& result);
bool executeJSNewEnumerateOp(const JSNewEnumerateOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::MutableHandleIdVector properties, bool enumerableOnly);
bool executeJSEnumerateOp(const JSEnumerateOpWrapper& fun, JSContext* cx, JS::HandleObject obj);
JSString* executeJSFunToStringOp(const JSFunToStringOpWrapper& fun, JSContext* cx, JS::HandleObject obj, bool isToSource);
bool executeJSResolveOp(const JSResolveOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool* resolvedp);
bool executeJSMayResolveOp(const JSMayResolveOpWrapper& fun, const JSAtomState& names, jsid id, JSObject* maybeObj);
void executeJSFinalizeOp(const JSFinalizeOpWrapper& fun, JS::GCContext* gcx, JSObject* obj);
void executeJSTraceOp(const JSTraceOpWrapper& fun, JSTracer* trc, JSObject* obj);
size_t executeJSObjectMovedOp(const JSObjectMovedOpWrapper& fun, JSObject* obj, JSObject* old);
bool executeLookupPropertyOp(const LookupPropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleObject objp, js::PropertyResult* propp);
bool executeDefinePropertyOp(const DefinePropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::Handle<JS::PropertyDescriptor> desc, JS::ObjectOpResult& result);
bool executeHasPropertyOp(const HasPropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool* foundp);
bool executeGetPropertyOp(const GetPropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleValue receiver, JS::HandleId id, JS::MutableHandleValue vp);
bool executeSetPropertyOp(const SetPropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleValue v, JS::HandleValue receiver, JS::ObjectOpResult& result);
bool executeGetOwnPropertyOp(const GetOwnPropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc);
bool executeDeletePropertyOp(const DeletePropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::ObjectOpResult& result);

/**
 * @brief Stores a native object in a reserved slot of its JavaScript wrapper.
 * @tparam T Native type the wrapper owns.
 * @param obj Wrapper object; its class must have at least @p slot + 1 reserved slots.
 * @param native Heap allocation that @ref finalizeNative will delete; may be null.
 * @param slot Reserved slot to write.
 *
 * A wrapper declares `JSCLASS_HAS_RESERVED_SLOTS(n)` and keeps its native half in
 * slot 0 as a private pointer whose static type the class itself names. There is no
 * runtime type tag, no global id table and no `shared_ptr<void>` round-trip: the
 * class's own finalizer knows exactly what to delete.
 */
template <typename T>
void setNativePointer(JSObject* obj, T* native, std::size_t slot = 0)
{
	JS_SetReservedSlot(obj, slot, JS::PrivateValue(static_cast<void*>(native)));
}

/**
 * @brief Reads back a pointer stored by @ref setNativePointer.
 * @tparam T Native type the wrapper owns.
 * @param obj Wrapper object; may be null, in which case null is returned.
 * @param slot Reserved slot to read.
 * @return The stored pointer, or null when the slot is empty.
 */
template <typename T>
T* getNativePointer(JSObject* obj, std::size_t slot = 0)
{
	if (!obj) return nullptr;
	return static_cast<T*>(JS::GetMaybePtrFromReservedSlot<void>(obj, slot));
}

/**
 * @brief Finalizer for a wrapper class that owns a `new`-allocated native object.
 * @tparam T Native type; `delete` must be able to release it.
 * @param gcx Unused GC context.
 * @param obj Wrapper being collected; null is tolerated.
 * @warning A class that mutates engine state from its finalizer (a mixer, an I/O
 *          device, a GUI scene) must **not** set `JSCLASS_BACKGROUND_FINALIZE`:
 *          background finalization runs on a GC helper thread and would race the
 *          audio callback or the resource factory.
 */
template <typename T>
void finalizeNative(JS::GCContext* gcx, JSObject* obj)
{
	(void)gcx;
	if (!obj) return;
	delete getNativePointer<T>(obj, 0);
}

// Other wrappers
namespace detail {

template <typename T>
struct is_basic_string : std::false_type {};
template <typename CharT, typename Traits, typename Alloc>
struct is_basic_string<std::basic_string<CharT, Traits, Alloc>> : std::true_type {};

template <typename T>
struct is_basic_string_view : std::false_type {};
template <typename CharT, typename Traits>
struct is_basic_string_view<std::basic_string_view<CharT, Traits>> : std::true_type {};

template <typename T>
inline constexpr bool is_string_like_v = is_basic_string<T>::value || is_basic_string_view<T>::value;

// Distinguishes std::set / std::unordered_set from SequentialContainer (std::list, std::deque)
template <typename T>
concept SetLike = Elv::Util::ContainerCommon<T> &&
				  !Elv::Util::VectorLike<T> &&
				  !Elv::Util::MapLike<T> &&
				  requires(T container, const typename T::value_type& value) {
					  typename T::key_type;
					  requires std::same_as<typename T::key_type, typename T::value_type>;
					  { container.insert(value) } -> std::same_as<std::pair<typename T::iterator, bool>>;
					  { container.contains(value) } -> std::same_as<bool>;
				  };

template <typename CharT>
JSString* createJSString(JSContext* cx, const CharT* data, size_t size) {
	if constexpr (sizeof(CharT) == 1) {
		JS::UTF8Chars uchars(reinterpret_cast<const char*>(data), size);
		return JS_NewStringCopyUTF8N(cx, uchars);
	} else if constexpr (sizeof(CharT) == 2) {
		return JS_NewUCStringCopyN(cx, reinterpret_cast<const char16_t*>(data), size);
	} else {
		// Fallback for 32-bit wchar_t / char32_t -> UTF-16
		std::u16string utf16;
		utf16.reserve(size);
		for (size_t i = 0; i < size; ++i) {
			uint32_t cp = static_cast<uint32_t>(data[i]);
			if (cp <= 0xFFFF) {
				utf16.push_back(static_cast<char16_t>(cp));
			} else {
				cp -= 0x10000;
				utf16.push_back(static_cast<char16_t>(0xD800 + (cp >> 10)));
				utf16.push_back(static_cast<char16_t>(0xDC00 + (cp & 0x3FF)));
			}
		}
		return JS_NewUCStringCopyN(cx, utf16.data(), utf16.size());
	}
}

template <typename CharT, typename Traits, typename Alloc = std::allocator<CharT>>
std::basic_string<CharT, Traits, Alloc> jsStringToNative(JSContext* cx, JSString* str) {
	if constexpr (std::is_same_v<CharT, char> && std::is_same_v<Traits, std::char_traits<char>>) {
		auto native = JsToNativeString(cx, str);
		return std::basic_string<CharT, Traits, Alloc>(native.begin(), native.end());
	} else {
		std::string utf8 = JsToNativeString(cx, str);
		std::basic_string<CharT, Traits, Alloc> result;
		result.reserve(utf8.size());
		for (unsigned char c : utf8) {
			result.push_back(static_cast<CharT>(c));
		}
		return result;
	}
}

} // namespace detail

// Primary template
template <typename T>
struct ReturnValueWrapper {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const T& value) {
		(void)cx;
		(void)value;
		rval.setUndefined();
	}
	static T fromValue(JSContext* cx, JS::HandleValue arg) {
		(void)cx;
		(void)arg;
		throw std::runtime_error("Unsupported type in ReturnValueWrapper!");
	}
};

// std::optional<T>
template <typename T>
struct ReturnValueWrapper<std::optional<T>> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const std::optional<T>& value) {
		if (!value.has_value()) {
			rval.setNull();
			return;
		}
		ReturnValueWrapper<std::decay_t<T>>::setReturnValue(cx, rval, *value);
	}

	static std::optional<T> fromValue(JSContext* cx, JS::HandleValue arg) {
		if (arg.isNullOrUndefined()) {
			return std::nullopt;
		}
		return ReturnValueWrapper<std::decay_t<T>>::fromValue(cx, arg);
	}
};

// Custom Elv::Util types
template <typename T>
struct ReturnValueWrapper<Elv::Util::IntegralIterable<T>> {
	static void setReturnValue(JSContext*, JS::MutableHandleValue rval, const Elv::Util::IntegralIterable<T>& value) {
		rval.setInt32(static_cast<int32_t>(value.var));
	}
	static Elv::Util::IntegralIterable<T> fromValue(JSContext*, JS::HandleValue arg) {
		return Elv::Util::IntegralIterable<T>(static_cast<T>(arg.toInt32()));
	}
};

template <typename T>
struct ReturnValueWrapper<Elv::Util::IntegralIterator<T>> {
	static void setReturnValue(JSContext*, JS::MutableHandleValue rval, const Elv::Util::IntegralIterator<T>& value) {
		rval.setInt32(static_cast<int32_t>(value.var));
	}
	static Elv::Util::IntegralIterator<T> fromValue(JSContext*, JS::HandleValue arg) {
		return Elv::Util::IntegralIterator<T>(static_cast<T>(arg.toInt32()));
	}
};

// Primitives
template <>
struct ReturnValueWrapper<bool> {
	static void setReturnValue(JSContext*, JS::MutableHandleValue rval, bool value) {
		rval.setBoolean(value);
	}
	static bool fromValue(JSContext*, JS::HandleValue arg) {
		return arg.toBoolean();
	}
};

template <>
struct ReturnValueWrapper<std::nullptr_t> {
	static void setReturnValue(JSContext*, JS::MutableHandleValue rval, std::nullptr_t) {
		rval.setNull();
	}
	static std::nullptr_t fromValue(JSContext*, JS::HandleValue) {
		return nullptr;
	}
};

template <>
struct ReturnValueWrapper<std::monostate> {
	static void setReturnValue(JSContext*, JS::MutableHandleValue rval, std::monostate) {
		rval.setNull();
	}
	static std::nullptr_t fromValue(JSContext*, JS::HandleValue) {
		return nullptr;
	}
};

template <>
struct ReturnValueWrapper<std::nullopt_t> {
	static void setReturnValue(JSContext*, JS::MutableHandleValue rval, std::nullopt_t) {
		rval.setNull();
	}
	static std::nullptr_t fromValue(JSContext*, JS::HandleValue) {
		return nullptr;
	}
};

template <std::integral T>
	requires (!std::same_as<T, bool>)
struct ReturnValueWrapper<T> {
	static void setReturnValue(JSContext*, JS::MutableHandleValue rval, T value) {
		if constexpr (sizeof(T) < 4 || (sizeof(T) == 4 && std::is_signed_v<T>)) {
			rval.setInt32(static_cast<int32_t>(value));
		} else if constexpr (sizeof(T) == 4 && std::is_unsigned_v<T>) {
			rval.setNumber(static_cast<uint32_t>(value));
		} else {
			rval.setNumber(static_cast<double>(value));
		}
	}

	static T fromValue(JSContext*, JS::HandleValue arg) {
		if constexpr (sizeof(T) < 4 || (sizeof(T) == 4 && std::is_signed_v<T>)) {
			return static_cast<T>(arg.toInt32());
		} else {
			return static_cast<T>(arg.toNumber());
		}
	}
};

template <std::floating_point T>
struct ReturnValueWrapper<T> {
	static void setReturnValue(JSContext*, JS::MutableHandleValue rval, T value) {
		rval.setDouble(static_cast<double>(value));
	}
	static T fromValue(JSContext*, JS::HandleValue arg) {
		return static_cast<T>(arg.toDouble());
	}
};

// Generic std::basic_string
template <typename CharT, typename Traits, typename Allocator>
struct ReturnValueWrapper<std::basic_string<CharT, Traits, Allocator>> {
	using DataType = std::basic_string<CharT, Traits, Allocator>;

	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const DataType& value) {
		JS::RootedString rstr(cx, detail::createJSString(cx, value.data(), value.size()));
		rval.setString(rstr);
	}

	static DataType fromValue(JSContext* cx, JS::HandleValue arg) {
		return detail::jsStringToNative<CharT, Traits, Allocator>(cx, arg.toString());
	}
};

// Generic std::basic_string_view
template <typename CharT, typename Traits>
struct ReturnValueWrapper<std::basic_string_view<CharT, Traits>> {
	using ViewType = std::basic_string_view<CharT, Traits>;
	using OwnedType = std::basic_string<CharT, Traits>;

	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, ViewType value) {
		JS::RootedString rstr(cx, detail::createJSString(cx, value.data(), value.size()));
		rval.setString(rstr);
	}

	// Returns an owning basic_string (implicitly convertible to basic_string_view)
	// so the underlying JS string data cannot dangle after GC.
	static OwnedType fromValue(JSContext* cx, JS::HandleValue arg) {
		return detail::jsStringToNative<CharT, Traits>(cx, arg.toString());
	}
};

// Fixed-size std::array
template <typename T, size_t Size>
struct ReturnValueWrapper<std::array<T, Size>> {
	using DataType = std::array<T, Size>;

	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const DataType& value) {
		JS::RootedObject robj(cx, JS::NewArrayObject(cx, Size));
		for (size_t i = 0; i < Size; ++i) {
			JS::RootedValue elem(cx);
			ReturnValueWrapper<std::decay_t<T>>::setReturnValue(cx, &elem, value[i]);
			JS_SetElement(cx, robj, i, elem);
		}
		rval.setObjectOrNull(robj);
	}

	static DataType fromValue(JSContext* cx, JS::HandleValue arg) {
		bool isArray = false;
		JS::IsArrayObject(cx, arg, &isArray);
		if (!isArray) {
			throw std::runtime_error("Argument is not an array!");
		}

		JS::RootedObject jsArray(cx, &arg.toObject());
		DataType nativeData{};
		for (size_t i = 0; i < Size; ++i) {
			JS::RootedValue elem(cx);
			JS_GetElement(cx, jsArray, i, &elem);
			nativeData[i] = ReturnValueWrapper<std::decay_t<T>>::fromValue(cx, elem);
		}
		return nativeData;
	}
};

// Concept-constrained VectorLike containers (std::vector, custom random-access vectors, etc.)
template <Elv::Util::VectorLike T>
	requires (!detail::is_string_like_v<T>)
struct ReturnValueWrapper<T> {
	using ValueType = typename T::value_type;

	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const T& value) {
		JS::RootedObject robj(cx, JS::NewArrayObject(cx, value.size()));
		for (size_t i = 0; i < value.size(); ++i) {
			JS::RootedValue elem(cx);
			ReturnValueWrapper<std::decay_t<ValueType>>::setReturnValue(cx, &elem, value[i]);
			JS_SetElement(cx, robj, i, elem);
		}
		rval.setObjectOrNull(robj);
	}

	static T fromValue(JSContext* cx, JS::HandleValue arg) {
		bool isArray = false;
		JS::IsArrayObject(cx, arg, &isArray);
		if (!isArray) {
			throw std::runtime_error("Argument is not an array!");
		}

		JS::RootedObject jsArray(cx, &arg.toObject());
		uint32_t jsArrLength = 0;
		JS::GetArrayLength(cx, jsArray, &jsArrLength);

		T nativeData;
		nativeData.reserve(jsArrLength);
		for (uint32_t i = 0; i < jsArrLength; ++i) {
			JS::RootedValue elem(cx);
			JS_GetElement(cx, jsArray, i, &elem);
			nativeData.push_back(ReturnValueWrapper<std::decay_t<ValueType>>::fromValue(cx, elem));
		}
		return nativeData;
	}
};

// Concept-constrained SequentialContainer (std::list, std::deque, etc.)
template <Elv::Util::SequentialContainer T>
	requires (!detail::SetLike<T> && !detail::is_string_like_v<T>)
struct ReturnValueWrapper<T> {
	using ValueType = typename T::value_type;

	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const T& value) {
		JS::RootedObject robj(cx, JS::NewArrayObject(cx, value.size()));
		size_t i = 0;
		for (const auto& item : value) {
			JS::RootedValue elem(cx);
			ReturnValueWrapper<std::decay_t<ValueType>>::setReturnValue(cx, &elem, item);
			JS_SetElement(cx, robj, i++, elem);
		}
		rval.setObjectOrNull(robj);
	}

	static T fromValue(JSContext* cx, JS::HandleValue arg) {
		bool isArray = false;
		JS::IsArrayObject(cx, arg, &isArray);
		if (!isArray) {
			throw std::runtime_error("Argument is not an array!");
		}

		JS::RootedObject jsArray(cx, &arg.toObject());
		uint32_t jsArrLength = 0;
		JS::GetArrayLength(cx, jsArray, &jsArrLength);

		T nativeData;
		for (uint32_t i = 0; i < jsArrLength; ++i) {
			JS::RootedValue elem(cx);
			JS_GetElement(cx, jsArray, i, &elem);
			nativeData.insert(nativeData.end(), ReturnValueWrapper<std::decay_t<ValueType>>::fromValue(cx, elem));
		}
		return nativeData;
	}
};

// Concept-constrained SetLike containers (std::set, std::unordered_set, etc.)
template <detail::SetLike T>
struct ReturnValueWrapper<T> {
	using KeyType = typename T::key_type;

	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const T& value) {
		JS::RootedObject robj(cx, JS::NewSetObject(cx));
		for (const auto& item : value) {
			JS::RootedValue elem(cx);
			ReturnValueWrapper<std::decay_t<KeyType>>::setReturnValue(cx, &elem, item);
			JS::SetAdd(cx, robj, elem);
		}
		rval.setObjectOrNull(robj);
	}

	static T fromValue(JSContext* cx, JS::HandleValue arg) {
		if (!arg.isObject()) {
			throw std::runtime_error("Argument is not a Set object!");
		}

		bool isSet = false;
		JS::RootedObject jsSet(cx, &arg.toObject());
		JS::IsSetObject(cx, jsSet, &isSet);
		if (!isSet) {
			throw std::runtime_error("Argument is not a Set!");
		}

		JS::RootedValue keysVal(cx);
		JS::SetKeys(cx, jsSet, &keysVal);
		JS::RootedObject keysObj(cx, &keysVal.toObject());

		uint32_t length = 0;
		JS::GetArrayLength(cx, keysObj, &length);

		T nativeData;
		for (uint32_t i = 0; i < length; ++i) {
			JS::RootedValue elem(cx);
			JS_GetElement(cx, keysObj, i, &elem);
			nativeData.insert(ReturnValueWrapper<std::decay_t<KeyType>>::fromValue(cx, elem));
		}
		return nativeData;
	}
};

// Concept-constrained MapLike containers (std::map, std::unordered_map, etc.)
template <Elv::Util::MapLike T>
struct ReturnValueWrapper<T> {
	using KeyType = typename T::key_type;
	using MappedType = typename T::mapped_type;

	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const T& value) {
		JS::RootedObject robj(cx, JS::NewMapObject(cx));
		for (const auto& [k, v] : value) {
			JS::RootedValue jsKey(cx);
			JS::RootedValue jsVal(cx);
			ReturnValueWrapper<std::decay_t<KeyType>>::setReturnValue(cx, &jsKey, k);
			ReturnValueWrapper<std::decay_t<MappedType>>::setReturnValue(cx, &jsVal, v);
			JS::MapSet(cx, robj, jsKey, jsVal);
		}
		rval.setObjectOrNull(robj);
	}

	static T fromValue(JSContext* cx, JS::HandleValue arg) {
		if (!arg.isObject()) {
			throw std::runtime_error("Argument is not a Map object!");
		}

		bool isMap = false;
		JS::RootedObject jsMap(cx, &arg.toObject());
		JS::IsMapObject(cx, jsMap, &isMap);
		if (!isMap) {
			throw std::runtime_error("Argument is not a Map!");
		}

		JS::RootedValue keysVal(cx);
		if (!JS::MapKeys(cx, jsMap, &keysVal)) {
			throw std::runtime_error("Failed to get keys from the Map");
		}

		JS::RootedObject keysObj(cx, &keysVal.toObject());
		uint32_t mapSize = JS::MapSize(cx, jsMap);

		T nativeData;
		for (uint32_t i = 0; i < mapSize; ++i) {
			JS::RootedValue jsKey(cx);
			JS::RootedValue jsVal(cx);
			JS_GetElement(cx, keysObj, i, &jsKey);
			JS::MapGet(cx, jsMap, jsKey, &jsVal);

			KeyType nativeKey = ReturnValueWrapper<std::decay_t<KeyType>>::fromValue(cx, jsKey);
			MappedType nativeValue = ReturnValueWrapper<std::decay_t<MappedType>>::fromValue(cx, jsVal);
			nativeData.emplace(std::move(nativeKey), std::move(nativeValue));
		}
		return nativeData;
	}
};

// Convenience dispatch
template <typename T>
void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const T& value) {
	ReturnValueWrapper<std::decay_t<T>>::setReturnValue(cx, rval, value);
}

// Argument extraction
template <typename... Args, std::size_t... Is>
auto getNativeArgumentsImpl(JSContext* cx, const JS::CallArgs& args, std::index_sequence<Is...>) {
	return std::make_tuple(ReturnValueWrapper<std::decay_t<Args>>::fromValue(cx, args.get(Is))...);
}

template <typename... Args>
auto getNativeArguments(JSContext* cx, const JS::CallArgs& args) {
	return getNativeArgumentsImpl<Args...>(cx, args, std::index_sequence_for<Args...>{});
}

// Convert C++ arguments into a JS::RootedValueVector in forward order (0 .. N-1)
template <typename... Args, std::size_t... Is>
void setJSValuesImpl(JSContext* cx, JS::RootedValueVector& valarr, std::index_sequence<Is...>, Args&&... args) {
	(setReturnValue(cx, valarr[Is], std::forward<Args>(args)), ...);
}

template <typename... Args>
void setJSValues(JSContext* cx, JS::RootedValueVector& valarr, Args&&... args) {
	setJSValuesImpl(cx, valarr, std::index_sequence_for<Args...>{}, std::forward<Args>(args)...);
}

// Unified free function wrapper
template <typename Ret, typename... Args>
bool jsFunctionWrapper(JSContext* cx, unsigned argc, JS::Value* vp, Ret (*func)(Args...)) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	if (argc != sizeof...(Args)) {
		JS_ReportErrorASCII(cx, "Wrong number of arguments (expected %zu, got %u)", sizeof...(Args), argc);
		return false;
	}

	try {
		auto nativeArgs = getNativeArguments<Args...>(cx, args);
		if constexpr (std::is_void_v<Ret>) {
			std::apply(func, std::move(nativeArgs));
			args.rval().setUndefined();
		} else {
			Ret result = std::apply(func, std::move(nativeArgs));
			setReturnValue(cx, args.rval(), result);
		}
		return true;
	} catch (const std::exception& e) {
		JS_ReportErrorASCII(cx, "Exception: %s", e.what());
		return false;
	}
}

// Unified member function wrapper
namespace detail {
template <typename T, typename MethodPtr, typename Ret, typename... Args>
bool invokeMemberMethod(JSContext* cx, unsigned argc, JS::Value* vp, MethodPtr method) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	if (!args.thisv().isObject()) {
		JS_ReportErrorASCII(cx, "Invalid 'this' object");
		return false;
	}

	auto obj = args.thisv().toObjectOrNull();
	auto ptr = getNativePointer<T>(obj, 0);
	if (!ptr) {
		JS_ReportErrorASCII(cx, "Native object pointer is null");
		return false;
	}

	if (argc != sizeof...(Args)) {
		JS_ReportErrorASCII(cx, "Wrong number of arguments (expected %zu, got %u)", sizeof...(Args), argc);
		return false;
	}

	try {
		auto nativeArgs = getNativeArguments<Args...>(cx, args);
		auto boundArgs = std::tuple_cat(std::make_tuple(ptr), std::move(nativeArgs));

		if constexpr (std::is_void_v<Ret>) {
			std::apply(method, std::move(boundArgs));
			args.rval().setUndefined();
		} else {
			Ret result = std::apply(method, std::move(boundArgs));
			setReturnValue(cx, args.rval(), result);
		}
		return true;
	} catch (const std::exception& e) {
		JS_ReportErrorASCII(cx, "Exception: %s", e.what());
		return false;
	}
}
} // namespace detail

template <typename T, typename Ret, typename... Args>
bool jsMemberFunctionWrapper(JSContext* cx, unsigned argc, JS::Value* vp, Ret (T::*method)(Args...)) {
	return detail::invokeMemberMethod<T, decltype(method), Ret, Args...>(cx, argc, vp, method);
}

template <typename T, typename Ret, typename... Args>
bool jsMemberFunctionWrapper(JSContext* cx, unsigned argc, JS::Value* vp, Ret (T::*method)(Args...) const) {
	return detail::invokeMemberMethod<T, decltype(method), Ret, Args...>(cx, argc, vp, method);
}

// Invoke a JS method on an object
template <typename... Args>
void invokeJSCallback(JSContext* cx, JS::HandleObject jsObject, const char* propertyName, Args&&... args) {
	if (!jsObject) {
		throw std::runtime_error("JS object is null");
	}

	bool hasProperty = false;
	if (!JS_HasProperty(cx, jsObject, propertyName, &hasProperty)) {
		throw std::runtime_error("Failed to check property");
	}
	if (!hasProperty) {
		return;
	}

	JS::RootedValue funcVal(cx);
	if (!JS_GetProperty(cx, jsObject, propertyName, &funcVal)) {
		throw std::runtime_error("Failed to get property");
	}
	if (!funcVal.isObject() || !JS_ObjectIsFunction(funcVal.toObjectOrNull())) {
		return;
	}

	JS::RootedFunction rootedFunc(cx, JS_ValueToFunction(cx, funcVal));

	JS::RootedValueVector valarr(cx);
	if (!valarr.resize(sizeof...(Args))) {
		throw std::runtime_error("Failed to allocate JS argument vector");
	}
	setJSValues(cx, valarr, std::forward<Args>(args)...);

	JS::RootedValue rval(cx);
	if (!JS_CallFunction(cx, jsObject, rootedFunc, valarr, &rval)) {
		throw std::runtime_error("Failed to call JS function");
	}
}

// Connect a C++ signal to a JS callback property
template <typename... SignalArgs>
void connectSignalToJS(JSContext* cx, JS::HandleObject jsObject, std::string propertyName, Elv::Util::Signal<SignalArgs...>& signal) {
	auto rootedObj = std::make_shared<JS::PersistentRootedObject>(cx, jsObject);

	signal.connect([cx, rootedObj, prop = std::move(propertyName)](SignalArgs... args) {
		try {
			if (!rootedObj || !(*rootedObj)) {
				return;
			}
			invokeJSCallback(cx, *rootedObj, prop.c_str(), std::forward<SignalArgs>(args)...);
		} catch (const std::exception& e) {
			JS_ReportErrorASCII(cx, "Exception in signal handler: %s", e.what());
		}
	});
}

}

#endif // JSUTIL_MH33_HPP
