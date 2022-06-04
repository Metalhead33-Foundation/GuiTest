TEMPLATE = app
CONFIG += console c++2a
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CFLAGS += -O3 -ffast-math -I/usr/include/freetype2 -I/usr/include/harfbuzz
QMAKE_CXXFLAGS += -O3 -ffast-math -I/usr/include/freetype2 -I/usr/include/harfbuzz
LIBS += -lSDL2 -lSDL2_image -lfreetype

SOURCES += \
        Pipeline/BasicPipeline.cpp \
        Pipeline/ColouredTexturedPipeline.cpp \
        Pipeline/EdgeFunction.cpp \
        Pipeline/GuiRenderer.cpp \
        Pipeline/ModelPipeline.cpp \
        Pipeline/TexturedPipeline.cpp \
        Pipeline/ZBuffer.cpp \
        System/AppSystem.cpp \
        System/FpsCounter.cpp \
        System/GuiRenderSystem.cpp \
        Text/Font.cpp \
        Text/FontRepository.cpp \
        Text/MmlParser.cpp \
        Text/RichTextProcessor.cpp \
        Text/RtProcessorStack.cpp \
        Texture/Texture.cpp \
        Util/ColourHelper.cpp \
        Util/TextureFromSurface.cpp \
        Util/TextureHelpers.cpp \
        Widget/BoxWidget.cpp \
        Widget/Cursor.cpp \
        Widget/IWidget.cpp \
        Widget/SimpleWidget.cpp \
        Widget/TexturedWidget.cpp \
        Widget/TickboxWidget.cpp \
        main.cpp

HEADERS += \
	Pipeline/BasicPipeline.hpp \
	Pipeline/ColouredTexturedPipeline.hpp \
	Pipeline/EdgeFunction.hpp \
	Pipeline/GuiRenderer.hpp \
	Pipeline/ModelPipeline.hpp \
	Pipeline/RenderingPipeline.hpp \
	Pipeline/TexturedPipeline.hpp \
	Pipeline/ZBuffer.hpp \
	System/AppSystem.hpp \
	System/FpsCounter.hpp \
	System/GuiRenderSystem.hpp \
	Text/Font.hpp \
	Text/FontRepository.hpp \
	Text/MmlParser.hpp \
	Text/RichTextProcessor.hpp \
	Text/RtProcessorStack.hpp \
	Texture/StandardTexture.hpp \
	Texture/StandardTexture.ipp \
	Texture/Texture.hpp \
	Util/ColourHelper.hpp \
	Util/Dither.hpp \
	Util/ITextReceiver.hpp \
	Util/NormDenorm.hpp \
	Util/PixelFormat.hpp \
	Util/TextureFromSurface.hpp \
	Util/TextureHelpers.hpp \
	Util/ThreadsafeContainer.hpp \
	Widget/BoxWidget.hpp \
	Widget/Cursor.hpp \
	Widget/IWidget.hpp \
	Widget/SimpleWidget.hpp \
	Widget/TexturedWidget.hpp \
	Widget/TickboxWidget.hpp

DISTFILES += \
	LICENSE \
	README.md \
	Text/MML.md
