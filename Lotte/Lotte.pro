TEMPLATE = lib
TARGET = Lotte 
CONFIG += shared c++2a
CONFIG -= qt

DEFINES += MH_LOTTE_EXPORT
INCLUDEPATH += $$PWD/..
LIBS += -L$$OUT_PWD/../Elvavena -lElvavena
LIBS += -L$$OUT_PWD/../Euphemy -lEuphemy
LIBS += -L$$OUT_PWD/../Kaldi -lKaldi

SOURCES += \
    Asset/LteGpuAssetStreamer.cpp

HEADERS += \
    Asset/LteGpuAssetStreamer.hpp \
    Text/LteFontFace.hpp \
    LotteLib.hpp

DISTFILES += \
    Lotte.pri \
    README.md
