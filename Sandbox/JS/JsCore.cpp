#include "JsCore.hpp"
#include "JsUtil.hpp"
#include "JsException.hpp"
#include <iostream>
#include <js/Array.h>
#include <js/MapAndSet.h>
#include <JS/Wrappers/Util/JsRandom.hpp>
#include <js/JSON.h>
#include <js/Conversions.h>
#include <js/Symbol.h>

namespace JS {
void Core::run(bool module)
{
	// Reuse the global and the realm `initialize()` already created instead of
	// making a second one: the pre-registered modules (and the classes they define)
	// live in that realm, and a wrapper object created against a prototype from
	// another compartment would be a cross-compartment wrapper, whose reserved
	// slots cannot hold the native pointer.
	if (!global || !*global) {
		return;
	}
	JSAutoRealm ar(cx.get(), global->get());
	registerGlobals(*global);
	if(module) {
		runToplevelModule("main.js");
	}
	else compileAndExecuteScript(nullptr, "main.js");
}

std::string Core::GetJsFileAsString(JS::CallArgs& args) const {
	std::string str0 = JsToNativeString(cx.get(), args.get(0).toString());
	return getCodeFromJsFile(str0,true);
}

void Core::insertModule(const std::string& moduleName, const ModuleCreator& creator, CreatedObjectType objectType, size_t arrSize)
{
	switch (objectType) {
		case JS::CreatedObjectType::PLAIN_OBJECT: {
			JS::RootedObject moduleObject(cx.get(),JS_NewPlainObject(cx.get()));
			JS_DefineProperty(cx.get(),moduleObject,"__esModule",JS::TrueHandleValue, JSPROP_READONLY);
			creator(*cx,moduleObject);
			moduleRegistry.emplace(moduleName, JS::PersistentRootedObject(cx.get(), moduleObject));
			break; }
		case JS::CreatedObjectType::ARRAY: {
			JS::RootedObject moduleObject(cx.get(),JS::NewArrayObject(cx.get(), arrSize));
			creator(*cx,moduleObject);
			moduleRegistry.emplace(moduleName, JS::PersistentRootedObject(cx.get(), moduleObject));
			break; }
		case JS::CreatedObjectType::MAP: {
			JS::RootedObject moduleObject(cx.get(),JS::NewMapObject(cx.get()));
			creator(*cx,moduleObject);
			moduleRegistry.emplace(moduleName, JS::PersistentRootedObject(cx.get(), moduleObject));
			break; }
		case JS::CreatedObjectType::SET: {
			JS::RootedObject moduleObject(cx.get(),JS::NewSetObject(cx.get()));
			creator(*cx,moduleObject);
			moduleRegistry.emplace(moduleName, JS::PersistentRootedObject(cx.get(), moduleObject));
			break; }
	}
}

void Core::insertModule(const std::string& moduleName, const JSClass& protoClass, const ModuleCreator& creator)
{
	JS::RootedObject moduleObject(cx.get(),JS_NewObject(cx.get(), &protoClass));
	JS_DefineProperty(cx.get(),moduleObject,"__esModule",JS::TrueHandleValue, JSPROP_READONLY);
	creator(*cx,moduleObject);
	moduleRegistry.emplace(moduleName, JS::PersistentRootedObject(cx.get(), moduleObject));
}

void Core::insertModule(const std::string& moduleName, const JSClass& protoClass, JS::Handle<JSObject*> proto, const ModuleCreator& creator)
{
	JS::RootedObject moduleObject(cx.get(),JS_NewObjectWithGivenProto(cx.get(), &protoClass, proto));
	JS_DefineProperty(cx.get(),moduleObject,"__esModule",JS::TrueHandleValue, JSPROP_READONLY);
	creator(*cx,moduleObject);
	moduleRegistry.emplace(moduleName, JS::PersistentRootedObject(cx.get(), moduleObject));
}

void Core::insertModule(const std::string& moduleName, JSNative call, unsigned nargs, unsigned flags)
{
	JS::RootedFunction fun(cx.get(), JS_NewFunction(cx.get(), call, nargs, flags, moduleName.c_str()));
	moduleRegistry.emplace(moduleName, JS::PersistentRootedObject(cx.get(), JS_GetFunctionObject(fun) ) );
}

void Core::executeWithinContext(const ContextAccessor& accessor)
{
	accessor(*cx);
}

// Callback used by JS_Stringify to append UTF-16 chunks to a std::string
static bool StringifyCallback(const char16_t* buf, uint32_t len, void* data) {
	auto* out = static_cast<std::string*>(data);
	// Assuming your JsToNativeString converts UTF-16 (char16_t*) to UTF-8 std::string.
	// Replace this loop with your UTF-16 -> UTF-8 converter (e.g., JS::DeflateStringToUTF8Buffer).
	for (uint32_t i = 0; i < len; ++i) {
		out->push_back(static_cast<char>(buf[i]));
	}
	return true;
}

static bool JsPrint(JSContext* cx, unsigned argc, JS::Value* vp) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	// Rooted variables protect GC things if JS_Stringify or JS::ToString triggers a GC
	JS::RootedValue arg(cx);
	JS::RootedString str(cx);
	JS::RootedValue space(cx, JS::Int32Value(2)); // 2-space indentation like Node.js

