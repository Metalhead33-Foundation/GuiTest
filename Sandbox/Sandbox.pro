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
# libyaml fills the same role for the localization documents: `Euph::Loc::Localization`
# is a plain key/value table, and `LocalizationYaml.cpp` is what reads a file into it.
CONFIG += link_pkgconfig
PKGCONFIG += nlohmann_json yaml-0.1

INCLUDEPATH += $$PWD/..

SOURCES += main.cpp sandbox.cpp \
    AppSystem.cpp \
    GameSystem.cpp \
    GuiScene.cpp \
    LocalizationYaml.cpp \
    ThemeDemo.cpp \
    ThemeJson.cpp \
    HarukaContext.cpp \
    JS/JsCore.cpp \
    JS/JsException.cpp \
    JS/JsUtil.cpp \
    JS/Wrappers/Audio/JsAudioPlayer.cpp \
    JS/Wrappers/Audio/JsSound.cpp \
    JS/Wrappers/GUI/JsGuiLabel.cpp \
    JS/Wrappers/GUI/JsGuiLayout.cpp \
    JS/Wrappers/GUI/JsGuiSlider.cpp \
    JS/Wrappers/GUI/JsGuiWrappers.cpp \
    JS/Wrappers/Io/JsIO.cpp \
    JS/Wrappers/Localization/JsLocalization.cpp \
    JS/Wrappers/System/JsSystem.cpp \
    JS/Wrappers/Util/JsRandom.cpp
HEADERS += sandbox.hpp \
    AppSystem.hpp \
    GameSystem.hpp \
    GuiScene.hpp \
    HarukaContext.hpp \
    LocalizationYaml.hpp \
    ThemeDemo.hpp \
    ThemeJson.hpp \
    ScriptHost.hpp \
    JS/JsCore.hpp \
    JS/JsException.hpp \
    JS/JsUtil.hpp \
    JS/Wrappers/Audio/JsAudioPlayer.hpp \
    JS/Wrappers/Audio/JsSound.hpp \
    JS/Wrappers/GUI/JsGuiLabel.hpp \
    JS/Wrappers/GUI/JsGuiLayout.hpp \
    JS/Wrappers/GUI/JsGuiSlider.hpp \
    JS/Wrappers/GUI/JsGuiWrappers.hpp \
    JS/Wrappers/Io/JsIO.hpp \
    JS/Wrappers/Localization/JsLocalization.hpp \
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
# Localization documents travel with the executable: the script reads them through
# PhysFS as `/localization/<language>/*.yml`. One copy rule per language keeps the
# directory structure, because `$$files()` on a recursive glob would flatten it and
# the loader looks the files up by language directory.
localization_english.files = $$files($$PWD/localization/english/*.yml)
localization_english.path = $$OUT_PWD/localization/english
COPIES += localization_english
gfx_fld.files = $$files($$PWD/gfx/*)
gfx_fld.path = $$OUT_PWD/gfx
COPIES += gfx_fld

DISTFILES += \
    README.md \
    scripts/main.js
