#ifndef JSCORE_MH33_HPP
#define JSCORE_MH33_HPP
#include <jsapi.h>
#include <js/CompilationAndEvaluation.h>
#include <js/SourceText.h>
#include <js/Initialization.h>
#include <js/Exception.h>
#include <js/Modules.h>
#include <functional>
#include <memory>
#include <map>
#include <MhLib/IoSys/MhIoSystem.hpp>
namespace  JS {
typedef std::function<bool(JSContext*)> JsTask;
typedef std::function<bool(JSContext*,unsigned,JS::Value*)> JsFun;
typedef std::unique_ptr<JSContext,decltype (&JS_DestroyContext)> JsContext;
typedef std::unique_ptr<JSRuntime,decltype (&JS_DestroyContext)> JsRuntime;
typedef std::function<std::string(const std::string&)> FileReadFunction;
class Core
{
public:
	typedef std::function<void(JSContext&,JS::RootedObject&)> ModuleCreator;
private:
	JsContext cx;
	std::map<std::string, JS::PersistentRootedObject> moduleRegistry;
	MH33::Io::sSystem iosys;
	std::unique_ptr<JS::RootedObject> global;
	std::unique_ptr<JSAutoRealm> realm;
	struct ErrorInterception : public JSErrorInterceptor {
		void interceptError(JSContext* cx, JS::HandleValue error);
	};
	ErrorInterception interceptor;
	static std::map<JSContext*,Core*> CONTEXT_MAP;
	void initialize();
public:
	// Static initializer
	static bool inintializeJs();
	static void shutdownJs();
	// Constructor and destructor
	Core(const MH33::Io::sSystem& iosys, bool initSelfHosting = true);
	Core(MH33::Io::sSystem&& iosys, bool initSelfHosting = true);
	~Core();
	// Internal Stuff
	const MH33::Io::sSystem &getIosys() const;
	void setIosys(const MH33::Io::sSystem &newIosys);
	void setIosys(MH33::Io::sSystem&& newIosys);
	std::string getCodeFromJsFile(const std::string& path, bool isRaw = false) const;
	// JS stuff
	JSObject* createGlobal();
	void registerGlobals(const JS::HandleObject &global);
	bool compileAndExecuteScript(const std::string code, JS::RootedValue* retval = nullptr, const std::string& scriptName = "noname");
	bool compileAndExecuteScript(const FileReadFunction& codereader, JS::RootedValue* retval = nullptr, const std::string& scriptName = "noname");
	bool compileAndExecuteScript(JS::RootedValue* retval = nullptr, const std::string& scriptName = "noname");
	bool compileAndExecuteCommonJSModule(const std::string code, JS::RootedValue& retval, const std::string& moduleName);
	bool compileAndExecuteCommonJSModule(const FileReadFunction& codereader, JS::RootedValue& retval, const std::string& moduleName);
	bool compileAndExecuteCommonJSModule(JS::RootedValue& retval, const std::string& moduleName);
	JSObject* compileModuleFromSource(const std::string code, const std::string& moduleName);
	JSObject* compileModuleFromSource(const FileReadFunction& codereader, const std::string& moduleName);
	JSObject* compileModuleFromSource(const std::string& moduleName);
	bool runToplevelModule(const std::string code, const std::string& moduleName);
	bool runToplevelModule(const FileReadFunction& codereader, const std::string& moduleName);
	bool runToplevelModule(const std::string& moduleName);
	std::string GetJsFileAsString(JS::CallArgs& args) const;
	void insertModule(const std::string& moduleName, const ModuleCreator& creator);
	void insertModule(const std::string& moduleName, const JSClass& protoClass, const ModuleCreator& creator);
	void insertModule(const std::string& moduleName, JSNative call, unsigned nargs, unsigned flags);
	// JSClass
	void run(bool module=false);
	// Static JS stuff
	static JSObject* JsResolveModuleHook(JSContext* cx, JS::HandleValue modulePrivate, JS::HandleObject moduleRequest);
	static bool JsDofile(JSContext* cx, unsigned argc, JS::Value* vp);
	static bool JsRequire(JSContext* cx, unsigned argc, JS::Value* vp);
};
}

#endif // JSCORE_MH33_HPP
