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
# The device provider composes the context provider with the executor policy keys.
SOURCES += ../KaldiGLCommon/GlDeviceConfig.cpp

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

# ─────────────────────────────────────────────────────────────────────────────
# HLSL -> SPIR-V at build time
#
# HLSL is the project's primary shading language: it is compiled to SPIR-V here
# with glslang, and the resulting blobs are what the unit tests hand to the same
# SPIR-V -> GLSL cross-compiler the OpenGL backends use. That makes the whole
# "one shader language, every backend" pipeline a build-time, verified fact
# instead of a claim in a README.
#
# This mirrors the WasmJit test harness, which compiles AssemblyScript sources
# into .wasm modules with `QMAKE_EXTRA_COMPILERS` and loads them at run time.
# Override the compiler from the command line:
#     qmake UnitTests.pro GLSLANG=/opt/glslang/bin/glslang
#
# NOTE: glslang's HLSL front-end is deprecated upstream (April 2026; removed at the
# next major version of glslang, see KhronosGroup/glslang#4210). This rule is kept
# because it is the only build-time proof that the shader assets compile at all.
# The intended replacement is DXC with `-HV 2016` plus its legacy flags, which keeps
# the accepted language at Shader Model 4/5 level even though DXC only emits Shader
# Model 6 profiles. The migration is deferred indefinitely; `docs/SHADERS.md` has the
# reference `dxc` invocation, the flag-by-flag rationale and the acceptance gate.
# ─────────────────────────────────────────────────────────────────────────────
isEmpty(GLSLANG) {
    GLSLANG = /usr/bin/glslang
}

SHADER_BUILD_DIR = $$OUT_PWD/shaders

exists($$GLSLANG) {
    DEFINES += KALDI_TEST_HAS_HLSL_SHADERS
    DEFINES += KALDI_TEST_SHADER_DIR=\\\"$$SHADER_BUILD_DIR/\\\"

    HLSL_VERTEX_SOURCES = $$files($$PWD/shaders/*.vert.hlsl)
    HLSL_FRAGMENT_SOURCES = $$files($$PWD/shaders/*.frag.hlsl)
    HLSL_COMPUTE_SOURCES = $$files($$PWD/shaders/*.comp.hlsl)

    hlsl_vert.name = HLSL vertex $$QMAKE_FILE_IN
    hlsl_vert.input = HLSL_VERTEX_SOURCES
    hlsl_vert.output = $$SHADER_BUILD_DIR/${QMAKE_FILE_BASE}.spv
    hlsl_vert.commands = $$QMAKE_MKDIR $$SHADER_BUILD_DIR && $$GLSLANG -D -V --target-env vulkan1.0 -S vert -e main ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
    hlsl_vert.dependency_type = TYPE_C
    hlsl_vert.CONFIG += no_link target_predeps
    QMAKE_EXTRA_COMPILERS += hlsl_vert

    hlsl_frag.name = HLSL fragment $$QMAKE_FILE_IN
    hlsl_frag.input = HLSL_FRAGMENT_SOURCES
    hlsl_frag.output = $$SHADER_BUILD_DIR/${QMAKE_FILE_BASE}.spv
    hlsl_frag.commands = $$QMAKE_MKDIR $$SHADER_BUILD_DIR && $$GLSLANG -D -V --target-env vulkan1.0 -S frag -e main ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
    hlsl_frag.dependency_type = TYPE_C
    hlsl_frag.CONFIG += no_link target_predeps
    QMAKE_EXTRA_COMPILERS += hlsl_frag

    hlsl_comp.name = HLSL compute $$QMAKE_FILE_IN
    hlsl_comp.input = HLSL_COMPUTE_SOURCES
    hlsl_comp.output = $$SHADER_BUILD_DIR/${QMAKE_FILE_BASE}.spv
    hlsl_comp.commands = $$QMAKE_MKDIR $$SHADER_BUILD_DIR && $$GLSLANG -D -V --target-env vulkan1.0 -S comp -e main ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
    hlsl_comp.dependency_type = TYPE_C
    hlsl_comp.CONFIG += no_link target_predeps
    QMAKE_EXTRA_COMPILERS += hlsl_comp
} else {
    warning("glslang was not found at $$GLSLANG; the HLSL->SPIR-V shader tests are disabled")
}

INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD

SOURCES += \
    tests/AssetRuntimeTests.cpp \
    tests/BackendConfigTests.cpp \
    tests/BitStreamTests.cpp \
    tests/CompressionTests.cpp \
    tests/ConfigTests.cpp \
    tests/CursorTests.cpp \
    tests/FontRoundTripTests.cpp \
    tests/FormatTranslationTests.cpp \
    tests/HarukaStreamerRingTests.cpp \
    tests/HttpTests.cpp \
    tests/ImageCodecTests.cpp \
    tests/KaldiCommandBufferTests.cpp \
    tests/KaldiDispatchSwitchTests.cpp \
    tests/KaldiDispatchTests.cpp \
    tests/KaldiGuiRendererTests.cpp \
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
    tests/ShaderAssetTests.cpp \
    tests/ShaderCompileTests.cpp \
    tests/UdpTransportTests.cpp \
    tests/WriteableTexture2DTests.cpp

HEADERS += \
    support/DispatchProbe.hpp \
    support/TestSupport.hpp

DISTFILES += \
    README.md \
    shaders/triangle.vert.hlsl \
    shaders/triangle.frag.hlsl \
    shaders/compute.comp.hlsl
