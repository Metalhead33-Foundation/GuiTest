#include "JsRandom.hpp"
#include <JS/JsUtil.hpp>
#include <iostream>
#include <string>
#include <cstring>

namespace JS {
template <typename engine> struct RNG_JS_ADAPTER {
	static std::string name;
	static ClassCreator creator;
	static bool isInitialized;
	static bool js_constructor(JSContext* cx, unsigned argc, JS::Value* vp) {
		return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		JS::RootedObject obj(cx, JS_NewObjectWithGivenProto(cx,&creator.protoClass,*creator.prototype));
		if(argc >= 1) {
			JS_SetReservedSlot(obj, 0, JS::PrivateValue( new engine( args.get(0).toInt32() ) ));
		} else {
			JS_SetReservedSlot(obj, 0, JS::PrivateValue( new engine() ));
		}
		//JS_DefineFunctions(cx,obj,creator.fs.data());
		args.rval().setObject(*obj);
		return true;
		}, cx, argc, vp);
	}
	static bool js_call(JSContext* cx, unsigned argc, JS::Value* vp) {
		return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
			(void)cx;
			JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
			engine* rnd = JS::GetMaybePtrFromReservedSlot<engine>(args.calleev().toObjectOrNull(), 0);
			args.rval().setNumber(rnd->operator()());
			return true;
		}, cx, argc, vp);
	}
	static bool js_call2(JSContext* cx, unsigned argc, JS::Value* vp) {
		return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
			(void)cx;
			JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
			engine* rnd = JS::GetMaybePtrFromReservedSlot<engine>(args.thisv().toObjectOrNull(), 0);
			args.rval().setNumber(rnd->operator()());
			return true;
		}, cx, argc, vp);
	}
	static bool js_discard(JSContext* cx, unsigned argc, JS::Value* vp) {
		return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
			(void)cx;
			JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
			engine* rnd = JS::GetMaybePtrFromReservedSlot<engine>(args.thisv().toObjectOrNull(), 0);
			if(argc >= 1) {
				rnd->discard( args.get(0).toInt32() );
			} else {
				rnd->discard(1);
			}
			args.rval().setUndefined();
			return true;
		}, cx, argc, vp);
	}
	static bool js_min(JSContext* cx, unsigned argc, JS::Value* vp) {
		return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
			(void)cx;
			JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
			args.rval().setNumber(engine::min());
			return true;
		}, cx, argc, vp);
	}
	static bool js_max(JSContext* cx, unsigned argc, JS::Value* vp) {
		return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp){
			(void)cx;
			JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
			args.rval().setNumber(engine::max());
			return true;
		}, cx, argc, vp);
	}
	static void js_finalizer(JS::GCContext* cx, JSObject* obj) {
		(void)cx;
		if(!obj) return;
		engine* privval = JS::GetMaybePtrFromReservedSlot<engine>(obj, 0);
		if(!privval) return;
		delete privval;
	}
	static JSObject* initialize(JSContext* cx, const JS::HandleObject &global, const std::string& sname) {
		if(!isInitialized) {
		name = sname;
		std::memset(&creator.protoClass, 0, sizeof(JSClass));
		std::memset(&creator.ops, 0, sizeof(JSClassOps));
		creator.ops.finalize = js_finalizer;
		creator.ops.call = js_call;
		creator.ops.construct = js_constructor;
		creator.protoClass.name = name.c_str();
		creator.protoClass.cOps = &creator.ops;
		creator.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_BACKGROUND_FINALIZE;
		creator.constructor = js_constructor;
		creator.fs.push_back(JS_FN("generate",js_call2,0,0));
		creator.fs.push_back(JS_FN("discard",js_discard,0,0));
		creator.fs.push_back(JS_FN("min",js_min,0,0));
		creator.fs.push_back(JS_FN("max",js_max,0,0));
		creator.fs.push_back(JS_FS_END);
		creator.static_fs.push_back(JS_FN("create",js_constructor,0,0));
		creator.static_fs.push_back(JS_FN("min",js_min,0,0));
		creator.static_fs.push_back(JS_FN("max",js_max,0,0));
		creator.static_fs.push_back(JS_FS_END);
		creator.ps.push_back(JS_PS_END);
		creator.static_ps.push_back(JS_PS_END);
		creator.nargs = 0;
		creator.prototype = std::make_unique<JS::RootedObject>(cx, JS_NewObject(cx,&creator.protoClass));
		isInitialized = true;
		}
		auto toReturn = creator.registerToJS(cx, global, *creator.prototype);
		auto reg = JS::RootedValue(cx,JS::ObjectOrNullValue(toReturn) );
		auto rootedReg = JS::RootedObject(cx,toReturn);
		JS_SetProperty(cx, global, name.c_str(), reg );
		return toReturn;
	}
};

