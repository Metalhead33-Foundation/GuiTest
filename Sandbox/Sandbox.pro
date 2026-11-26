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

# nlohmann_json is the theme/procedural data parser. It is deliberately only linked
# here (and in UnitTests): no library of ours may depend on a JSON parser, and no
# algorithm works on JSON directly - the loaders in this directory parse into the
# plain structs of `Lotte::Gui::ThemeDefinition`.
CONFIG += link_pkgconfig
PKGCONFIG += nlohmann_json

INCLUDEPATH += $$PWD/..

SOURCES += main.cpp sandbox.cpp \
    AppSystem.cpp \
    GameSystem.cpp \
    GuiScene.cpp \
    ThemeDemo.cpp \
    ThemeJson.cpp \
    HarukaContext.cpp \
    JS/JsCore.cpp \
    JS/JsException.cpp \
    JS/JsUtil.cpp \
    JS/Wrappers/Audio/JsAudioPlayer.cpp \
    JS/Wrappers/GUI/JsGuiWrappers.cpp \
    JS/Wrappers/Io/JsIO.cpp \
    JS/Wrappers/System/JsSystem.cpp \
    JS/Wrappers/Util/JsRandom.cpp
HEADERS += sandbox.hpp \
    AppSystem.hpp \
    GameSystem.hpp \
    GuiScene.hpp \
    HarukaContext.hpp \
    ThemeDemo.hpp \
    ThemeJson.hpp \
    ScriptHost.hpp \
    JS/JsCore.hpp \
    JS/JsException.hpp \
    JS/JsUtil.hpp \
    JS/Wrappers/Audio/JsAudioPlayer.hpp \
    JS/Wrappers/GUI/JsGuiWrappers.hpp \
    JS/Wrappers/Io/JsIO.hpp \
    JS/Wrappers/System/JsSystem.hpp \
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
gfx_fld.files = $$files($$PWD/gfx/*)
gfx_fld.path = $$OUT_PWD/gfx
COPIES += gfx_fld

DISTFILES += \
    README.md \
    scripts/main.js
