INCLUDEPATH += $$quote($$PWD/..)
# Use a relative path from the root of the build tree
LOTTE_BUILD_DIR = $$shadowed($$PWD)
LIBS += -L$$LOTTE_BUILD_DIR -lHaruka
