TEMPLATE = app
CONFIG += console c++2a
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -lSDL2

SOURCES += \
        System/AppSystem.cpp \
        System/IniConfiguration.cpp \
        System/TestSystem.cpp \
        main.cpp

HEADERS += \
    GFX/ImageType.hpp \
    GFX/NativeHandle.hpp \
    GFX/Pipeline.hpp \
    GFX/Texture.hpp \
    GFX/UniformBuffer.hpp \
    GFX/VertexBuffer.hpp \
    GFX/VertexBufferAdapter.hpp \
    GFX/VertexFormat.hpp \
    System/AppSystem.hpp \
    System/IniConfiguration.hpp \
    System/TestSystem.hpp
