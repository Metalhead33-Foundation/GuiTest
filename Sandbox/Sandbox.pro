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

# mozjs / SpiderMonkey is a temporary dependency!
# pkg-config --libs --cflags mozjs-153
QMAKE_CXXFLAGS += -isystem /usr/include/mozjs-153
LIBS += -lmozjs-153
INCLUDEPATH += -I/usr/include/nspr

INCLUDEPATH += $$PWD/..

SOURCES += main.cpp sandbox.cpp \
    AppSystem.cpp \
    GameSystem.cpp \
    HarukaContext.cpp \
    JS/JsCore.cpp \
    JS/JsException.cpp \
    JS/JsUtil.cpp \
    JS/Wrappers/Audio/JsAudioPlayer.cpp \
    JS/Wrappers/GUI/JsGuiWrappers.cpp \
    JS/Wrappers/Io/JsIO.cpp \
    JS/Wrappers/Util/JsRandom.cpp
HEADERS += sandbox.hpp \
    AppSystem.hpp \
    GameSystem.hpp \
    HarukaContext.hpp \
    JS/JsCore.hpp \
    JS/JsException.hpp \
    JS/JsUtil.hpp \
    JS/Wrappers/Audio/JsAudioPlayer.hpp \
    JS/Wrappers/GUI/JsGuiWrappers.hpp \
    JS/Wrappers/Io/JsIO.hpp \
    JS/Wrappers/Util/JsRandom.hpp

# The Sandbox renders its cursor images from `cursors/`; qmake's file_copies
# feature mirrors them into the build directory next to the executable, so the
# working directory of a run-from-build-dir launch finds `cursors/*.png`.
CONFIG += file_copies
cursorImages.files = $$files($$PWD/cursors/*.png)
cursorImages.path = $$OUT_PWD/cursors
COPIES += cursorImages
js_scripts.files = $$files($$PWD/scripts/*.js)
js_scripts.path = $$OUT_PWD/scripts
COPIES += js_scripts

DISTFILES += \
    README.md \
    scripts/main.js
