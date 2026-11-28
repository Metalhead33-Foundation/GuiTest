#ifndef JSGUIWRAPPERS_HPP
#define JSGUIWRAPPERS_HPP
/**
 * @file JsGuiWrappers.hpp
 * @brief Declares the `GUI` module: buttons the script creates and clicks.
 *
 * `GUI.Button` is the demonstration the sandbox was built for: the script owns
 * the geometry, the look and the click handler, while the widget itself and the
 * input routing stay in `Sandbox::GuiScene`. The wrapper's job is exactly the
 * translation between the two.
 */
#include <JS/JsUtil.hpp>

namespace Sandbox {
class ScriptHost;
}

namespace JS {

/**
 * @brief Registers the `GUI` module on a module object.
 * @param cx Context that owns every object created here.
 * @param global Module object to define the classes on.
 * @param host Engine services the module talks to; must outlive the context.
 */
void RegisterGuiClasses(JSContext* cx, const JS::HandleObject& global, Sandbox::ScriptHost& host);

}

#endif // JSGUIWRAPPERS_HPP
