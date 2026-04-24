TEMPLATE = lib
TARGET = KaldiVK 
CONFIG += shared c++2a
CONFIG -= qt

DEFINES += MH_KALDI_IMPL_EXPORT

# Avoid build errors with no files:
SOURCES += 
HEADERS += \
    KldImplLib.hpp

DISTFILES += \
    README.md
