TEMPLATE = app
CONFIG += console c++2a
CONFIG -= qt

TARGET = MyGameApp

# Link to our local libraries
LIBS += -L$$OUT_PWD/../Elvavena -lElvavena
LIBS += -L$$OUT_PWD/../Euphemy -lEuphemy
LIBS += -L$$OUT_PWD/../Lotte -lLotte

INCLUDEPATH += $$PWD/..

SOURCES += main.cpp test.cpp
HEADERS += test.hpp
