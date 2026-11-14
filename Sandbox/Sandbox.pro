TEMPLATE = app

# Manual playground application: see README.md. Builds on the shared libraries only.
CONFIG += console c++2a
CONFIG -= qt

TARGET = Sandbox

# Link to our local libraries
LIBS += -L$$OUT_PWD/../Elvavena -lElvavena
LIBS += -L$$OUT_PWD/../Euphemy -lEuphemy
LIBS += -L$$OUT_PWD/../Lotte -lLotte
LIBS += -L$$OUT_PWD/../Haruka -lHaruka

INCLUDEPATH += $$PWD/..

SOURCES += main.cpp sandbox.cpp
HEADERS += sandbox.hpp
