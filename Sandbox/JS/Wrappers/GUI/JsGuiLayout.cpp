#include "JsGuiLayout.hpp"
#include <ScriptHost.hpp>
#include <GuiScene.hpp>
#include <Lotte/Gui/LteGuiViewport.hpp>
#include <js/Object.h>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace JS {

namespace {

/// @brief The host the registered functions talk to; see `JsSystem.cpp`.
Sandbox::ScriptHost* gHost = nullptr;

/// @brief The `GUI.Layout` wrapper class.
ClassCreator LayoutClass;

/// @brief Reads a numeric property, falling back when it is missing or not a number.
double numberProperty(JSContext* cx, JS::HandleObject object, const char* name, double fallback)
{
	JS::RootedValue value(cx);
	if (!JS_GetProperty(cx, object, name, &value) || !value.isNumber()) {
		return fallback;
	}
	return value.toNumber();
}

/// @brief Reads a string property, or `std::nullopt` when it is absent.
std::optional<std::string> stringProperty(JSContext* cx, JS::HandleObject object, const char* name)
{
	JS::RootedValue value(cx);
	if (!JS_GetProperty(cx, object, name, &value) || !value.isString()) {
		return std::nullopt;
	}
	return JsToNativeString(cx, value.toString());
}

/// @brief The viewport behind `this`, never null.
std::shared_ptr<Lotte::Gui::GuiViewport> layoutFromThis(JS::CallArgs& args)
{
	auto holder = getNativePointer<std::shared_ptr<Lotte::Gui::GuiViewport>>(
		args.thisv().toObjectOrNull());
	if (!holder || !*holder) {
		throw std::runtime_error("GUI.Layout: the layout is not valid");
	}
	return *holder;
}

/// @brief The scene the active layout belongs to, never null.
Sandbox::GuiScene& scene()
{
	if (gHost == nullptr || gHost->gui == nullptr) {
		throw std::runtime_error("GUI.Layout: graphics is not available");
	}
	return *gHost->gui;
}

/// @brief `new GUI.Layout(options)`.
bool js_gui_layout_constructor(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		Sandbox::GuiScene& gui = scene();
		JS::RootedObject options(cx, JS_NewPlainObject(cx));
		if (argc >= 1 && args.get(0).isObject()) {
			options = &args.get(0).toObject();
		}

		Lotte::Gui::ViewportSettings settings;
		settings.canvas = glm::fvec2(
			static_cast<float>(numberProperty(cx, options, "width", double(settings.canvas.x))),
			static_cast<float>(numberProperty(cx, options, "height", double(settings.canvas.y))));
		settings.uiScale =
			static_cast<float>(numberProperty(cx, options, "uiScale", double(settings.uiScale)));
		settings.minimumScale = static_cast<float>(
			numberProperty(cx, options, "minScale", double(settings.minimumScale)));
		settings.maximumScale = static_cast<float>(
			numberProperty(cx, options, "maxScale", double(settings.maximumScale)));
		if (const std::optional<std::string> mode = stringProperty(cx, options, "mode");
			mode.has_value()) {
			Lotte::Gui::ScaleMode parsed = Lotte::Gui::ScaleMode::Fit;
			if (!Lotte::Gui::parseScaleMode(*mode, parsed)) {
				throw std::runtime_error("GUI.Layout: unknown mode '" + *mode + "'");
			}
			settings.mode = parsed;
		}

		// The canvas is new, the window is not: the resolution comes from the scene so a
		// script never has to pass it (and never has to know it).
		const auto current = gui.viewport();
		auto viewport = std::make_shared<Lotte::Gui::GuiViewport>(
			settings, current ? current->resolution() : glm::fvec2(1.0f));
		gui.setViewport(viewport);

		JS::RootedObject object(cx,
			JS_NewObjectWithGivenProto(cx, &LayoutClass.protoClass, *LayoutClass.prototype));
		setNativePointer(object, new std::shared_ptr<Lotte::Gui::GuiViewport>(viewport));
		args.rval().setObject(*object);
		return true;
	}, cx, argc, vp);
}

