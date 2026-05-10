INCLUDEPATH += $$quote($$PWD/..)
# Use a relative path from the root of the build tree
LOTTE_BUILD_DIR = $$shadowed($$PWD)
ELVAVENA_BUILD_DIR = $$shadowed($$PWD/../Elvavena)
EUPHEMY_BUILD_DIR = $$shadowed($$PWD/../Euphemy)
LIBS += -L$$LOTTE_BUILD_DIR -lLotte
LIBS += -L$$EUPHEMY_BUILD_DIR -lEuphemy
LIBS += -L$$ELVAVENA_BUILD_DIR -lElvavena
# Kaldi is header-only for Lotte consumers. Keep the top-level INCLUDEPATH,
# but do not add -lKaldi or libKaldi here.
