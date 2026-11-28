#ifndef JSGUISLIDER_HPP
#define JSGUISLIDER_HPP
/**
 * @file JsGuiSlider.hpp
 * @brief Declares `GUI.Slider`: a continuous control the script reads and writes.
 *
 * The wrapper is the thin JavaScript side of `Lotte::Gui::Slider`. All of the value
 * maths, the thumb geometry and the theme lookup live in Lotte; this file only
 * translates an options object into a widget and a widget back into properties, which
 * is the same split `GUI.Button` uses.
 */
#include <JS/JsUtil.hpp>

namespace Sandbox {
class ScriptHost;
}

namespace JS {

/**
 * @brief Adds the `Slider` class to an existing `GUI` module object.
 * @param cx Context that owns every object created here.
 * @param global Module object to define the class on.
 * @param host Engine services the class talks to; must outlive the context.
 */
void RegisterGuiSliderClass(JSContext* cx, const JS::HandleObject& global, Sandbox::ScriptHost& host);

}

#endif // JSGUISLIDER_HPP
