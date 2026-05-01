TEMPLATE = lib
TARGET = Elvavena
CONFIG += shared c++2a
CONFIG -= qt

DEFINES += MH_UTIL_EXPORT

INCLUDEPATH += $$PWD/..

SOURCES += \
    Io/ElvIoDeviceProxy.cpp \
    Util/ElvDynamicLib.cpp \
    Util/ElvEventLoop.cpp \
    Util/ElvThreadPool.cpp

HEADERS += \
    Io/ElvDataStream.hpp \
    Io/ElvIoDevice.hpp \
    Io/ElvIoDeviceProxy.hpp \
    Io/ElvIoSystem.hpp \
    Util/ElvAlexandrescuResource.hpp \
    Util/ElvAllocatorBasic.hpp \
    Util/ElvBinaryTree.hpp \
    Util/ElvBitmapAllocator.hpp \
    Util/ElvChunkyArray.hpp \
    Util/ElvCommandQueue.hpp \
    Util/ElvContainerBasic.hpp \
    Util/ElvContinuousIterator.hpp \
    Util/ElvDither.hpp \
    Util/ElvDynamicLib.hpp \
    Util/ElvEdgeFunction.hpp \
    Util/ElvEndianness.hpp \
    Util/ElvEventLoop.hpp \
    Util/ElvException.hpp \
    Util/ElvFixedPoint.hpp \
    Util/ElvFixedPointMath.hpp \
    Util/ElvFreelist.hpp \
    Util/ElvFuzzy.hpp \
    Util/ElvHeapArray.hpp \
    Util/ElvIntegralIterator.hpp \
    Util/ElvInterpolation.hpp \
    Util/ElvIntrusiveArray.hpp \
    Util/ElvMathUtil.hpp \
    Util/ElvNormDenorm.hpp \
    Util/ElvObserv.hpp \
    Util/ElvSpanHelpers.hpp \
    Util/ElvStackAllocator.hpp \
    Util/ElvStringhashMap.hpp \
    Util/ElvThreadPool.hpp \
    Util/ElvThreadsafeContainer.hpp \
    Util/ElvThreadsafeQueue.hpp \
    Util/ElvUtilGlobals.hpp \
    Util/half.hpp

DISTFILES += \
    Elvavena.pri \
    README.md