	for (unsigned i = 0; i < args.length(); ++i) {
		if (i > 0) {
			std::cout << " ";
		}

		arg = args[i];

		switch (arg.type()) {
			case JS::ValueType::Undefined:
				std::cout << "undefined";
				break;

			case JS::ValueType::Null:
				std::cout << "null";
				break;

			case JS::ValueType::Boolean:
				std::cout << (arg.toBoolean() ? "true" : "false");
				break;

			case JS::ValueType::Int32:
				std::cout << arg.toInt32();
				break;

			case JS::ValueType::Double:
				std::cout << arg.toDouble();
				break;

			case JS::ValueType::BigInt:
				str = JS::ToString(cx, arg);
				if (!str) return false;
				std::cout << JsToNativeString(cx, str) << "n";
				break;

			case JS::ValueType::Symbol: {
				JS::RootedSymbol sym(cx, arg.toSymbol());
				str = JS::GetSymbolDescription(cx, sym);
				if (str) {
					std::cout << "Symbol(" << JsToNativeString(cx, str) << ")";
				} else {
					// GetSymbolDescription returns nullptr for Symbol() with no description
					std::cout << "Symbol()";
				}
				break;
			}

			case JS::ValueType::String:
				// Top-level strings print raw without quotes (matching Node/browser behavior)
				std::cout << JsToNativeString(cx, arg.toString());
				break;

			case JS::ValueType::Object: {
				std::string jsonOut;
				// Attempt pretty-printed JSON serialization
				if (JS_Stringify(cx, &arg, nullptr, space, StringifyCallback, &jsonOut)) {
					// JS_Stringify succeeds but leaves jsonOut empty for functions or undefined-only objects
					if (!jsonOut.empty()) {
						std::cout << jsonOut;
						break;
					}
				} else {
					// Clear pending exception (e.g. TypeError from circular references) before falling back
					JS_ClearPendingException(cx);
				}

				// Fallback for functions, circular objects, Maps/Sets, etc. (prints "[object Object]" or function source)
				str = JS::ToString(cx, arg);
				if (!str) return false;
				std::cout << JsToNativeString(cx, str);
				break;
			}

			default:
				std::cout << "[Unprintable]";
				break;
		}
	}

	std::cout << std::endl;
	args.rval().setUndefined();
	return true;
}

bool Core::JsDofile(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([] (JSContext* cx, unsigned argc, JS::Value* vp) {
		auto heart = CONTEXT_MAP[cx];
		JS::CompileOptions options(cx);
		options.setForceFullParse();
		options.setForceStrictMode();
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto str0 = JsToNativeString(cx, args.get(0).toString());
		options.setFileAndLine(str0.c_str(), 1);
		std::string code = heart->GetJsFileAsString(args);
		JS::RootedValue rval(cx);
		heart->compileAndExecuteScript(code, &rval, str0);
		args.rval().set(rval);
		return true;
	}, cx, argc, vp);
}
bool Core::JsRequire(JSContext* cx, unsigned argc, JS::Value* vp) {
	return executeJSNative([] (JSContext* cx, unsigned argc, JS::Value* vp) {
		auto heart = CONTEXT_MAP[cx];
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		std::string str0 = JsToNativeString(cx, args.get(0).toString());
		JS::RootedValue rval(cx);
		heart->compileAndExecuteCommonJSModule(rval, str0);
		args.rval().set(rval);
		return true;
	}, cx, argc, vp);
}

