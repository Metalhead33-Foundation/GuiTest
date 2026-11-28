TEMPLATE = lib
TARGET = Euphemy
CONFIG += shared c++2a
CONFIG -= qt

DEFINES += MH_EUPH_EXPORT

# External Dependencies
LIBS += -fopenmp -ldl -lpng16 -lgif -lturbojpeg -lsndfile -lsamplerate -lopenmpt \
        -lwebp -lwebpdecoder -lwebpdemux -lwebpmux -lzstd -lsnappy -lphysfs -lssl -lcrypto

# io_uring is optional. When liburing is present the wrapper is compiled and
# linked; otherwise EuphIoUring.cpp provides runtime-unavailable stubs.
CONFIG += link_pkgconfig
system(pkg-config --exists liburing) {
    PKGCONFIG += liburing
    DEFINES += MH_HAS_IO_URING
}

INCLUDEPATH += $$PWD/..

# Link to sibling library
LIBS += -L$$OUT_PWD/../Elvavena -lElvavena

SOURCES += \
 Asset/EuphAssetRuntime.cpp \
 Config/EuphConfiguration.cpp \
 Config/EuphConfigurationElement.cpp \
 Io/EuphBufferDevice.cpp \
 Io/EuphConstBufferDevice.cpp \
 Io/EuphCryptoProxyDevice.cpp \
 Io/EuphFile.cpp \
 Io/EuphMemoryDevice.cpp \
 Io/EuphMemoryMapped.cpp \
 Io/EuphNullDevice.cpp \
 Io/EuphPlatformDependentFileBase.cpp \
 Io/EuphPmrMemoryDevice.cpp \
 Io/EuphRandomDevice.cpp \
 Io/EuphSocket.cpp \
 Io/EuphSnappy.cpp \
 Io/EuphStdfileWrapper.cpp \
 Io/EuphTempFile.cpp \
 Io/EuphZstd.cpp \
 Io/PhysFS.cpp \
 Localization/EuphLocalization.cpp \
 Media/Audio/EuphConvolver.cpp \
 Media/Audio/EuphModuleRenderer.cpp \
 Media/Audio/EuphResampler.cpp \
 Media/Audio/EuphSoundFile.cpp \
 Media/Audio/EuphSoundClip.cpp \
 Media/Audio/FFTConvolver/AudioFFT.cpp \
 Media/Audio/FFTConvolver/FFTConvolver.cpp \
 Media/Audio/FFTConvolver/TwoStageFFTConvolver.cpp \
 Media/Audio/FFTConvolver/Utilities.cpp \
 Media/Image/EuphDDS.cpp \
 Media/Image/EuphGIF.cpp \
 Media/Image/EuphImage.cpp \
 Media/Image/EuphImageDecodeTarget.cpp \
 Media/Image/EuphJPEG.cpp \
 Media/Image/EuphPNG.cpp \
 Media/Image/EuphStandardColour.cpp \
 Media/Image/EuphTGA.cpp \
 Media/Image/EuphWEBP.cpp \
 Memory/EuphMemory.cpp \
 Net/EuphHttpClient.cpp \
 Net/EuphHttpConnection.cpp \
 Net/EuphHttpServer.cpp \
 Platform/EuphEpollPoller.cpp \
 Platform/EuphEventLoopWakeup.cpp \
 Platform/EuphEventPollerFactory.cpp \
 Platform/EuphIocp.cpp \
 Platform/EuphIoUring.cpp \
 Platform/EuphKqueuePoller.cpp \
 Platform/EuphWSAPollPoller.cpp \
 Util/EuphNoise.cpp

HEADERS += \
 Asset/EuphAssetRuntime.hpp \
 Asset/EuphAssetTypes.hpp \
 Config/EuphConfiguration.hpp \
 Config/EuphConfigurationElement.hpp \
 Config/EuphLib.hpp \
 Io/EuphBufferDevice.hpp \
 Io/EuphConstBufferDevice.hpp \
 Io/EuphCryptoProxyDevice.hpp \
 Io/EuphFile.hpp \
 Io/EuphMemoryDevice.hpp \
 Io/EuphMemoryMapped.hpp \
 Io/EuphNullDevice.hpp \
 Io/EuphPlatformDependentFileBase.hpp \
 Io/EuphPmrMemoryDevice.hpp \
 Io/EuphRandomDevice.hpp \
 Io/EuphSocket.hpp \
 Io/EuphSnappy.hpp \
 Io/EuphStdfileWrapper.hpp \
 Io/EuphTempFile.hpp \
 Io/EuphZstd.hpp \
 Io/PhysFS.hpp \
 Localization/EuphLocalization.hpp \
 Media/Audio/EuphAudioIterator.hpp \
 Media/Audio/EuphConvolver.hpp \
 Media/Audio/EuphModuleRenderer.hpp \
 Media/Audio/EuphResampler.hpp \
 Media/Audio/EuphSoundFile.hpp \
 Media/Audio/EuphSoundClip.hpp \
 Media/Audio/FFTConvolver/AudioFFT.h \
 Media/Audio/FFTConvolver/FFTConvolver.h \
 Media/Audio/FFTConvolver/TwoStageFFTConvolver.h \
 Media/Audio/FFTConvolver/Utilities.h \
 Media/Image/EuphDDS.hpp \
 Media/Image/EuphGIF.hpp \
 Media/Image/EuphImage.hpp \
 Media/Image/EuphImageDecodeTarget.hpp \
 Media/Image/EuphImageType.hpp \
 Media/Image/EuphJPEG.hpp \
 Media/Image/EuphPNG.hpp \
 Media/Image/EuphPixelFormat.hpp \
 Media/Image/EuphStandardColour.hpp \
 Media/Image/EuphTGA.hpp \
 Media/Image/EuphWEBP.hpp \
 Memory/EuphMemory.hpp \
 Net/EuphHttpClient.hpp \
 Net/EuphHttpConnection.hpp \
 Net/EuphHttpServer.hpp \
 Net/EuphUdpTransport.hpp \
 Platform/EuphEpollPoller.hpp \
 Platform/EuphEventLoopWakeup.hpp \
 Platform/EuphEventPollerFactory.hpp \
 Platform/EuphIocp.hpp \
 Platform/EuphIoUring.hpp \
 Platform/EuphKqueuePoller.hpp \
 Platform/EuphWSAPollPoller.hpp \
 Util/EuphNoise.hpp

DISTFILES += \
    ../docs/networking/HTTP.md \
    ../docs/networking/NETCODE.md \
    ../docs/networking/index.md \
    Euphemy.pri \
    Media/Audio/FFTConvolver/FFTConvolver_COPYING.txt \
    Media/Audio/FFTConvolver/FFTConvolver_README.md \
    README.md
