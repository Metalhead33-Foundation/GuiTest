#ifndef JSGUIWRAPPERS_HPP
#define JSGUIWRAPPERS_HPP
#include <JS/JsUtil.hpp>
namespace JS {
void RegisterGuiClasses(JSContext* cx, const JS::HandleObject &global);
}

#endif // JSGUIWRAPPERS_HPP