void reportError(JSContext* cx, const char* message, JSErrorReport* report) {
	throw new Exception(cx,message,report);
}

void Core::initialize()
{
	global = std::make_unique<JS::RootedObject>(cx.get(),createGlobal());
	realm = std::make_unique<JSAutoRealm>(cx.get(),global->get());
	//JS::SetModuleResolveHook(JS_GetRuntime(cx.get()), JsResolveModuleHook);
	JS_SetErrorInterceptorCallback(JS_GetRuntime(cx.get()), &interceptor);
	insertModule("dofile", JsDofile, 1, 0);
	insertModule("print", JsPrint, 1, 0);
	insertModule("console", [](JSContext& ctx,JS::RootedObject& obj) {
		JS_DefineFunction(&ctx, obj, "log", JsPrint, 1, 0);
	});

	insertModule("random", [](JSContext& ctx,JS::RootedObject& obj) {
		CreateRandomIzerClass(&ctx,obj);
	});
	/*
	//
	//JS::RootedObject consoleObject(cx.get(),compileModuleFromSource("export const name = \"console\";","console"));
	JS::RootedObject consoleObject(cx.get(),JS_NewPlainObject(cx.get()));
	//JS_DefineProperty(cx.get(),consoleObject,"__esModule",true,0);
	JS_DefineProperty(cx.get(),consoleObject,"__esModule",JS::TrueHandleValue, JSPROP_READONLY);
	JS_DefineFunction(cx.get(), consoleObject, "log", JsPrint, 1, 0);
	//JS::RootedObject randomObj(cx.get(),compileModuleFromSource("export const name = \"random\";","random"));
	JS::RootedObject randomObj(cx.get(),JS_NewPlainObject(cx.get()));
	JS_DefineProperty(cx.get(),randomObj,"__esModule",JS::TrueHandleValue, JSPROP_READONLY);
	CreateRandomIzerClass(cx.get(), randomObj);
	JS::RootedObject ioObj(cx.get(),JS_NewPlainObject(cx.get()));
	JS_DefineProperty(cx.get(),ioObj,"__esModule",JS::TrueHandleValue, JSPROP_READONLY);
	CreateIOClasses(cx.get(), ioObj);
	moduleRegistry.emplace("console", JS::PersistentRootedObject(cx.get(), consoleObject));
	moduleRegistry.emplace("random", JS::PersistentRootedObject(cx.get(), randomObj));
	moduleRegistry.emplace("IO", JS::PersistentRootedObject(cx.get(), ioObj));
	*/
}

bool Core::inintializeJs()
{
	return JS_Init();
}

void Core::shutdownJs()
{
	// The wrapper classes keep their prototypes in file-static `ClassCreator` objects,
	// whose destructors run *after* this function returns. Unrooting a `RootedObject`
	// after `JS_ShutDown()` writes into a destroyed runtime, so the roots are dropped
	// here, while it is still alive.
	clearAllClassPrototypes();
	JS_ShutDown();
}
void Core::registerGlobals(const JS::HandleObject &global)
{
	JS_DefineFunction(cx.get(),global,"require",JsRequire, 1, 0);
	JS_DefineProperty(cx.get(),global,"print",moduleRegistry["print"], 0);
	JS_DefineProperty(cx.get(),global,"dofile",moduleRegistry["dofile"], 0);
	JS_DefineProperty(cx.get(),global,"console",moduleRegistry["console"], 0);
}

std::map<JSContext*,Core*> Core::CONTEXT_MAP;

static const std::vector<std::string> js_postfixes = {
	"",
	".js",
	".mjs"
};
std::string Core::getCodeFromJsFile(const std::string &path, bool isRaw) const
{
	std::unique_ptr<Elv::Io::Device> iodev(nullptr);
	if(isRaw) {
		iodev = std::unique_ptr<Elv::Io::Device>(iosys.open(path,Elv::Io::Mode::READ));
	} else {
		std::string path0 = path;
		if(path0.starts_with("./")) {
			path0 = std::string_view(++(++std::begin(path0)), std::end(path0));
		}
		iodev = std::unique_ptr<Elv::Io::Device>(iosys.open(path0,Elv::Io::Mode::READ));
		if(!iodev->isValid()) {
			for(const auto& it : js_postfixes) {
				iodev = std::unique_ptr<Elv::Io::Device>(iosys.open("/scripts/" + path0 + it,Elv::Io::Mode::READ));
				if(iodev->isValid()) break;
			}
		}
	}
	if(!iodev->isValid()) throw std::runtime_error("Error opening file for reading!");
	return iodev->readAllAsString<char>();
}


