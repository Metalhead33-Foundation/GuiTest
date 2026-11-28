#include "JsSystem.hpp"
#include <ScriptHost.hpp>
#include <GuiScene.hpp>
#include <Lotte/Gui/LteThemeDefinition.hpp>
#include <js/Array.h>
#include <js/Object.h>
#include <cstdio>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <utility>

namespace JS {

namespace {

/// @brief The host the registered functions talk to.
///
/// A file static, not a hidden parameter, because SpiderMonkey has no user-data
/// slot on a native function and `this` is the module object. The sandbox runs
/// one script context at a time (`Core::CONTEXT_MAP` makes the same assumption),
/// and the host outlives that context, so a single pointer is enough here.
Sandbox::ScriptHost* gHost = nullptr;

/// @brief The `System.Texture` wrapper class.
ClassCreator TextureClass;

/// @brief Reads a numeric property, falling back when it is missing or not a number.
double numberProperty(JSContext* cx, JS::HandleObject object, const char* name, double fallback)
{
	JS::RootedValue value(cx);
	if (!JS_GetProperty(cx, object, name, &value) || !value.isNumber()) {
		return fallback;
	}
	return value.toNumber();
}

/// @brief The shared texture a wrapper object holds.
std::shared_ptr<Sandbox::ScriptTexture> textureFromThis(JS::CallArgs& args)
{
	auto holder = getNativePointer<std::shared_ptr<Sandbox::ScriptTexture>>(
		args.thisv().toObjectOrNull());
	if (!holder || !*holder) {
		throw std::runtime_error("System.Texture: the texture is not valid");
	}
	return *holder;
}

/// @brief `new System.Texture(path)`.
bool js_texture_constructor(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Sandbox::ScriptHost* host = gHost;
		if (host == nullptr || host->gui == nullptr) {
			throw std::runtime_error("System.Texture: graphics is not available");
		}
		if (argc < 1) {
			throw std::runtime_error("System.Texture: expected an image path");
		}
		const std::string path = JsToNativeString(cx, args.get(0).toString());
		auto texture = host->gui->loadTexture(host->io, path);
		JS::RootedObject object(cx,
			JS_NewObjectWithGivenProto(cx, &TextureClass.protoClass, *TextureClass.prototype));
		setNativePointer(object, new std::shared_ptr<Sandbox::ScriptTexture>(std::move(texture)));
		args.rval().setObject(*object);
		return true;
	}, cx, argc, vp);
}

/// @brief Finalizer of `System.Texture`: drops the shared texture.
void js_texture_finalizer(JS::GCContext* gcx, JSObject* obj)
{
	finalizeNative<std::shared_ptr<Sandbox::ScriptTexture>>(gcx, obj);
}

/// @brief `texture.path`.
bool js_texture_get_path(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), textureFromThis(args)->path());
		return true;
	}, cx, argc, vp);
}

/// @brief `texture.width`.
bool js_texture_get_width(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), textureFromThis(args)->width());
		return true;
	}, cx, argc, vp);
}

/// @brief `texture.height`.
bool js_texture_get_height(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), textureFromThis(args)->height());
		return true;
	}, cx, argc, vp);
}

/// @brief `System.loadTexture(path)`: the same object `new System.Texture(path)` builds.
bool js_system_load_texture(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return js_texture_constructor(cx, argc, vp);
}

/// @brief `System.setRichText(mml, options)`: hands a layout request to the host.
bool js_system_set_rich_text(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Sandbox::ScriptHost* host = gHost;
		if (host == nullptr || !host->setRichText) {
			throw std::runtime_error("System.setRichText: text is not available");
		}
		if (argc < 1) {
			throw std::runtime_error("System.setRichText: expected an MML string");
		}
		Sandbox::RichTextRequest request;
		request.mml = JsToNativeString(cx, args.get(0).toString());
		if (argc >= 2 && args.get(1).isObject()) {
			JS::RootedObject options(cx, &args.get(1).toObject());
			request.origin.x = static_cast<float>(numberProperty(cx, options, "x", 0.0));
			request.origin.y = static_cast<float>(numberProperty(cx, options, "y", 0.0));
			request.spacing = static_cast<float>(numberProperty(cx, options, "spacing", 0.0));
			request.wordSpacing =
				static_cast<float>(numberProperty(cx, options, "wordSpacing", 0.0));
			request.scale = static_cast<float>(numberProperty(cx, options, "scale", 1.0));
		}
		host->setRichText(request);
		args.rval().setUndefined();
		return true;
	}, cx, argc, vp);
}

