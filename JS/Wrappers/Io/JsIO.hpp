#ifndef JSIO_HPP
#define JSIO_HPP
#include <jsapi.h>
#include <random>
#include <js/CompilationAndEvaluation.h>
#include <js/SourceText.h>
#include <js/Initialization.h>
#include <js/Exception.h>
#include <js/Class.h>
#include <js/Proxy.h>
namespace JS {
void CreateIOClasses(JSContext* cx, const JS::HandleObject &global);
}
#endif // JSIO_HPP
