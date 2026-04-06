TEMPLATE = aux
CONFIG += interface c++2a no_link
TARGET = Kaldi 
CONFIG -= qt

# Prevent the creation of any actual library files
# This makes the 'install' and 'build' steps do nothing for the binary
LIBS = 
DESTDIR =

include(Kaldi.pri)

# Avoid build errors with no files:
HEADERS += \
    KaldiOperation.hpp \
    KldVertexFormat.hpp

DISTFILES += \
    Kaldi.pri \
    README.md
