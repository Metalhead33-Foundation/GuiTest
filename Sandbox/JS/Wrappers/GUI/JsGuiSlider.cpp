#include "JsGuiSlider.hpp"
#include <ScriptHost.hpp>
#include <GuiScene.hpp>
#include <js/Object.h>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace JS {

namespace {

/// @brief The host the registered functions talk to; see `JsSystem.cpp`.
Sandbox::ScriptHost* gHost = nullptr;

/// @brief The `GUI.Slider` wrapper class.
ClassCreator SliderClass;

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

/// @brief The slider behind `this`, never null.
std::shared_ptr<Lotte::Gui::Slider> sliderFromThis(JS::CallArgs& args)
{
	auto holder = getNativePointer<std::shared_ptr<Lotte::Gui::Slider>>(
		args.thisv().toObjectOrNull());
	if (!holder || !*holder) {
		throw std::runtime_error("GUI.Slider: the widget is not valid");
	}
	return *holder;
}

/// @brief `new GUI.Slider(options)`.
bool js_gui_slider_constructor(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Sandbox::ScriptHost* host = gHost;
		if (host == nullptr || host->gui == nullptr) {
			throw std::runtime_error("GUI.Slider: graphics is not available");
		}
		JS::RootedObject options(cx, JS_NewPlainObject(cx));
		if (argc >= 1 && args.get(0).isObject()) {
			options = &args.get(0).toObject();
		}
		Lotte::Gui::SliderOrientation orientation = Lotte::Gui::SliderOrientation::Horizontal;
		JS::RootedValue orientationValue(cx);
		if (JS_GetProperty(cx, options, "orientation", &orientationValue) &&
			orientationValue.isString()) {
			const std::string name = JsToNativeString(cx, orientationValue.toString());
			if (name == "vertical") {
				orientation = Lotte::Gui::SliderOrientation::Vertical;
			} else if (name != "horizontal") {
				throw std::runtime_error("GUI.Slider: unknown orientation '" + name + "'");
			}
		}
		auto slider = std::make_shared<Lotte::Gui::Slider>(orientation);
		if (host->theme != nullptr) {
			slider->setTheme(host->theme);
		}
		slider->setRange(float(numberProperty(cx, options, "min", 0.0)),
						 float(numberProperty(cx, options, "max", 1.0)),
						 float(numberProperty(cx, options, "step", 0.0)));
		slider->setValue(float(numberProperty(cx, options, "value", 0.0)));
		// Canvas units, like every other widget: the viewport turns them into pixels.
		Sandbox::CanvasRect rect;
		rect.origin.x = static_cast<float>(numberProperty(cx, options, "x", 0.0));
		rect.origin.y = static_cast<float>(numberProperty(cx, options, "y", 0.0));
		rect.size.x = static_cast<float>(numberProperty(cx, options, "width", 240.0));
		rect.size.y = static_cast<float>(numberProperty(cx, options, "height", 32.0));
		host->gui->setCanvasRect(*slider, rect);
		slider->setFlag<Lotte::Gui::WidgetStateFlag::Enabled>(
			boolProperty(cx, options, "enabled", true));
		slider->setHidden(boolProperty(cx, options, "hidden", false));

		JS::RootedObject object(cx,
			JS_NewObjectWithGivenProto(cx, &SliderClass.protoClass, *SliderClass.prototype));
		setNativePointer(object, new std::shared_ptr<Lotte::Gui::Slider>(slider));
		// The handler exists before the widget reaches the scene, so no drag can land
		// on an object whose property is not there yet.
		JS::RootedValue undefined(cx, JS::UndefinedValue());
		JS_DefineProperty(cx, object, "onChange", undefined, JSPROP_ENUMERATE | JSPROP_PERMANENT);
		connectSignalToJS(cx, object, "onChange", slider->valueChanged);
		host->gui->addSlider(slider);
		args.rval().setObject(*object);
		return true;
	}, cx, argc, vp);
}

/// @brief Finalizer of `GUI.Slider`: drops the shared widget.
void js_gui_slider_finalizer(JS::GCContext* gcx, JSObject* obj)
{
	finalizeNative<std::shared_ptr<Lotte::Gui::Slider>>(gcx, obj);
}

