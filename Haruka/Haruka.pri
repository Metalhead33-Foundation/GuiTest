INCLUDEPATH += $$quote($$PWD/..)
# Use a relative path from the root of the build tree
HARU_BUILD_DIR = $$shadowed($$PWD)
LIBS += -L$$HARU_BUILD_DIR -lHaruka
