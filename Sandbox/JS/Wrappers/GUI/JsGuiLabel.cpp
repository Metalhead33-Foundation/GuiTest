#include "JsGuiLabel.hpp"
#include <ScriptHost.hpp>
#include <GuiScene.hpp>
#include <Lotte/Gui/LteThemeDefinition.hpp>
#include <Lotte/Text/LteMarkupColour.hpp>
#include <js/Object.h>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <utility>

namespace JS {

namespace {

/// @brief The host the registered functions talk to; see `JsSystem.cpp`.
Sandbox::ScriptHost* gHost = nullptr;

/// @brief The `GUI.Label` wrapper class.
ClassCreator LabelClass;

/// @brief Native half of one label: the widget plus the key it was resolved from.
struct LabelHandle {
	/** @brief The scene-owned label. */
	std::shared_ptr<Sandbox::ScriptLabel> label;
	/** @brief Localization key the text came from, empty for a literal. */
	std::string key;
};

/// @brief Reads a numeric property, falling back when it is missing or not a number.
double numberProperty(JSContext* cx, JS::HandleObject object, const char* name, double fallback)
{
	JS::RootedValue value(cx);
	if (!JS_GetProperty(cx, object, name, &value) || !value.isNumber()) {
		return fallback;
	}
	return value.toNumber();
}

/// @brief Reads a boolean property, falling back when it is missing.
bool boolProperty(JSContext* cx, JS::HandleObject object, const char* name, bool fallback)
{
	JS::RootedValue value(cx);
	if (!JS_GetProperty(cx, object, name, &value) || value.isUndefined()) {
		return fallback;
	}
	return value.toBoolean();
}

/// @brief Reads a string property, falling back when it is missing.
std::optional<std::string> stringProperty(JSContext* cx, JS::HandleObject object, const char* name)
{
	JS::RootedValue value(cx);
	if (!JS_GetProperty(cx, object, name, &value) || !value.isString()) {
		return std::nullopt;
	}
	return JsToNativeString(cx, value.toString());
}

/// @brief Resolves the text the label should show.
std::string resolveText(Sandbox::ScriptHost& host, const std::optional<std::string>& literal,
						const std::optional<std::string>& key)
{
	if (key.has_value() && host.localization != nullptr) {
		return std::string(host.localization->text(*key));
	}
	if (literal.has_value()) {
		return *literal;
	}
	if (key.has_value()) {
		// No table loaded: show the key, which is what a missing translation does too.
		return *key;
	}
	return std::string();
}

/**
 * @brief Builds the attributes a label draws with.
 *
 * Order of preference: an explicit `style` object (usually `System.textStyle()`'s
 * result), then a theme element's caption styling, then a semantic role, falling back
 * to the theme's label role. Nothing here hard-codes a font or a size.
 */
Lotte::Text::TextAttributes resolveAttributes(JSContext* cx, JS::HandleObject options,
											 Sandbox::ScriptHost& host)
{
	Lotte::Text::TextAttributes attributes;
	if (host.theme != nullptr) {
		attributes = host.theme->textStyle(Lotte::Gui::ThemeTextRole::Label, 0u);
		if (const std::optional<std::string> elementName = stringProperty(cx, options, "element")) {
			Lotte::Gui::ThemeElement element = Lotte::Gui::ThemeElement::Count;
			if (Lotte::Gui::parseThemeElement(*elementName, element)) {
				if (const auto elementStyle = host.theme->elementTextStyle(element, 0u)) {
					attributes = *elementStyle;
				}
			}
		} else if (const std::optional<std::string> roleName = stringProperty(cx, options, "role")) {
			Lotte::Gui::ThemeTextRole role = Lotte::Gui::ThemeTextRole::Count;
			if (Lotte::Gui::parseThemeTextRole(*roleName, role)) {
				attributes = host.theme->textStyle(role, 0u);
			}
		}
	}
	JS::RootedValue styleValue(cx);
	if (JS_GetProperty(cx, options, "style", &styleValue) && styleValue.isObject()) {
		JS::RootedObject style(cx, &styleValue.toObject());
		if (const std::optional<std::string> face = stringProperty(cx, style, "face")) {
			attributes.familyName = *face;
		}
		attributes.size = static_cast<float>(numberProperty(cx, style, "size", attributes.size));
		attributes.weight = static_cast<Lotte::Text::FontWeight>(static_cast<unsigned>(
			numberProperty(cx, style, "weight",
						   static_cast<double>(static_cast<unsigned>(attributes.weight)))));
		attributes.italic = boolProperty(cx, style, "italic", attributes.italic);
		attributes.underline = boolProperty(cx, style, "underline", attributes.underline);
		if (const std::optional<std::string> colour = stringProperty(cx, style, "colour")) {
			Lotte::Text::Colour parsed = attributes.colour;
			if (Lotte::Text::parseMarkupColour(*colour, parsed)) {
				attributes.colour = parsed;
			}
		}
	}
	return attributes;
}

/// @brief The label behind `this`, never null.
LabelHandle* labelFromThis(JS::CallArgs& args)
{
	LabelHandle* handle = getNativePointer<LabelHandle>(args.thisv().toObjectOrNull());
	if (handle == nullptr || !handle->label) {
		throw std::runtime_error("GUI.Label: the label is not valid");
	}
	return handle;
}

/// @brief Reads an alignment name from the options object.
Sandbox::LabelAlign readAlign(JSContext* cx, JS::HandleObject options)
{
	const std::optional<std::string> name = stringProperty(cx, options, "align");
	if (!name.has_value() || *name == "centre" || *name == "center") {
		return Sandbox::LabelAlign::Centre;
	}
	if (*name == "left") {
		return Sandbox::LabelAlign::Left;
	}
	if (*name == "right") {
		return Sandbox::LabelAlign::Right;
	}
	throw std::runtime_error("GUI.Label: unknown align '" + *name + "'");
}

/// @brief `new GUI.Label(options)`.
bool js_gui_label_constructor(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Sandbox::ScriptHost* host = gHost;
		if (host == nullptr || host->gui == nullptr) {
			throw std::runtime_error("GUI.Label: graphics is not available");
		}
		JS::RootedObject options(cx, JS_NewPlainObject(cx));
		if (argc >= 1 && args.get(0).isObject()) {
			options = &args.get(0).toObject();
		}
		const std::optional<std::string> literal = stringProperty(cx, options, "text");
		const std::optional<std::string> key = stringProperty(cx, options, "key");
		auto label = std::make_shared<Sandbox::ScriptLabel>(
			resolveText(*host, literal, key), resolveAttributes(cx, options, *host));
		label->setAlign(readAlign(cx, options));
		label->setScale(static_cast<float>(numberProperty(cx, options, "scale", 1.0)));
		// x/y/width/height are canvas units, exactly like `GUI.Button`'s: the scene derives
		// the pixel box and the font scale from its viewport, so a layout is resolution-free.
		Sandbox::CanvasRect rect;
		rect.origin.x = static_cast<float>(numberProperty(cx, options, "x", 0.0));
		rect.origin.y = static_cast<float>(numberProperty(cx, options, "y", 0.0));
		rect.size.x = static_cast<float>(numberProperty(cx, options, "width", 240.0));
		rect.size.y = static_cast<float>(numberProperty(cx, options, "height", 32.0));
		host->gui->setCanvasRect(*label, rect);
		label->setHidden(boolProperty(cx, options, "hidden", false));

		JS::RootedObject object(cx,
			JS_NewObjectWithGivenProto(cx, &LabelClass.protoClass, *LabelClass.prototype));
		setNativePointer(object, new LabelHandle{ label, key.value_or(std::string()) });
		host->gui->addLabel(label);
		args.rval().setObject(*object);
		return true;
	}, cx, argc, vp);
}

