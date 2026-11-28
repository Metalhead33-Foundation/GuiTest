#ifndef JSGUILABEL_HPP
#define JSGUILABEL_HPP
/**
 * @file JsGuiLabel.hpp
 * @brief Declares `GUI.Label`: a text block the script creates and updates.
 *
 * A label is the JavaScript face of `Sandbox::ScriptLabel`. Its text may be a literal
 * or a localization key, and its styling is either an object `System.textStyle()`
 * returned or the name of a theme element/role - there is no reason for a script to
 * spell out a font or a size when the theme already decides them.
 */
#include <JS/JsUtil.hpp>

namespace Sandbox {
class ScriptHost;
}

namespace JS {

/**
 * @brief Adds the `Label` class to an existing `GUI` module object.
 * @param cx Context that owns every object created here.
 * @param global Module object to define the class on.
 * @param host Engine services the class talks to; must outlive the context.
 */
void RegisterGuiLabelClass(JSContext* cx, const JS::HandleObject& global, Sandbox::ScriptHost& host);

}

#endif // JSGUILABEL_HPP
