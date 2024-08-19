#ifndef JSUTIL_MH33_HPP
#define JSUTIL_MH33_HPP
#include <jsapi.h>
#include <mozjs-115/js/Object.h>
#include <mozjs-115/js/Array.h>
#include <mozjs-115/js/MapAndSet.h>
#include <mozjs-115/js/Id.h>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <functional>
#include <memory>
#include <MhLib/Util/MhIntegralIterator.hpp>
#include <GUI/sigslot.h>

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

uintptr_t storeSharedVoidPtr(const std::shared_ptr<void>& ptr);
uintptr_t storeSharedVoidPtr(std::shared_ptr<void>&& ptr);
uintptr_t storeSharedVoidPtrToObj(JSObject* obj, uint32_t index, const std::shared_ptr<void>& ptr);
uintptr_t storeSharedVoidPtrToObj(JSObject* obj, uint32_t index, std::shared_ptr<void>&& ptr);
std::shared_ptr<void> getSharedVoidPtr(uintptr_t id);
std::shared_ptr<void> getSharedVoidPtrFromObj(JSObject* obj, size_t slot);
void removeSmartPointer(uintptr_t id);
template<typename T> uintptr_t storeSmartPointer(const std::shared_ptr<T>& ptr) {
	return storeSharedVoidPtr(ptr);
}
template<typename T> uintptr_t storeSmartPointer(std::shared_ptr<T>&& ptr) {
	return storeSharedVoidPtr(std::move(ptr));
}
template<typename T> uintptr_t storeSmartPointerToObj(JSObject* obj, uint32_t index, const std::shared_ptr<T>& ptr) {
	return storeSharedVoidPtrToObj(obj,index,ptr);
}
template<typename T> uintptr_t storeSmartPointerToObj(JSObject* obj, uint32_t index, std::shared_ptr<T>&& ptr) {
	return storeSharedVoidPtrToObj(obj,index,std::move(ptr));
}

template<typename T> std::shared_ptr<T> getSmartPointer(uintptr_t id) {
	return std::static_pointer_cast<T>(getSharedVoidPtr(id));
}
template<typename T> std::shared_ptr<T> getSmartPointerFromObj(JSObject* obj, size_t slot) {
	return std::static_pointer_cast<T>(getSharedVoidPtrFromObj(obj,slot));
}
void js_finalizer_shared(JS::GCContext* cx, JSObject* obj);

