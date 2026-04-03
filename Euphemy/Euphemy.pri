INCLUDEPATH += $$quote($$PWD/..)
INCLUDEPATH += /usr/include/mozjs-115 /usr/include/nspr /usr/include/freetype2 .
# Use a relative path from the root of the build tree
EUPH_BUILD_DIR = $$shadowed($$PWD)
LIBS += -L$$EUPH_BUILD_DIR -lEuphemy
