#ifndef JSIO_HPP
#define JSIO_HPP
/**
 * @file JsIO.hpp
 * @brief Declares the `IO` module: file access through `Elv::Io::System`.
 *
 * The legacy wrapper mirrored MhLib's `Io::Device` hierarchy (`File`,
 * `PhysFsFile`, `StdIn`, `StdOut`, `StdErr`). Elvavena has one `Device`
 * interface and one `System` factory, and the sandbox already mounts everything
 * through PhysFS, so one `IO.File` covers what those five classes did.
 */
#include <JS/JsUtil.hpp>

namespace Sandbox {
class ScriptHost;
}

namespace JS {

/**
 * @brief Registers the `IO` module on a module object.
 * @param cx Context that owns every object created here.
 * @param global Module object to define the classes and helpers on.
 * @param host Engine services the module talks to; must outlive the context.
 */
void CreateIOClasses(JSContext* cx, const JS::HandleObject& global, Sandbox::ScriptHost& host);

}

#endif // JSIO_HPP
