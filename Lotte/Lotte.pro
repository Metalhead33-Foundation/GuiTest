TEMPLATE = lib
TARGET = Lotte
CONFIG += shared c++2a
CONFIG -= qt

DEFINES += MH_LOTTE_EXPORT
INCLUDEPATH += $$PWD/..
LIBS += -L$$OUT_PWD/../Elvavena -lElvavena
LIBS += -L$$OUT_PWD/../Euphemy -lEuphemy
# glslang compiles HLSL/GLSL to SPIR-V. `glslang-default-resource-limits` provides
# the upstream default TBuiltInResource that compileShaderProgram() hands to
# TShader::parse(); linking it avoids maintaining our own copy of the limits table.
LIBS += -lglslang -lSPIRV -lglslang-default-resource-limits
# Kaldi is used by Lotte as a header-only dependency. Do not add -lKaldi
# or libKaldi here; there is no Kaldi library target to link against.
#
# utfcpp (https://github.com/nemtrif/utfcpp) is likewise header-only and is
# consumed as <utf8cpp/utf8.h> from the system include path, so no -l or
# separate INCLUDEPATH is required.

SOURCES += \
    Asset/LteGpuAssetStreamer.cpp \
    Conf/LteBackendConfig.cpp \
    GFX/LteClipSpaceConventions.cpp \
    GFX/LteKaldiContext.cpp \
    GFX/LtePipeline.cpp \
    GFX/LteResourceFactory.cpp \
    GFX/LteWriteableTexture2D.cpp \
    Gui/LteCursor.cpp \
    Gui/LteKaldiGuiRenderer.cpp \
    Text/LteHtmlEntities.cpp \
    Text/LteMarkupColour.cpp \
    Text/LteMarkupStack.cpp \
    Text/LteMml.cpp \
    Text/LteRichText.cpp \
    Text/LteTextLayout.cpp

HEADERS += \
    Asset/LteGpuAssetStreamer.hpp \
    Conf/LteBackendConfig.hpp \
    GFX/LteClipSpaceConventions.hpp \
    GFX/LteImageConvert.hpp \
    GFX/LteKaldiContext.hpp \
    GFX/LteMesh.hpp \
    GFX/LtePipeline.hpp \
    GFX/LteResourceFactory.hpp \
    GFX/LteWriteableTexture2D.hpp \
    Gui/LteCursor.hpp \
    Gui/LteGuiRenderCache.hpp \
    Gui/LteGuiRenderer.hpp \
    Gui/LteGuiVertex.hpp \
    Gui/LteKaldiGuiRenderer.hpp \
    LteKld.hpp \
    Text/LteFontAtlas.hpp \
    Text/LteFontFace.hpp \
    Text/LteHtmlEntities.hpp \
    Text/LteMarkupColour.hpp \
    Text/LteMarkupStack.hpp \
    Text/LteMml.hpp \
    Text/LteRichText.hpp \
    Text/LteTextAttributes.hpp \
    Text/LteTextLayout.hpp \
    Text/LteUtf.hpp \
    VectorGraphic/LteStoredVectorImage.hpp \
    LotteLib.hpp

DISTFILES += \
    Lotte.pri \
    README.md \
    Text/MML.md
