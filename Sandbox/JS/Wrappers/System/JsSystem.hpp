#ifndef JSSYSTEM_HPP
#define JSSYSTEM_HPP
/**
 * @file JsSystem.hpp
 * @brief Declares the `System` module: textures, rich text and viewport facts.
 *
 * The module is what makes the sandbox script the source of truth for the demo:
 * the text block, the button skin and the window size all come from here instead
 * of being compiled into `GameSystem`.
 */
#include <JS/JsUtil.hpp>
#include <memory>

namespace Sandbox {
class ScriptHost;
class ScriptTexture;
}

namespace JS {

/**
 * @brief Registers the `System` module on a module object.
 * @param cx Context that owns every object created here.
 * @param global Module object to define the classes and functions on.
 * @param host Engine services the module talks to; must outlive the context.
 */
void RegisterSystemClasses(JSContext* cx, const JS::HandleObject& global, Sandbox::ScriptHost& host);

/**
 * @brief Returns the registered `System.Texture` class.
 * @return The class creator, or null before @ref RegisterSystemClasses ran.
 */
const ClassCreator* SystemTextureClass();

/**
 * @brief Extracts the native texture behind a `System.Texture` wrapper.
 * @param cx Context the value belongs to.
 * @param value Candidate wrapper value.
 * @return The shared texture, or null when @p value is not a texture wrapper.
 */
std::shared_ptr<Sandbox::ScriptTexture> textureFromValue(JSContext* cx, JS::HandleValue value);

}

#endif // JSSYSTEM_HPP