template <> std::string RNG_JS_ADAPTER<std::minstd_rand0>::name = "";
template <> ClassCreator RNG_JS_ADAPTER<std::minstd_rand0>::creator = {};
template <> bool RNG_JS_ADAPTER<std::minstd_rand0>::isInitialized = false;
template <> std::string RNG_JS_ADAPTER<std::minstd_rand>::name = "";
template <> ClassCreator RNG_JS_ADAPTER<std::minstd_rand>::creator = {};
template <> bool RNG_JS_ADAPTER<std::minstd_rand>::isInitialized = false;
template <> std::string RNG_JS_ADAPTER<std::mt19937>::name = "";
template <> ClassCreator RNG_JS_ADAPTER<std::mt19937>::creator = {};
template <> bool RNG_JS_ADAPTER<std::mt19937>::isInitialized = false;
template <> std::string RNG_JS_ADAPTER<std::mt19937_64>::name = "";
template <> ClassCreator RNG_JS_ADAPTER<std::mt19937_64>::creator = {};
template <> bool RNG_JS_ADAPTER<std::mt19937_64>::isInitialized = false;
template <> std::string RNG_JS_ADAPTER<std::ranlux24_base>::name = "";
template <> ClassCreator RNG_JS_ADAPTER<std::ranlux24_base>::creator = {};
template <> bool RNG_JS_ADAPTER<std::ranlux24_base>::isInitialized = false;
template <> std::string RNG_JS_ADAPTER<std::ranlux48_base>::name = "";
template <> ClassCreator RNG_JS_ADAPTER<std::ranlux48_base>::creator = {};
template <> bool RNG_JS_ADAPTER<std::ranlux48_base>::isInitialized = false;
template <> std::string RNG_JS_ADAPTER<std::ranlux24>::name = "";
template <> ClassCreator RNG_JS_ADAPTER<std::ranlux24>::creator = {};
template <> bool RNG_JS_ADAPTER<std::ranlux24>::isInitialized = false;
template <> std::string RNG_JS_ADAPTER<std::ranlux48>::name = "";
template <> ClassCreator RNG_JS_ADAPTER<std::ranlux48>::creator = {};
template <> bool RNG_JS_ADAPTER<std::ranlux48>::isInitialized = false;
template <> std::string RNG_JS_ADAPTER<std::knuth_b>::name = "";
template <> ClassCreator RNG_JS_ADAPTER<std::knuth_b>::creator = {};
template <> bool RNG_JS_ADAPTER<std::knuth_b>::isInitialized = false;

bool CreateRandomIzerClass(JSContext* cx, const JS::HandleObject &global) {
	RNG_JS_ADAPTER<std::minstd_rand0>::initialize(cx,global,"minstd_rand0");
	RNG_JS_ADAPTER<std::minstd_rand>::initialize(cx,global,"minstd_rand");
	RNG_JS_ADAPTER<std::mt19937>::initialize(cx,global,"mt19937");
	RNG_JS_ADAPTER<std::mt19937_64>::initialize(cx,global,"mt19937_64");
	RNG_JS_ADAPTER<std::ranlux24_base>::initialize(cx,global,"ranlux24_base");
	RNG_JS_ADAPTER<std::ranlux48_base>::initialize(cx,global,"ranlux48_base");
	RNG_JS_ADAPTER<std::ranlux24>::initialize(cx,global,"ranlux24");
	RNG_JS_ADAPTER<std::ranlux48>::initialize(cx,global,"ranlux48");
	RNG_JS_ADAPTER<std::knuth_b>::initialize(cx,global,"knuth_b");
	return true;
}
}
