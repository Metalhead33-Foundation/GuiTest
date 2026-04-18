TEMPLATE = lib
TARGET = Euphemy
CONFIG += shared c++2a
CONFIG -= qt

DEFINES += MH_EUPH_EXPORT

# External Dependencies (moved from your original file)
LIBS += -lmozjs-115 -lX11 -lSDL2 -lSDL2_image -lfreetype -fopenmp -ldl -lpng16 \
        -lgif -lturbojpeg -lsndfile -lsamplerate -lopenmpt -lwebp -lwebpdecoder \
        -lwebpdemux -lwebpmux -lzstd -lsnappy -lphysfs -lssl -lcrypto \
        -lglslang -lSPIRV -lspirv-cross-core -lspirv-cross-glsl -lyaml-cpp

INCLUDEPATH += $$PWD/..
INCLUDEPATH += /usr/include/mozjs-115 /usr/include/nspr /usr/include/freetype2 .

# Link to sibling library
LIBS += -L$$OUT_PWD/../Elvavena -lElvavena

SOURCES += \
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
 Io/EuphStdfileWrapper.cpp \
 Io/EuphTempFile.cpp \
 Io/EuphZstd.cpp \
 Io/PhysFS.cpp \
 Media/Audio/EuphModuleRenderer.cpp \
 Media/Audio/EuphResampler.cpp \
 Media/Audio/EuphSoundFile.cpp \
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
 Memory/EuphMemory.cpp

HEADERS += \
 Config/EuphConfiguration.hpp \
 Config/EuphConfigurationElement.hpp \
 Config/EuphLib.hpp \
 Config/GlobalConfig.hpp \
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
 Io/EuphStdfileWrapper.hpp \
 Io/EuphTempFile.hpp \
 Io/EuphZstd.hpp \
 Io/PhysFS.hpp \
 Media/Audio/EuphAudioIterator.hpp \
 Media/Audio/EuphModuleRenderer.hpp \
 Media/Audio/EuphResampler.hpp \
 Media/Audio/EuphSoundFile.hpp \
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
 Memory/EuphMemory.hpp

DISTFILES += \
    Euphemy.pri \
    Media/Audio/FFTConvolver/FFTConvolver_COPYING.txt \
    Media/Audio/FFTConvolver/FFTConvolver_README.md \
    README.md