// Other wrappers
template <typename T> struct ReturnValueWrapper {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const T& value) {
		(void)value;
		// Default case for non-specialized types
		rval.setUndefined();
	}
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, T&& value) {
		(void)value;
		// Default case for non-specialized types
		rval.setUndefined();
	}
	static T fromValue(JSContext* cx, const HandleValue& arg) {
		throw std::runtime_error("Unknown type!");
	}
};
template <typename T> struct ReturnValueWrapper<MH33::Util::IntegralIterable<T>> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, MH33::Util::IntegralIterable<T> value) {
		// Default case for non-specialized types
		rval.setInt32(static_cast<int32_t>(value.var));
	}
	static MH33::Util::IntegralIterable<T> fromValue(JSContext* cx, const HandleValue& arg) {
		return MH33::Util::IntegralIterable<T>(static_cast<T>(arg.toInt32()));
	}
};
template <typename T> struct ReturnValueWrapper<MH33::Util::IntegralIterator<T>> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, MH33::Util::IntegralIterator<T> value) {
		// Default case for non-specialized types
		rval.setInt32(static_cast<int32_t>(value.var));
	}
	static MH33::Util::IntegralIterator<T> fromValue(JSContext* cx, const HandleValue& arg) {
		return MH33::Util::IntegralIterator<T>(static_cast<T>(arg.toInt32()));
	}
};
template <> struct ReturnValueWrapper<bool> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, bool value) {
		// Default case for non-specialized types
		rval.setBoolean(value);
	}
	static bool fromValue(JSContext* cx, const HandleValue& arg) {
		return arg.toBoolean();
	}
};
template <> struct ReturnValueWrapper<std::nullptr_t> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, std::nullptr_t) {
		// Default case for non-specialized types
		rval.setNull();
	}
	static std::nullptr_t fromValue(JSContext* cx, const HandleValue& arg) {
		return nullptr;
	}
};
template <> struct ReturnValueWrapper<uint8_t> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, uint8_t value) {
		// Default case for non-specialized types
		rval.setInt32(static_cast<int32_t>(value));
	}
	static uint8_t fromValue(JSContext* cx, const JS::Value& arg) {
		return static_cast<uint8_t>(arg.toInt32());
	}
};
template <> struct ReturnValueWrapper<int8_t> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, int8_t value) {
		// Default case for non-specialized types
		rval.setInt32(static_cast<int32_t>(value));
	}
	static int8_t fromValue(JSContext* cx, const HandleValue& arg) {
		return static_cast<int8_t>(arg.toInt32());
	}
};
template <> struct ReturnValueWrapper<uint16_t> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, uint16_t value) {
		// Default case for non-specialized types
		rval.setInt32(static_cast<int32_t>(value));
	}
	static uint16_t fromValue(JSContext* cx, const HandleValue& arg) {
		return static_cast<uint16_t>(arg.toInt32());
	}
};
template <> struct ReturnValueWrapper<int16_t> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, int16_t value) {
		// Default case for non-specialized types
		rval.setInt32(static_cast<int32_t>(value));
	}
	static int16_t fromValue(JSContext* cx, const HandleValue& arg) {
		return static_cast<int16_t>(arg.toInt32());
	}
};
template <> struct ReturnValueWrapper<uint32_t> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, uint16_t value) {
		// Default case for non-specialized types
		rval.setNumber(value);
	}
	static uint32_t fromValue(JSContext* cx, const HandleValue& arg) {
		return static_cast<uint32_t>(arg.toNumber());
	}
};
template <> struct ReturnValueWrapper<int32_t> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, int16_t value) {
		// Default case for non-specialized types
		rval.setInt32(value);
	}
	static int32_t fromValue(JSContext* cx, const HandleValue& arg) {
		return arg.toInt32();
	}
};
template <> struct ReturnValueWrapper<uint64_t> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, uint64_t value) {
		// Default case for non-specialized types
		rval.setNumber(value);
	}
	static uint64_t fromValue(JSContext* cx, const HandleValue& arg) {
		return static_cast<uint64_t>(arg.toNumber());
	}
};
template <> struct ReturnValueWrapper<int64_t> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, int64_t value) {
		// Default case for non-specialized types
		rval.setNumber(value);
	}
	static int64_t fromValue(JSContext* cx, const HandleValue& arg) {
		return static_cast<int64_t>(arg.toNumber());
	}
};
template <> struct ReturnValueWrapper<float> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, float value) {
		// Default case for non-specialized types
		rval.setDouble(static_cast<double>(value));
	}
	static float fromValue(JSContext* cx, const HandleValue& arg) {
		return static_cast<float>(arg.toDouble());
	}
};
template <> struct ReturnValueWrapper<double> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, double value) {
		// Default case for non-specialized types
		rval.setDouble(value);
	}
	static double fromValue(JSContext* cx, const HandleValue& arg) {
		return arg.toDouble();
	}
};
template <> struct ReturnValueWrapper<std::string> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const std::string& value) {
		JS::UTF8Chars uchars(value.data(),value.size());
		JS::RootedString rstr(cx, JS_NewStringCopyUTF8N(cx,uchars));
		// Default case for non-specialized types
		rval.setString(rstr);
	}
	static std::string fromValue(JSContext* cx, const HandleValue& arg) {
		return JsToNativeString(cx, arg.toString());
	}
};
template <> struct ReturnValueWrapper<std::string_view> {
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const std::string_view& value) {
		JS::UTF8Chars uchars(value.data(),value.size());
		JS::RootedString rstr(cx, JS_NewStringCopyUTF8N(cx,uchars));
		// Default case for non-specialized types
		rval.setString(rstr);
	}
	static std::string fromValue(JSContext* cx, const HandleValue& arg) {
		return JsToNativeString(cx, arg.toString());
	}
};
// Fixed-size array
template <typename T, size_t size> struct ReturnValueWrapper<std::array<T,size>> {
	typedef std::array<T,size> DataType;
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const DataType& value) {
		JS::RootedObject robj(cx, JS::NewArrayObject(cx, size));
		for(size_t i = 0; i < size; ++i) {
			JS::RootedValue rootval(cx);
			ReturnValueWrapper<T>::setReturnValue(cx, rootval, value[i]);
			JS_SetElement(cx,robj,i,rootval);
		}
		rval.setObjectOrNull(robj);
	}
	static DataType fromValue(JSContext* cx, const HandleValue& arg) {
		DataType nativeData;
		bool isArray = false;
		JS::IsArrayObject(cx, arg, &isArray);
		if(!isArray) throw std::runtime_error("Argument is not an array!");
		JS::RootedObject jsArray(cx, arg.toObject());
		for(size_t i = 0; i < size; ++i) {
			JS::RootedValue rval(cx);
			JS_GetElement(cx,jsArray,i,&rval);
			nativeData[i] = ReturnValueWrapper<T>::fromValue(cx, rval);
		}
		return nativeData;
	}
};
// Variable-size vector
template <typename T, typename Allocator> struct ReturnValueWrapper<std::vector<T,Allocator>> {
	typedef std::vector<T,Allocator> DataType;
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const DataType& value) {
		JS::RootedObject robj(cx, JS::NewArrayObject(cx, value.size()));
		for(size_t i = 0; i < value.size(); ++i) {
			JS::RootedValue rootval(cx);
			ReturnValueWrapper<T>::setReturnValue(cx, rootval, value[i]);
			JS_SetElement(cx,robj,i,rootval);
		}
		rval.setObjectOrNull(robj);
	}
	static DataType fromValue(JSContext* cx, const HandleValue& arg) {
		bool isArray = false;
		JS::IsArrayObject(cx, arg, &isArray);
		if(!isArray) throw std::runtime_error("Argument is not an array!");
		JS::RootedObject jsArray(cx, arg.toObject());
		uint32_t jsArrLength;
		JS::GetArrayLength(cx,jsArray,&jsArrLength);
		DataType nativeData(jsArrLength);
		for(size_t i = 0; i < jsArrLength; ++i) {
			JS::RootedValue rval(cx);
			JS_GetElement(cx,jsArray,i,&rval);
			nativeData[i] = ReturnValueWrapper<T>::fromValue(cx, rval);
		}
		return nativeData;
	}
};
// Set
template <typename Key, typename Compare, typename Allocator> struct ReturnValueWrapper<std::set<Key,Compare,Allocator>> {
	typedef std::set<Key,Compare,Allocator> DataType;
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const DataType& value) {
		JS::RootedObject robj(cx, JS::NewSetObject(cx));
		for(const auto& it : value) {
			JS::RootedValue rootval(cx);
			ReturnValueWrapper<Key>::setReturnValue(cx, rootval, it);
			JS::SetAdd(cx,robj,rootval);
		}
		rval.setObjectOrNull(robj);
	}
	static DataType fromValue(JSContext* cx, const HandleValue& arg) {
		bool isSet = false;
		JS::RootedObject jsSet(cx, arg.toObject());
		JS::IsSetObject(cx, jsSet, &isSet);
		if(!isSet) throw std::runtime_error("Argument is not a set!");
		JS::RootedValue rootvalForSet(cx);
		JS::SetKeys(cx, jsSet, &rootvalForSet);
		JS::RootedObject rootvalForSetObj(cx, rootvalForSet.toObject());
		uint32_t jsArrLength;
		JS::GetArrayLength(cx,rootvalForSetObj,&jsArrLength);
		DataType nativeData;
		for(size_t i = 0; i < jsArrLength; ++i) {
			JS::RootedValue rval(cx);
			JS_GetElement(cx,rootvalForSetObj,i,&rval);
			nativeData.insert(ReturnValueWrapper<Key>::fromValue(cx, rval));
		}
		return nativeData;
	}
};
// Unordered Set
template <typename Key, typename Hash, typename KeyEqual, typename Allocator> struct ReturnValueWrapper<std::unordered_set<Key,Hash,KeyEqual,Allocator>> {
	typedef std::unordered_set<Key,Hash,KeyEqual,Allocator> DataType;
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const DataType& value) {
		JS::RootedObject robj(cx, JS::NewSetObject(cx));
		for(const auto& it : value) {
			JS::RootedValue rootval(cx);
			ReturnValueWrapper<Key>::setReturnValue(cx, rootval, it);
			JS::SetAdd(cx,robj,rootval);
		}
		rval.setObjectOrNull(robj);
	}
	static DataType fromValue(JSContext* cx, const HandleValue& arg) {
		bool isSet = false;
		JS::RootedObject jsSet(cx, arg.toObject());
		JS::IsSetObject(cx, jsSet, &isSet);
		if(!isSet) throw std::runtime_error("Argument is not a set!");
		JS::RootedValue rootvalForSet(cx);
		JS::SetKeys(cx, jsSet, &rootvalForSet);
		JS::RootedObject rootvalForSetObj(cx, rootvalForSet.toObject());
		uint32_t jsArrLength;
		JS::GetArrayLength(cx,rootvalForSetObj,&jsArrLength);
		DataType nativeData;
		for(size_t i = 0; i < jsArrLength; ++i) {
			JS::RootedValue rval(cx);
			JS_GetElement(cx,rootvalForSetObj,i,&rval);
			nativeData.insert(ReturnValueWrapper<Key>::fromValue(cx, rval));
		}
		return nativeData;
	}
};
// Map
template <typename Key, typename T, typename Compare, typename Allocator> struct ReturnValueWrapper<std::map<Key,T,Compare,Allocator>> {
	typedef std::map<Key,T,Compare,Allocator> DataType;
	typedef DataType::const_iterator const_iterator;
	static void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const DataType& value) {
		JS::RootedObject robj(cx, JS::NewMapObject(cx));
		for(const_iterator it = std::begin(value); it != std::end(value); ++it) {
			JS::RootedValue rootval1(cx);
			ReturnValueWrapper<Key>::setReturnValue(cx, rootval1, it.first);
			JS::RootedValue rootval2(cx);
			ReturnValueWrapper<T>::setReturnValue(cx, rootval2, it.second);
			JS::MapSet(cx,robj,rootval1,rootval2);
		}
		rval.setObjectOrNull(robj);
	}
	static DataType fromValue(JSContext* cx, const HandleValue& arg) {
		bool isMap = false;
		JS::RootedObject jsMap(cx, arg.toObject());
		JS::IsMapObject(cx, jsMap, &isMap);
		if(!jsMap) throw std::runtime_error("Argument is not a map!");
		DataType nativeData;
		JS::RootedValue keys(cx);
		JS::RootedValue value(cx);
		if (!JS::MapKeys(cx, jsMap, &keys)) {
			throw std::runtime_error("Failed to get keys from the map");
		}
		if (!JS::MapValues(cx, jsMap, &value)) {
			throw std::runtime_error("Failed to get values from the map");
		}
		JS::RootedObject keysObj(cx, keys.toObject());
		uint32_t mapSize = JS::MapSize(cx, jsMap);
		for(uint32_t i = 0; i < mapSize; ++i) {
			JS::RootedValue key(cx);
			JS::RootedValue value(cx);
			JS_GetElement(cx,keysObj,i,&key);
			JS::MapGet(cx,jsMap,key,&value);
			Key nativeKey = ReturnValueWrapper<Key>::fromValue(cx,key);
			T nativeValue = ReturnValueWrapper<T>::fromValue(cx,value);
			nativeData.emplace(std::move(nativeKey),std::move(nativeValue));
		}
		return nativeData;
	}
};

