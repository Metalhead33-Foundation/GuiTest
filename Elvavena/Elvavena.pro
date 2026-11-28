TEMPLATE = lib
TARGET = Elvavena
CONFIG += shared c++2a
CONFIG -= qt

DEFINES += MH_UTIL_EXPORT

INCLUDEPATH += $$PWD/..

SOURCES += \
    Io/ElvDataSerializationSchema.cpp \
    Io/ElvIoDeviceProxy.cpp \
    Net/ElvNetReplicationSchema.cpp \
    Net/ElvNetSession.cpp \
    Net/ElvHttp.cpp \
    Util/ElvDynamicLib.cpp \
    Util/ElvEventLoop.cpp \
    Util/ElvPollingEventLoop.cpp \
    Util/ElvThreadPool.cpp

HEADERS += \
    Io/ElvAsyncCompletion.hpp \
    Io/ElvBitStream.hpp \
    Io/ElvDataSerializationSchema.hpp \
    Io/ElvDataStream.hpp \
    Io/ElvStreamBuffer.hpp \
    Io/ElvEventLoopWakeup.hpp \
    Io/ElvEventPoller.hpp \
    Io/ElvIoAsync.hpp \
    Io/ElvIoDevice.hpp \
    Io/ElvIoDeviceProxy.hpp \
    Io/ElvIoSystem.hpp \
    Net/ElvDatagramTransport.hpp \
    Net/ElvLoopbackTransport.hpp \
    Net/ElvHttp.hpp \
    Net/ElvNetClockSync.hpp \
    Net/ElvNetCongestion.hpp \
    Net/ElvNetFieldDelta.hpp \
    Net/ElvNetFragment.hpp \
    Net/ElvNetId.hpp \
    Net/ElvNetInterpolation.hpp \
    Net/ElvNetInterest.hpp \
    Net/ElvNetLockstep.hpp \
    Net/ElvNetPrediction.hpp \
    Net/ElvNetProtocol.hpp \
    Net/ElvNetReplicationPipeline.hpp \
    Net/ElvNetReplicationRegistry.hpp \
    Net/ElvNetReplicationSchema.hpp \
    Net/ElvNetRpc.hpp \
    Net/ElvNetSequence.hpp \
    Net/ElvNetSnapshot.hpp \
    Net/ElvNetSession.hpp \
    Net/ElvNetTypes.hpp \
    Util/ElvAlexandrescuResource.hpp \
    Util/ElvAllocatorBasic.hpp \
    Util/ElvAsyncExecutor.hpp \
    Util/ElvBinaryTree.hpp \
    Util/ElvBitmapAllocator.hpp \
    Util/ElvChecksum.hpp \
    Util/ElvChunkyArray.hpp \
    Util/ElvCommandQueue.hpp \
    Util/ElvContainerBasic.hpp \
    Util/ElvContiguousRingBuffer.hpp \
    Util/ElvContinuousIterator.hpp \
    Util/ElvDeferStack.hpp \
    Util/ElvDither.hpp \
    Util/ElvDoubleBufferedVector.hpp \
    Util/ElvDynamicLib.hpp \
    Util/ElvEdgeFunction.hpp \
    Util/ElvEndianness.hpp \
    Util/ElvEventLoop.hpp \
    Util/ElvPollingEventLoop.hpp \
    Util/ElvException.hpp \
    Util/ElvFixedPoint.hpp \
    Util/ElvFixedPointMath.hpp \
    Util/ElvFreelist.hpp \
    Util/ElvFuzzy.hpp \
    Util/ElvHeapArray.hpp \
    Util/ElvIdPool.hpp \
    Util/ElvIntegralIterator.hpp \
    Util/ElvInterpolation.hpp \
    Util/ElvIntrusiveArray.hpp \
    Util/ElvMathUtil.hpp \
    Util/ElvNormDenorm.hpp \
    Util/ElvObserv.hpp \
    Util/ElvRingBuffer.hpp \
    Util/ElvScopeGuard.hpp \
    Util/ElvSpanHelpers.hpp \
    Util/ElvStackAllocator.hpp \
    Util/ElvStringhashMap.hpp \
    Util/ElvThreadPool.hpp \
    Util/ElvThreadsafeContainer.hpp \
    Util/ElvThreadsafeQueue.hpp \
    Util/ElvUtilGlobals.hpp \
    Util/half.hpp

DISTFILES += \
    Elvavena.pri \
    README.md
