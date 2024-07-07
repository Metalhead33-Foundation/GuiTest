#ifndef JSRANDOM_H
#define JSRANDOM_H
#include <jsapi.h>
#include <random>
#include <js/CompilationAndEvaluation.h>
#include <js/SourceText.h>
#include <js/Initialization.h>
#include <js/Exception.h>
#include <js/Class.h>
#include <js/Proxy.h>

namespace JS {
bool CreateRandomIzerClass(JSContext* cx, const JS::HandleObject &global);
}
#endif // JSRANDOM_H