/// @brief Finalizer of `GUI.Layout`: drops the shared viewport.
void js_gui_layout_finalizer(JS::GCContext* gcx, JSObject* obj)
{
	finalizeNative<std::shared_ptr<Lotte::Gui::GuiViewport>>(gcx, obj);
}

} // namespace


namespace {

/// @brief `layout.width`: the virtual canvas width a layout is written in.
bool js_layout_get_width(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), layoutFromThis(args)->canvasSize().x);
		return true;
	}, cx, argc, vp);
}

/// @brief `layout.height`: the virtual canvas height a layout is written in.
bool js_layout_get_height(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), layoutFromThis(args)->canvasSize().y);
		return true;
	}, cx, argc, vp);
}

/// @brief `layout.scale`: real pixels per canvas unit.
bool js_layout_get_scale(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), layoutFromThis(args)->scale());
		return true;
	}, cx, argc, vp);
}

/// @brief `layout.aspect`: width / height of the canvas.
bool js_layout_get_aspect(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), layoutFromThis(args)->aspect());
		return true;
	}, cx, argc, vp);
}

/// @brief `layout.visibleWidth`: canvas units the window shows, which may exceed the canvas.
bool js_layout_get_visible_width(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), layoutFromThis(args)->visibleSize().x);
		return true;
	}, cx, argc, vp);
}

/// @brief `layout.visibleHeight`: see `visibleWidth`.
bool js_layout_get_visible_height(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), layoutFromThis(args)->visibleSize().y);
		return true;
	}, cx, argc, vp);
}

/// @brief `layout.mode`: `"fit"` or `"fill"`.
bool js_layout_get_mode(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		const char* name = Lotte::Gui::scaleModeName(layoutFromThis(args)->settings().mode);
		setReturnValue(cx, args.rval(), std::string_view(name != nullptr ? name : "fit"));
		return true;
	}, cx, argc, vp);
}

/// @brief `layout.resolution`: the real window size, for the rare case that needs it.
bool js_layout_get_resolution(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		const glm::fvec2 size = layoutFromThis(args)->resolution();
		JS::RootedObject resolution(cx, JS_NewPlainObject(cx));
		JS_DefineProperty(cx, resolution, "width", double(size.x),
			JSPROP_READONLY | JSPROP_ENUMERATE);
		JS_DefineProperty(cx, resolution, "height", double(size.y),
			JSPROP_READONLY | JSPROP_ENUMERATE);
		args.rval().setObject(*resolution);
		return true;
	}, cx, argc, vp);
}

/// @brief `layout.uiScale`, the player-facing multiplier on top of the fitted scale.
bool js_layout_get_ui_scale(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		(void)argc;
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		setReturnValue(cx, args.rval(), layoutFromThis(args)->settings().uiScale);
		return true;
	}, cx, argc, vp);
}

/// @brief `layout.uiScale = value`: rescales the GUI and re-applies every rectangle.
bool js_layout_set_ui_scale(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		auto viewport = layoutFromThis(args);
		Lotte::Gui::ViewportSettings settings = viewport->settings();
		settings.uiScale = static_cast<float>(args.get(0).toNumber());
		viewport->setSettings(settings);
		// The scene keeps the rectangles in canvas units; asking it to re-apply is what
		// makes a live scale change visible without rebuilding the GUI.
		scene().applyViewport();
		args.rval().set(args.get(0));
		return true;
	}, cx, argc, vp);
}

/**
 * @brief Shared body of the normalized helpers.
 * @param horizontal True for the `x`/`w` pair, false for `y`/`h`.
 */
bool layoutNormalized(JSContext* cx, JS::CallArgs& args, bool horizontal)
{
	auto viewport = layoutFromThis(args);
	const glm::fvec2& canvas = viewport->canvasSize();
	const float fraction = static_cast<float>(args.get(0).toNumber());
	setReturnValue(cx, args.rval(), fraction * (horizontal ? canvas.x : canvas.y));
	return true;
}

