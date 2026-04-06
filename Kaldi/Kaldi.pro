TEMPLATE = lib
CONFIG += interface c++2a
TARGET = Kaldi 
CONFIG -= qt

# Avoid build errors with no files:
HEADERS +=

DISTFILES += \
    Kaldi.pri \
    README.md