/// @brief Finalizer of `GUI.Label`: drops the shared label.
void js_gui_label_finalizer(JS::GCContext* gcx, JSObject* obj)
{
	finalizeNative<LabelHandle>(gcx, obj);
}

} // namespace

namespace {

/// @brief `label.text`.
bool js_label_get_text(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), labelFromThis(args)->label->text());
		return true;
	}, cx, argc, vp);
}

/// @brief `label.text = value`: sets a literal and forgets the key it came from.
bool js_label_set_text(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		LabelHandle* handle = labelFromThis(args);
		handle->key.clear();
		handle->label->setText(JsToNativeString(cx, args.get(0).toString()));
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

/// @brief `label.key`.
bool js_label_get_key(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), labelFromThis(args)->key);
		return true;
	}, cx, argc, vp);
}

/// @brief `label.key = value`: re-resolves the caption from the localization table.
bool js_label_set_key(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		LabelHandle* handle = labelFromThis(args);
		handle->key = JsToNativeString(cx, args.get(0).toString());
		const Sandbox::ScriptHost* host = gHost;
		handle->label->setText(host != nullptr && host->localization != nullptr
			? std::string(host->localization->text(handle->key))
			: handle->key);
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

/// @brief `label.hidden`.
bool js_label_get_hidden(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), labelFromThis(args)->label->hidden());
		return true;
	}, cx, argc, vp);
}

