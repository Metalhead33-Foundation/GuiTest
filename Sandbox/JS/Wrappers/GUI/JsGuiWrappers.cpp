#include "JsGuiWrappers.hpp"
#include <ScriptHost.hpp>
#include <GuiScene.hpp>
#include <JS/Wrappers/System/JsSystem.hpp>
#include <Lotte/Text/LteMarkupColour.hpp>
#include <js/Array.h>
#include <js/Object.h>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace JS {

namespace {

/// @brief The host the registered functions talk to; see `JsSystem.cpp`.
Sandbox::ScriptHost* gHost = nullptr;

/// @brief The `GUI.Button` wrapper class.
ClassCreator ButtonClass;

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

/// @brief The button behind `this`, never null.
std::shared_ptr<Sandbox::ScriptButton> buttonFromThis(JS::CallArgs& args)
{
	auto holder = getNativePointer<std::shared_ptr<Sandbox::ScriptButton>>(
		args.thisv().toObjectOrNull());
	if (!holder || !*holder) {
		throw std::runtime_error("GUI.Button: the widget is not valid");
	}
	return *holder;
}

/// @brief Reads a colour property written as an MML colour string.
glm::fvec4 colourProperty(JSContext* cx, JS::HandleObject object, const char* name,
   const glm::fvec4& fallback)
{
	JS::RootedValue value(cx);
	if (!JS_GetProperty(cx, object, name, &value) || !value.isString()) {
		return fallback;
	}
	const std::string text = JsToNativeString(cx, value.toString());
	Lotte::Text::Colour colour = fallback;
	if (!Lotte::Text::parseMarkupColour(text, colour)) {
		return fallback;
	}
	return colour;
}

/// @brief Reads the first @p count numbers of a JavaScript array.
bool readNumberArray(JSContext* cx, JS::HandleObject array, uint32_t count, double (&out)[4])
{
	bool isArray = false;
	JS::IsArrayObject(cx, array, &isArray);
	if (!isArray) return false;
	uint32_t length = 0;
	JS::GetArrayLength(cx, array, &length);
	if (length < count) return false;
	for (uint32_t index = 0; index < count; ++index) {
		JS::RootedValue element(cx);
		JS_GetElement(cx, array, index, &element);
		if (!element.isNumber()) return false;
		out[index] = element.toNumber();
	}
	return true;
}

/// @brief Builds an atlas rectangle from `[u0, v0, u1, v1]`.
Lotte::Gui::TextureCoordDuo makeQuad(const double (&numbers)[4])
{
	Lotte::Gui::TextureCoordDuo duo;
	duo.first = glm::fvec2(float(numbers[0]), float(numbers[1]));
	duo.second = glm::fvec2(float(numbers[2]), float(numbers[3]));
	return duo;
}

/// @brief Reads `uv`: four numbers for every state, or eight `[u0, v0, u1, v1]` entries.
std::array<Lotte::Gui::TextureCoordDuo, Lotte::Gui::WidgetStateCount>
readAtlasTable(JSContext* cx, JS::HandleValue value)
{
	std::array<Lotte::Gui::TextureCoordDuo, Lotte::Gui::WidgetStateCount> table {};
	for (Lotte::Gui::TextureCoordDuo& entry : table) {
		entry.first = glm::fvec2(0.0f, 0.0f);
		entry.second = glm::fvec2(1.0f, 1.0f);
	}
	if (!value.isObject()) return table;
	JS::RootedObject array(cx, &value.toObject());
	bool isArray = false;
	JS::IsArrayObject(cx, array, &isArray);
	if (!isArray) return table;
	uint32_t length = 0;
	JS::GetArrayLength(cx, array, &length);
	double numbers[4] = { 0.0, 0.0, 1.0, 1.0 };
	if (length == 4) {
		if (readNumberArray(cx, array, 4, numbers)) {
			std::fill(table.begin(), table.end(), makeQuad(numbers));
		}
		return table;
	}
	for (uint32_t index = 0; index < length && index < table.size(); ++index) {
		JS::RootedValue element(cx);
		JS_GetElement(cx, array, index, &element);
		if (!element.isObject()) continue;
		JS::RootedObject quadObject(cx, &element.toObject());
		if (readNumberArray(cx, quadObject, 4, numbers)) {
			table[index] = makeQuad(numbers);
		}
	}
	return table;
}

} // namespace