/// @brief Formats a linear colour as `#RRGGBBAA`, the shape the GUI wrappers accept.
std::string colourToHex(const glm::fvec4& colour)
{
	const auto byte = [](float value) {
		const float clamped = glm::clamp(value, 0.0f, 1.0f);
		return static_cast<unsigned>(clamped * 255.0f + 0.5f);
	};
	char text[10] = {};
	std::snprintf(text, sizeof(text), "#%02x%02x%02x%02x", byte(colour.r), byte(colour.g),
				  byte(colour.b), byte(colour.a));
	return text;
}

/// @brief Publishes text attributes as a plain JavaScript object.
void defineTextStyle(JSContext* cx, JS::HandleObject target,
					 const Lotte::Text::TextAttributes& attributes)
{
	const std::string colour = colourToHex(attributes.colour);
	JS::RootedString face(cx, JS_NewStringCopyUTF8N(cx,
		JS::UTF8Chars(attributes.familyName.data(), attributes.familyName.size())));
	JS::RootedString colourString(cx, JS_NewStringCopyUTF8N(cx,
		JS::UTF8Chars(colour.data(), colour.size())));
	JS_DefineProperty(cx, target, "face", face, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, target, "colour", colourString, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, target, "size", double(attributes.size), JSPROP_ENUMERATE);
	JS_DefineProperty(cx, target, "weight",
		double(static_cast<unsigned>(attributes.effectiveWeight())), JSPROP_ENUMERATE);
	JS_DefineProperty(cx, target, "italic", attributes.italic, JSPROP_ENUMERATE);
	JS_DefineProperty(cx, target, "underline", attributes.underline, JSPROP_ENUMERATE);
}

/// @brief `System.textStyle(target)`: the theme's styling for a role or an element.
bool js_system_text_style(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Sandbox::ScriptHost* host = gHost;
		if (host == nullptr || host->theme == nullptr) {
			throw std::runtime_error("System.textStyle: no theme is loaded");
		}
		if (argc < 1 || !args.get(0).isString()) {
			throw std::runtime_error("System.textStyle: expected a role or element name");
		}
		const std::string name = JsToNativeString(cx, args.get(0).toString());
		Lotte::Text::TextAttributes attributes;
		Lotte::Gui::ThemeElement element = Lotte::Gui::ThemeElement::Count;
		Lotte::Gui::ThemeTextRole role = Lotte::Gui::ThemeTextRole::Count;
		std::optional<Lotte::Text::TextAttributes> elementStyle;
		if (Lotte::Gui::parseThemeElement(name, element)) {
			// An element's own `text` block wins; a theme that only styles roles still
			// answers with the label role, which keeps a caption readable.
			elementStyle = host->theme->elementTextStyle(element, 0u);
			attributes = elementStyle.has_value()
				? *elementStyle
				: host->theme->textStyle(Lotte::Gui::ThemeTextRole::Label, 0u);
		} else if (Lotte::Gui::parseThemeTextRole(name, role)) {
			attributes = host->theme->textStyle(role, 0u);
		} else {
			throw std::runtime_error("System.textStyle: unknown role or element '" + name + "'");
		}
		JS::RootedObject object(cx, JS_NewPlainObject(cx));
		defineTextStyle(cx, object, attributes);
		args.rval().setObject(*object);
		return true;
	}, cx, argc, vp);
}

/// @brief `System.quit()`: asks the application to leave its main loop.
bool js_system_quit(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		if (gHost != nullptr && gHost->requestQuit) {
			gHost->requestQuit();
		}
		args.rval().setUndefined();
		return true;
	}, cx, argc, vp);
}

