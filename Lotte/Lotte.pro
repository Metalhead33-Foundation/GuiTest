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
# Also, glslang's HLSL part is deprecated and will be removed in the future.
# We might replace it with DXC or make HLSL-to-SPIRV compilations offline-only in the future.
LIBS += -lglslang -lSPIRV -lglslang-default-resource-limits
# Kaldi is used by Lotte as a header-only dependency. Do not add -lKaldi
# or libKaldi here; there is no Kaldi library target to link against.
#
# utfcpp (https://github.com/nemtrif/utfcpp) is likewise header-only and is
# consumed as <utf8cpp/utf8.h> from the system include path, so no -l or
# separate INCLUDEPATH is required.

# The Assimp dependency is to be reconsidered in the future.
# We MIGHT create tooling that converts assimp-importable assets into our own bespoke format.
# Might. No guarantees. Still, to be considered, to keep the library lean.
LIBS += -lassimp

# CSS support for MML is optional. libcss (NetSurf) is a C library needed by one
# module only, and `Text/LteCssStyle.cpp` compiles to nothing without the define,
# so the library keeps building on hosts that do not have it installed. The vendored
# checkout under extern/netsurf is preferred over the host's copy; see the .pri.
include($$PWD/../extern/netsurf/netsurf.pri)


SOURCES += \
    Asset/LteAssimpIO.cpp \
    Asset/LteGpuAssetStreamer.cpp \
    Conf/LteBackendConfig.cpp \
    GFX/LteClipSpaceConventions.cpp \
    GFX/LteKaldiContext.cpp \
    GFX/LtePipeline.cpp \
    GFX/LteResourceFactory.cpp \
    GFX/LteWriteableTexture2D.cpp \
    Gui/LteCursor.cpp \
    Gui/LteGuiRenderable.cpp \
    Gui/LteGuiRootNode.cpp \
    Gui/LteTexturedWidget.cpp \
    Gui/LteWidget.cpp \
    Gui/LteKaldiGuiRenderer.cpp \
    Gui/LteThemeDefinition.cpp \
    Gui/LteThemeCss.cpp \
    Procedural/LteGuiAtlas.cpp \
    Procedural/LteGuiMaterials.cpp \
    Procedural/LteSurfaceCanvas.cpp \
    Text/LteCssStyle.cpp \
    Text/LteFontAtlas.cpp \
    Text/LteFontRepository.cpp \
    Text/LteHtmlEntities.cpp \
    Text/LteMarkupColour.cpp \
    Text/LteMarkupStack.cpp \
    Text/LteMml.cpp \
    Text/LteRichText.cpp \
    Text/LteTextLayout.cpp \
    Text/LteTextRenderer.cpp

HEADERS += \
    Asset/LteAssimpIO.hpp \
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
    Gui/LteGuiRenderable.hpp \
    Gui/LteGuiRootNode.hpp \
    Gui/LteTexturedWidget.hpp \
    Gui/LteTheme.hpp \
    Gui/LteWidget.hpp \
    Gui/LteGuiRenderCache.hpp \
    Gui/LteGuiRenderer.hpp \
    Gui/LteGuiVertex.hpp \
    Gui/LteKaldiGuiRenderer.hpp \
    Gui/LteThemeDefinition.hpp \
    Gui/LteThemeCss.hpp \
    Procedural/LteGuiAtlas.hpp \
    Procedural/LteGuiAtlasParts.hpp \
    Procedural/LteGuiMaterials.hpp \
    Procedural/LteSdf.hpp \
    Procedural/LteSurfaceCanvas.hpp \
    LteKld.hpp \
    Text/LteFontAtlas.hpp \
    Text/LteFontFace.hpp \
    Text/LteFontRepository.hpp \
    Text/LteFontWeight.hpp \
    Text/LteHtmlEntities.hpp \
    Text/LteCssStyle.hpp \
    Text/LteMarkupColour.hpp \
    Text/LteMarkupStack.hpp \
    Text/LteMml.hpp \
    Text/LteRichText.hpp \
    Text/LteTextAttributes.hpp \
    Text/LteTextLayout.hpp \
    Text/LteTextRenderer.hpp \
    Text/LteUtf.hpp \
    VectorGraphic/LteStoredVectorImage.hpp \
    LotteLib.hpp

DISTFILES += \
    ../docs/text/MML_CSS.md \
    ../docs/text/index.md \
    Lotte.pri \
    README.md \
    Text/MML.md