/// @brief `layout.x(fraction)`: a fraction of the canvas width, in canvas units.
bool js_layout_x(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		return layoutNormalized(cx, args, true);
	}, cx, argc, vp);
}

/// @brief `layout.y(fraction)`: a fraction of the canvas height, in canvas units.
bool js_layout_y(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		return layoutNormalized(cx, args, false);
	}, cx, argc, vp);
}

/// @brief `layout.w(fraction)`: a width as a fraction of the canvas width.
bool js_layout_w(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		return layoutNormalized(cx, args, true);
	}, cx, argc, vp);
}

/// @brief `layout.h(fraction)`: a height as a fraction of the canvas height.
bool js_layout_h(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return executeJSNative([](JSContext* cx, unsigned argc, JS::Value* vp) {
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		return layoutNormalized(cx, args, false);
	}, cx, argc, vp);
}

} // namespace



void RegisterGuiLayoutClass(JSContext* cx, const JS::HandleObject& global, Sandbox::ScriptHost& host)
{
	gHost = &host;

	std::memset(&LayoutClass.protoClass, 0, sizeof(JSClass));
	std::memset(&LayoutClass.ops, 0, sizeof(JSClassOps));
	LayoutClass.name = "Layout";
	LayoutClass.nargs = 1;
	LayoutClass.constructor = js_gui_layout_constructor;
	LayoutClass.ops.construct = js_gui_layout_constructor;
	LayoutClass.ops.finalize = js_gui_layout_finalizer;
	LayoutClass.protoClass.cOps = &LayoutClass.ops;
	LayoutClass.protoClass.name = "Layout";
	// Foreground finalization: the finalizer releases a viewport the scene shares.
	LayoutClass.protoClass.flags = JSCLASS_HAS_RESERVED_SLOTS(1);
	LayoutClass.prototype = std::make_unique<JS::RootedObject>(cx,
		JS_NewObject(cx, &LayoutClass.protoClass));
	LayoutClass.ps.push_back(JS_PSG("width", js_layout_get_width, JSPROP_ENUMERATE));
	LayoutClass.ps.push_back(JS_PSG("height", js_layout_get_height, JSPROP_ENUMERATE));
	LayoutClass.ps.push_back(JS_PSGS("uiScale", js_layout_get_ui_scale,
		js_layout_set_ui_scale, JSPROP_ENUMERATE));
	LayoutClass.ps.push_back(JS_PSG("scale", js_layout_get_scale, JSPROP_ENUMERATE));
	LayoutClass.ps.push_back(JS_PSG("aspect", js_layout_get_aspect, JSPROP_ENUMERATE));
	LayoutClass.ps.push_back(JS_PSG("visibleWidth", js_layout_get_visible_width,
		JSPROP_ENUMERATE));
	LayoutClass.ps.push_back(JS_PSG("visibleHeight", js_layout_get_visible_height,
		JSPROP_ENUMERATE));
	LayoutClass.ps.push_back(JS_PSG("mode", js_layout_get_mode, JSPROP_ENUMERATE));
	LayoutClass.ps.push_back(JS_PSG("resolution", js_layout_get_resolution, JSPROP_ENUMERATE));
	LayoutClass.ps.push_back(JS_PS_END);
	LayoutClass.static_ps.push_back(JS_PS_END);
	LayoutClass.fs.push_back(JS_FN("x", js_layout_x, 1, 0));
	LayoutClass.fs.push_back(JS_FN("y", js_layout_y, 1, 0));
	LayoutClass.fs.push_back(JS_FN("w", js_layout_w, 1, 0));
	LayoutClass.fs.push_back(JS_FN("h", js_layout_h, 1, 0));
	LayoutClass.fs.push_back(JS_FS_END);
	LayoutClass.static_fs.push_back(JS_FS_END);
	LayoutClass.registerToJS(cx, global, *LayoutClass.prototype);
}

} // namespace JS
