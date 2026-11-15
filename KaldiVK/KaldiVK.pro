TEMPLATE = lib
TARGET = KaldiVK 
CONFIG += shared c++2a
CONFIG -= qt

INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../glad/include
DEFINES += MH_KALDI_IMPL_EXPORT
include(../Kaldi/Kaldi.pri)

# Avoid build errors with no files:
SOURCES +=  \
    ../glad/src/vulkan.c \
    VkUtil.cpp \
    KaldiVulkanDevice.cpp
HEADERS += \
    VkUtil.hpp \
    KaldiVulkanDevice.hpp \
    KldImplLib.h

DISTFILES += \
    README.md
