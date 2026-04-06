TEMPLATE = lib
TARGET = Lotte 
CONFIG += shared c++2a
CONFIG -= qt

DEFINES += MH_LOTTE_EXPORT
LIBS += -L$$OUT_PWD/../Elvavena -lElvavena
LIBS += -L$$OUT_PWD/../Euphemy -lEuphemy
LIBS += -L$$OUT_PWD/../Kaldi -lKaldi

# Avoid build errors with no files:
SOURCES += 
HEADERS += \
    LotteLib.hpp

DISTFILES += \
    Lotte.pri \
    README.md