namespace {

/// @brief `new GUI.Button(options)`.
bool js_gui_button_constructor(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Sandbox::ScriptHost* host = gHost;
		if (host == nullptr || host->gui == nullptr) {
			throw std::runtime_error("GUI.Button: graphics is not available");
		}
		if (argc < 1 || !args.get(0).isObject()) {
			throw std::runtime_error("GUI.Button: expected an options object");
		}
		JS::RootedObject options(cx, &args.get(0).toObject());

		Sandbox::ButtonStyle style;
		JS::RootedValue coloursValue(cx);
		if (JS_GetProperty(cx, options, "colours", &coloursValue) && coloursValue.isObject()) {
			JS::RootedObject colours(cx, &coloursValue.toObject());
			style.colours[0] = colourProperty(cx, colours, "idle", style.colours[0]);
			style.colours[1] = colourProperty(cx, colours, "hover", style.colours[1]);
			style.colours[2] = colourProperty(cx, colours, "active", style.colours[2]);
			style.colours[3] = colourProperty(cx, colours, "disabled", style.colours[3]);
		}

		Lotte::Gui::GuiTextureBinding binding {};
		JS::RootedValue textureValue(cx);
		if (JS_GetProperty(cx, options, "texture", &textureValue) && textureValue.isObject()) {
			if (auto texture = textureFromValue(cx, textureValue)) {
				binding = texture->binding();
			}
		}
		JS::RootedValue uvValue(cx);
		if (JS_GetProperty(cx, options, "uv", &uvValue)) {
			style.coordinates = readAtlasTable(cx, uvValue);
		}

		auto button = std::make_shared<Sandbox::ScriptButton>(binding, style);
		const double x = numberProperty(cx, options, "x", 0.0);
		const double y = numberProperty(cx, options, "y", 0.0);
		const double width = numberProperty(cx, options, "width", 128.0);
		const double height = numberProperty(cx, options, "height", 48.0);
		button->setPosition(glm::fvec2(float(x), float(y)),
			glm::fvec2(float(x + width), float(y + height)));
		button->setFlag<Lotte::Gui::WidgetStateFlag::Enabled>(
			boolProperty(cx, options, "enabled", true));
		button->setHidden(boolProperty(cx, options, "hidden", false));

		JS::RootedObject object(cx,
			JS_NewObjectWithGivenProto(cx, &ButtonClass.protoClass, *ButtonClass.prototype));
		setNativePointer(object, new std::shared_ptr<Sandbox::ScriptButton>(button));
		// The handler lives on the wrapper, so defining it before `addButton()` means
		// no click can reach an object whose property is not there yet.
		JS::RootedValue undefined(cx, JS::UndefinedValue());
		JS_DefineProperty(cx, object, "onClick", undefined, JSPROP_ENUMERATE | JSPROP_PERMANENT);
		connectSignalToJS(cx, object, "onClick", button->activated);
		host->gui->addButton(button);
		args.rval().setObject(*object);
		return true;
	}, cx, argc, vp);
}

/// @brief Finalizer of `GUI.Button`: drops the shared widget.
void js_gui_button_finalizer(JS::GCContext* gcx, JSObject* obj)
{
	finalizeNative<std::shared_ptr<Sandbox::ScriptButton>>(gcx, obj);
}

/// @brief `button.x`.
bool js_button_get_x(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), buttonFromThis(args)->topLeft().x);
		return true;
	}, cx, argc, vp);
}

/// @brief `button.x = value`.
bool js_button_set_x(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto button = buttonFromThis(args);
		const float x = static_cast<float>(args.get(0).toNumber());
		button->setPosition(glm::fvec2(x, button->topLeft().y),
			glm::fvec2(x + button->size().x, button->bottomRight().y));
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

/// @brief `button.y`.
bool js_button_get_y(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), buttonFromThis(args)->topLeft().y);
		return true;
	}, cx, argc, vp);
}

/// @brief `button.y = value`.
bool js_button_set_y(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto button = buttonFromThis(args);
		const float y = static_cast<float>(args.get(0).toNumber());
		button->setPosition(glm::fvec2(button->topLeft().x, y),
			glm::fvec2(button->bottomRight().x, y + button->size().y));
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

} // namespace



namespace {

/// @brief `button.width`.
bool js_button_get_width(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), buttonFromThis(args)->size().x);
		return true;
	}, cx, argc, vp);
}

/// @brief `button.width = value`.
bool js_button_set_width(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto button = buttonFromThis(args);
		const float width = static_cast<float>(args.get(0).toNumber());
		button->setBottomRight(
			glm::fvec2(button->topLeft().x + width, button->bottomRight().y));
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

/// @brief `button.height`.
bool js_button_get_height(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), buttonFromThis(args)->size().y);
		return true;
	}, cx, argc, vp);
}