/// @brief `System.setConfig(section, key, value)`: writes one number into the ini model.
bool js_system_set_config(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Sandbox::ScriptHost* host = gHost;
		if (host == nullptr || host->config == nullptr) {
			throw std::runtime_error("System.setConfig: the configuration is not writable");
		}
		if (argc < 3 || !args.get(0).isString() || !args.get(1).isString() ||
			!args.get(2).isNumber()) {
			throw std::runtime_error(
				"System.setConfig: expected (section, key, number)");
		}
		const std::string section = JsToNativeString(cx, args.get(0).toString());
		const std::string key = JsToNativeString(cx, args.get(1).toString());
		const double value = args.get(2).toNumber();
		// Numeric values are written as floats: that is how the volume keys are
		// declared, and `ConfigurationSection::setFloat` replaces the value in place
		// instead of adding a second entry with a different type.
		host->config->getSections()[section].setFloat(key, float(value));
		args.rval().setUndefined();
		return true;
	}, cx, argc, vp);
}

} // namespace

const ClassCreator* SystemTextureClass()
{
	return TextureClass.prototype ? &TextureClass : nullptr;
}

std::shared_ptr<Sandbox::ScriptTexture> textureFromValue(JSContext* cx, JS::HandleValue value)
{
	if (!value.isObject() || !TextureClass.prototype) {
		return nullptr;
	}
	JS::RootedObject object(cx, &value.toObject());
	if (!JS_InstanceOf(cx, object, &TextureClass.protoClass, nullptr)) {
		// `JS_InstanceOf` reports a pending exception; the caller treats a missing
		// texture as "no texture", so the report is cleared here instead of
		// aborting the constructor.
		JS_ClearPendingException(cx);
		return nullptr;
	}
	auto holder = getNativePointer<std::shared_ptr<Sandbox::ScriptTexture>>(object);
	return holder != nullptr ? *holder : nullptr;
}

void RegisterSystemClasses(JSContext* cx, const JS::HandleObject& global, Sandbox::ScriptHost& host)
{
	gHost = &host;

	std::memset(&TextureClass.protoClass, 0, sizeof(JSClass));
	std::memset(&TextureClass.ops, 0, sizeof(JSClassOps));
	TextureClass.name = "Texture";
	TextureClass.nargs = 1;
	TextureClass.constructor = js_texture_constructor;
	TextureClass.ops.construct = js_texture_constructor;
	TextureClass.ops.finalize = js_texture_finalizer;
	TextureClass.protoClass.cOps = &TextureClass.ops;
	TextureClass.protoClass.name = "Texture";
	// No JSCLASS_BACKGROUND_FINALIZE: the finalizer releases a Kaldi-backed
	// resource, which must happen on the thread that owns the resource factory.
	TextureClass.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1);
	TextureClass.prototype = std::make_unique<JS::RootedObject>(cx,
		JS_NewObject(cx, &TextureClass.protoClass));
	TextureClass.ps.push_back(JS_PSG("path", js_texture_get_path, JSPROP_ENUMERATE));
	TextureClass.ps.push_back(JS_PSG("width", js_texture_get_width, JSPROP_ENUMERATE));
	TextureClass.ps.push_back(JS_PSG("height", js_texture_get_height, JSPROP_ENUMERATE));
	TextureClass.ps.push_back(JS_PS_END);
	TextureClass.static_ps.push_back(JS_PS_END);
	TextureClass.fs.push_back(JS_FS_END);
	TextureClass.static_fs.push_back(JS_FS_END);
	TextureClass.registerToJS(cx, global, *TextureClass.prototype);

	JS_DefineFunction(cx, global, "loadTexture", js_system_load_texture, 1, JSPROP_ENUMERATE);
	JS_DefineFunction(cx, global, "setRichText", js_system_set_rich_text, 2, JSPROP_ENUMERATE);
	JS_DefineFunction(cx, global, "textStyle", js_system_text_style, 1, JSPROP_ENUMERATE);
	JS_DefineFunction(cx, global, "quit", js_system_quit, 0, JSPROP_ENUMERATE);
	JS_DefineFunction(cx, global, "setConfig", js_system_set_config, 3, JSPROP_ENUMERATE);

	JS::RootedObject viewport(cx, JS_NewPlainObject(cx));
	JS_DefineProperty(cx, viewport, "width", double(host.viewport.x),
		JSPROP_READONLY | JSPROP_ENUMERATE);
	JS_DefineProperty(cx, viewport, "height", double(host.viewport.y),
		JSPROP_READONLY | JSPROP_ENUMERATE);
	JS_DefineProperty(cx, global, "viewport", viewport, JSPROP_READONLY | JSPROP_ENUMERATE);
}

} // namespace JS
