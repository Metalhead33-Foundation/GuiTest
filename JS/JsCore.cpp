#include "JsCore.hpp"
#include "JsUtil.hpp"
#include "JsException.hpp"
#include <iostream>
#include <js/Array.h>
#include <js/MapAndSet.h>
#include <JS/Wrappers/Util/JsRandom.hpp>
#include <JS/Wrappers/Io/JsIO.hpp>
#include <JS/Wrappers/Audio/JsAudioPlayer.hpp>
#include <JS/Wrappers/GUI/JsGuiWrappers.hpp>

namespace JS {
void Core::run(bool module)
{
	JS::RootedObject global(cx.get(), createGlobal());
	if (!global) {
	  return;
	}

	JSAutoRealm ar(cx.get(), global);
	registerGlobals(global);
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

static bool JsPrint(JSContext* cx, unsigned argc, JS::Value* vp) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  auto arg0 = args.get(0);
  /*
  Double = JSVAL_TYPE_DOUBLE,
  Int32 = JSVAL_TYPE_INT32,
  Boolean = JSVAL_TYPE_BOOLEAN,
  Undefined = JSVAL_TYPE_UNDEFINED,
  Null = JSVAL_TYPE_NULL,
  Magic = JSVAL_TYPE_MAGIC,
  String = JSVAL_TYPE_STRING,
  Symbol = JSVAL_TYPE_SYMBOL,
  PrivateGCThing = JSVAL_TYPE_PRIVATE_GCTHING,
  BigInt = JSVAL_TYPE_BIGINT,
*/
  switch (arg0.type()) {
  case JS::ValueType::Double: std::cout << arg0.toDouble() << std::endl; break;
  case JS::ValueType::Int32: std::cout << arg0.toInt32() << std::endl; break;
  case JS::ValueType::Boolean: std::cout << (arg0.toBoolean() ? "true" : "false") << std::endl; break;
  case JS::ValueType::String: std::cout << JsToNativeString(cx, arg0.toString()) << std::endl; break;
  default: std::cout << "Unprintable" << std::endl;
  }
  args.rval().setNull();
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
	JS::SetModuleResolveHook(JS_GetRuntime(cx.get()), JsResolveModuleHook);
	JS_SetErrorInterceptorCallback(JS_GetRuntime(cx.get()), &interceptor);
	insertModule("dofile", JsDofile, 1, 0);
	insertModule("print", JsPrint, 1, 0);
	insertModule("console", [](JSContext& ctx,JS::RootedObject& obj) {
		JS_DefineFunction(&ctx, obj, "log", JsPrint, 1, 0);
	});
	insertModule("random", [](JSContext& ctx,JS::RootedObject& obj) {
		CreateRandomIzerClass(&ctx,obj);
	});
	insertModule("IO", [](JSContext& ctx,JS::RootedObject& obj) {
		CreateIOClasses(&ctx,obj);
	});
	insertModule("Audio", [](JSContext& ctx,JS::RootedObject& obj) {
		RegisterAudioPlayerClasses(&ctx,obj);
	});
	insertModule("GUI", [](JSContext& ctx,JS::RootedObject& obj) {
		RegisterGuiClasses(&ctx,obj);
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
const MH33::Io::sSystem &Core::getIosys() const
{
	return iosys;
}

void Core::setIosys(const MH33::Io::sSystem &newIosys)
{
	iosys = newIosys;
}

void Core::setIosys(MH33::Io::sSystem &&newIosys)
{
	iosys = std::move(newIosys);
}

static const std::vector<std::string> js_postfixes = {
	"",
	".js",
	".mjs"
};
std::string Core::getCodeFromJsFile(const std::string &path, bool isRaw) const
{
	std::unique_ptr<MH33::Io::Device> iodev(nullptr);
	if(isRaw) {
		iodev = std::unique_ptr<MH33::Io::Device>(iosys->open(path,MH33::Io::Mode::READ));
	} else {
		std::string path0 = path;
		if(path0.starts_with("./")) {
			path0 = std::string_view(++(++std::begin(path0)), std::end(path0));
		}
		iodev = std::unique_ptr<MH33::Io::Device>(iosys->open(path0,MH33::Io::Mode::READ));
		if(!iodev->isValid()) {
			for(const auto& it : js_postfixes) {
				iodev = std::unique_ptr<MH33::Io::Device>(iosys->open("/scripts/" + path0 + it,MH33::Io::Mode::READ));
				if(iodev->isValid()) break;
			}
		}
	}
	if(!iodev->isValid()) throw std::runtime_error("Error opening file for reading!");
	return iodev->readAllAsString();
}


Core::Core(const MH33::Io::sSystem &iosys, bool initSelfHosting)
	: cx(JsContext(JS_NewContext(JS::DefaultHeapMaxBytes),JS_DestroyContext)), iosys(iosys)
{
	CONTEXT_MAP.emplace(cx.get(),this);
	if (initSelfHosting && !JS::InitSelfHostedCode(cx.get())) {
	  throw std::runtime_error("Failed to initialize self-hosting code!");
	}
	initialize();
}

Core::Core(MH33::Io::sSystem &&iosys, bool initSelfHosting)
	: cx(JsContext(JS_NewContext(JS::DefaultHeapMaxBytes),JS_DestroyContext)), iosys(std::move(iosys))
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
