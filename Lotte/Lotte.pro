TEMPLATE = lib
TARGET = Lotte
CONFIG += shared c++2a
CONFIG -= qt

DEFINES += MH_LOTTE_EXPORT
INCLUDEPATH += $$PWD/..
LIBS += -L$$OUT_PWD/../Elvavena -lElvavena
LIBS += -L$$OUT_PWD/../Euphemy -lEuphemy
# Kaldi is used by Lotte as a header-only dependency. Do not add -lKaldi
# or libKaldi here; there is no Kaldi library target to link against.

SOURCES += \
    Asset/LteGpuAssetStreamer.cpp

HEADERS += \
    Asset/LteGpuAssetStreamer.hpp \
    LteKld.hpp \
    Text/LteFontFace.hpp \
    VectorGraphic/LteStoredVectorImage.hpp \
    LotteLib.hpp

DISTFILES += \
    Lotte.pri \
    README.md
