INCLUDEPATH += $$quote($$PWD/..)
# Use a relative path from the root of the build tree
ELV_BUILD_DIR = $$shadowed($$PWD)
LIBS += -L$$ELV_BUILD_DIR -lElvavena
