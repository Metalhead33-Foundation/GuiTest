TEMPLATE = app
CONFIG += console c++2a
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -fconcepts-diagnostics-depth=16

LIBS += -lmozjs-115 -lX11 -lSDL2 -lSDL2_image -lfreetype -fopenmp -ldl -lpng16 -lgif -lturbojpeg -lsndfile -lsamplerate -lopenmpt -lwebp -lwebpdecoder -lwebpdemux  -lwebpmux -lzstd -lsnappy -lphysfs -lssl -lcrypto -lglslang -lSPIRV -lspirv-cross-core -lspirv-cross-glsl -lspirv-cross-reflect -lyaml-cpp
INCLUDEPATH += /usr/include/mozjs-115
INCLUDEPATH += /usr/include/nspr
INCLUDEPATH += $$PWD
INCLUDEPATH += /usr/include/freetype2
INCLUDEPATH += I/usr/include/harfbuzz
DEFINES += WINSYS_X
DEFINES += DESKTOP_GL

debug {
  DEFINES += _DEBUG
}

SOURCES += \
    Elvavena/Io/ElvIoDeviceProxy.cpp \
    Elvavena/Util/ElvDynamicLib.cpp \
    Elvavena/Util/ElvEventLoop.cpp \
    Euphemy/Config/EuphConfiguration.cpp \
    Euphemy/Config/EuphConfigurationElement.cpp \
    Euphemy/Io/EuphBufferDevice.cpp \
    Euphemy/Io/EuphCryptoProxyDevice.cpp \
    Euphemy/Io/EuphFile.cpp \
    Euphemy/Io/EuphMemoryMapped.cpp \
    Euphemy/Io/EuphNullDevice.cpp \
    Euphemy/Io/EuphPlatformDependentFileBase.cpp \
    Euphemy/Io/EuphRandomDevice.cpp \
    Euphemy/Io/EuphSocket.cpp \
    Euphemy/Io/EuphStdfileWrapper.cpp \
    Euphemy/Io/EuphTempFile.cpp \
    Euphemy/Io/EuphZstd.cpp \
    Euphemy/Io/PhysFS.cpp \
    Euphemy/Memory/EuphMemory.cpp \
    main.cpp \
    test.cpp

HEADERS += \
    Elvavena/Io/ElvDataStream.hpp \
    Elvavena/Io/ElvIoDevice.hpp \
    Elvavena/Io/ElvIoDeviceProxy.hpp \
    Elvavena/Io/ElvIoSystem.hpp \
    Elvavena/Util/ElvAlexandrescuResource.hpp \
    Elvavena/Util/ElvAllocatorBasic.hpp \
    Elvavena/Util/ElvBinaryTree.hpp \
    Elvavena/Util/ElvBitmapAllocator.hpp \
    Elvavena/Util/ElvChunkyArray.hpp \
    Elvavena/Util/ElvCommandQueue.hpp \
    Elvavena/Util/ElvContainerBasic.hpp \
    Elvavena/Util/ElvContinuousIterator.hpp \
    Elvavena/Util/ElvDynamicLib.hpp \
    Elvavena/Util/ElvEndianness.hpp \
    Elvavena/Util/ElvEventLoop.hpp \
    Elvavena/Util/ElvException.hpp \
    Elvavena/Util/ElvFixedPoint.hpp \
    Elvavena/Util/ElvFixedPointMath.hpp \
    Elvavena/Util/ElvFreelist.hpp \
    Elvavena/Util/ElvHeapArray.hpp \
    Elvavena/Util/ElvIntegralIterator.hpp \
    Elvavena/Util/ElvInterpolation.hpp \
    Elvavena/Util/ElvMathUtil.hpp \
    Elvavena/Util/ElvNormDenorm.hpp \
    Elvavena/Util/ElvObserv.hpp \
    Elvavena/Util/ElvSpanHelpers.hpp \
    Elvavena/Util/ElvStackAllocator.hpp \
    Elvavena/Util/ElvThreadsafeContainer.hpp \
    Elvavena/Util/ElvThreadsafeQueue.hpp \
    Elvavena/Util/ElvUtilGlobals.hpp \
    Elvavena/Util/half.hpp \
    Euphemy/Config/EuphConfiguration.hpp \
    Euphemy/Config/EuphConfigurationElement.hpp \
    Euphemy/Config/EuphLib.hpp \
    Euphemy/Config/GlobalConfig.hpp \
    Euphemy/Io/EuphBufferDevice.hpp \
    Euphemy/Io/EuphCryptoProxyDevice.hpp \
    Euphemy/Io/EuphFile.hpp \
    Euphemy/Io/EuphMemoryMapped.hpp \
    Euphemy/Io/EuphNullDevice.hpp \
    Euphemy/Io/EuphPlatformDependentFileBase.hpp \
    Euphemy/Io/EuphRandomDevice.hpp \
    Euphemy/Io/EuphSocket.hpp \
    Euphemy/Io/EuphStdfileWrapper.hpp \
    Euphemy/Io/EuphTempFile.hpp \
    Euphemy/Io/EuphZstd.hpp \
    Euphemy/Io/PhysFS.hpp \
    Euphemy/Media/EuphImageDecodeTarget.hpp \
    Euphemy/Media/EuphImageType.hpp \
    Euphemy/Memory/EuphMemory.hpp \
    test.hpp
