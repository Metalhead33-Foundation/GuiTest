#ifndef JSLOCALIZATION_HPP
#define JSLOCALIZATION_HPP
/**
 * @file JsLocalization.hpp
 * @brief Declares the `Localization` module: the loaded string table as properties.
 *
 * The module object holds one read-only, enumerable property per localization key, so
 * a script can write `L.menu_new_game` and a typo shows up as `undefined` rather than
 * as an exception. Nothing about the *format* leaks here: the table was filled by
 * `Sandbox::parseLocalizationYaml()` and this file only publishes it.
 *
 * Keys are copied verbatim, which means a key that is not a valid JavaScript
 * identifier (`menu.exit`) can only be reached through `get()`; the dotted form is
 * discouraged for that reason and every shipped table uses underscores.
 */
#include <JS/JsUtil.hpp>

namespace Sandbox {
class ScriptHost;
}

namespace JS {

/**
 * @brief Fills a module object with the loaded localization entries.
 * @param cx Context that owns every object created here.
 * @param global Module object to define the properties on.
 * @param host Engine services the module talks to; must outlive the context.
 */
void RegisterLocalizationModule(JSContext* cx, const JS::HandleObject& global, Sandbox::ScriptHost& host);

}

#endif // JSLOCALIZATION_HPP
