TEMPLATE = lib
TARGET = Haruka
CONFIG += shared c++2a
CONFIG -= qt

LIBS += -lsndfile -lsamplerate -lopenmpt
INCLUDEPATH += $$PWD/..

DEFINES += MH_HARUKA_EXPORT
# Avoid build errors with no files:
SOURCES +=  \
    Core/HrkAmbisonicPanner.cpp \
    Core/HrkAudioError.cpp \
    Core/HrkMatrixPanner.cpp \
    Core/HrkMixer.cpp \
    Core/HrkProxyPlayable.cpp \
    Core/HrkResampler.cpp \
    IO/HrkModuleStreamer.cpp \
    IO/HrkPlaybackState.cpp \
    IO/HrkPlaybackStatePlayable.cpp \
    IO/HrkRingBufferedStreamer.cpp \
    IO/HrkSoundSource.cpp \
    IO/HrkSoundStreamer.cpp
HEADERS += \
    Core/HrkAmbisonicPanner.hpp \
    Core/HrkAudioError.hpp \
    Core/HrkAudioPacket.hpp \
    Core/HrkMatrixPanner.hpp \
    Core/HrkMixer.hpp \
    Core/HrkPlayable.hpp \
    Core/HrkProxyPlayable.hpp \
    Core/HrkResampler.hpp \
    HarukaLib.hpp \
    IO/HrkModuleStreamer.hpp \
    IO/HrkPlaybackState.hpp \
    IO/HrkPlaybackStatePlayable.hpp \
    IO/HrkRingBufferedStreamer.hpp \
    IO/HrkSoundBuffer.hpp \
    IO/HrkSoundSource.hpp \
    IO/HrkSoundStreamer.hpp

DISTFILES += \
    Haruka.pri \
    README.md
