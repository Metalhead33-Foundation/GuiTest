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
LIBS += -lSDL2

INCLUDEPATH += $$PWD/..

SOURCES += main.cpp sandbox.cpp \
    AppSystem.cpp \
    GameSystem.cpp \
    HarukaContext.cpp
HEADERS += sandbox.hpp \
    AppSystem.hpp \
    GameSystem.hpp \
    HarukaContext.hpp

# The Sandbox renders its cursor images from `cursors/`; qmake's file_copies
# feature mirrors them into the build directory next to the executable, so the
# working directory of a run-from-build-dir launch finds `cursors/*.png`.
CONFIG += file_copies
cursorImages.files = $$files($$PWD/cursors/*.png)
cursorImages.path = $$OUT_PWD/cursors
COPIES += cursorImages