/// @brief `button.height = value`.
bool js_button_set_height(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto button = buttonFromThis(args);
		const float height = static_cast<float>(args.get(0).toNumber());
		button->setBottomRight(
			glm::fvec2(button->bottomRight().x, button->topLeft().y + height));
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

/// @brief `button.enabled`.
bool js_button_get_enabled(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(),
			buttonFromThis(args)->getFlag(Lotte::Gui::WidgetStateFlag::Enabled));
		return true;
	}, cx, argc, vp);
}

/// @brief `button.enabled = value`.
bool js_button_set_enabled(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		buttonFromThis(args)->setFlag<Lotte::Gui::WidgetStateFlag::Enabled>(
			args.get(0).toBoolean());
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

/// @brief `button.hidden`.
bool js_button_get_hidden(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), buttonFromThis(args)->hidden());
		return true;
	}, cx, argc, vp);
}

/// @brief `button.hidden = value`.
bool js_button_set_hidden(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		buttonFromThis(args)->setHidden(args.get(0).toBoolean());
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

/// @brief `button.hovered`: true while the pointer is over the button.
bool js_button_get_hovered(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(),
			buttonFromThis(args)->getFlag(Lotte::Gui::WidgetStateFlag::InMouseFocus));
		return true;
	}, cx, argc, vp);
}

/// @brief `button.pressed`: true while the button is held down.
bool js_button_get_pressed(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), buttonFromThis(args)->pressed());
		return true;
	}, cx, argc, vp);
}

/// @brief `button.remove()`: takes the widget off the screen.
bool js_button_remove(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Sandbox::ScriptHost* host = gHost;
		if (host == nullptr || host->gui == nullptr) {
			throw std::runtime_error("GUI.Button.remove: graphics is not available");
		}
		host->gui->removeButton(buttonFromThis(args).get());
		args.rval().setUndefined();
		return true;
	}, cx, argc, vp);
}

} // namespace

void RegisterGuiClasses(JSContext* cx, const JS::HandleObject& global, Sandbox::ScriptHost& host)
{
	gHost = &host;

	std::memset(&ButtonClass.protoClass, 0, sizeof(JSClass));
	std::memset(&ButtonClass.ops, 0, sizeof(JSClassOps));
	ButtonClass.name = "Button";
	ButtonClass.nargs = 1;
	ButtonClass.constructor = js_gui_button_constructor;
	ButtonClass.ops.construct = js_gui_button_constructor;
	ButtonClass.ops.finalize = js_gui_button_finalizer;
	ButtonClass.protoClass.cOps = &ButtonClass.ops;
	ButtonClass.protoClass.name = "Button";
	// Foreground finalization: the finalizer releases a shared widget, and that
	// widget's signal owns a JavaScript root.
	ButtonClass.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1);
	ButtonClass.prototype = std::make_unique<JS::RootedObject>(cx,
		JS_NewObject(cx, &ButtonClass.protoClass));
	ButtonClass.ps.push_back(JS_PSGS("x", js_button_get_x, js_button_set_x, JSPROP_ENUMERATE));
	ButtonClass.ps.push_back(JS_PSGS("y", js_button_get_y, js_button_set_y, JSPROP_ENUMERATE));
	ButtonClass.ps.push_back(JS_PSGS("width", js_button_get_width, js_button_set_width,
		JSPROP_ENUMERATE));
	ButtonClass.ps.push_back(JS_PSGS("height", js_button_get_height, js_button_set_height,
		JSPROP_ENUMERATE));
	ButtonClass.ps.push_back(JS_PSGS("enabled", js_button_get_enabled, js_button_set_enabled,
		JSPROP_ENUMERATE));
	ButtonClass.ps.push_back(JS_PSGS("hidden", js_button_get_hidden, js_button_set_hidden,
		JSPROP_ENUMERATE));
	ButtonClass.ps.push_back(JS_PSG("hovered", js_button_get_hovered, JSPROP_ENUMERATE));
	ButtonClass.ps.push_back(JS_PSG("pressed", js_button_get_pressed, JSPROP_ENUMERATE));
	ButtonClass.ps.push_back(JS_PS_END);
	ButtonClass.static_ps.push_back(JS_PS_END);
	ButtonClass.fs.push_back(JS_FN("remove", js_button_remove, 0, 0));
	ButtonClass.fs.push_back(JS_FS_END);
	ButtonClass.static_fs.push_back(JS_FS_END);
	ButtonClass.registerToJS(cx, global, *ButtonClass.prototype);
}

} // namespace JS