/// @brief `slider.value`.
bool js_slider_get_value(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), sliderFromThis(args)->value());
		return true;
	}, cx, argc, vp);
}

/// @brief `slider.value = value`.
bool js_slider_set_value(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto slider = sliderFromThis(args);
		slider->setValue(static_cast<float>(args.get(0).toNumber()));
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

/// @brief `slider.min`.
bool js_slider_get_min(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), sliderFromThis(args)->minimum());
		return true;
	}, cx, argc, vp);
}

/// @brief `slider.max`.
bool js_slider_get_max(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), sliderFromThis(args)->maximum());
		return true;
	}, cx, argc, vp);
}

/// @brief `slider.enabled`.
bool js_slider_get_enabled(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(),
			sliderFromThis(args)->getFlag(Lotte::Gui::WidgetStateFlag::Enabled));
		return true;
	}, cx, argc, vp);
}

/// @brief `slider.enabled = value`.
bool js_slider_set_enabled(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		sliderFromThis(args)->setFlag<Lotte::Gui::WidgetStateFlag::Enabled>(
			args.get(0).toBoolean());
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

/// @brief `slider.hidden`.
bool js_slider_get_hidden(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), sliderFromThis(args)->hidden());
		return true;
	}, cx, argc, vp);
}

/// @brief `slider.hidden = value`.
bool js_slider_set_hidden(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		sliderFromThis(args)->setHidden(args.get(0).toBoolean());
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

/// @brief `slider.remove()`: takes the widget off the screen.
bool js_slider_remove(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext*, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Sandbox::ScriptHost* host = gHost;
		if (host == nullptr || host->gui == nullptr) {
			throw std::runtime_error("GUI.Slider.remove: graphics is not available");
		}
		host->gui->removeWidget(sliderFromThis(args).get());
		args.rval().setUndefined();
		return true;
	}, cx, argc, vp);
}

} // namespace

void RegisterGuiSliderClass(JSContext* cx, const JS::HandleObject& global, Sandbox::ScriptHost& host)
{
	gHost = &host;

	std::memset(&SliderClass.protoClass, 0, sizeof(JSClass));
	std::memset(&SliderClass.ops, 0, sizeof(JSClassOps));
	SliderClass.name = "Slider";
	SliderClass.nargs = 1;
	SliderClass.constructor = js_gui_slider_constructor;
	SliderClass.ops.construct = js_gui_slider_constructor;
	SliderClass.ops.finalize = js_gui_slider_finalizer;
	SliderClass.protoClass.cOps = &SliderClass.ops;
	SliderClass.protoClass.name = "Slider";
	// Foreground finalization: the finalizer releases a shared widget whose signal
	// owns a JavaScript root.
	SliderClass.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1);
	SliderClass.prototype = std::make_unique<JS::RootedObject>(cx,
		JS_NewObject(cx, &SliderClass.protoClass));
	SliderClass.ps.push_back(JS_PSGS("value", js_slider_get_value, js_slider_set_value,
		JSPROP_ENUMERATE));
	SliderClass.ps.push_back(JS_PSG("min", js_slider_get_min, JSPROP_ENUMERATE));
	SliderClass.ps.push_back(JS_PSG("max", js_slider_get_max, JSPROP_ENUMERATE));
	SliderClass.ps.push_back(JS_PSGS("enabled", js_slider_get_enabled, js_slider_set_enabled,
		JSPROP_ENUMERATE));
	SliderClass.ps.push_back(JS_PSGS("hidden", js_slider_get_hidden, js_slider_set_hidden,
		JSPROP_ENUMERATE));
	SliderClass.ps.push_back(JS_PS_END);
	SliderClass.static_ps.push_back(JS_PS_END);
	SliderClass.fs.push_back(JS_FN("remove", js_slider_remove, 0, 0));
	SliderClass.fs.push_back(JS_FS_END);
	SliderClass.static_fs.push_back(JS_FS_END);
	SliderClass.registerToJS(cx, global, *SliderClass.prototype);
}

} // namespace JS
