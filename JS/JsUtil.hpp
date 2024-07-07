#ifndef JSUTIL_MH33_HPP
#define JSUTIL_MH33_HPP
#include <jsapi.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>

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

intptr_t storeSharedVoidPtr(const std::shared_ptr<void>& ptr);
intptr_t storeSharedVoidPtr(std::shared_ptr<void>&& ptr);
intptr_t storeSharedVoidPtrToObj(JSObject* obj, uint32_t index, const std::shared_ptr<void>& ptr);
intptr_t storeSharedVoidPtrToObj(JSObject* obj, uint32_t index, std::shared_ptr<void>&& ptr);
std::shared_ptr<void> getSharedVoidPtr(intptr_t id);
std::shared_ptr<void> getSharedVoidPtrFromObj(JSObject* obj, size_t slot);
void removeSmartPointer(intptr_t id);
template<typename T> intptr_t storeSmartPointer(const std::shared_ptr<T>& ptr) {
	return storeSharedVoidPtr(ptr);
}
template<typename T> intptr_t storeSmartPointer(std::shared_ptr<T>&& ptr) {
	return storeSharedVoidPtr(std::move(ptr));
}
template<typename T> intptr_t storeSmartPointerToObj(JSObject* obj, uint32_t index, const std::shared_ptr<T>& ptr) {
	return storeSharedVoidPtrToObj(obj,index,ptr);
}
template<typename T> intptr_t storeSmartPointerToObj(JSObject* obj, uint32_t index, std::shared_ptr<T>&& ptr) {
	return storeSharedVoidPtrToObj(obj,index,std::move(ptr));
}

template<typename T> std::shared_ptr<T> getSmartPointer(intptr_t id) {
	return std::static_pointer_cast<T>(getSharedVoidPtr(id));
}
template<typename T> std::shared_ptr<T> getSmartPointerFromObj(JSObject* obj, size_t slot) {
	return std::static_pointer_cast<T>(getSharedVoidPtrFromObj(obj,slot));
}
void js_finalizer_shared(JS::GCContext* cx, JSObject* obj);

}

#endif // JSUTIL_MH33_HPP