template <typename T> void setReturnValue(JSContext* cx, JS::MutableHandleValue rval, const T& value) {
	ReturnValueWrapper<T>::setReturnValue(cx, rval, value);
}
template <typename T> void fromValue(JSContext* cx, const HandleValue& arg) {
	return ReturnValueWrapper<T>::fromValue(cx, arg);
}
template <typename T> void exactArg(JSContext* cx, const JS::CallArgs& args, unsigned index) {
	return ReturnValueWrapper<T>::fromValue(cx, args.get(index));
}

// Helper function to unpack JS::CallArgs into a std::tuple
template <typename Tuple, std::size_t... I>
bool unpackArgumentsImpl(JSContext* cx, const JS::CallArgs& args, Tuple& tuple, std::index_sequence<I...>) {
	return (fromValue(cx, args[I], std::get<I>(tuple)) && ...);
}

template <typename... Args>
std::tuple<Args...> getNativeArguments(JSContext* cx, const JS::CallArgs& args) {
	return getNativeArgumentsImpl<Args...>(cx, args, std::index_sequence_for<Args...>{});
}

template <typename... Args, std::size_t... Is>
std::tuple<Args...> getNativeArgumentsImpl(JSContext* cx, const JS::CallArgs& args, std::index_sequence<Is...>) {
	return std::make_tuple(ReturnValueWrapper<Args>().fromValue(cx, args.get(Is))...);
}

