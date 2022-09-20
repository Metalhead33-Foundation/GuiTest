TEMPLATE = app
CONFIG += console c++2a
CONFIG -= app_bundle
CONFIG -= qt
INCLUDEPATH += $$PWD
QMAKE_CFLAGS += -O3 -fopenmp -ffast-math -I/usr/include/freetype2 -I/usr/include/harfbuzz
QMAKE_CXXFLAGS += -O3 -fopenmp -ffast-math -I/usr/include/freetype2 -I/usr/include/harfbuzz
LIBS += -lSDL2 -lSDL2_image -lfreetype -fopenmp -ldl

SOURCES += \
	AGL/GlGui.cpp \
	AGL/GlTexture2D.cpp \
        EGL/EglContext.cpp \
        EGL/EglDisplay.cpp \
        EGL/egl.c \
        GL/GlBuffer.cpp \
        GL/GlFramebuffer.cpp \
        GL/GlRenderbuffer.cpp \
        GL/GlSampler.cpp \
        GL/GlShader.cpp \
        GL/GlShaderProgram.cpp \
        GL/GlTexture.cpp \
        GL/GlVAO.cpp \
        GL/gles2.c \
	Pipeline/GuiRenderer.cpp \
	Pipeline/IFontTexture.cpp \
	SoftPipeline/BasicPipeline.cpp \
	SoftPipeline/ColouredTexturedPipeline.cpp \
	SoftPipeline/EdgeFunction.cpp \
	SoftPipeline/ModelPipeline.cpp \
	SoftPipeline/TexturedPipeline.cpp \
	SoftPipeline/ZBuffer.cpp \
	System/AcceleratedGuiRenderSystem.cpp \
        System/AppSystem.cpp \
        System/FpsCounter.cpp \
        System/GuiRenderSystem.cpp \
        Text/Font.cpp \
        Text/FontRepository.cpp \
        Text/MmlParser.cpp \
        Text/RichTextProcessor.cpp \
        Text/RtProcessorStack.cpp \
        Texture/Texture.cpp \
        Texture/TextureAtlas.cpp \
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
	AGL/GlGui.hpp \
	AGL/GlTexture2D.hpp \
	EGL/EglContext.hpp \
	EGL/EglDisplay.hpp \
	EGL/egl.h \
	EGL/eglplatform.h \
	GL/GlBuffer.hpp \
	GL/GlBufferArray.hpp \
	GL/GlFramebuffer.hpp \
	GL/GlFramebufferArray.hpp \
	GL/GlRenderbuffer.hpp \
	GL/GlRenderbufferArray.hpp \
	GL/GlSampler.hpp \
	GL/GlSamplerArray.hpp \
	GL/GlShader.hpp \
	GL/GlShaderProgram.hpp \
	GL/GlTexture.hpp \
	GL/GlTextureArray.hpp \
	GL/GlVAO.hpp \
	GL/GlVaoArray.hpp \
	GL/gles2.h \
	GL/khrplatform.h \
	Pipeline/GuiRenderer.hpp \
	Pipeline/IFontTexture.hpp \
	Pipeline/ITexture.hpp \
	Pipeline/ITextureAtlas.hpp \
	SoftPipeline/BasicPipeline.hpp \
	SoftPipeline/ColouredTexturedPipeline.hpp \
	SoftPipeline/EdgeFunction.hpp \
	SoftPipeline/ModelPipeline.hpp \
	SoftPipeline/RenderingPipeline.hpp \
	SoftPipeline/TexturedPipeline.hpp \
	SoftPipeline/ZBuffer.hpp \
	System/AcceleratedGuiRenderSystem.hpp \
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
	Texture/TextureAtlas.hpp \
	Util/ColourHelper.hpp \
	Util/Dither.hpp \
	Util/ITextReceiver.hpp \
	Util/NormDenorm.hpp \
	Util/PixelFormat.hpp \
	Util/TextureFromSurface.hpp \
	Util/TextureHelpers.hpp \
	Util/ThreadsafeContainer.hpp \
	VertexFormat.hpp \
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
