#include "JsLocalization.hpp"
#include <ScriptHost.hpp>
#include <js/Object.h>
#include <stdexcept>
#include <string>

namespace JS {

namespace {

/// @brief The host the registered functions talk to; see `JsSystem.cpp`.
Sandbox::ScriptHost* gHost = nullptr;

/// @brief `get(key)`: the text for a key, or the key itself when it is unknown.
bool js_localization_get(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		if (argc < 1 || !args.get(0).isString()) {
			throw std::runtime_error("Localization.get: expected a key");
		}
		const std::string key = JsToNativeString(cx, args.get(0).toString());
		const Sandbox::ScriptHost* host = gHost;
		// `text()` already answers with the key when the table has no entry, which is
		// the behaviour a missing caption needs: something readable, never nothing.
		const std::string value = host != nullptr && host->localization != nullptr
			? std::string(host->localization->text(key))
			: key;
		setReturnValue(cx, args.rval(), value);
		return true;
	}, cx, argc, vp);
}

} // namespace

void RegisterLocalizationModule(JSContext* cx, const JS::HandleObject& global, Sandbox::ScriptHost& host)
{
	gHost = &host;
	if (host.localization != nullptr) {
		for (const auto& [key, value] : host.localization->all()) {
			JS::RootedString text(cx,
				JS_NewStringCopyUTF8N(cx, JS::UTF8Chars(value.data(), value.size())));
			JS_DefineProperty(cx, global, key.c_str(), text,
				JSPROP_READONLY | JSPROP_ENUMERATE);
		}
	}
	JS_DefineFunction(cx, global, "get", js_localization_get, 1, JSPROP_ENUMERATE);
	// `count` makes an empty table visible from the script, which is how a run without
	// localization files gets noticed instead of silently showing keys everywhere.
	JS_DefineProperty(cx, global, "count",
		double(host.localization != nullptr ? host.localization->size() : 0u),
		JSPROP_READONLY | JSPROP_ENUMERATE);
}

} // namespace JS
