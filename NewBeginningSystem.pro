TEMPLATE = app
CONFIG += console c++2a
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -lmozjs-115 -lSDL2 -lSDL2_image -lfreetype -fopenmp -ldl -lpng16 -lgif -lturbojpeg -lsndfile -lsamplerate -lopenmpt -lwebp -lwebpdecoder -lwebpdemux  -lwebpmux -lzstd -lsnappy -lphysfs -lssl -lcrypto
INCLUDEPATH += /usr/include/mozjs-115
INCLUDEPATH += /usr/include/nspr
INCLUDEPATH += $$PWD

SOURCES += \
        MhLib/Gimmick/MhWordGenerator.cpp \
        MhLib/Io/MhBufferWrapper.cpp \
        MhLib/Io/MhFile.cpp \
        MhLib/Io/MhFixedBufferWrapper.cpp \
        MhLib/Io/MhProxyReadStream.cpp \
        MhLib/Io/MhProxyWriteStream.cpp \
        MhLib/Io/MhSnappyCompressor.cpp \
        MhLib/Io/MhSnappyDecompressor.cpp \
        MhLib/Io/MhSocket.cpp \
        MhLib/Io/MhSslDecryptor.cpp \
        MhLib/Io/MhSslEncryptor.cpp \
        MhLib/Io/MhStderr.cpp \
        MhLib/Io/MhStdin.cpp \
        MhLib/Io/MhStdout.cpp \
        MhLib/Io/MhTemporaryFile.cpp \
        MhLib/Io/MhZstdCompressor.cpp \
        MhLib/Io/MhZstdDecompressor.cpp \
        MhLib/Io/PhysFSIO.cpp \
        MhLib/IoSys/MhFilesystem.cpp \
        MhLib/IoSys/PhysFSIoSystem.cpp \
        MhLib/Media/AdvancedAudio/FX/MhClampEffect.cpp \
        MhLib/Media/AdvancedAudio/FX/MhConvolver.cpp \
        MhLib/Media/AdvancedAudio/FX/MhCrushEffect.cpp \
        MhLib/Media/AdvancedAudio/FX/MhImpulseGenerator.cpp \
        MhLib/Media/AdvancedAudio/FX/SampleLevelEffect.cpp \
        MhLib/Media/AdvancedAudio/MhAmbisonicPanner.cpp \
        MhLib/Media/AdvancedAudio/MhAudioBuffer.cpp \
        MhLib/Media/AdvancedAudio/MhAudioEffectSlot.cpp \
        MhLib/Media/AdvancedAudio/MhAudioError.cpp \
        MhLib/Media/AdvancedAudio/MhAudioMixer.cpp \
        MhLib/Media/AdvancedAudio/MhAudioStreamer.cpp \
        MhLib/Media/AdvancedAudio/MhFramerateConverter.cpp \
        MhLib/Media/AdvancedAudio/MhMatrixPanner.cpp \
        MhLib/Media/AdvancedAudio/MhModulePlayer.cpp \
        MhLib/Media/AdvancedAudio/MhPositionalPanner.cpp \
        MhLib/Media/AdvancedAudio/MhSoundSource.cpp \
        MhLib/Media/Audio/FFTConvolver/AudioFFT.cpp \
        MhLib/Media/Audio/FFTConvolver/FFTConvolver.cpp \
        MhLib/Media/Audio/FFTConvolver/IrBuffer.cpp \
        MhLib/Media/Audio/FFTConvolver/Utilities.cpp \
        MhLib/Media/Audio/MhModuleRenderer.cpp \
        MhLib/Media/Audio/MhResampler.cpp \
        MhLib/Media/Audio/MhSoundFile.cpp \
        MhLib/Media/Image/MhDDS.cpp \
        MhLib/Media/Image/MhGIF.cpp \
        MhLib/Media/Image/MhJPEG.cpp \
        MhLib/Media/Image/MhPNG.cpp \
        MhLib/Media/Image/MhTGA.cpp \
        MhLib/Media/Image/MhWEBP.cpp \
        MhLib/SIMD/private/x86/MhFloat32x4_private.cpp \
        MhLib/SIMD/private/x86/MhFloat32x8_private.cpp \
        MhLib/SIMD/private/x86/MhFloat64x2_private.cpp \
        MhLib/SIMD/private/x86/MhInt32x4_private.cpp \
        MhLib/Util/ColourHelper.cpp \
        MhLib/Util/MhDynamicLib.cpp \
        MhLib/Util/MhRNG.cpp \
        MhLib/Util/MhUUID.cpp \
        MhLib/Util/TextureHelpers.cpp \
        System/AppSystem.cpp \
        System/IniConfiguration.cpp \
        System/TestSystem.cpp \
        main.cpp