Core::Core(Elv::Io::System& iosys, bool initSelfHosting)
	: cx(JsContext(JS_NewContext(JS::DefaultHeapMaxBytes),JS_DestroyContext)), iosys(iosys)
{
	CONTEXT_MAP.emplace(cx.get(),this);
	if (initSelfHosting && !JS::InitSelfHostedCode(cx.get())) {
	  throw std::runtime_error("Failed to initialize self-hosting code!");
	}
	initialize();
}

Core::~Core()
{
	auto thisElement = CONTEXT_MAP.find(cx.get());
	if(thisElement != std::end(CONTEXT_MAP)) {
		CONTEXT_MAP.erase(thisElement);
	}
}

static JSClass BoilerplateGlobalClass = { .name = "BoilerplateGlobal", .flags = JSCLASS_GLOBAL_FLAGS, .cOps = &JS::DefaultGlobalClassOps };
JSObject *Core::createGlobal()
{
	JS::RealmOptions options;
	return JS_NewGlobalObject(cx.get(), &BoilerplateGlobalClass, nullptr, JS::FireOnNewGlobalHook, options);
}

bool Core::compileAndExecuteScript(const std::string code, JS::RootedValue *retval, const std::string &scriptName)
{
	JS::CompileOptions options(cx.get());
	options.setFileAndLine(scriptName.c_str(), 1);
	options.setForceFullParse();
	options.setForceStrictMode();
	JS::SourceText<mozilla::Utf8Unit> source;
	if (!source.init(cx.get(), code.c_str(), code.length(), JS::SourceOwnership::Borrowed)) {
		throw std::runtime_error("Code initialization failed!");
	}

	auto script = JS::RootedScript(cx.get(), JS::Compile(cx.get(),options,source));
	if (!script) {
		throw Exception(cx.get());
	}
	JS::MutableHandleValue rv(retval);
	if(retval) {
		if (!JS_ExecuteScript(cx.get(), script, rv)) {
			throw Exception(cx.get());
		}
	} else {
		if (!JS_ExecuteScript(cx.get(), script)) {
			throw Exception(cx.get());
		}
	}
	if(JS_IsExceptionPending(cx.get())) throw Exception(cx.get());
	return true;
}
bool Core::compileAndExecuteScript(const FileReadFunction &codereader, JS::RootedValue *retval, const std::string &scriptName)
{
	return compileAndExecuteScript(codereader(scriptName), retval, scriptName);
}

bool Core::compileAndExecuteScript(JS::RootedValue *retval, const std::string &scriptName)
{
	return compileAndExecuteScript([this](const std::string& filename) { return getCodeFromJsFile(filename); }, retval, scriptName );
}

bool Core::compileAndExecuteCommonJSModule(const std::string code, JS::RootedValue &retval, const std::string &moduleName)
{
	JS::CompileOptions options(cx.get());
	options.setFileAndLine(moduleName.c_str(), 1);
	options.setForceFullParse();
	options.setForceStrictMode();
	options.setModule();

	// Create a new global object for the module execution context.
	JS::RootedObject moduleGlobal(cx.get(), createGlobal());
	JSAutoRealm ar(cx.get(), moduleGlobal);
	registerGlobals(moduleGlobal);
	// Define the exports object.
	JS::RootedObject exportsObj(cx.get(), JS_NewPlainObject(cx.get()));
	JS_DefineProperty(cx.get(), moduleGlobal, "exports", exportsObj, JSPROP_ENUMERATE);

	JS::SourceText<mozilla::Utf8Unit> source;
	if (!source.init(cx.get(), code.c_str(), code.length(), JS::SourceOwnership::Borrowed)) {
		throw std::runtime_error("Code initialization failed!");
	}

	auto script = JS::RootedScript(cx.get(), JS::Compile(cx.get(),options,source));
	if (!script) {
		throw Exception(cx.get());
	}
	if (!JS_ExecuteScript(cx.get(), script)) {
		throw Exception(cx.get());
	}
	if(JS_IsExceptionPending(cx.get())) throw Exception(cx.get());
	retval.setObject(*exportsObj);
	moduleRegistry.emplace(moduleName, JS::PersistentRootedObject(cx.get(), exportsObj));
	return true;
}