// Helper function to convert C++ arguments to JS::Value
template <typename T, typename... Args>
std::enable_if_t<!std::is_void_v<T>, void> setJSValues(JSContext* cx, JS::RootedValueVector& valarr, T firstArg, Args... restArgs) {
	setReturnValue(cx, valarr[sizeof...(Args)], firstArg);
	if constexpr (sizeof...(Args) > 0) {
		setJSValues(cx, valarr, restArgs...);
	}
}

// Specialization for empty argument list
template <typename T>
std::enable_if_t<std::is_void_v<T>, void> setJSValues(JSContext* cx, JS::RootedValueVector&) {}

// Template for wrapping free function calls
template <typename Ret, typename... Args>
std::enable_if_t<!std::is_void_v<Ret>, bool> jsFunctionWrapper(JSContext* cx, unsigned argc, JS::Value* vp, Ret(*func)(Args...)) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	// Ensure the argument count matches
	if (argc != sizeof...(Args)) {
		JS_ReportErrorASCII(cx, "Wrong number of arguments");
		return false;
	}

	try {
		std::tuple<Args...> nativeArgs = getNativeArguments<Args...>(cx, args);
		Ret result = std::apply(func, nativeArgs);
		setReturnValue(cx, args.rval(), result);
		return true;
	} catch (const std::exception& e) {
		JS_ReportErrorASCII(cx, "Exception: %s", e.what());
		return false;
	}
}
// Template for wrapping free function calls
template <typename Ret, typename... Args>
std::enable_if_t<std::is_void_v<Ret>, bool> jsFunctionWrapper(JSContext* cx, unsigned argc, JS::Value* vp, Ret(*func)(Args...)) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	// Ensure the argument count matches
	if (argc != sizeof...(Args)) {
		JS_ReportErrorASCII(cx, "Wrong number of arguments");
		return false;
	}

	try {
		std::tuple<Args...> nativeArgs = getNativeArguments<Args...>(cx, args);
		std::apply(func, nativeArgs);
		args.rval().setUndefined();
		return true;
	} catch (const std::exception& e) {
		JS_ReportErrorASCII(cx, "Exception: %s", e.what());
		return false;
	}
}
// Template for wrapping member function calls
template <typename T, typename Ret, typename... Args>
std::enable_if_t<!std::is_void_v<Ret>, bool> jsMemberFunctionWrapper(JSContext* cx, unsigned argc, JS::Value* vp, Ret (T::*Method)(Args...)) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	auto obj = args.thisv().toObjectOrNull();
	auto ptr = getSmartPointerFromObj<T>(obj, 0);

	// Ensure the argument count matches
	if (argc != sizeof...(Args)) {
		JS_ReportErrorASCII(cx, "Wrong number of arguments");
		return false;
	}

	try {
		std::tuple<Args...> nativeArgs = getNativeArguments<Args...>(cx, args);
		//Ret result = std::apply(Method, ptr.get(), nativeArgs);
		//Ret result = callMethod(ptr.get(), Method, nativeArgs);
		Ret result = std::apply([ptr, Method](Args... params) { return (ptr.get()->*Method)(params...); }, nativeArgs);
		setReturnValue(cx, args.rval(), result);
		return true;
	} catch (const std::exception& e) {
		JS_ReportErrorASCII(cx, "Exception: %s", e.what());
		return false;
	}
}
// Template for wrapping member function calls
template <typename T, typename Ret, typename... Args>
std::enable_if_t<std::is_void_v<Ret>, bool> jsMemberFunctionWrapper(JSContext* cx, unsigned argc, JS::Value* vp, Ret (T::*Method)(Args...)) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	auto obj = args.thisv().toObjectOrNull();
	auto ptr = getSmartPointerFromObj<T>(obj, 0);

	// Ensure the argument count matches
	if (argc != sizeof...(Args)) {
		JS_ReportErrorASCII(cx, "Wrong number of arguments");
		return false;
	}

	try {
		std::tuple<Args...> nativeArgs = getNativeArguments<Args...>(cx, args);
		std::apply([ptr, Method](Args... params) { return (ptr.get()->*Method)(params...); }, nativeArgs);
		args.rval().setUndefined();
		return true;
	} catch (const std::exception& e) {
		JS_ReportErrorASCII(cx, "Exception: %s", e.what());
		return false;
	}
}
// Template for wrapping const member function calls
template <typename T, typename Ret, typename... Args>
std::enable_if_t<!std::is_void_v<Ret>, bool> jsMemberFunctionWrapper(JSContext* cx, unsigned argc, JS::Value* vp, Ret (T::*Method)(Args...) const) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	auto obj = args.thisv().toObjectOrNull();
	auto ptr = getSmartPointerFromObj<T>(obj, 0);

	// Ensure the argument count matches
	if (argc != sizeof...(Args)) {
		JS_ReportErrorASCII(cx, "Wrong number of arguments");
		return false;
	}

	try {
		std::tuple<Args...> nativeArgs = getNativeArguments<Args...>(cx, args);
		Ret result = std::apply([ptr, Method](Args... params) { return (ptr.get()->*Method)(params...); }, nativeArgs);
		setReturnValue(cx, args.rval(), result);
		return true;
	} catch (const std::exception& e) {
		JS_ReportErrorASCII(cx, "Exception: %s", e.what());
		return false;
	}
}
// Template for wrapping const member function calls
template <typename T, typename Ret, typename... Args>
std::enable_if_t<std::is_void_v<Ret>, bool> jsMemberFunctionWrapper(JSContext* cx, unsigned argc, JS::Value* vp, Ret (T::*Method)(Args...) const) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	auto obj = args.thisv().toObjectOrNull();
	auto ptr = getSmartPointerFromObj<T>(obj, 0);

	// Ensure the argument count matches
	if (argc != sizeof...(Args)) {
		JS_ReportErrorASCII(cx, "Wrong number of arguments");
		return false;
	}

	try {
		std::tuple<Args...> nativeArgs = getNativeArguments<Args...>(cx, args);
		//Ret result = std::apply(Method, ptr.get(), nativeArgs);
		//Ret result = callMethod(ptr.get(), Method, nativeArgs);
		std::apply([ptr, Method](Args... params) { return (ptr.get()->*Method)(params...); }, nativeArgs);
		args.rval().setUndefined();
		return true;
	} catch (const std::exception& e) {
		JS_ReportErrorASCII(cx, "Exception: %s", e.what());
		return false;
	}
}