/// @brief `label.hidden = value`.
bool js_label_set_hidden(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		labelFromThis(args)->label->setHidden(args.get(0).toBoolean());
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

/// @brief `label.box(x, y, width, height)`: repositions the text block, in canvas units.
bool js_label_set_box(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto label = labelFromThis(args)->label;
		if (gHost == nullptr || gHost->gui == nullptr) {
			throw std::runtime_error("GUI.Label.box: graphics is not available");
		}
		Sandbox::CanvasRect rect;
		rect.origin.x = static_cast<float>(args.get(0).toNumber());
		rect.origin.y = static_cast<float>(args.get(1).toNumber());
		rect.size.x = static_cast<float>(args.get(2).toNumber());
		rect.size.y = static_cast<float>(args.get(3).toNumber());
		gHost->gui->setCanvasRect(*label, rect);
		args.rval().setUndefined();
		return true;
	}, cx, argc, vp);
}

/// @brief `label.remove()`: takes the label off the screen.
bool js_label_remove(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Sandbox::ScriptHost* host = gHost;
		if (host == nullptr || host->gui == nullptr) {
			throw std::runtime_error("GUI.Label.remove: graphics is not available");
		}
		host->gui->removeLabel(labelFromThis(args)->label.get());
		args.rval().setUndefined();
		return true;
	}, cx, argc, vp);
}

} // namespace

void RegisterGuiLabelClass(JSContext* cx, const JS::HandleObject& global, Sandbox::ScriptHost& host)
{
	gHost = &host;

	std::memset(&LabelClass.protoClass, 0, sizeof(JSClass));
	std::memset(&LabelClass.ops, 0, sizeof(JSClassOps));
	LabelClass.name = "Label";
	LabelClass.nargs = 1;
	LabelClass.constructor = js_gui_label_constructor;
	LabelClass.ops.construct = js_gui_label_constructor;
	LabelClass.ops.finalize = js_gui_label_finalizer;
	LabelClass.protoClass.cOps = &LabelClass.ops;
	LabelClass.protoClass.name = "Label";
	// Foreground finalization: the finalizer drops a widget the scene still owns.
	LabelClass.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1);
	LabelClass.prototype = std::make_unique<JS::RootedObject>(cx,
		JS_NewObject(cx, &LabelClass.protoClass));
	LabelClass.ps.push_back(JS_PSGS("text", js_label_get_text, js_label_set_text,
		JSPROP_ENUMERATE));
	LabelClass.ps.push_back(JS_PSGS("key", js_label_get_key, js_label_set_key,
		JSPROP_ENUMERATE));
	LabelClass.ps.push_back(JS_PSGS("hidden", js_label_get_hidden, js_label_set_hidden,
		JSPROP_ENUMERATE));
	LabelClass.ps.push_back(JS_PS_END);
	LabelClass.static_ps.push_back(JS_PS_END);
	LabelClass.fs.push_back(JS_FN("box", js_label_set_box, 4, 0));
	LabelClass.fs.push_back(JS_FN("remove", js_label_remove, 0, 0));
	LabelClass.fs.push_back(JS_FS_END);
	LabelClass.static_fs.push_back(JS_FS_END);
	LabelClass.registerToJS(cx, global, *LabelClass.prototype);
}

} // namespace JS
