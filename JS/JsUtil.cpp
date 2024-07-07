#include "JsUtil.hpp"
#include <exception>
#include <unordered_map>
#include <js/Class.h>
#include <js/Proxy.h>
namespace JS {

static std::unordered_map<intptr_t, std::shared_ptr<void>> smartPointerTable;
static std::mutex smartPointerTableMutex;

intptr_t storeSharedVoidPtr(const std::shared_ptr<void>& ptr) {
	std::lock_guard<std::mutex> lock(smartPointerTableMutex);
	intptr_t id = reinterpret_cast<intptr_t>(ptr.get());
	smartPointerTable[id] = ptr;
	return id;
}

intptr_t storeSharedVoidPtrToObj(JSObject* obj, uint32_t index, const std::shared_ptr<void>& ptr)
{
	intptr_t id = storeSharedVoidPtr(ptr);
	JS_SetReservedSlot(obj, index, JS::PrivateValue( reinterpret_cast<void*>(id) ) );
	return id;
}
intptr_t storeSharedVoidPtr(std::shared_ptr<void>&& ptr) {
	std::lock_guard<std::mutex> lock(smartPointerTableMutex);
	intptr_t id = reinterpret_cast<intptr_t>(ptr.get());
	smartPointerTable[id] = std::move(ptr);
	return id;
}

intptr_t storeSharedVoidPtrToObj(JSObject* obj, uint32_t index, std::shared_ptr<void>&& ptr)
{
	intptr_t id = storeSharedVoidPtr(std::move(ptr));
	JS_SetReservedSlot(obj, index, JS::PrivateValue( reinterpret_cast<void*>(id) ) );
	return id;
}
std::shared_ptr<void> getSharedVoidPtr(intptr_t id) {
	std::lock_guard<std::mutex> lock(smartPointerTableMutex);
	auto it = smartPointerTable.find(id);
	if (it != smartPointerTable.end()) {
		return it->second;
	}
	return nullptr;
}

std::shared_ptr<void> getSharedVoidPtrFromObj(JSObject* obj, size_t slot)
{
	intptr_t id = reinterpret_cast<intptr_t>(JS::GetMaybePtrFromReservedSlot<void>(obj,slot));
	return getSharedVoidPtr(id);
}
void removeSmartPointer(intptr_t id) {
	std::lock_guard<std::mutex> lock(smartPointerTableMutex);
	smartPointerTable.erase(id);
}

JSObject* ClassCreator::registerToJS(JSContext* cx, JS::HandleObject obj)
{
	JSObject* toReturn = JS_InitClass(cx,obj,&protoClass,nullptr,name.c_str(),constructor,nargs,ps.data(),fs.data(),static_ps.data(),static_fs.data());
	auto rootedReg = JS::RootedObject(cx,toReturn);
	auto rootedVal = JS::RootedValue(cx,JS::ObjectOrNullValue(toReturn) );
	JS_SetProperty(cx, obj, name.c_str(), rootedVal );
	return toReturn;
}
JSObject* ClassCreator::registerToJS(JSContext* cx, JS::HandleObject obj, JS::HandleObject prototype)
{
	JS_DefineFunctions(cx,prototype,fs.data());
	JS_DefineProperties(cx,prototype,ps.data());
	JSObject* toReturn =  JS_InitClass(cx,obj,&protoClass,prototype,name.c_str(),constructor,nargs,ps.data(),fs.data(),static_ps.data(),static_fs.data());
	auto rootedReg = JS::RootedObject(cx,toReturn);
	auto rootedVal = JS::RootedValue(cx,JS::ObjectOrNullValue(toReturn) );
	JS_SetProperty(cx, obj, name.c_str(), rootedVal );
	return toReturn;
}
bool executeJSNative(const JSNativeFunctionWrapper& fun, JSContext* cx, unsigned argc, JS::Value* vp) {
	try {
		return fun(cx, argc, vp);
	}  catch (std::exception& exp) {
		JS_ReportErrorASCII(cx, "JSNativeFunction: %s", exp.what());
		return false;
	}
}
bool executeJSAddPropertyOp(const JSAddPropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleValue v) {
	try {
		return fun(cx, obj, id, v);
	}  catch (std::exception& exp) {
		JS_ReportErrorASCII(cx, "JSAddPropertyOp: %s", exp.what());
		return false;
	}
}
bool executeJSDeletePropertyOp(const JSDeletePropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::ObjectOpResult& result) {
	try {
		return fun(cx, obj, id, result);
	}  catch (std::exception& exp) {
		JS_ReportErrorASCII(cx, "SDeletePropertyOp: %s", exp.what());
		return false;
	}
}
bool executeJSNewEnumerateOp(const JSNewEnumerateOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::MutableHandleIdVector properties, bool enumerableOnly) {
	try {
		return fun(cx, obj, properties, enumerableOnly);
	}  catch (std::exception& exp) {
		JS_ReportErrorASCII(cx, "JSNewEnumerateOp: %s", exp.what());
		return false;
	}
}
bool executeJSEnumerateOp(const JSEnumerateOpWrapper& fun, JSContext* cx, JS::HandleObject obj) {
	try {
		return fun(cx, obj);
	}  catch (std::exception& exp) {
		JS_ReportErrorASCII(cx, "JSEnumerateOp: %s", exp.what());
		return false;
	}
}
JSString* executeJSFunToStringOp(const JSFunToStringOpWrapper& fun, JSContext* cx, JS::HandleObject obj, bool isToSource) {
	try {
		return fun(cx, obj, isToSource);
	}  catch (std::exception& exp) {
		JS_ReportErrorASCII(cx, "JSFunToStringOp: %s", exp.what());
		return nullptr;
	}
}
bool executeJSResolveOp(const JSResolveOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool* resolvedp) {
	try {
		return fun(cx, obj, id, resolvedp);
	}  catch (std::exception& exp) {
		JS_ReportErrorASCII(cx, "JSResolveOp: %s", exp.what());
		return false;
	}
}
bool executeJSMayResolveOp(const JSMayResolveOpWrapper& fun, const JSAtomState& names, jsid id, JSObject* maybeObj) {
	try {
		return fun(names, id, maybeObj);
	}  catch (std::exception& exp) {
		throw exp;
	}
}
void executeJSFinalizeOp(const JSFinalizeOpWrapper& fun, JS::GCContext* gcx, JSObject* obj) {
	try {
		return fun(gcx, obj);
	}  catch (std::exception& exp) {
		throw exp;
	}
}
void executeJSTraceOp(const JSTraceOpWrapper& fun, JSTracer* trc, JSObject* obj) {
	try {
		return fun(trc, obj);
	}  catch (std::exception& exp) {
		throw exp;
	}
}
size_t executeJSObjectMovedOp(const JSObjectMovedOpWrapper& fun, JSObject* obj, JSObject* old) {
	try {
		return fun(obj, old);
	}  catch (std::exception& exp) {
		throw exp;
	}
}
bool executeLookupPropertyOp(const LookupPropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleObject objp, js::PropertyResult* propp) {
	try {
		return fun(cx, obj, id, objp, propp);
	}  catch (std::exception& exp) {
		JS_ReportErrorASCII(cx, "LookupPropertyOp: %s", exp.what());
		return false;
	}
}
bool executeDefinePropertyOp(const DefinePropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::Handle<JS::PropertyDescriptor> desc, JS::ObjectOpResult& result) {
	try {
		return fun(cx, obj, id, desc, result);
	}  catch (std::exception& exp) {
		JS_ReportErrorASCII(cx, "DefinePropertyOp: %s", exp.what());
		return false;
	}
}
bool executeHasPropertyOp(const HasPropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool* foundp) {
	try {
		return fun(cx, obj, id, foundp);
	}  catch (std::exception& exp) {
		JS_ReportErrorASCII(cx, "HasPropertyOp: %s", exp.what());
		return false;
	}
}
bool executeGetPropertyOp(const GetPropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleValue receiver, JS::HandleId id, JS::MutableHandleValue vp) {
	try {
		return fun(cx, obj, receiver, id, vp);
	}  catch (std::exception& exp) {
		JS_ReportErrorASCII(cx, "GetPropertyOp: %s", exp.what());
		return false;
	}
}
bool executeSetPropertyOp(const SetPropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleValue v, JS::HandleValue receiver, JS::ObjectOpResult& result) {
	try {
		return fun(cx, obj, id, v, receiver, result);
	}  catch (std::exception& exp) {
		JS_ReportErrorASCII(cx, "SetPropertyOp: %s", exp.what());
		return false;
	}
}
bool executeGetOwnPropertyOp(const GetOwnPropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandle<mozilla::Maybe<JS::PropertyDescriptor>> desc) {
	try {
		return fun(cx, obj, id, desc);
	}  catch (std::exception& exp) {
		JS_ReportErrorASCII(cx, "GetOwnPropertyOp: %s", exp.what());
		return false;
	}
}
bool executeDeletePropertyOp(const DeletePropertyOpWrapper& fun, JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::ObjectOpResult& result) {
	try {
		return fun(cx, obj, id, result);
	}  catch (std::exception& exp) {
		JS_ReportErrorASCII(cx, "DeletePropertyOp: %s", exp.what());
		return false;
	}
}

std::string JsToNativeString(JSContext *cx, JSString *jsstring)
{
	JS::GetStringLength(jsstring);
	JS::RootedString rootedJsStr(cx,jsstring);
	auto encoded = JS_EncodeStringToUTF8(cx,rootedJsStr);
	return std::string(encoded.get());
}

void js_finalizer_shared(GCContext* cx, JSObject* obj)
{
	(void)cx;
	if(!obj) return;
	auto privval = reinterpret_cast<intptr_t>(JS::GetMaybePtrFromReservedSlot<void>(obj, 0));
	if(!privval) return;
	removeSmartPointer(privval);
}

}