template <typename... Args>
void invokeJSCallback(JSContext* cx, JS::HandleObject jsObject, const char* propertyName, Args... args) {
	// Ensure the JS object is valid
	if (!jsObject) {
		throw std::runtime_error("JS object is null");
	}

	// Check if the property exists
	bool hasProperty;
	if (!JS_HasProperty(cx, jsObject, propertyName, &hasProperty)) {
		throw std::runtime_error("Failed to check property");
	}

	if (!hasProperty) return;

	// Get the property (assumed to be a function)
	JS::RootedValue rval(cx);
	if (!JS_GetProperty(cx, jsObject, propertyName, &rval)) {
		throw std::runtime_error("Failed to get property");
	}

	if (!rval.isObject() || !JS_ObjectIsFunction(rval.toObjectOrNull())) {
		return; // Not a callable function, return early
	}

	JS::RootedFunction rootedFunc(cx, JS_ValueToFunction(cx, rval));

	// Convert the arguments to JS::Value
	JS::RootedValueVector valarr(cx);
	valarr.resize(sizeof...(args));
	setJSValues(cx, valarr, args...);

	// Call the JavaScript function
	JS::RootedValue rva2l(cx);
	if (!JS_CallFunction(cx, jsObject, rootedFunc, valarr, &rva2l)) {
		throw std::runtime_error("Failed to call function");
	}
}

template <typename... SignalArgs>
void connectSignalToJS(JSContext* cx, JS::PersistentRootedObject jsObject, const char* propertyName, sigslot::signal<SignalArgs...>& signal) {
	signal.connect([cx, jsObject, propertyName](SignalArgs... args) {
		try {
			invokeJSCallback(cx, jsObject, propertyName, args...);
		} catch (const std::exception& e) {
			JS_ReportErrorASCII(cx, "Exception in signal handler: %s", e.what());
		}
	});
};


}

#endif // JSUTIL_MH33_HPP
