TEMPLATE = lib
TARGET = KaldiVK 
CONFIG += shared c++2a
CONFIG -= qt

INCLUDEPATH += $$PWD/..
DEFINES += MH_KALDI_IMPL_EXPORT
include(../Kaldi.pri)

# Avoid build errors with no files:
SOURCES +=  \
    KaldiVulkanDevice.cpp
HEADERS += \
    KaldiVulkanDevice.hpp \
    KldImplLib.h

DISTFILES += \
    README.md
