TEMPLATE = lib
TARGET = Haruka
CONFIG += shared c++2a
CONFIG -= qt

DEFINES += MH_HARUKA_EXPORT
# Avoid build errors with no files:
SOURCES += 
HEADERS += \
    HarukaLib.hpp

DISTFILES += \
    Haruka.pri \
    README.md
