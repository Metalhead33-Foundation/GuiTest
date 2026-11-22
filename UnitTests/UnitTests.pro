TEMPLATE = app
CONFIG += console c++2a link_pkgconfig
CONFIG -= qt

TARGET = UnitTests

# Catch2 is consumed from the system installation (headers in <catch2/...>, shared
# libraries providing the framework and main()). The pkg-config module is the portable
# way to reach both; it expands to "-lCatch2Main -lCatch2".
PKGCONFIG += catch2-with-main

# Link to our local libraries
LIBS += -L$$OUT_PWD/../Elvavena -lElvavena
LIBS += -L$$OUT_PWD/../Euphemy -lEuphemy
LIBS += -L$$OUT_PWD/../Lotte -lLotte
LIBS += -L$$OUT_PWD/../Haruka -lHaruka
# VkUtil is a pure translation layer shipped inside the (otherwise stubbed)
# Vulkan backend, so the test binary can link it without creating a device.
LIBS += -L$$OUT_PWD/../KaldiVK -lKaldiVK
INCLUDEPATH += $$PWD/../glad/include
# GlContext.hpp only needs SDL's window-manager structs, so the context tests can
# include it without linking a GL library.
INCLUDEPATH += $$PWD/../KaldiGLCommon

# The DXGI table only needs <dxgiformat.h>, which the apitrace DXSDK mirror ships
# as a self-contained C header (no windows.h). Compiling it straight into the test
# binary on non-Windows hosts verifies the values instead of merely asserting them
# in a README. On Windows the table already lives inside libKaldiD3D11/12, so it is
# skipped here to avoid duplicate symbols.
# The GL config provider is plain translation code (no glad, no context), so the
# configuration chain can be tested without linking a GL library.
SOURCES += ../KaldiGLCommon/GlContextConfig.cpp

# SPIR-V -> GLSL is exactly what the GL backends do with Lotte's output. Compiling it
# into the test binary proves the "one Vulkan blob for every backend" pipeline end to
# end without creating a GL context.
SOURCES += ../KaldiGLCommon/GlSpirvShader.cpp
LIBS += -lspirv-cross-core -lspirv-cross-glsl

!win32 {
    DEFINES += KALDID3D_FORMAT_TESTS
    INCLUDEPATH += $$PWD/../extern/DXSDK/Include
    SOURCES += ../KaldiD3DCommon/D3DUtil.cpp
}

INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD

SOURCES += \
    tests/AssetRuntimeTests.cpp \
    tests/BackendConfigTests.cpp \
    tests/BitStreamTests.cpp \
    tests/CompressionTests.cpp \
    tests/ConfigTests.cpp \
    tests/FontRoundTripTests.cpp \
    tests/FormatTranslationTests.cpp \
    tests/HarukaStreamerRingTests.cpp \
    tests/HttpTests.cpp \
    tests/ImageCodecTests.cpp \
    tests/KaldiDispatchSwitchTests.cpp \
    tests/KaldiDispatchTests.cpp \
    tests/KaldiHalTests.cpp \
    tests/NativeWindowTests.cpp \
    tests/NetClockRpcTests.cpp \
    tests/NetPredictionInterpolationTests.cpp \
    tests/NetReplicationPipelineTests.cpp \
    tests/NetReplicationTests.cpp \
    tests/NetSessionTests.cpp \
    tests/NetSnapshotTests.cpp \
    tests/NetTransportTests.cpp \
    tests/PlatformWrappersTests.cpp \
    tests/ShaderCompileTests.cpp \
    tests/UdpTransportTests.cpp

HEADERS += \
    support/DispatchProbe.hpp \
    support/TestSupport.hpp

DISTFILES += README.md
