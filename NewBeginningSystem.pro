TEMPLATE = app
CONFIG += console c++2a
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -lmozjs-115 -lX11 -lSDL2 -lSDL2_image -lfreetype -fopenmp -ldl -lpng16 -lgif -lturbojpeg -lsndfile -lsamplerate -lopenmpt -lwebp -lwebpdecoder -lwebpdemux  -lwebpmux -lzstd -lsnappy -lphysfs -lssl -lcrypto -lglslang -lSPIRV -lspirv-cross-core -lspirv-cross-glsl -lspirv-cross-reflect -lyaml-cpp
INCLUDEPATH += /usr/include/mozjs-115
INCLUDEPATH += /usr/include/nspr
INCLUDEPATH += $$PWD
INCLUDEPATH += /usr/include/freetype2
INCLUDEPATH += I/usr/include/harfbuzz
DEFINES += WINSYS_X
DEFINES += DESKTOP_GL

debug {
  DEFINES += _DEBUG
}
