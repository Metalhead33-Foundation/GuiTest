INCLUDEPATH += $$quote($$PWD/..)
# Use a relative path from the root of the build tree
LOTTE_BUILD_DIR = $$shadowed($$PWD)
ELVAVENA_BUILD_DIR = $$shadowed($$PWD/../Elvavena)
EUPHEMY_BUILD_DIR = $$shadowed($$PWD/../Euphemy)
KALDI_BUILD_DIR = $$shadowed($$PWD/../Kaldi)
LIBS += -L$$LOTTE_BUILD_DIR -lLotte
LIBS += -L$$EUPHEMY_BUILD_DIR -lEuphemy
LIBS += -L$$KALDI_BUILD_DIR -lKaldi
LIBS += -L$$ELVAVENA_BUILD_DIR -lElvavena
