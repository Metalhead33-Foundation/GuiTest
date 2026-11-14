INCLUDEPATH += $$quote($$PWD/..)
# Use a relative path from the root of the build tree
EUPH_BUILD_DIR = $$shadowed($$PWD)
LIBS += -L$$EUPH_BUILD_DIR -lEuphemy
