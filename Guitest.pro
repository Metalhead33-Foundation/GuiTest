TEMPLATE = app
CONFIG += console c++2a
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CFLAGS += -O3 -ffast-math
QMAKE_CXXFLAGS += -O3 -ffast-math
LIBS += -lSDL2

SOURCES += \
        Pipeline/BasicPipeline.cpp \
        Pipeline/EdgeFunction.cpp \
        Pipeline/GuiRenderer.cpp \
        Pipeline/ModelPipeline.cpp \
        Pipeline/TexturedPipeline.cpp \
        Pipeline/ZBuffer.cpp \
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
	Pipeline/BasicPipeline.hpp \
	Pipeline/EdgeFunction.hpp \
	Pipeline/GuiRenderer.hpp \
	Pipeline/ModelPipeline.hpp \
	Pipeline/RenderingPipeline.hpp \
	Pipeline/TexturedPipeline.hpp \
	Pipeline/ZBuffer.hpp \
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
	Util/ThreadsafeContainer.hpp \
	Widget/BoxWidget.hpp \
	Widget/IWidget.hpp \
	Widget/SimpleWidget.hpp \
	Widget/TexturedWidget.hpp \
	Widget/TickboxWidget.hpp