HEADERS += \
    GFX/Abstract/ComputeShader.hpp \
    GFX/Abstract/Framebuffer.hpp \
    GFX/Abstract/GfxResourceFactory.hpp \
    GFX/Abstract/NativeHandle.hpp \
    GFX/Abstract/Pipeline.hpp \
    GFX/Abstract/Texture.hpp \
    GFX/Abstract/UniformBuffer.hpp \
    GFX/Abstract/VertexBuffer.hpp \
    GFX/Abstract/VertexBufferAdapter.hpp \
    GFX/Abstract/VertexFormat.hpp \
    MhLib/Gimmick/MhGimmick.hpp \
    MhLib/Gimmick/MhWordGenerator.hpp \
    MhLib/Interface/MhBinaryTree.hpp \
    MhLib/Interface/MhComparable.hpp \
    MhLib/Interface/MhEquatable.hpp \
    MhLib/Interface/MhObserv.hpp \
    MhLib/Io/MhBufferWrapper.hpp \
    MhLib/Io/MhDataStream.hpp \
    MhLib/Io/MhFile.hpp \
    MhLib/Io/MhFixedBufferWrapper.hpp \
    MhLib/Io/MhIo.hpp \
    MhLib/Io/MhIoDevice.hpp \
    MhLib/Io/MhProxyReadStream.hpp \
    MhLib/Io/MhProxyWriteStream.hpp \
    MhLib/Io/MhSnappyCompressor.hpp \
    MhLib/Io/MhSnappyDecompressor.hpp \
    MhLib/Io/MhSocket.hpp \
    MhLib/Io/MhSslDecryptor.hpp \
    MhLib/Io/MhSslEncryptor.hpp \
    MhLib/Io/MhStderr.hpp \
    MhLib/Io/MhStdin.hpp \
    MhLib/Io/MhStdout.hpp \
    MhLib/Io/MhTemporaryFile.hpp \
    MhLib/Io/MhZstdCompressor.hpp \
    MhLib/Io/MhZstdDecompressor.hpp \
    MhLib/Io/PhysFSIO.hpp \
    MhLib/IoSys/MhFilesystem.hpp \
    MhLib/IoSys/MhIoSystem.hpp \
    MhLib/IoSys/PhysFSIoSystem.hpp \
    MhLib/Media/AdvancedAudio/FX/MhClampEffect.hpp \
    MhLib/Media/AdvancedAudio/FX/MhConvolver.hpp \
    MhLib/Media/AdvancedAudio/FX/MhCrushEffect.hpp \
    MhLib/Media/AdvancedAudio/FX/MhImpulseGenerator.hpp \
    MhLib/Media/AdvancedAudio/FX/SampleLevelEffect.hpp \
    MhLib/Media/AdvancedAudio/MhAdvancedAudio.hpp \
    MhLib/Media/AdvancedAudio/MhAmbisonicPanner.hpp \
    MhLib/Media/AdvancedAudio/MhAudioBuffer.hpp \
    MhLib/Media/AdvancedAudio/MhAudioEffect.hpp \
    MhLib/Media/AdvancedAudio/MhAudioEffectSlot.hpp \
    MhLib/Media/AdvancedAudio/MhAudioError.hpp \
    MhLib/Media/AdvancedAudio/MhAudioMixer.hpp \
    MhLib/Media/AdvancedAudio/MhAudioPacket.hpp \
    MhLib/Media/AdvancedAudio/MhAudioPlayable.hpp \
    MhLib/Media/AdvancedAudio/MhAudioStatus.hpp \
    MhLib/Media/AdvancedAudio/MhAudioStreamer.hpp \
    MhLib/Media/AdvancedAudio/MhFramerateConverter.hpp \
    MhLib/Media/AdvancedAudio/MhMatrixPanner.hpp \
    MhLib/Media/AdvancedAudio/MhModulePlayer.hpp \
    MhLib/Media/AdvancedAudio/MhPositionalPanner.hpp \
    MhLib/Media/AdvancedAudio/MhSoundSource.hpp \
    MhLib/Media/Audio/FFTConvolver/AudioFFT.h \
    MhLib/Media/Audio/FFTConvolver/FFTConvolver.h \
    MhLib/Media/Audio/FFTConvolver/IrBuffer.hpp \
    MhLib/Media/Audio/FFTConvolver/Utilities.h \
    MhLib/Media/Audio/MhAudio.hpp \
    MhLib/Media/Audio/MhAudioIterator.hpp \
    MhLib/Media/Audio/MhModuleRenderer.hpp \
    MhLib/Media/Audio/MhResampler.hpp \
    MhLib/Media/Audio/MhSoundFile.hpp \
    MhLib/Media/Image/MhDDS.hpp \
    MhLib/Media/Image/MhDecodeTarget.hpp \
    MhLib/Media/Image/MhGIF.hpp \
    MhLib/Media/Image/MhImage.hpp \
    MhLib/Media/Image/MhImageType.hpp \
    MhLib/Media/Image/MhJPEG.hpp \
    MhLib/Media/Image/MhPNG.hpp \
    MhLib/Media/Image/MhStandardColourFormat.hpp \
    MhLib/Media/Image/MhTGA.hpp \
    MhLib/Media/Image/MhWEBP.hpp \
    MhLib/SIMD/MhSIMD.hpp \
    MhLib/SIMD/private/fallback/MhFallbackVector.hpp \
    MhLib/SIMD/private/x86/MhFloat32x4_private.hpp \
    MhLib/SIMD/private/x86/MhFloat32x8_private.hpp \
    MhLib/SIMD/private/x86/MhFloat64x2_private.hpp \
    MhLib/SIMD/private/x86/MhInt32x4_private.hpp \
    MhLib/Util/ColourHelper.hpp \
    MhLib/Util/Dither.hpp \
    MhLib/Util/ITextReceiver.hpp \
    MhLib/Util/MhBuffer.hpp \
    MhLib/Util/MhDynamicLib.hpp \
    MhLib/Util/MhEndianness.hpp \
    MhLib/Util/MhFixedStack.hpp \
    MhLib/Util/MhGlobals.hpp \
    MhLib/Util/MhIntegralIterator.hpp \
    MhLib/Util/MhNormDenorm.hpp \
    MhLib/Util/MhRNG.hpp \
    MhLib/Util/MhThreadsafeQueue.hpp \
    MhLib/Util/MhUUID.hpp \
    MhLib/Util/NormDenorm.hpp \
    MhLib/Util/PixelFormat.hpp \
    MhLib/Util/TextureHelpers.hpp \
    MhLib/Util/ThreadsafeContainer.hpp \
    MhLib/Util/half.hpp \
    System/AppSystem.hpp \
    System/IniConfiguration.hpp \
    System/TestSystem.hpp

DISTFILES += \
    MhLib/Media/Audio/FFTConvolver/COPYING.txt \
    MhLib/Media/Audio/FFTConvolver/README.md
