TEMPLATE = lib
TARGET = KaldiVK 
CONFIG += shared c++2a
CONFIG -= qt

INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../glad/include
DEFINES += MH_KALDI_IMPL_EXPORT
include(../Kaldi/Kaldi.pri)

# `CreateTexture2DFromDecodeTarget` consumes a Euphemy `DecodeTarget`, so the
# backend now has a real link-time dependency on the Euphemy runtime (the GL
# backends get it through GlUtil the same way).
LIBS += -L$$OUT_PWD/../Euphemy -lEuphemy
LIBS += -L$$OUT_PWD/../Elvavena -lElvavena

# glad's vulkan.c defines the platform surface entry points (glad_vkCreateXlibSurfaceKHR,
# ...) only when the matching VK_USE_PLATFORM_* macros are set while *it* is compiled.
# VkPlatform.hpp sets them for our own units; these defines cover glad's C source.
unix:!macx {
    DEFINES += VK_USE_PLATFORM_XLIB_KHR
    DEFINES += VK_USE_PLATFORM_WAYLAND_KHR
}
win32: DEFINES += VK_USE_PLATFORM_WIN32_KHR
android: DEFINES += VK_USE_PLATFORM_ANDROID_KHR

# Avoid build errors with no files:
SOURCES +=  \
    ../glad/src/vulkan.c \
    VkPlatform.cpp \
    VkContext.cpp \
    VkCapabilities.cpp \
    VkCommandExecutor.cpp \
    VkExecutorFrames.cpp \
    VkExecutorResources.cpp \
    VkExecutorPipelines.cpp \
    VkExecutorTransfer.cpp \
    VkUtil.cpp \
    KaldiVulkanDevice.cpp
HEADERS += \
    VkPlatform.hpp \
    VkContext.hpp \
    VkCapabilities.hpp \
    VkCommandExecutor.hpp \
    VkUtil.hpp \
    KaldiVulkanDevice.hpp \
    KldImplLib.h

DISTFILES += \
    README.md