bool Core::compileAndExecuteCommonJSModule(const FileReadFunction& codereader, JS::RootedValue &retval, const std::string &moduleName)
{
	auto moduleFind = moduleRegistry.find(moduleName);
	if(moduleFind != std::end(moduleRegistry)) {
		retval.setObject(*moduleFind->second);
		return true;
	}
	return compileAndExecuteCommonJSModule(codereader(moduleName), retval, moduleName);
}

bool Core::compileAndExecuteCommonJSModule(JS::RootedValue &retval, const std::string &moduleName)
{
	return compileAndExecuteCommonJSModule([this](const std::string& filename) { return getCodeFromJsFile(filename); }, retval, moduleName );
}

JSObject *Core::compileModuleFromSource(const std::string code, const std::string &moduleName)
{
	JS::CompileOptions options(cx.get());
	options.setFileAndLine(moduleName.c_str(), 1);
	options.setForceFullParse();
	options.setForceStrictMode();
	options.setModule();

	JS::SourceText<mozilla::Utf8Unit> source;
	if (!source.init(cx.get(), code.c_str(), code.length(), JS::SourceOwnership::Borrowed)) {
		throw std::runtime_error("Code initialization failed!");
	}

	auto mod = JS::CompileModule(cx.get(),options,source);
	if(!mod) {
		throw Exception(cx.get());
	}
	else {
		moduleRegistry.emplace(moduleName, JS::PersistentRootedObject(cx.get(), mod));
		return mod;
	}
}

JSObject *Core::compileModuleFromSource(const FileReadFunction& codereader, const std::string &moduleName)
{
	auto moduleFind = moduleRegistry.find(moduleName);
	if(moduleFind != std::end(moduleRegistry)) {
		return moduleFind->second;
	}
	return compileModuleFromSource(codereader(moduleName), moduleName);
}

JSObject *Core::compileModuleFromSource(const std::string &moduleName)
{
	return compileModuleFromSource([this](const std::string& filename) { return getCodeFromJsFile(filename); }, moduleName );
}

bool Core::runToplevelModule(const std::string code, const std::string &moduleName)
{
	JS::RootedObject rootModule(cx.get(), compileModuleFromSource(code, moduleName));

	if(!JS::ModuleLink(cx.get(),rootModule)) {
		throw Exception(cx.get());
	}
	JS::RootedValue rval(cx.get());
	if(JS::ModuleEvaluate(cx.get(), rootModule, &rval)) {
		if(JS_IsExceptionPending(cx.get())) throw Exception(cx.get());
		return true;
	} else throw Exception(cx.get());
}

bool Core::runToplevelModule(const FileReadFunction &codereader, const std::string &moduleName)
{
	JS::RootedObject rootModule(cx.get(), compileModuleFromSource(codereader, moduleName));

	if(!JS::ModuleLink(cx.get(),rootModule)) {
		throw Exception(cx.get());
	}
	JS::RootedValue rval(cx.get());
	if(JS::ModuleEvaluate(cx.get(), rootModule, &rval)) {
		if(JS_IsExceptionPending(cx.get())) throw Exception(cx.get());
		return true;
	} else throw Exception(cx.get());
}

bool Core::runToplevelModule(const std::string &moduleName)
{
	return runToplevelModule([this](const std::string& filename) { return getCodeFromJsFile(filename); }, moduleName );
}

JSObject *Core::JsResolveModuleHook(JSContext *cx, JS::HandleValue modulePrivate, JS::HandleObject moduleRequest)
{
	(void)modulePrivate;
	auto heart = CONTEXT_MAP[cx];
	JS::Rooted<JSString*> specifierString(cx, JS::GetModuleRequestSpecifier(cx, moduleRequest));
	if (!specifierString) {
		return nullptr;
	}
	std::string specCharsAscii = JsToNativeString(cx, specifierString);
	return heart->compileModuleFromSource(specCharsAscii);
}

void Core::ErrorInterception::interceptError(JSContext *cx, JS::HandleValue error) {
	throw Exception(cx, error);
}
}
