#ifndef JSGUILAYOUT_HPP
#define JSGUILAYOUT_HPP
/**
 * @file JsGuiLayout.hpp
 * @brief Declares `GUI.Layout`: the virtual canvas a script lays its GUI out on.
 *
 * A layout is the only place a resolution is allowed to matter. The script names a
 * design size (`width`/`height`, e.g. 800x600), the real window is fitted to it, and
 * every widget it creates afterwards is positioned in *canvas* units. The class also
 * exposes the aspect ratio and normalized helpers, so a layout may be written as
 * fractions (`UI.w(0.5)`) instead of numbers when that is clearer.
 *
 * See `Lotte::Gui::GuiViewport` for the mapping itself; this file only translates the
 * options object and the properties.
 */
#include <JS/JsUtil.hpp>

namespace Sandbox {
class ScriptHost;
}

namespace JS {

/**
 * @brief Adds the `Layout` class to an existing `GUI` module object.
 * @param cx Context that owns every object created here.
 * @param global Module object to define the class on.
 * @param host Engine services the class talks to; must outlive the context.
 */
void RegisterGuiLayoutClass(JSContext* cx, const JS::HandleObject& global, Sandbox::ScriptHost& host);

}

#endif // JSGUILAYOUT_HPP
