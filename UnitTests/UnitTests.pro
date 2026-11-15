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

!win32 {
    DEFINES += KALDID3D_FORMAT_TESTS
    INCLUDEPATH += $$PWD/../extern/DXSDK/Include
    SOURCES += ../KaldiD3DCommon/D3DUtil.cpp
}

INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD

SOURCES += \
    tests/AssetRuntimeTests.cpp \
    tests/CompressionTests.cpp \
    tests/ConfigTests.cpp \
    tests/FontRoundTripTests.cpp \
    tests/BackendConfigTests.cpp \
    tests/FormatTranslationTests.cpp \
    tests/NativeWindowTests.cpp \
    tests/ImageCodecTests.cpp \
    tests/ShaderCompileTests.cpp

HEADERS += \
    support/TestSupport.hpp

DISTFILES += README.md
