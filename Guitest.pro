TEMPLATE = app
CONFIG += console c++2a
CONFIG -= app_bundle
CONFIG -= qt
LIBS += -lSDL2

SOURCES += \
        System/AppSystem.cpp \
        System/GuiRenderSystem.cpp \
        Texture/Texture.cpp \
        Util/TextureFromSurface.cpp \
        Util/TextureHelpers.cpp \
        Widget/BoxWidget.cpp \
        Widget/IWidget.cpp \
        Widget/SimpleWidget.cpp \
        Widget/TexturedWidget.cpp \
        Widget/TickboxWidget.cpp \
        main.cpp

HEADERS += \
	System/AppSystem.hpp \
	System/GuiRenderSystem.hpp \
	Texture/StandardTexture.hpp \
	Texture/Texture.hpp \
	Util/Dither.hpp \
	Util/ITextReceiver.hpp \
	Util/NormDenorm.hpp \
	Util/PixelFormat.hpp \
	Util/TextureFromSurface.hpp \
	Util/TextureHelpers.hpp \
	Widget/BoxWidget.hpp \
	Widget/IWidget.hpp \
	Widget/SimpleWidget.hpp \
	Widget/TexturedWidget.hpp \
	Widget/TickboxWidget.hpp
