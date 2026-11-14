#include "sandbox.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <Elvavena/Util/ElvHeapArray.hpp>
#include <Elvavena/Util/ElvChunkyArray.hpp>
#include <Elvavena/Util/ElvIntegralIterator.hpp>
#include <Elvavena/Util/ElvFreelist.hpp>
#include <Elvavena/Util/ElvBitmapAllocator.hpp>
#include <Elvavena/Util/ElvThreadPool.hpp>
#include <Elvavena/Io/ElvIoAsync.hpp>
#include <Euphemy/Asset/EuphAssetRuntime.hpp>
#include <Lotte/Asset/LteGpuAssetStreamer.hpp>
#include <vector>
#include <Euphemy/Io/EuphBufferDevice.hpp>
#include <Euphemy/Io/EuphMemoryDevice.hpp>
#include <Elvavena/Io/ElvDataStream.hpp>
#include <map>
#include <Euphemy/Io/EuphRandomDevice.hpp>
#include <Euphemy/Io/EuphZstd.hpp>
#include <Euphemy/Io/EuphCryptoProxyDevice.hpp>
#include <Euphemy/Io/EuphFile.hpp>
#include <Euphemy/Io/EuphTempFile.hpp>
#include <Elvavena/Util/ElvDynamicLib.hpp>
#include <Euphemy/Config/EuphConfiguration.hpp>
#include <fstream>
#include <Euphemy/Memory/EuphMemory.hpp>
#include <Elvavena/Util/ElvInterpolation.hpp>
#include <Elvavena/Util/ElvFixedPoint.hpp>
#include <Euphemy/Media/Image/EuphTGA.hpp>
#include <Euphemy/Media/Image/EuphPixelFormat.hpp>
#include <Euphemy/Media/Image/EuphTGA.hpp>
#include <Euphemy/Media/Image/EuphPNG.hpp>
#include <Euphemy/Media/Image/EuphImage.hpp>
#include <iostream>
#include <memory>
#include <memory_resource>
#include <span>
#include <stdexcept>
#include <stop_token>
#include <thread>
#include <chrono>
#include <Lotte/Text/LteFontFace.hpp>
#include <Lotte/VectorGraphic/LteStoredVectorImage.hpp>
#include <Lotte/Text/LteMml.hpp>
#include <Lotte/Text/LteUtf.hpp>
#include <Lotte/Text/LteHtmlEntities.hpp>
#include <Lotte/Text/LteMarkupColour.hpp>
#include <Lotte/Text/LteTextLayout.hpp>
#include <Lotte/Text/LteFontAtlas.hpp>
#include <Lotte/Gui/LteGuiVertex.hpp>
#include <Lotte/Gui/LteGuiRenderer.hpp>
#include <Lotte/Gui/LteGuiRenderCache.hpp>
#include <Lotte/GFX/LteMesh.hpp>
#include <Haruka/Core/HrkPlayable.hpp>
#include <Haruka/FX/HrkEffectSlot.hpp>
#include <Haruka/FX/HrkClampEffect.hpp>
#include <Haruka/FX/HrkCrushEffect.hpp>
#include <Haruka/FX/HrkConvolver.hpp>
#include <Haruka/FX/HrkImpulseGenerator.hpp>
#include <Elvavena/Util/ElvSignal.hpp>

template<class T>
struct Mallocator
{
	typedef T value_type;

	Mallocator() = default;

	template<class U>
	constexpr Mallocator(const Mallocator <U>&) noexcept {}

	[[nodiscard]] T* allocate(std::size_t n)
	{
		if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
			throw std::bad_array_new_length();

		if (auto p = static_cast<T*>(std::malloc(n * sizeof(T))))
		{
			report(p, n);
			return p;
		}

		throw std::bad_alloc();
	}

	void deallocate(T* p, std::size_t n) noexcept
	{
		report(p, n, 0);
		std::free(p);
	}
private:
	void report(T* p, std::size_t n, bool alloc = true) const
	{
		std::cout << (alloc ? "Alloc: " : "Dealloc: ") << sizeof(T) * n
				  << " bytes at " << std::hex << std::showbase
				  << reinterpret_cast<void*>(p) << std::dec << '\n';
	}
};

/*enum class AllocatorSubsystem {
	AUDIO,
	VIDEO,
	SCRIPT
};
struct WodMemoryManager {
	typedef std::unique_ptr<Elv::Util::FreelistMemoryManager> uManager;
private:
	static uManager manager;
	static std::mutex managerMutex;
public:
	static Elv::Util::FreelistMemoryManager& getStaticManager() {
		std::scoped_lock lock(managerMutex);
		if(!manager) {
			manager = uManager(new Elv::Util::FreelistMemoryManager(
								   { 1024*8, 1024, 1024 }
								   ));
		}
		return *manager;
	}
};
WodMemoryManager::uManager WodMemoryManager::manager = nullptr;
std::mutex WodMemoryManager::managerMutex;

template <AllocatorSubsystem subsysId> struct WodMemoryManagerSubsystem {
private:
	static Elv::Util::ContiguousFreeListAllocator* alloc;
public:
	WodMemoryManagerSubsystem() {
		if(!alloc) {
			alloc = WodMemoryManager::getStaticManager().getAllocator(static_cast<size_t>(subsysId));
		}
	}
	Elv::Util::Blk allocateBlock(std::size_t n) noexcept {
		return alloc->allocateBlock(n);
	}
	void deallocateBlock(const Elv::Util::Blk& blk) noexcept {
		alloc->deallocateBlock(blk);
	}
	bool ownsBlock(const Elv::Util::Blk& blk) const noexcept {
		return alloc->ownsBlock(blk);
	}
};
template <AllocatorSubsystem subsysId> Elv::Util::ContiguousFreeListAllocator* WodMemoryManagerSubsystem<subsysId>::alloc = nullptr;


//typedef Elv::Util::AlexandrescuAllocatorAdapter<Elv::Util::StaticBitmapAllocator<8,1024>,int> IntAllocator;
//typedef Elv::Util::AlexandrescuAllocatorAdapter<Elv::Util::StaticFreeListAllocator<sizeof(int)*20000>,int> IntAllocator;
struct RandomStruct;
typedef Elv::Util::AlexandrescuAllocatorAdapter<WodMemoryManagerSubsystem<AllocatorSubsystem::AUDIO>,int> IntAllocator;
typedef Elv::Util::AlexandrescuAllocatorAdapter<WodMemoryManagerSubsystem<AllocatorSubsystem::AUDIO>,RandomStruct> StructAllocator;
DEFINE_STRUCT_PTRS_WITH_ALLOC(RandomStruct,StructAllocator)

struct RandomStruct {
	int whatever;
};*/

static const std::map<int,std::string> baseMap {
	{ 2, "Hello World!" },
	{ 4, "Hello 4!" },
	{ 6, "Hello 6!" },
	{ 8, "Hello 8!" },
	{ 10, "Hello 10!" },
};
//static const std::vector<unsigned char> Encryptionkey { 'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\n', 0 };
static const std::vector<unsigned char> Encryptionkey { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
												 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
												 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
												 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31 };

namespace {

void requireCondition(bool condition, const char* message)
{
	if (!condition)
		throw std::runtime_error(message);
}

bool bytesEqual(std::span<const std::byte> lhs, std::span<const std::byte> rhs)
{
	return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template<typename Future>
void requireRuntimeError(Future& future, const char* message)
{
	bool threw = false;
	try {
		(void)future.get();
	} catch (const std::runtime_error&) {
		threw = true;
	}
	requireCondition(threw, message);
}

template<typename Future>
void requireCanceled(Future& future, const char* message)
{
	bool canceled = false;
	try {
		(void)future.get();
	} catch (const Elv::Io::AsyncOperationCanceled&) {
		canceled = true;
	}
	requireCondition(canceled, message);
}

class ThrowingDevice : public Elv::Io::Device {
public:
	size_t read(void*, size_t, size_t) override
	{
		throw std::runtime_error("ThrowingDevice read failed");
	}

	size_t write(const void*, size_t, size_t) override
	{
		throw std::runtime_error("ThrowingDevice write failed");
	}

	int seek(long, Elv::Io::SeekOrigin) override
	{
		throw std::runtime_error("ThrowingDevice seek failed");
	}

	long tell() override
	{
		return 0;
	}

	size_t size() override
	{
		return 1;
	}

	bool eof() override
	{
		return false;
	}

	Elv::Io::Mode getMode() const override
	{
		return Elv::Io::Mode::READ_WRITE;
	}

	bool flush() override
	{
		return true;
	}

	bool isValid() const override
	{
		return true;
	}
};

class ZeroReadDevice : public Elv::Io::Device {
public:
	size_t read(void*, size_t, size_t) override
	{
		return 0;
	}

	size_t write(const void*, size_t, size_t) override
	{
		return 0;
	}

	int seek(long, Elv::Io::SeekOrigin) override
	{
		return 0;
	}

	long tell() override
	{
		return 0;
	}

	size_t size() override
	{
		return 4;
	}

	bool eof() override
	{
		return false;
	}

	Elv::Io::Mode getMode() const override
	{
		return Elv::Io::Mode::READ;
	}

	bool flush() override
	{
		return true;
	}

	bool isValid() const override
	{
		return true;
	}
};

} // namespace

void testAsyncIo()
{
	using AsyncIo = Elv::Io::Async<Elv::Util::ThreadPool>;
	using ByteResult = Elv::Io::AsyncResult<size_t>;
	using SpanResult = Elv::Io::AsyncResult<std::span<const std::byte> >;

	Elv::Util::ThreadPool pool(2);
	const std::array<std::byte, 4> source {
		std::byte{0x10},
		std::byte{0x20},
		std::byte{0x30},
		std::byte{0x40}
	};

	Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
	requireCondition(AsyncIo::write(pool, device, source.data(), 1, source.size()).get() == source.size(), "Async::write returned the wrong byte count");
	requireCondition(AsyncIo::flush(pool, device).get(), "Async::flush failed");
	requireCondition(AsyncIo::size(pool, device).get() == source.size(), "Async::size returned the wrong size");
	requireCondition(AsyncIo::seek(pool, device, 0, Elv::Io::SeekOrigin::SET).get() == 0, "Async::seek failed");

	std::array<std::byte, 4> destination {};
	requireCondition(AsyncIo::read(pool, device, destination.data(), 1, destination.size()).get() == destination.size(), "Async::read returned the wrong byte count");
	requireCondition(bytesEqual(destination, source), "Async::read copied the wrong bytes");
	requireCondition(AsyncIo::tell(pool, device).get() == static_cast<long>(source.size()), "Async::tell returned the wrong position");
	requireCondition(AsyncIo::eof(pool, device).get(), "Async::eof should report end of memory device");
	requireCondition(AsyncIo::isValid(pool, device).get(), "Async::isValid should report true");

	bool seekThenCalled = false;
	requireCondition(AsyncIo::seekThen(pool, device, 0, Elv::Io::SeekOrigin::SET, [&](const Elv::Io::AsyncResult<int>& result) {
		seekThenCalled = true;
		requireCondition(result.has_value() && result.value() == 0, "Async::seekThen callback received the wrong result");
	}).get() == 0, "Async::seekThen future returned the wrong result");
	requireCondition(seekThenCalled, "Async::seekThen did not invoke its callback");

	requireCondition(AsyncIo::tellThen(pool, device, [](const Elv::Io::AsyncResult<long>& result) {
		requireCondition(result.has_value() && result.value() == 0, "Async::tellThen callback received the wrong result");
	}).get() == 0, "Async::tellThen future returned the wrong result");

	requireCondition(AsyncIo::sizeThen(pool, device, [&](const Elv::Io::AsyncResult<size_t>& result) {
		requireCondition(result.has_value() && result.value() == source.size(), "Async::sizeThen callback received the wrong result");
	}).get() == source.size(), "Async::sizeThen future returned the wrong result");

	requireCondition(AsyncIo::flushThen(pool, device, [](const Elv::Io::AsyncResult<bool>& result) {
		requireCondition(result.has_value() && result.value(), "Async::flushThen callback received the wrong result");
	}).get(), "Async::flushThen future returned the wrong result");

	requireCondition(AsyncIo::isValidThen(pool, device, [](const Elv::Io::AsyncResult<bool>& result) {
		requireCondition(result.has_value() && result.value(), "Async::isValidThen callback received the wrong result");
	}).get(), "Async::isValidThen future returned the wrong result");

	requireCondition(!AsyncIo::eofThen(pool, device, [](const Elv::Io::AsyncResult<bool>& result) {
		requireCondition(result.has_value() && !result.value(), "Async::eofThen callback received the wrong result");
	}).get(), "Async::eofThen future returned the wrong result");

	bool readThenCalled = false;
	size_t readThenBytes = 0;
	std::array<std::byte, 2> partial {};
	requireCondition(AsyncIo::readThen(pool, device, partial.data(), 1, partial.size(), [&](const ByteResult& result) {
		readThenCalled = true;
		requireCondition(result.has_value(), "Async::readThen callback expected success");
		readThenBytes = result.value();
	}).get() == partial.size(), "Async::readThen future returned the wrong byte count");
	requireCondition(readThenCalled && readThenBytes == partial.size(), "Async::readThen callback was not invoked correctly");
	requireCondition(bytesEqual(partial, std::span<const std::byte>(source.data(), partial.size())), "Async::readThen copied the wrong bytes");

	requireCondition(AsyncIo::seek(pool, device, 0, Elv::Io::SeekOrigin::SET).get() == 0, "Async::seek failed before callback exception test");
	size_t throwingCallbackCalls = 0;
	auto callbackThrowFuture = AsyncIo::readThen(pool, device, partial.data(), 1, 1, [&](const ByteResult& result) {
		++throwingCallbackCalls;
		requireCondition(result.has_value() && result.value() == 1, "Async::readThen callback exception test received the wrong result");
		throw std::runtime_error("Async::readThen callback failed");
	});
	requireRuntimeError(callbackThrowFuture, "Async::readThen future should propagate callback exceptions");
	requireCondition(throwingCallbackCalls == 1, "Async::readThen should not invoke a throwing callback twice");

	Euph::Io::MemoryDevice writeThenDevice(Elv::Io::Mode::WRITE);
	bool writeThenCalled = false;
	requireCondition(AsyncIo::writeThen(pool, writeThenDevice, source.data(), 1, source.size(), [&](const ByteResult& result) {
		writeThenCalled = true;
		requireCondition(result.has_value() && result.value() == source.size(), "Async::writeThen callback received the wrong result");
	}).get() == source.size(), "Async::writeThen future returned the wrong byte count");
	requireCondition(writeThenCalled, "Async::writeThen did not invoke its callback");

	requireCondition(AsyncIo::seek(pool, device, 0, Elv::Io::SeekOrigin::SET).get() == 0, "Async::seek failed before readAllThen");
	bool readAllThenCalled = false;
	auto readAllThenFuture = AsyncIo::readAllThen(pool, device, std::pmr::get_default_resource(), [&](const SpanResult& result) {
		readAllThenCalled = true;
		requireCondition(result.has_value(), "Async::readAllThen callback expected success");
		requireCondition(bytesEqual(result.value(), source), "Async::readAllThen callback received the wrong bytes");
	});
	const auto readAllThenBytes = readAllThenFuture.get();
	requireCondition(readAllThenCalled, "Async::readAllThen did not invoke its callback");
	requireCondition(bytesEqual(readAllThenBytes, source), "Async::readAllThen future returned the wrong bytes");

	Euph::Io::MemoryDevice customReadAllDevice(Elv::Io::Mode::READ, std::vector<std::byte>(source.begin(), source.end()));
	const auto customChunkBytes = AsyncIo::readAll(pool, customReadAllDevice, std::pmr::get_default_resource(), 2).get();
	requireCondition(bytesEqual(customChunkBytes, source), "Async::readAll with custom chunk size returned the wrong bytes");

	Euph::Io::MemoryDevice customReadAllSharedDevice(Elv::Io::Mode::READ, std::vector<std::byte>(source.begin(), source.end()));
	const auto customChunkSharedBytes = AsyncIo::readAllShared(pool, customReadAllSharedDevice, std::pmr::get_default_resource(), 2).get();
	requireCondition(bytesEqual(*customChunkSharedBytes, source), "Async::readAllShared with custom chunk size returned the wrong bytes");

	Euph::Io::MemoryDevice customReadAllThenDevice(Elv::Io::Mode::READ, std::vector<std::byte>(source.begin(), source.end()));
	bool customReadAllThenCalled = false;
	std::vector<std::byte> customReadAllThenCopy;
	auto customReadAllThenFuture = AsyncIo::readAllThen(pool, customReadAllThenDevice, std::pmr::get_default_resource(), 2, [&](const SpanResult& result) {
		customReadAllThenCalled = true;
		requireCondition(result.has_value(), "Async::readAllThen custom chunk callback expected success");
		customReadAllThenCopy.assign(result.value().begin(), result.value().end());
		requireCondition(bytesEqual(result.value(), source), "Async::readAllThen custom chunk callback received the wrong bytes");
	});
	const auto customReadAllThenBytes = customReadAllThenFuture.get();
	requireCondition(customReadAllThenCalled, "Async::readAllThen with custom chunk size did not invoke its callback");
	requireCondition(bytesEqual(customReadAllThenCopy, source), "Async::readAllThen callback span was not valid during callback");
	requireCondition(bytesEqual(customReadAllThenBytes, source), "Async::readAllThen with custom chunk size returned the wrong bytes");

	requireCondition(AsyncIo::seek(pool, device, 0, Elv::Io::SeekOrigin::SET).get() == 0, "Async::seek failed before custom PMR read");
	std::array<std::byte, 1024> pmrStorage {};
	std::pmr::monotonic_buffer_resource memRes(pmrStorage.data(), pmrStorage.size());
	const auto customPmrBytes = AsyncIo::readAll(pool, device, &memRes).get();
	requireCondition(bytesEqual(customPmrBytes, source), "Async::readAll with custom PMR returned the wrong bytes");

	ThrowingDevice throwingDevice;
	bool readFailureCallback = false;
	std::array<std::byte, 1> scratch {};
	auto failingRead = AsyncIo::readThen(pool, throwingDevice, scratch.data(), 1, scratch.size(), [&](const ByteResult& result) {
		readFailureCallback = true;
		requireCondition(!result.has_value(), "Async::readThen failure callback expected an error");
		requireCondition(result.error() != nullptr, "Async::readThen failure callback did not receive an exception");
		bool valueThrew = false;
		try {
			(void)result.value();
		} catch (const std::runtime_error&) {
			valueThrew = true;
		}
		requireCondition(valueThrew, "AsyncResult::value should rethrow read failure");
	});
	requireRuntimeError(failingRead, "Async::readThen future should rethrow device read failure");
	requireCondition(readFailureCallback, "Async::readThen did not invoke its failure callback");

	bool writeFailureCallback = false;
	auto failingWrite = AsyncIo::writeThen(pool, throwingDevice, source.data(), 1, source.size(), [&](const ByteResult& result) {
		writeFailureCallback = true;
		requireCondition(!result.has_value(), "Async::writeThen failure callback expected an error");
		requireCondition(result.error() != nullptr, "Async::writeThen failure callback did not receive an exception");
	});
	requireRuntimeError(failingWrite, "Async::writeThen future should rethrow device write failure");
	requireCondition(writeFailureCallback, "Async::writeThen did not invoke its failure callback");

	bool readAllFailureCallback = false;
	auto failingReadAll = AsyncIo::readAllThen(pool, throwingDevice, [&](const SpanResult& result) {
		readAllFailureCallback = true;
		requireCondition(!result.has_value(), "Async::readAllThen failure callback expected an error");
		requireCondition(result.error() != nullptr, "Async::readAllThen failure callback did not receive an exception");
	});
	requireRuntimeError(failingReadAll, "Async::readAllThen future should rethrow device read failure");
	requireCondition(readAllFailureCallback, "Async::readAllThen did not invoke its failure callback");

	std::stop_source cancelBeforeSource;
	cancelBeforeSource.request_stop();
	Euph::Io::MemoryDevice cancelBeforeDevice(Elv::Io::Mode::READ, std::vector<std::byte>(source.begin(), source.end()));
	auto cancelBeforeFuture = AsyncIo::readAll(pool, cancelBeforeDevice, cancelBeforeSource.get_token());
	requireCanceled(cancelBeforeFuture, "Async::readAll should throw when canceled before start");

	std::stop_source chunkStopSource;
	Euph::Io::MemoryDevice chunkDevice(Elv::Io::Mode::READ, std::vector<std::byte>(source.begin(), source.end()));
	size_t chunkCallbacks = 0;
	auto chunkFuture = AsyncIo::readChunks(pool, chunkDevice, chunkStopSource.get_token(), 2, [&](std::span<const std::byte> chunk) {
		++chunkCallbacks;
		requireCondition(chunk.size() == 2, "Async::readChunks returned the wrong chunk size");
		chunkStopSource.request_stop();
	});
	requireCanceled(chunkFuture, "Async::readChunks should throw after cancellation is requested");
	requireCondition(chunkCallbacks == 1, "Async::readChunks should stop after the first canceled chunk");

	Euph::Io::MemoryDevice chunksThenDevice(Elv::Io::Mode::READ, std::vector<std::byte>(source.begin(), source.end()));
	size_t chunksThenBytes = 0;
	size_t chunksThenCompletionBytes = 0;
	bool chunksThenComplete = false;
	auto chunksThenFuture = AsyncIo::readChunksThen(pool, chunksThenDevice, 2, [&](std::span<const std::byte> chunk) {
		chunksThenBytes += chunk.size();
	}, [&](const ByteResult& result) {
		chunksThenComplete = true;
		requireCondition(result.has_value(), "Async::readChunksThen completion expected success");
		chunksThenCompletionBytes = result.value();
	});
	requireCondition(chunksThenFuture.get() == source.size(), "Async::readChunksThen returned the wrong total byte count");
	requireCondition(chunksThenComplete, "Async::readChunksThen did not invoke its completion callback");
	requireCondition(chunksThenBytes == source.size() && chunksThenCompletionBytes == source.size(), "Async::readChunksThen reported the wrong byte totals");

	Euph::Io::MemoryDevice borrowedChunksThenDevice(Elv::Io::Mode::READ, std::vector<std::byte>(source.begin(), source.end()));
	std::array<std::byte, 2> borrowedChunk {};
	size_t borrowedChunksThenBytes = 0;
	bool borrowedChunksThenComplete = false;
	auto borrowedChunksThenFuture = AsyncIo::readChunksThen(pool, borrowedChunksThenDevice, borrowedChunk.data(), borrowedChunk.size(), [&](std::span<const std::byte> chunk) {
		borrowedChunksThenBytes += chunk.size();
	}, [&](const ByteResult& result) {
		borrowedChunksThenComplete = true;
		requireCondition(result.has_value() && result.value() == source.size(), "Async::readChunksThen borrowed completion received the wrong result");
	});
	requireCondition(borrowedChunksThenFuture.get() == source.size(), "Async::readChunksThen borrowed overload returned the wrong total byte count");
	requireCondition(borrowedChunksThenComplete && borrowedChunksThenBytes == source.size(), "Async::readChunksThen borrowed overload did not report all bytes");

	ThrowingDevice chunksThenThrowingDevice;
	bool chunksThenFailureComplete = false;
	auto chunksThenFailureFuture = AsyncIo::readChunksThen(pool, chunksThenThrowingDevice, 2, [](std::span<const std::byte>) {
		throw std::runtime_error("Async::readChunksThen should not deliver a chunk after device failure");
	}, [&](const ByteResult& result) {
		chunksThenFailureComplete = true;
		requireCondition(!result.has_value(), "Async::readChunksThen device failure completion expected an error");
		requireCondition(result.error() != nullptr, "Async::readChunksThen device failure completion did not receive an exception");
	});
	requireRuntimeError(chunksThenFailureFuture, "Async::readChunksThen future should rethrow device failures");
	requireCondition(chunksThenFailureComplete, "Async::readChunksThen did not invoke completion callback after device failure");

	std::stop_source chunksThenStopSource;
	Euph::Io::MemoryDevice chunksThenCancelDevice(Elv::Io::Mode::READ, std::vector<std::byte>(source.begin(), source.end()));
	size_t chunksThenCancelCallbacks = 0;
	bool chunksThenCancelComplete = false;
	auto chunksThenCancelFuture = AsyncIo::readChunksThen(pool, chunksThenCancelDevice, chunksThenStopSource.get_token(), 2, [&](std::span<const std::byte> chunk) {
		++chunksThenCancelCallbacks;
		requireCondition(chunk.size() == 2, "Async::readChunksThen cancellation returned the wrong chunk size");
		chunksThenStopSource.request_stop();
	}, [&](const ByteResult& result) {
		chunksThenCancelComplete = true;
		requireCondition(!result.has_value(), "Async::readChunksThen cancellation completion expected an error");
		requireCondition(result.error() != nullptr, "Async::readChunksThen cancellation completion did not receive an exception");
	});
	requireCanceled(chunksThenCancelFuture, "Async::readChunksThen future should throw after cancellation is requested");
	requireCondition(chunksThenCancelComplete && chunksThenCancelCallbacks == 1, "Async::readChunksThen cancellation did not stop after the first chunk");

	Euph::Io::MemoryDevice chunksThenCallbackFailureDevice(Elv::Io::Mode::READ, std::vector<std::byte>(source.begin(), source.end()));
	bool chunksThenCallbackFailureComplete = false;
	auto chunksThenCallbackFailureFuture = AsyncIo::readChunksThen(pool, chunksThenCallbackFailureDevice, 2, [](std::span<const std::byte>) {
		throw std::runtime_error("Async::readChunksThen per-chunk callback failed");
	}, [&](const ByteResult& result) {
		chunksThenCallbackFailureComplete = true;
		requireCondition(!result.has_value(), "Async::readChunksThen chunk callback failure completion expected an error");
		requireCondition(result.error() != nullptr, "Async::readChunksThen chunk callback failure completion did not receive an exception");
	});
	requireRuntimeError(chunksThenCallbackFailureFuture, "Async::readChunksThen future should rethrow per-chunk callback failures");
	requireCondition(chunksThenCallbackFailureComplete, "Async::readChunksThen did not invoke completion callback after per-chunk callback failure");

	ZeroReadDevice zeroReadDevice;
	auto zeroReadFuture = AsyncIo::readAll(pool, zeroReadDevice);
	requireRuntimeError(zeroReadFuture, "Async::readAll should fail when read returns zero before the expected byte count");

	Elv::Io::sDevice sharedDevice = std::make_shared<Euph::Io::MemoryDevice>(Elv::Io::Mode::READ, std::vector<std::byte>(source.begin(), source.end()));
	auto sharedFuture = AsyncIo::readAll(pool, sharedDevice);
	sharedDevice.reset();
	const auto sharedBytes = sharedFuture.get();
	requireCondition(bytesEqual(sharedBytes, source), "Async shared-device overload returned the wrong bytes");

	std::cout << "Async I/O tests passed" << std::endl;
}

namespace {

void makeResident(Euph::Asset::ResourceRegistry& registry, Euph::Asset::AssetId id)
{
	using Euph::Asset::ResidencyState;
	requireCondition(registry.setState(id, ResidencyState::Requested), "Known -> Requested should be legal");
	requireCondition(registry.setState(id, ResidencyState::LoadingIO), "Requested -> LoadingIO should be legal");
	requireCondition(registry.setState(id, ResidencyState::Decoding), "LoadingIO -> Decoding should be legal");
	requireCondition(registry.setState(id, ResidencyState::Resident), "Decoding -> Resident should be legal");
}

void pumpScheduler(Euph::Asset::StreamingScheduler& scheduler, Euph::Asset::AssetId id, Euph::Asset::ResidencyState wanted)
{
	for (int i = 0; i < 200; ++i) {
		scheduler.pollCompletions();
		if (scheduler.state(id) == wanted)
			return;
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	throw std::runtime_error("StreamingScheduler did not reach the expected state");
}

} // namespace

void testAssetRuntime()
{
	using namespace Euph::Asset;

	ResourceRegistry registry;

	const auto texture = registry.createHandle<ImageResource>(1);
	makeResident(registry, 1);
	requireCondition(registry.isAlive(texture), "Fresh texture handle should be alive");
	requireCondition(registry.destroy(1), "Destroying a live resource should succeed");
	const auto textureAgain = registry.createHandle<ImageResource>(1);
	requireCondition(!registry.isAlive(texture), "Old handle generation should be invalid after destroy/recreate");
	requireCondition(registry.isAlive(textureAgain), "Recreated handle should be alive");
	requireCondition(texture.generation != textureAgain.generation, "Recreated handle should have a different generation");
	requireCondition(!registry.setState(1, ResidencyState::LoadingIO), "Resident-like illegal state transition should be rejected");

	AssetCatalog catalog;
	requireCondition(catalog.addRecord({ 2, "dep.bin", 0, 4, 4, Compression::None, "blob", {} }), "Dependency record should be accepted");
	requireCondition(catalog.addRecord({ 3, "parent.bin", 0, 4, 4, Compression::None, "blob", { 2 } }), "Parent record should be accepted");
	requireCondition(catalog.addRecord({ 7, "offset.bin", 2, 4, 4, Compression::None, "blob", {} }), "Offset record should be accepted");
	requireCondition(catalog.findByPath("parent.bin") && catalog.findByPath("parent.bin")->id == 3, "Catalog path lookup should work");
	requireCondition(catalog.addRecord({ 3, "parent-renamed.bin", 0, 4, 4, Compression::None, "blob", { 2 } }), "Catalog should allow replacing an existing record");
	requireCondition(catalog.findByPath("parent.bin") == nullptr && catalog.findByPath("parent-renamed.bin")->id == 3, "Catalog should update path indexes when replacing records");
	requireCondition(catalog.validateDependencies().ok, "Catalog dependencies should validate");
	requireCondition(catalog.dependenciesOf(3).size() == 1, "Catalog should expose dependency spans");

	DependencyGraph graph(catalog);
	const DependencyResult deps = graph.resolveTransitive(3);
	requireCondition(deps.ok && deps.orderedDependencies.size() == 1 && deps.orderedDependencies.front() == 2, "Dependency graph should resolve transitive dependencies");
	requireCondition(graph.hasDependency(3, 2), "Dependency graph should answer transitive dependency queries");
	requireCondition(graph.dependentsOf(2).size() == 1 && graph.dependentsOf(2).front() == 3, "Dependency graph should list dependents");

	registry.createHandle<GenericBlobResource>(2);
	registry.createHandle<GenericBlobResource>(3);

	Elv::Util::ThreadPool pool(2);
	auto loader = [](const AssetRecord& record) -> Elv::Io::sDevice {
		std::vector<std::byte> data;
		if (record.id == 7) {
			data = { std::byte { 0x00 }, std::byte { 0x01 }, std::byte { 0x10 }, std::byte { 0x20 }, std::byte { 0x30 }, std::byte { 0x40 }, std::byte { 0x7f } };
		} else {
			data.assign(static_cast<std::size_t>(record.storedSize), static_cast<std::byte>(record.id));
		}
		return std::make_shared<Euph::Io::MemoryDevice>(Elv::Io::Mode::READ, std::move(data));
	};
	auto decoder = [](const AssetRecord&, std::span<const std::byte> bytes) {
		DecodedAsset decoded;
		decoded.payload = makeBytePayload(std::vector<std::byte>(bytes.begin(), bytes.end()));
		decoded.cpuBytes = decoded.payload.bytes;
		return decoded;
	};

	StreamingScheduler scheduler(catalog, registry, pool, loader, decoder);
	requireCondition(scheduler.request(3, { 5 }), "Parent request should be accepted");
	requireCondition(registry.state(3) == ResidencyState::WaitingDependencies, "Parent should wait until dependency is resident");
	pumpScheduler(scheduler, 2, ResidencyState::Resident);
	pumpScheduler(scheduler, 3, ResidencyState::Resident);
	requireCondition(registry.cpuCost(3) == 4, "Decoded resource should account CPU cost");
	requireCondition(registry.payload(3) && registry.payload(3)->bytes == 4, "Decoded resource should retain resident payload");

	requireCondition(scheduler.request(7, { 1 }), "Offset request should be accepted");
	pumpScheduler(scheduler, 7, ResidencyState::Resident);
	const ResidentPayload* offsetPayload = registry.payload(7);
	requireCondition(offsetPayload && offsetPayload->bytes == 4, "Offset asset should read only stored bytes");
	auto offsetBytes = std::static_pointer_cast<std::vector<std::byte> >(offsetPayload->data);
	requireCondition(offsetBytes->at(0) == std::byte { 0x10 } && offsetBytes->at(3) == std::byte { 0x40 }, "Offset asset should read from AssetRecord::offset");

	AssetCatalog badCatalog;
	badCatalog.addRecord({ 4, "bad.bin", 0, 1, 1, Compression::None, "blob", { 404 } });
	ResourceRegistry badRegistry;
	StreamingScheduler badScheduler(badCatalog, badRegistry, pool, loader, decoder);
	requireCondition(badScheduler.request(4), "Missing dependency request should produce a terminal state");
	requireCondition(badRegistry.state(4) == ResidencyState::Failed && badRegistry.failureReason(4) == FailureReason::MissingDependency, "Missing dependency should fail with diagnostic reason");

	AssetCatalog cycleCatalog;
	cycleCatalog.addRecord({ 5, "a.bin", 0, 1, 1, Compression::None, "blob", { 6 } });
	cycleCatalog.addRecord({ 6, "b.bin", 0, 1, 1, Compression::None, "blob", { 5 } });
	ResourceRegistry cycleRegistry;
	StreamingScheduler cycleScheduler(cycleCatalog, cycleRegistry, pool, loader, decoder);
	requireCondition(cycleScheduler.request(5), "Cycle request should produce a terminal state");
	requireCondition(cycleRegistry.state(5) == ResidencyState::Failed && cycleRegistry.failureReason(5) == FailureReason::DependencyCycle, "Dependency cycle should fail with diagnostic reason");

	ResourceRegistry refRegistry;
	const auto leasedHandle = refRegistry.createHandle<GenericBlobResource>(8);
	makeResident(refRegistry, 8);
	{
		ResourceLease<GenericBlobResource> lease(refRegistry, leasedHandle);
		requireCondition(lease && refRegistry.strongRefs(8) == 1, "ResourceLease should retain resources");
		requireCondition(!refRegistry.destroy(8), "Destroy should reject strongly retained resources");
	}
	requireCondition(refRegistry.strongRefs(8) == 0 && refRegistry.destroy(8), "ResourceLease should release resources on destruction");

	ResourceRegistry budgetRegistry;
	AssetCatalog budgetCatalog;
	budgetCatalog.addRecord({ 10, "pinned.bin", 0, 1, 1, Compression::None, "blob", {} });
	budgetCatalog.addRecord({ 11, "old.bin", 0, 1, 1, Compression::None, "blob", {} });
	budgetCatalog.addRecord({ 12, "important.bin", 0, 1, 1, Compression::None, "blob", {} });
	budgetCatalog.addRecord({ 13, "dependency.bin", 0, 1, 1, Compression::None, "blob", {} });
	budgetCatalog.addRecord({ 14, "dependent.bin", 0, 1, 1, Compression::None, "blob", { 13 } });
	ResidencyManager residency(budgetRegistry, &budgetCatalog);
	for (AssetId id : { AssetId(10), AssetId(11), AssetId(12) }) {
		budgetRegistry.createHandle<GenericBlobResource>(id);
		makeResident(budgetRegistry, id);
		budgetRegistry.setCost(id, BudgetKind::CpuBytes, 100);
		requireCondition(residency.makeEvictable(id), "Resident resource should become evictable");
	}
	for (AssetId id : { AssetId(13), AssetId(14) }) {
		budgetRegistry.createHandle<GenericBlobResource>(id);
		makeResident(budgetRegistry, id);
		budgetRegistry.setCost(id, BudgetKind::CpuBytes, 100);
		requireCondition(residency.makeEvictable(id), "Resident dependency resources should become evictable");
	}
	budgetRegistry.setPriority(10, { 0 });
	budgetRegistry.setPriority(11, { 0 });
	budgetRegistry.setPriority(12, { 10 });
	budgetRegistry.setPriority(13, { 0 });
	budgetRegistry.setPriority(14, { 20 });
	budgetRegistry.touch(10, 1);
	budgetRegistry.touch(11, 2);
	budgetRegistry.touch(12, 0);
	budgetRegistry.touch(13, 0);
	budgetRegistry.touch(14, 0);
	residency.pin(10);
	residency.setBudget(BudgetKind::CpuBytes, 250);
	const EvictionResult evicted = residency.evictUntilWithinBudgetDetailed();
	requireCondition(evicted.evicted.size() == 3, "Residency manager should evict enough resources to satisfy budget");
	requireCondition(evicted.evicted[0] == 11 && evicted.evicted[1] == 12 && evicted.evicted[2] == 14, "Eviction should respect pinning, priority, touch age, and deterministic ordering");
	requireCondition(budgetRegistry.state(10) == ResidencyState::Evictable, "Pinned resource should not be evicted");
	requireCondition(std::find(evicted.blocked.begin(), evicted.blocked.end(), 13) != evicted.blocked.end(), "Dependency resource should be protected while its dependent is resident");

	ResourceRegistry placeholderRegistry;
	const auto fallback = placeholderRegistry.createHandle<ImageResource>(30);
	const auto wanted = placeholderRegistry.createHandle<ImageResource>(31);
	makeResident(placeholderRegistry, 30);
	placeholderRegistry.setPlaceholder<ImageResource>(fallback);
	requireCondition(placeholderRegistry.resolveOrPlaceholder(wanted) == fallback, "Nonresident resource should resolve to typed placeholder");

	AssetCatalog failingCatalog;
	failingCatalog.addRecord({ 40, "iofail.bin", 0, 1, 1, Compression::None, "blob", {} });
	ResourceRegistry failingRegistry;
	auto failingLoader = [](const AssetRecord&) -> Elv::Io::sDevice {
		return std::make_shared<ThrowingDevice>();
	};
	StreamingScheduler failingScheduler(failingCatalog, failingRegistry, pool, failingLoader, decoder);
	requireCondition(failingScheduler.request(40), "Failing I/O request should be accepted");
	pumpScheduler(failingScheduler, 40, ResidencyState::Failed);
	requireCondition(failingRegistry.failureReason(40) == FailureReason::IoError, "I/O failure should be reported on resource state");

	std::cout << "Asset runtime tests passed" << std::endl;
}

void testGpuAssetStreamer()
{
	using namespace Euph::Asset;
	using namespace Lotte::Asset;

	ResourceRegistry cpuRegistry;
	cpuRegistry.createHandle<GenericBlobResource>(100);
	makeResident(cpuRegistry, 100);
	cpuRegistry.setPayload(100, makeBytePayload({ std::byte { 1 }, std::byte { 2 }, std::byte { 3 }, std::byte { 4 } }));

	GpuAssetStreamer gpu(cpuRegistry, 1000);
	gpu.setBudget(16);
	requireCondition(gpu.requestBuffer(100, { 7 }, { 1 }), "GPU buffer upload request should be accepted");
	GpuCommandBatch upload = gpu.recordUploads();
	requireCondition(upload.commandCount() == 1 && upload.uploads.size() == 1, "GPU upload recording should emit one create command");
	requireCondition(upload.uploads.front().assetId == 100 && upload.uploads.front().handle == 1000, "GPU upload should allocate a stable Kaldi handle");
	requireCondition(upload.retainedPayloads.size() == 1, "GPU upload batch should retain source payload memory");
	requireCondition(!gpu.handle(100), "GPU handle should not be visible before upload commit");
	gpu.commitUploads(upload);
	requireCondition(gpu.handle(100) && *gpu.handle(100) == 1000, "GPU handle should become visible after upload commit");
	requireCondition(gpu.currentUsage() == 4, "GPU streamer should account committed upload bytes");

	cpuRegistry.createHandle<GenericBlobResource>(101);
	requireCondition(gpu.requestBuffer(101), "Requesting a nonresident CPU resource should be accepted for diagnostic failure");
	(void)gpu.recordUploads();
	requireCondition(gpu.state(101) == GpuResidencyState::Failed && gpu.failureReason(101) == GpuFailureReason::CpuResourceNotResident, "GPU streamer should fail nonresident CPU resources with a diagnostic");

	for (AssetId id : { AssetId(102), AssetId(103) }) {
		cpuRegistry.createHandle<GenericBlobResource>(id);
		makeResident(cpuRegistry, id);
		cpuRegistry.setPayload(id, makeBytePayload({ static_cast<std::byte>(id == 102 ? 0x12 : 0x13), std::byte { 0 }, std::byte { 1 }, std::byte { 2 } }));
	}
	requireCondition(gpu.requestBuffer(102, {}, { 1 }), "Low-priority GPU upload should be queued");
	requireCondition(gpu.requestBuffer(103, {}, { 9 }), "High-priority GPU upload should be queued");
	GpuCommandBatch priorityUploads = gpu.recordUploads();
	requireCondition(priorityUploads.uploads.size() == 2 && priorityUploads.uploads[0].assetId == 103 && priorityUploads.uploads[1].assetId == 102, "GPU uploads should record in priority order");
	gpu.commitUploads(priorityUploads);
	requireCondition(gpu.currentUsage() == 12, "GPU usage should include all committed buffers");

	gpu.setBudget(64);
	cpuRegistry.createHandle<ImageResource>(105);
	makeResident(cpuRegistry, 105);
	auto decodeTarget = std::make_shared<Euph::Media::Image::DecodeTarget>();
	decodeTarget->setFormat(Euph::Media::Image::Format::RGBA8U);
	decodeTarget->addFrame(2, 2);
	cpuRegistry.setPayload(105, makeDecodeTargetPayload(decodeTarget));
	requireCondition(gpu.requestTexture(105, { 1, false }, { 10 }), "Texture upload from DecodeTarget should be accepted");
	GpuCommandBatch textureUpload = gpu.recordUploads();
	requireCondition(textureUpload.uploads.size() == 1 && textureUpload.uploads.front().kind == GpuResourceKind::Texture, "Texture upload should record a Kaldi texture command");
	requireCondition(textureUpload.retainedPayloads.size() == 1, "Texture upload should retain the DecodeTarget through command recording");
	gpu.commitUploads(textureUpload);
	requireCondition(gpu.handle(105).has_value(), "Texture handle should become visible after commit");

	requireCondition(gpu.makeEvictable(100) && gpu.makeEvictable(102) && gpu.makeEvictable(103) && gpu.makeEvictable(105), "Committed GPU resources should become evictable");
	gpu.pin(100);
	requireCondition(gpu.retain(102), "GPU retain should protect resources from eviction");
	gpu.setPriority(102, { 0 });
	gpu.setPriority(103, { 0 });
	gpu.setPriority(105, { 20 });
	gpu.touch(102, 2);
	gpu.touch(103, 1);
	gpu.touch(105, 0);
	gpu.setBudget(24);
	GpuCommandBatch evictions = gpu.recordEvictions();
	requireCondition(evictions.destroys.size() == 1 && evictions.destroys.front().assetId == 103, "Budget eviction should skip pinned/retained resources and evict the oldest low-priority resource");
	gpu.commitEvictions(evictions);
	requireCondition(!gpu.handle(103), "Committed eviction should hide the destroyed handle");
	requireCondition(gpu.currentUsage() == 24, "Pinned, retained, and high-priority resources should remain resident after partial eviction");

	requireCondition(gpu.release(102), "GPU release should drop the protection reference");
	gpu.setBudget(20);
	GpuCommandBatch followupEvictions = gpu.recordEvictions();
	requireCondition(followupEvictions.destroys.size() == 1 && followupEvictions.destroys.front().assetId == 102, "Released resource should become eligible on the next budget pass");
	gpu.commitEvictions(followupEvictions);
	requireCondition(gpu.currentUsage() == 20, "Second eviction should update GPU usage");

	requireCondition(!gpu.requestEvict(100), "Pinned resources should reject explicit eviction requests");
	gpu.unpin(100);
	requireCondition(gpu.requestEvict(100), "Unpinned resources should accept explicit eviction requests");
	GpuCommandBatch explicitDestroy = gpu.recordEvictions();
	requireCondition(explicitDestroy.destroys.size() == 1 && explicitDestroy.destroys.front().assetId == 100, "Explicit eviction should record even when it is not enough to satisfy budget");
	gpu.commitEvictions(explicitDestroy);

	cpuRegistry.createHandle<GenericBlobResource>(106);
	makeResident(cpuRegistry, 106);
	cpuRegistry.setPayload(106, makeBytePayload({ std::byte { 6 }, std::byte { 6 }, std::byte { 6 }, std::byte { 6 } }));
	gpu.setBudget(64);
	requireCondition(gpu.requestBuffer(106), "Upload failure test should queue a buffer");
	GpuCommandBatch failedUpload = gpu.recordUploads();
	requireCondition(failedUpload.uploads.size() == 1, "Upload failure test should record a create command before simulated rejection");
	gpu.failUploads(failedUpload, GpuFailureReason::BackendRejected, "simulated rejection");
	requireCondition(gpu.state(106) == GpuResidencyState::Failed && !gpu.handle(106), "Rejected upload should fail without publishing a handle");

	std::cout << "GPU asset streamer tests passed" << std::endl;
}

void randomDeviceTest() {
	std::vector<int> intVec(12,0);
	std::cout << "Before random: ";
	for(const auto& it : intVec) {
		std::cout << it << ' ';
	}
	Euph::Io::RandomDevice randdev(Euph::Io::RandomSource::URANDOM);
	Elv::Io::DataStream<Elv::Util::Endian::Native> randStream(randdev);
	for(auto& it : intVec)
	{
		randStream >> it;
	}
	std::cout << "\nAfter random: ";
	for(const auto& it : intVec) {
		std::cout << it << ' ';
	}
	std::cout << std::endl;}
void testMapDatastream() {
	Euph::Io::UniqueChunkedArrayBuffer<256,Mallocator<std::array<std::byte,256>>> buff;
	Elv::Io::DataStream<Elv::Util::Endian::Big> datastream(buff);
	datastream << baseMap;
	{
		std::map<int,std::string> newMap;
		buff.seek(0, Elv::Io::SeekOrigin::SET);
		datastream >> newMap;
		for(auto it = newMap.begin(); it != newMap.end(); ++it)
		{
			std::cout << it->first << ' ' << it->second << std::endl;
		}
	}
}
void testMemoryAllocator() {
	typedef Elv::Util::AlexandrescuAllocatorAdapter<Elv::Util::StaticBitmapAllocator<8,1024>,int> IntAllocator;
	std::vector<int,IntAllocator> intarr(2);
	intarr.reserve(600);
	for(int i = 0; i < 600;++i) {
		intarr.emplace_back(i);
	}
	std::span<int> intspan(intarr);
	for(const auto& it : intspan) {
		std::cout << it << std::endl;
	}
}
void testIntegralIterable() {
	Elv::Util::IntegralIterable<int> range(10);
	Elv::Util::IntegralIterable<int> range2(20);
	range = range2;
	for(auto it = std::rbegin(range); it != std::rend(range); ++it) {
		std::cout << *it << std::endl;
	}
	for(const auto& it : range) {
		std::cout << it << std::endl;
	}
}
template <class WriteDataClass, class ReadDataClass> void testProxyWithMaps() {
	Euph::Io::UniqueChunkedArrayBuffer<1024,Mallocator<std::array<std::byte,1024>>> buff;
	// Stage 1: Writing into our buffer
	{
		WriteDataClass compressor(&buff);
		Elv::Io::DataStream<Elv::Util::Endian::Big> datastream(compressor);
		datastream << baseMap;
		compressor.flush();
	}
	// Stage 2: Reading back
	{
		std::map<int,std::string> newMap;
		buff.seek(0, Elv::Io::SeekOrigin::SET);
		ReadDataClass decompressor(&buff);
		Elv::Io::DataStream<Elv::Util::Endian::Big> datastream(decompressor);
		datastream >> newMap;
		for(auto it = newMap.begin(); it != newMap.end(); ++it)
		{
			std::cout << it->first << ' ' << it->second << std::endl;
		}
	}
}
template <class WriteDataClassA, class WirteDataClassB, class ReadDataClassA, class ReadDataClassB> void testProxyWithMaps2() {
	Euph::Io::UniqueChunkedArrayBuffer<1024,Mallocator<std::array<std::byte,1024>>> buff;
	// Stage 1: Writing into our buffer
	{
		WirteDataClassB encryptor(&buff);
		WriteDataClassA compressor(&encryptor);
		Elv::Io::DataStream<Elv::Util::Endian::Big> datastream(compressor);
		datastream << baseMap;
		compressor.flush();
		encryptor.flush();
	}
	// Stage 2: Reading back
	{
		std::map<int,std::string> newMap;
		buff.seek(0, Elv::Io::SeekOrigin::SET);
		ReadDataClassB decryptor(&buff);
		ReadDataClassA decompressor(&decryptor);
		Elv::Io::DataStream<Elv::Util::Endian::Big> datastream(decompressor);
		datastream >> newMap;
		for(auto it = newMap.begin(); it != newMap.end(); ++it)
		{
			std::cout << it->first << ' ' << it->second << std::endl;
		}
	}
}
template <class WriteDataClass, class ReadDataClass> void testProxyWithLargeData() {
	Euph::Io::UniqueChunkedArrayBuffer<1024,Mallocator<std::array<std::byte,1024>>> buff;
	std::vector<int> integers(492768);
	std::vector<int> integers2;
	// Stage 0: Filling the vector
	{
		Euph::Io::RandomDevice randdev(Euph::Io::RandomSource::URANDOM);
		Elv::Io::DataStream<Elv::Util::Endian::Native> datastream(randdev);
		datastream.readElementsInto<int>(integers.begin(),integers.end());
	}
	// Stage 1: Writing into our buffer
	{
		WriteDataClass compressor(&buff);
		Elv::Io::DataStream<Elv::Util::Endian::Big> datastream(compressor);
		datastream << integers;
		compressor.flush();
	}
	// Stage 2: Reading back
	{
		buff.seek(0, Elv::Io::SeekOrigin::SET);
		ReadDataClass decompressor(&buff);
		Elv::Io::DataStream<Elv::Util::Endian::Big> datastream(decompressor);
		datastream >> integers2;
	}
	// Stage 3: Comparing the values
	if(integers.size() != integers2.size()) throw std::runtime_error("Their sizes are supposed to be the same element count!!");
	for(size_t i = 0; i < integers.size(); ++i)
	{
		if(integers[i] != integers2[i]) {
			std::cout << "Fail! " << integers[i] << " and " << integers2[i] << " are not the same! Decompression/decryption went wrong at index [" << i << "]!" << std::endl;
			return;
		}
	}
	std::cout << "Success!" << std::endl;
}
template <class WriteDataClassA, class WirteDataClassB, class ReadDataClassA, class ReadDataClassB> void testProxyWithLargeData2() {
	Euph::Io::UniqueChunkedArrayBuffer<1024,Mallocator<std::array<std::byte,1024>>> buff;
	std::vector<int> integers(492768);
	std::vector<int> integers2;
	// Stage 0: Filling the vector
	{
		Euph::Io::RandomDevice randdev(Euph::Io::RandomSource::URANDOM);
		Elv::Io::DataStream<Elv::Util::Endian::Native> datastream(randdev);
		datastream.readElementsInto<int>(integers.begin(),integers.end());
	}
	// Stage 1: Writing into our buffer
	{
		WirteDataClassB encryptor(&buff);
		WriteDataClassA compressor(&encryptor);
		Elv::Io::DataStream<Elv::Util::Endian::Big> datastream(compressor);
		datastream << integers;
		compressor.flush();
		encryptor.flush();
	}
	// Stage 2: Reading back
	{
		buff.seek(0, Elv::Io::SeekOrigin::SET);
		ReadDataClassB decryptor(&buff);
		ReadDataClassA decompressor(&decryptor);
		Elv::Io::DataStream<Elv::Util::Endian::Big> datastream(decompressor);
		datastream >> integers2;
	}
	// Stage 3: Comparing the values
	if(integers.size() != integers2.size()) throw std::runtime_error("Their sizes are supposed to be the same element count!!");
	for(size_t i = 0; i < integers.size(); ++i)
	{
		if(integers[i] != integers2[i]) {
			std::cout << "Fail! " << integers[i] << " and " << integers2[i] << " are not the same! Decompression/decryption went wrong at index [" << i << "]!" << std::endl;
			return;
		}
	}
	std::cout << "Success!" << std::endl;
}
void testZstdWithMaps() {
	testProxyWithMaps<Euph::Io::ZstdCompressor,Euph::Io::ZstdDecompressor>();
}
void testZstdWithLargeData() {
	testProxyWithLargeData<Euph::Io::ZstdCompressor,Euph::Io::ZstdDecompressor>();
}
class EncryptionTest : public Euph::Io::CryptoWriteDeviceProxy {
public:
	EncryptionTest(Elv::Io::Device* underlyingDevice) : CryptoWriteDeviceProxy(underlyingDevice,1024,2048, Euph::Io::CipherType::ARIA_192_CTR, Encryptionkey) {

	}
};
class DecryptionTest : public Euph::Io::CryptoReadDeviceProxy {
public:
	DecryptionTest(Elv::Io::Device* underlyingDevice) : CryptoReadDeviceProxy(underlyingDevice,1024,1024, Euph::Io::CipherType::ARIA_192_CTR, Encryptionkey) {

	}
};

void testEncryptionWithMaps()
{
	testProxyWithMaps<EncryptionTest,DecryptionTest>();
}

void testEncryptionWithLargeData()
{
	testProxyWithLargeData<EncryptionTest,DecryptionTest>();
}

void testCompressionAndEncryptionWithMaps()
{
	testProxyWithMaps2<Euph::Io::ZstdCompressor,EncryptionTest,Euph::Io::ZstdDecompressor,DecryptionTest>();
}

void testCompressionAndEncryptionWithLargeData()
{
	testProxyWithLargeData2<Euph::Io::ZstdCompressor,EncryptionTest,Euph::Io::ZstdDecompressor,DecryptionTest>();
}

void testMemoryMapped()
{
	std::unique_ptr<Euph::Io::MemoryMapped> mapped(new Euph::Io::MemoryMappedFile("/home/legacy/allpages_remain.txt", Elv::Io::Mode::READ));
	std::string_view strToRead(static_cast<char*>(mapped->data()), mapped->size());
	std::cout << strToRead << std::endl;
}

void testMemoryMappedTemp()
{
	Euph::Io::MemoryMappedTempFile mmapped(1024);
	std::memset(mmapped.data(),0,1024);
	static_cast<char*>(mmapped.data())[0] = 'H';
	static_cast<char*>(mmapped.data())[1] = 'e';
	static_cast<char*>(mmapped.data())[2] = 'l';
	static_cast<char*>(mmapped.data())[3] = 'l';
	static_cast<char*>(mmapped.data())[4] = 'o';
	static_cast<char*>(mmapped.data())[5] = '\n';
	mmapped.flushSync(true);
	std::cout << static_cast<char*>(mmapped.data()) << std::endl;
}

static const char* FileModeToPath(Euph::Io::TemporaryFileCreationMode mode)
{
	switch (mode) {
		case Euph::Io::TemporaryFileCreationMode::MKSTEMP:
			return "";
		case Euph::Io::TemporaryFileCreationMode::SHM_OPEN:
			return "/hello";
		case Euph::Io::TemporaryFileCreationMode::MEMFD_CREATE:
			return "hello";
	}
	return nullptr;
}

typedef const char* (*myfunc)();
void testMemoryMappedTempDlopenNoClose(Euph::Io::TemporaryFileCreationMode mode)
{
	std::unique_ptr<Euph::Io::MemoryMappedTempFile> tempFile(nullptr);
	{
		//Euph::Io::File dllFile("/usr/lib64/libGL.so", Elv::Io::Mode::READ);
		// libsimple.so
		Euph::Io::File dllFile("/home/legacy/libsimple.so", Elv::Io::Mode::READ);
		tempFile = std::unique_ptr<Euph::Io::MemoryMappedTempFile>(new Euph::Io::MemoryMappedTempFile(dllFile.size(),mode,FileModeToPath(mode) ));
		dllFile.read(tempFile->data(),1,tempFile->size());
		tempFile->flushSync();
	}
	std::cout << "Beginning of mapped region: [" << tempFile->data() << ']' << std::endl;
	std::cout << "End of mapped region: [" << static_cast<void*>(static_cast<std::byte*>(tempFile->data()) + tempFile->size()) << ']' << std::endl;

	Elv::Util::DynamicLibrary dynLib(tempFile->getFilePath());
	/*void* glClearFunc = dynLib.link("glClear");
	if(glClearFunc != nullptr) std::cout << "Success! glClear is at address [" << glClearFunc << "]." << std::endl;
	else std::cout << "Failure! glClear is a nullptr! Linking failed!" << std::endl;*/
	myfunc hello_world =  dynLib.sym<myfunc>("get_hello_world");
	if(hello_world != nullptr) std::cout << "Success! get_hello_world is at address [" << hello_world << "]." << std::endl;
	else std::cout << "Failure! hello_world is a nullptr! Linking failed!" << std::endl;
	std::cout << hello_world() << std::endl;
}

void testMemoryMappedTempDlopenClose(Euph::Io::TemporaryFileCreationMode mode)
{
	std::unique_ptr<Elv::Util::DynamicLibrary> dynLib(nullptr);
	{
		std::unique_ptr<Euph::Io::MemoryMappedTempFile> tempFile(nullptr);
		{
			Euph::Io::File dllFile("/home/legacy/libsimple.so", Elv::Io::Mode::READ);
			tempFile = std::unique_ptr<Euph::Io::MemoryMappedTempFile>(new Euph::Io::MemoryMappedTempFile(dllFile.size(),mode,FileModeToPath(mode)));
			std::cout << tempFile->getFilePath() << std::endl;
			dllFile.read(tempFile->data(),1,tempFile->size());
			tempFile->flushSync();
		}
		dynLib = std::unique_ptr<Elv::Util::DynamicLibrary>(new Elv::Util::DynamicLibrary(tempFile->getFilePath()));
	}
	myfunc hello_world =  dynLib->sym<myfunc>("get_hello_world");
	if(hello_world != nullptr) std::cout << "Success! get_hello_world is at address [" << reinterpret_cast<void*>(hello_world) << "]." << std::endl;
	else std::cout << "Failure! hello_world is a nullptr! Linking failed!" << std::endl;
	std::cout << hello_world() << std::endl;
}

void testConfigFile()
{
	std::ifstream ifs;
	ifs.open("hello.ini");
	Euph::Conf::Configuration config(ifs);
	std::cout << config;
}
void testConfiguration()
{
	Euph::Conf::Configuration config;
	std::cout << config;
}

typedef Elv::Util::AlexandrescuAllocatorAdapter<Elv::Util::StaticBitmapAllocator<32,1024>,int> IntAllocator;
//typedef Elv::Util::AlexandrescuAllocatorAdapter<Elv::Util::StaticFreeListAllocator<sizeof(int)*20000>,int> IntAllocator;

void testCustomAllocator()
{
	Elv::Util::SharedHeapArray<int,IntAllocator> heapArr(1024);
	for(size_t i = 0; i < heapArr.size(); ++i) {
		heapArr[i] = i;
	}
	for(const auto& it : heapArr) {
		std::cout << it << std::endl;
	}
}

template <typename T> void testInterpolator() {
	// Testing simple linear interpolation
	{
		T d0 = static_cast<T>(0.0);
		T d1 = static_cast<T>(0.66);
		T w = static_cast<T>(0.5);
		T wMinusOne = static_cast<T>(1.0)-w;

		std::cout << "d0 = " << d0 << std::endl;
		std::cout << "d1 = " << d1 << std::endl;
		std::cout << "w = " << w << std::endl;
		std::cout << "1-w = " << wMinusOne << std::endl;
		std::cout << "d0 * (1-w) = " << d0*wMinusOne << std::endl;
		std::cout << "d1 * w = " << d1*w << std::endl;
		T output = Elv::Util::lerp(d0,d1,w);
		std::cout << "(d0 * (1-w)) + (d1 * w) = " << output << std::endl;
	}
}

void IsFixedPoint(const Elv::Util::FixedPoint auto& value)
{
	float flt = value.to_float();
}

void testInterpolationFloat()
{
	testInterpolator<float>();
}

void testInterpolationFixed()
{
	IsFixedPoint(Elv::Util::fixed32::from_float(0.33f));
	testInterpolator<Elv::Util::fixed32>();
}

void testTGA()
{
	Euph::Media::Image::DecodeTarget decodeTarget;
	{
		Euph::Media::Image::Image2D<Euph::Media::Image::PixelBGR_U8> stdimg(256,256);
		stdimg.clearToColour([](const glm::fvec2& normalizedPos){
			float dx = normalizedPos.x - 0.5f;
			float dy = normalizedPos.y - 0.5f;
			float distance = 1.0f - (std::sqrt(dx * dx + dy * dy) / 0.7071f);
			return glm::fvec4(normalizedPos.x,normalizedPos.y,distance,1.0f);
		},true);
		decodeTarget = fromImageToDecodeTarget(stdimg);
	}
	{
		Euph::Io::File dllFile("/tmp/testImg.tga", Elv::Io::Mode::WRITE);
		Euph::Media::Image::TGA::encode(dllFile,decodeTarget);
	}
}

void testPNG()
{
	Euph::Media::Image::DecodeTarget decodeTarget;
	{
		Euph::Media::Image::Image2D<Euph::Media::Image::PixelRGB_U8> stdimg(256,256);
		stdimg.clearToColour([](const glm::fvec2& normalizedPos){
			float dx = normalizedPos.x - 0.5f;
			float dy = normalizedPos.y - 0.5f;
			float distance = 1.0f - (std::sqrt(dx * dx + dy * dy) / 0.7071f);
			return glm::fvec4(normalizedPos.x,normalizedPos.y,distance,1.0f);
		},true);
		decodeTarget = fromImageToDecodeTarget(stdimg);
	}
	{
		Euph::Io::File dllFile("/tmp/testImg.png", Elv::Io::Mode::WRITE);
		Euph::Media::Image::PNG::encode(dllFile,decodeTarget,0.5f);
	}
}

void testMemoryArenas()
{
	using namespace Euph::Memory;

	requireCondition(&MemoryManager::resource(AllocatorSubsystem::AUDIO) == std::pmr::get_default_resource(),
		"Uninitialized arena access should fall back to the default PMR resource");

	PoolSizes sizes;
	sizes.scratch = 1u << 20;
	sizes.audio = 1u << 20;
	sizes.video = 1u << 20;
	sizes.script = 1u << 20;
	requireCondition(MemoryManager::initialize(sizes), "Arena initialization should succeed");
	requireCondition(MemoryManager::initialized(), "Arena manager should report initialized");
	requireCondition(!MemoryManager::initialize(sizes), "A second initialization should be rejected");

	auto& arena = MemoryManager::resource(AllocatorSubsystem::AUDIO);
	requireCondition(&arena != std::pmr::get_default_resource(), "Initialized arena should expose its own resource");

	void* overAligned = arena.allocate(64, 64);
	requireCondition(reinterpret_cast<std::uintptr_t>(overAligned) % 64u == 0,
		"Arena should honor over-aligned requests");
	arena.deallocate(overAligned, 64, 64);

	std::pmr::polymorphic_allocator<int> alloc(&arena);
	std::pmr::vector<int> values(alloc);
	for (int i = 0; i < 4096; ++i)
		values.push_back(i);
	requireCondition(values.size() == 4096, "PMR vector should grow through the arena");
	requireCondition(values.front() == 0 && values.back() == 4095, "PMR vector contents should round-trip");
	std::cout << "Memory arena tests passed" << std::endl;
}

void testFontRoundTrip()
{
	using namespace Lotte::Text;

	PreprocessedFontFace face;
	face.familyName = "RoundTrip";
	face.type = SDFType::MSDF;
	face.distType = DistanceType::Euclidean;
	face.bitmapSize = 32;
	face.bitmapLogicalSize = 24;
	face.bitmapPadding = 2;
	face.hasVert = true;
	face.imageFormat = { 'P', 'N', 'G', '\0', '\0', '\0', '\0', '\0' };
	face.ascender = 1.5f;
	face.descender = -0.5f;
	face.faceHeight = 2.0f;
	face.maxAdvance = 3.0f;
	face.unitsPerEm = 1000;
	face.kerning[65][66] = glm::fvec2(1.0f, 2.0f);

	StoredCharacter glyph;
	glyph.valid = true;
	glyph.width = 8;
	glyph.height = 9;
	glyph.bearingX = -1;
	glyph.bearingY = 2;
	glyph.advanceX = 7;
	glyph.advanceY = 10;
	glyph.metricWidth = 8.0f;
	glyph.metricHeight = 9.0f;
	glyph.horiBearingX = -1.0f;
	glyph.horiBearingY = 2.0f;
	glyph.horiAdvance = 7.0f;
	glyph.vertBearingX = 0.5f;
	glyph.vertBearingY = 1.0f;
	glyph.vertAdvance = 10.0f;
	glyph.sdf = { std::byte { 0x01 }, std::byte { 0x02 }, std::byte { 0x03 } };
	face.glyphs[65] = glyph;

	Euph::Io::MemoryDevice fontDevice(Elv::Io::Mode::READ_WRITE);
	FontDataStream<Euph::Io::MemoryDevice> fontStream(fontDevice);
	fontStream << face;
	fontDevice.seek(0, Elv::Io::SeekOrigin::SET);

	PreprocessedFontFace decodedFace;
	fontStream >> decodedFace;
	requireCondition(decodedFace.familyName == face.familyName, "Font family name should round-trip");
	requireCondition(decodedFace.type == face.type && decodedFace.distType == face.distType,
		"Font SDF and distance types should round-trip");
	requireCondition(decodedFace.bitmapSize == face.bitmapSize
			&& decodedFace.bitmapLogicalSize == face.bitmapLogicalSize
			&& decodedFace.bitmapPadding == face.bitmapPadding,
		"Font bitmap metadata should round-trip");
	requireCondition(decodedFace.unitsPerEm == face.unitsPerEm, "Font unitsPerEm should round-trip");
	requireCondition(decodedFace.glyphs.size() == 1 && decodedFace.glyphs.count(65) == 1,
		"Font glyph TOC should round-trip");
	const StoredCharacter& decodedGlyph = decodedFace.glyphs.at(65);
	requireCondition(decodedGlyph.valid && decodedGlyph.width == glyph.width
			&& decodedGlyph.height == glyph.height && decodedGlyph.sdf == glyph.sdf,
		"Font glyph payload should round-trip");
	requireCondition(decodedFace.kerning.at(65).at(66) == glm::fvec2(1.0f, 2.0f),
		"Font kerning map should round-trip");

	using VectorImage = Lotte::VectorGraphic::StoredVectorImage;
	using VectorMipmap = Lotte::VectorGraphic::Mipmap;
	VectorImage image;
	image.processingSize = 64;
	image.actualSize = 48;
	image.padding = 4;
	image.logicalX = 1.0f;
	image.logicalY = 2.0f;
	image.logicalWidth = 3.0f;
	image.logicalHeight = 4.0f;
	image.aspectRatio = 0.75f;
	image.minX = -1.0f;
	image.maxX = 1.0f;
	image.minY = -2.0f;
	image.maxY = 2.0f;
	image.distanceRangeX = 4.0f;
	image.distanceRangeY = 8.0f;
	image.encodingFlags = static_cast<std::uint32_t>(Lotte::VectorGraphic::EncodingFlag::Inverted);
	image.midpointAdjustment = 0.5f;
	image.type = SDFType::SDF;
	image.distType = DistanceType::Manhattan;
	VectorMipmap mipmap;
	mipmap.data = { std::byte { 0xAA }, std::byte { 0xBB } };
	image.mipmaps.push_back(std::move(mipmap));

	Euph::Io::MemoryDevice imageDevice(Elv::Io::Mode::READ_WRITE);
	Lotte::VectorGraphic::VectorGraphicDataStream<Euph::Io::MemoryDevice> imageStream(imageDevice);
	imageStream << image;
	imageDevice.seek(0, Elv::Io::SeekOrigin::SET);

	VectorImage decodedImage;
	imageStream >> decodedImage;
	requireCondition(decodedImage.processingSize == image.processingSize
			&& decodedImage.actualSize == image.actualSize
			&& decodedImage.padding == image.padding,
		"Vector-image sizes should round-trip");
	requireCondition(decodedImage.logicalX == image.logicalX
			&& decodedImage.logicalWidth == image.logicalWidth
			&& decodedImage.aspectRatio == image.aspectRatio,
		"Vector-image logical bounds should round-trip");
	requireCondition(decodedImage.type == image.type && decodedImage.distType == image.distType,
		"Vector-image SDF metadata should round-trip");
	requireCondition(decodedImage.mipmaps.size() == 1
			&& decodedImage.mipmaps.front().data == image.mipmaps.front().data,
		"Vector-image mipmaps should round-trip");
	std::cout << "Font/vector-image round-trip tests passed" << std::endl;
}

void testFontParser()
{
	Euph::Io::File fontFile("/home/legacy/vinque.bin", Elv::Io::Mode::READ);
	Lotte::Text::FontDataStream stream(fontFile);
	Lotte::Text::FontFace fface;
	stream >> fface;
	std::cout <<  "Family name: " << fface.familyName << "\n"
			  << "Number of elements: " << fface.glyphs.size() << "\n"
			  << "Padding: " << fface.bitmapPadding << "\n"
			  << "Pixel size: " << fface.bitmapSize << "\n"
			  << "Logical size: " << fface.bitmapLogicalSize << std::endl;
}

namespace {

/** @brief Playable that fills every sample with a constant value. */
class ConstantSource : public Hrk::Playable
{
private:
	float value;
public:
	/**
	 * @brief Creates a constant source.
	 * @param constant Value written to every sample.
	 */
	explicit ConstantSource(float constant) : value(constant) {}
	/** @brief Fills the packet with the constant value. */
	Hrk::FrameCount outputTo(Hrk::Output& output) override
	{
		if (output.samplerate == Hrk::SAMPLE_RATE_DONT_CARE) output.samplerate = Hrk::SampleRate(48000);
		if (!output.dst) return Hrk::FrameCount(0);
		const uintptr_t channels = uintptr_t(output.channels.var);
		for (uintptr_t f = 0; f < uintptr_t(output.frameCount.var); ++f) {
			for (uintptr_t c = 0; c < channels; ++c) output.dst[f * channels + c] = value;
		}
		return output.frameCount;
	}
};

/** @brief Effect that records the interleaving layout it was handed. */
class LayoutProbeEffect : public Hrk::Effect
{
public:
	/** @brief Set when the effect received channel-planar data. */
	bool sawPlanar = false;
	/** @brief Set when the effect received interleaved data. */
	bool sawInterleaved = false;
	/** @brief Passes the block through unchanged. */
	void process(const Hrk::ModificationStream& stream) override
	{
		if (stream.interleaving == Hrk::InterleavingType::SEPARATE_CHANNELS) sawPlanar = true;
		else sawInterleaved = true;
		const Hrk::SampleCount samples = Hrk::framesToSamples(stream.frameCount, stream.channels);
		const float* src = stream.src ? stream.src : stream.dst;
		for (uintptr_t i = 0; i < uintptr_t(samples.var); ++i) stream.dst[i] = src[i];
	}
	/** @brief Reports that this effect needs one plane per channel. */
	Hrk::InterleavingType getRequiredInterleaving() const override { return Hrk::InterleavingType::SEPARATE_CHANNELS; }
};

/** @brief Builds a two-glyph font face used by the layout tests. */
Lotte::Text::PreprocessedFontFace makeTestFontFace()
{
	using namespace Lotte::Text;
	PreprocessedFontFace face;
	face.familyName = "testface";
	face.faceHeight = 10.0f;
	face.ascender = 8.0f;
	face.descender = -2.0f;
	face.bitmapLogicalSize = 64;
	face.bitmapSize = 64;
	for (char32_t codePoint : { U'A', U'B' }) {
		StoredCharacter character;
		character.valid = true;
		// Model FontPacker's outline path: the SDF is generated from the *metric* box,
		// so the raw FreeType bitmap fields are never filled in.
		character.metricWidth = 8.0f;
		character.metricHeight = 10.0f;
		character.horiBearingX = 1.0f;
		character.horiBearingY = 8.0f;
		character.horiAdvance = 9.0f;
		character.advanceX = 9 * 64;
		face.glyphs.emplace(std::uint32_t(codePoint), std::move(character));
	}
	return face;
}

/** @brief Atlas mapping that pretends every glyph covers the whole texture. */
struct WholeTextureMapping : public Lotte::Text::GlyphAtlasMapping
{
	/** @brief Maps every code point to the full unit square. */
	bool mapGlyph(char32_t, glm::fvec2& uv0, glm::fvec2& uv1) const override
	{
		uv0 = glm::fvec2(0.0f);
		uv1 = glm::fvec2(1.0f);
		return true;
	}
};

} // namespace

void testAudioEffects()
{
	using namespace Hrk;

	{
		ConstantSource source(0.5f);
		EffectSlot slot(ChannelCount(2), SampleRate(48000), FrameCount(64));
		slot.setPlayable(&source);
		FX::ClampEffect clamp{ FX::Clamp{ 0.25f } };
		slot.addEffect(&clamp, 1.0f);
		requireCondition(!slot.requiresDeinterlacing(),
			"Sample-level effects must not request channel-planar input");

		float buffer[64 * 2] = {};
		Output out;
		out.dst = buffer;
		out.frameCount = FrameCount(64);
		out.samplerate = SampleRate(48000);
		out.channels = ChannelCount(2);
		out.interleaving = InterleavingType::INTERLEAVED;
		const FrameCount produced = slot.outputTo(out);
		requireCondition(produced.var == 64, "EffectSlot must produce the requested frame count");
		requireCondition(std::abs(buffer[0] - 1.0f) < 1e-5f,
			"A fully wet normalizing clamp must map 0.5 to 1.0");

		requireCondition(slot.setWet(0, 0.5f), "setWet must accept an existing index");
		std::fill(std::begin(buffer), std::end(buffer), 0.0f);
		slot.outputTo(out);
		requireCondition(std::abs(buffer[0] - 0.75f) < 1e-5f,
			"A half-wet stage must blend dry and processed signals");
	}

	{
		ConstantSource source(1.0f);
		EffectSlot monoSlot(ChannelCount(1), SampleRate(48000), FrameCount(16));
		monoSlot.setPlayable(&source);
		LayoutProbeEffect probe;
		monoSlot.addEffect(&probe, 1.0f);
		requireCondition(monoSlot.requiresDeinterlacing(),
			"Adding a planar effect must update the cached layout requirement");

		float buffer[16] = {};
		Output out;
		out.dst = buffer;
		out.frameCount = FrameCount(16);
		out.samplerate = SampleRate(48000);
		out.channels = ChannelCount(1);
		out.interleaving = InterleavingType::INTERLEAVED;
		requireCondition(monoSlot.outputTo(out).var == 16, "Mono slot must produce frames");
		requireCondition(!probe.sawPlanar && probe.sawInterleaved,
			"Mono streams must skip the deinterlacing pass");

		requireCondition(monoSlot.removeEffect(&probe), "removeEffect must find the effect");
		requireCondition(!monoSlot.requiresDeinterlacing(),
			"Removing the planar effect must clear the cached requirement");
	}

	{
		ConstantSource source(1.0f);
		EffectSlot stereoSlot(ChannelCount(2), SampleRate(48000), FrameCount(16));
		stereoSlot.setPlayable(&source);
		LayoutProbeEffect probe;
		stereoSlot.addEffect(&probe, 1.0f);
		float buffer[32] = {};
		Output out;
		out.dst = buffer;
		out.frameCount = FrameCount(16);
		out.samplerate = SampleRate(48000);
		out.channels = ChannelCount(2);
		out.interleaving = InterleavingType::INTERLEAVED;
		stereoSlot.outputTo(out);
		requireCondition(probe.sawPlanar, "Stereophonic planar effects must receive separate channel planes");
		requireCondition(std::abs(buffer[0] - 1.0f) < 1e-5f && std::abs(buffer[31] - 1.0f) < 1e-5f,
			"Planar round-trip must preserve every sample");
	}

	{
		ConstantSource source(0.25f);
		EffectSlot slot(ChannelCount(2), SampleRate(48000), FrameCount(8));
		slot.setPlayable(&source);
		float buffer[16] = {};
		Output out;
		out.dst = buffer;
		out.frameCount = FrameCount(8);
		out.samplerate = SampleRate(48000);
		out.channels = ChannelCount(2);
		out.interleaving = InterleavingType::SEPARATE_CHANNELS;
		slot.outputTo(out);
		requireCondition(std::abs(buffer[0] - 0.25f) < 1e-5f && std::abs(buffer[8] - 0.25f) < 1e-5f,
			"A channel-planar output request must be filled plane by plane");
	}

	{
		// Filter and delay IR synthesis.
		auto lowpass = FX::createLowpassFilter(SampleRate(48000), SampleRate(1000), true);
		requireCondition(lowpass && lowpass->getFrameCount().var >= 3,
			"A low-pass generator must return a non-empty kernel");
		requireCondition(std::abs(lowpass->data()[(lowpass->getFrameCount().var - 1) / 2]) <= 1.001f,
			"A normalized low-pass kernel must peak at 1.0");

		auto highpass = FX::createHighpassFilter(SampleRate(48000), SampleRate(1000), false);
		requireCondition(highpass && highpass->getFrameCount().var >= 3,
			"A high-pass generator must return a non-empty kernel");

		requireCondition(!FX::createBandpassFilter(SampleRate(48000), SampleRate(5000), SampleRate(1000)),
			"An inverted band-pass request must be rejected");
		requireCondition(!FX::createDelay(SampleRate(48000), FrameCount(0)),
			"A zero-length delay must be rejected");
	}

	{
		// The IR is a single tap two frames in, so the output must contain exactly
		// one impulse. FFTConvolver pipelines its blocks internally, so the position
		// of that impulse is implementation-defined; its count is not.
		auto ir = FX::createDelay(SampleRate(48000), FrameCount(2), 1.0f);
		requireCondition(ir && ir->getFrameCount().var == 2, "A delay IR must have the requested length");
		requireCondition(std::abs(ir->data()[1] - 1.0f) < 1e-6f, "A delay IR must place its tap at the end");

		FX::Convolver convolver(ChannelCount(1));
		requireCondition(convolver.setImpulseResponse(ChannelIndex(0), ir),
			"A single-channel convolver must accept an impulse response");
		requireCondition(convolver.isReady(), "A configured convolver must report ready");

		constexpr uintptr_t frames = 8192;
		std::vector<float> input(frames, 0.0f);
		std::vector<float> output(frames, 0.0f);
		input[0] = 1.0f;

		ModificationStream stream;
		stream.src = input.data();
		stream.dst = output.data();
		stream.frameCount = FrameCount(frames);
		stream.samplerate = SampleRate(48000);
		stream.channels = ChannelCount(1);
		stream.interleaving = InterleavingType::SEPARATE_CHANNELS;
		convolver.process(stream);

		uintptr_t impulses = 0;
		for (float sample : output) {
			if (std::abs(sample) > 0.5f) ++impulses;
		}
		requireCondition(impulses == 1, "Convolving an impulse with a delay IR must yield one impulse");
	}

	{
		FX::Convolver unconfigured(ChannelCount(1));
		float buffer[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
		ModificationStream stream;
		stream.dst = buffer;
		stream.frameCount = FrameCount(4);
		stream.samplerate = SampleRate(48000);
		stream.channels = ChannelCount(1);
		stream.interleaving = InterleavingType::SEPARATE_CHANNELS;

		bool threw = false;
		try {
			unconfigured.process(stream);
		} catch (const std::runtime_error&) {
			threw = true;
		}
		requireCondition(threw, "A convolver without impulse responses must throw");
	}

	std::cout << "Audio effect tests passed" << std::endl;
}


void testTextPipeline()
{
	using namespace Lotte::Text;

	std::string utf8;
	requireCondition(decodeNamedHtmlEntity("amp", utf8) && utf8 == "&", "&amp; must decode to an ampersand");
	requireCondition(decodeHtmlEntity("#65", utf8) && utf8 == "A", "&#65; must decode to A");
	requireCondition(decodeHtmlEntity("#x41", utf8) && utf8 == "A", "&#x41; must decode to A");
	requireCondition(!decodeNamedHtmlEntity("definitelynotanentity", utf8),
		"An unknown named reference must be reported");
	requireCondition(getNamedHtmlEntityCount() > 1000,
		"The salvaged HTML entity table must stay populated");

	{
		// UTF-8 <-> UTF-32 via utfcpp, including malformed and astral input.
		requireCondition(encodeUtf8(U"a\u00e9\u20ac\U0001F600") == "a\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80",
			"Encoding must produce UTF-8 for BMP and astral code points");
		const std::u32string decoded = decodeUtf8("a\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80");
		requireCondition(decoded == U"a\u00e9\u20ac\U0001F600", "Decoding must invert encoding");

		const std::u32string malformed = decodeUtf8("ok\xff\xfe" "!");
		requireCondition(malformed.size() == 5 && malformed[0] == U'o' && malformed[1] == U'k'
				&& malformed[2] == ReplacementCharacter && malformed[4] == U'!',
			"Malformed bytes must decode to U+FFFD instead of throwing");

		std::string encodedInvalid;
		appendCodePointUtf8(encodedInvalid, char32_t(0xD800));
		requireCondition(encodedInvalid == "\xef\xbf\xbd", "A lone surrogate must encode as U+FFFD");

		std::u32string appended = U"x";
		requireCondition(appendUtf8(appended, "\xc3\xa9") == 1 && appended == U"x\u00e9",
			"appendUtf8 must report the number of appended code points");
	}

	Colour colour;
	requireCondition(parseMarkupColour("#80FF00", colour) && std::abs(colour.r - 128.0f / 255.0f) < 1e-5f,
		"Hexadecimal colours must be parsed per component");
	requireCondition(parseMarkupColour("BLUE", colour) && colour.b == 1.0f && colour.r == 0.0f,
		"Named colours must be case-insensitive");
	requireCondition(parseMarkupColour("1.0, 0.5, 0.25", colour) && std::abs(colour.g - 0.5f) < 1e-5f,
		"Normalized component colours must be parsed");
	requireCondition(!parseMarkupColour("notacolour", colour), "Unknown colour names must be rejected");

	{
		RichTextProcessor processor;
		MmlParser parser(&processor);
		parser.parse("<b>bold</b> and &lt;plain&gt; [color=red]red[/color][br]line2");
		processor.flush();
		const TextRunList& runs = processor.getRuns();
		requireCondition(runs.size() == 4, "The MML sample must produce four runs");
		requireCondition(runs[0].text == U"bold" && runs[0].attributes.bold, "Bold text must produce a bold run");
		requireCondition(runs[1].text == U" and <plain> " && !runs[1].attributes.bold,
			"Entity escapes must decode inside plain text");
		requireCondition(runs[2].text == U"red" && std::abs(runs[2].attributes.colour.r - 1.0f) < 1e-5f,
			"A colour tag must recolour only its own run");
		requireCondition(runs[3].text == U"\nline2", "A line-break tag must emit a newline");
	}

	{
		RichTextProcessor processor;
		MmlParser parser(&processor);
		parser.parse("[b]a[i]b[/i]c[/b]d");
		processor.flush();
		const TextRunList& runs = processor.getRuns();
		requireCondition(runs.size() == 4, "Nested tags must close back to the outer attribute set");
		requireCondition(runs[0].text == U"a" && runs[0].attributes.bold && !runs[0].attributes.italic,
			"The first nested run must be bold only");
		requireCondition(runs[1].text == U"b" && runs[1].attributes.bold && runs[1].attributes.italic,
			"The inner run must be bold and italic");
		requireCondition(runs[2].text == U"c" && runs[2].attributes.bold && !runs[2].attributes.italic,
			"Popping italic must restore the bold-only state");
		requireCondition(runs[3].text == U"d" && !runs[3].attributes.bold,
			"Popping bold must restore the default state");
	}

	{
		// Stray delimiters must survive as text instead of being swallowed.
		RichTextProcessor processor;
		MmlParser parser(&processor);
		parser.parse("a > b ] c");
		processor.flush();
		requireCondition(processor.getRuns().size() == 1 && processor.getRuns()[0].text == U"a > b ] c",
			"Delimiters outside a tag must be emitted as text");
	}

	const PreprocessedFontFace face = makeTestFontFace();
	TextLayoutOptions options;
	options.origin = glm::fvec2(10.0f, 20.0f);
	// The face metrics are expressed at 10 px, so ask for 10 px to get a scale of 1.
	TextAttributes baseAttributes;
	baseAttributes.size = 10.0f;
	const TextRunList sample = { TextRun{ U"AB\nA", baseAttributes } };
	const TextLayout layout = layoutRuns(sample, [&face](const std::string&, bool) { return &face; }, options);

	requireCondition(layout.glyphs.size() == 3, "Three printable code points must produce three glyphs");
	requireCondition(layout.styles.size() == 1, "Identical attributes must share one style");
	requireCondition(std::abs(layout.glyphs[0].position.x - 11.0f) < 1e-5f
			&& std::abs(layout.glyphs[0].position.y - 12.0f) < 1e-5f,
		"Glyph placement must apply the bearing from the baseline");
	requireCondition(std::abs(layout.glyphs[2].position.x - 11.0f) < 1e-5f
			&& std::abs(layout.glyphs[2].position.y - 22.0f) < 1e-5f,
		"A newline must return the pen to the line start and advance by the face line height");
	requireCondition(std::abs(layout.endOffset.x - 19.0f) < 1e-5f,
		"The end offset must follow the 26.6 advance converted to pixels");
	requireCondition(std::abs(layout.height - 32.0f) < 1e-5f,
		"The layout height must be the last baseline plus the descender");

	{
		// Kerning offsets are pixels at the face's reference size, so they scale with
		// the run but are not divided by 64 like the advance is.
		PreprocessedFontFace kernedFace = makeTestFontFace();
		kernedFace.kerning[U'A'].emplace(std::uint32_t(U'B'), glm::fvec2(-1.0f, 0.0f));
		const TextRunList kernedRuns = { TextRun{ U"AB", baseAttributes } };
		const TextLayout kernedLayout = layoutRuns(kernedRuns,
			[&kernedFace](const std::string&, bool) { return &kernedFace; }, options);
		requireCondition(kernedLayout.glyphs.size() == 2, "Kerned text must still produce two glyphs");
		requireCondition(std::abs(kernedLayout.glyphs[1].position.x - 19.0f) < 1e-5f,
			"Kerning must pull the second glyph one pixel to the left");
		requireCondition(std::abs(kernedLayout.endOffset.x - 27.0f) < 1e-5f,
			"Kerning must shorten the laid-out width");
		requireCondition(std::abs(layout.glyphs[1].position.x - 20.0f) < 1e-5f,
			"A face without kerning must keep the unkerned advance");

		// A zero-offset pair must not shift anything, and a missing pair must be a no-op.
		PreprocessedFontFace zeroKern = makeTestFontFace();
		zeroKern.kerning[U'A'].emplace(std::uint32_t(U'B'), glm::fvec2(0.0f, 0.0f));
		const TextLayout zeroLayout = layoutRuns(kernedRuns,
			[&zeroKern](const std::string&, bool) { return &zeroKern; }, options);
		requireCondition(std::abs(zeroLayout.glyphs[1].position.x - 20.0f) < 1e-5f,
			"A zero kerning offset must be a no-op");
	}

	{
		TextAttributes italic = baseAttributes;
		italic.italic = true;
		const TextRunList italicRuns = { TextRun{ U"A", italic } };
		const TextLayout italicLayout = layoutRuns(italicRuns, [&face](const std::string&, bool) { return &face; });
		requireCondition(!italicLayout.glyphs.empty()
				&& std::abs(italicLayout.glyphs[0].shear - 5.0f) < 1e-5f,
			"Italic runs must store half the glyph height as shear");
	}

	{
		TextAttributes underlined = baseAttributes;
		underlined.underline = true;
		const TextRunList underlinedRuns = { TextRun{ U"AB", underlined } };
		const TextLayout underlinedLayout = layoutRuns(underlinedRuns,
			[&face](const std::string&, bool) { return &face; }, options);
		requireCondition(underlinedLayout.lines.size() == 1, "An underlined run must emit one decoration line");
		requireCondition(std::abs(underlinedLayout.lines[0].to.x - 28.0f) < 1e-5f,
			"The underline must span the laid-out text");
		requireCondition(std::abs(underlinedLayout.lines[0].to.y - 21.0f) < 1e-5f,
			"The underline must sit just below the baseline");
	}

	{
		// Faces assembled from pre-rendered bitmaps have no metric box; the raw bitmap
		// fields must then be used instead, and the 26.6 advance is the only one present.
		PreprocessedFontFace bitmapFace = makeTestFontFace();
		for (auto& entry : bitmapFace.glyphs) {
			StoredCharacter& character = entry.second;
			character.metricWidth = 0.0f;
			character.metricHeight = 0.0f;
			character.horiAdvance = 0.0f;
			character.width = 8;
			character.height = 10;
			character.bearingX = 1;
			character.bearingY = 8;
		}
		const TextLayout fallback = layoutRuns({ TextRun{ U"AB", baseAttributes } },
			[&bitmapFace](const std::string&, bool) { return &bitmapFace; }, options);
		requireCondition(fallback.glyphs.size() == 2
				&& std::abs(fallback.glyphs[0].position.x - 11.0f) < 1e-5f
				&& std::abs(fallback.glyphs[0].position.y - 12.0f) < 1e-5f
				&& std::abs(fallback.endOffset.x - 28.0f) < 1e-5f,
			"Faces without a metric box must fall back to the raw bitmap fields");
	}

	WholeTextureMapping mapping;
	std::vector<GlyphQuad> quads;
	std::vector<std::uint32_t> indices;
	requireCondition(appendGlyphQuads(layout, mapping, quads, &indices) == 3,
		"Every mapped glyph must produce a quad");
	requireCondition(quads.size() == 3 && indices.size() == 18, "Each quad must emit two triangles");
	requireCondition(quads[0].uv[3] == glm::fvec2(1.0f) && quads[0].position[3] == glm::fvec2(19.0f, 22.0f),
		"Quad corners must follow the layout geometry");

	std::vector<Lotte::Gui::ColouredTexturedGuiVertex> vertices;
	requireCondition(Lotte::Gui::appendGlyphVertices(layout, quads, vertices) == 3,
		"Glyph quads must expand into vertices");
	requireCondition(vertices.size() == 12, "Each glyph must expand into four vertices");

	std::cout << "Text pipeline tests passed" << std::endl;
}


void testSignals()
{
	Elv::Util::Signal<int> signal;
	int total = 0;
	auto direct = signal.connect([&total](int value) { total += value; });
	requireCondition(signal.getConnectionCount() == 1, "One connection must be tracked");
	signal.emit(2);
	requireCondition(total == 2, "A connected slot must receive the emission");

	{
		Elv::Util::ScopedConnection<int> scoped(signal.connect([&total](int value) { total += value * 10; }));
		signal.emit(1);
		requireCondition(total == 13, "Every live slot must receive the emission");
		requireCondition(signal.getConnectionCount() == 2, "Two connections must be tracked");
	}
	signal.emit(1);
	requireCondition(total == 14, "A scoped connection must disconnect on destruction");

	{
		Elv::Util::Signal<int> reentrant;
		int calls = 0;
		Elv::Util::Connection<int> victim;
		victim = reentrant.connect([&calls, &victim](int) {
			++calls;
			victim.disconnect();
		});
		reentrant.connect([&calls](int) { ++calls; });
		reentrant.emit(0);
		reentrant.emit(0);
		requireCondition(calls == 3, "Disconnecting from inside a slot must not break the emission");
	}

	direct.disconnect();
	signal.emit(1);
	requireCondition(total == 14, "An explicit disconnect must stop delivery");
	requireCondition(signal.getConnectionCount() == 0, "No live connection may remain");

	std::cout << "Signal tests passed" << std::endl;
}

void testGeometryBatches()
{
	using namespace Lotte;

	{
		// `commands` must outlive the batch: buffer proxies record destroy commands.
		CommandBuffer commands;
		Gui::GuiGeometryBatch<Gui::ColouredGuiVertex> batch;
		Gui::ColouredGuiVertex quad[4];
		quad[0].position = glm::fvec2(0.0f, 0.0f);
		quad[1].position = glm::fvec2(10.0f, 0.0f);
		quad[2].position = glm::fvec2(0.0f, 10.0f);
		quad[3].position = glm::fvec2(10.0f, 10.0f);
		batch.appendQuad(quad[0], quad[1], quad[2], quad[3]);
		requireCondition(batch.getVertices().size() == 4 && batch.getIndices().size() == 6,
			"A quad must append four vertices and six indices");

		batch.recordUpload(commands);
		requireCondition(batch.isUploaded(), "recordUpload must create the vertex and index buffers");
		requireCondition(batch.getResidentRange().indexCount == 6, "The resident range must cover the quad");
		batch.recordBind(commands);
		batch.recordDraw(commands);
		// Two create commands, two binds and one indexed draw.
		requireCondition(commands.getCommandBuffer().getWriteBufferAsSpan_unsafe().size() == 5,
			"Upload, binding and draw must all record commands");
		batch.clear();
		requireCondition(batch.empty(), "clear must drop the CPU caches");
	}

	{
		CommandBuffer commands;
		Gfx::MeshBuilder<Gui::ColouredGuiVertex> mesh;
		Gui::ColouredGuiVertex vertices[4] = {};
		const std::uint32_t triangleIndices[6] = { 0, 1, 2, 1, 2, 3 };
		mesh.appendVertices(vertices, 4);
		mesh.appendIndices(triangleIndices, 6);
		mesh.endGroup();
		mesh.appendVertices(vertices, 4);
		mesh.appendIndices(triangleIndices, 6);
		mesh.endGroup();

		requireCondition(mesh.getRanges().size() == 2, "Two groups must be recorded");
		requireCondition(mesh.getRanges()[1].indexOffset == 6 && mesh.getRanges()[1].vertexOffset == 4,
			"The second group must start after the first one");

		mesh.recordUpload(commands);
		mesh.recordBind(commands);
		mesh.recordDrawAll(commands);

		const Gfx::MeshView view = mesh.getMeshView();
		requireCondition(view.isValid() && view.indexCount == 12 && view.vertexCount == 8,
			"The mesh view must expose the uploaded element counts");
		requireCondition(Gui::ColouredGuiVertex::getVertexDescriptor().stride == sizeof(Gui::ColouredGuiVertex),
			"The vertex descriptor stride must match the vertex size");
		requireCondition(Gui::ColouredGuiVertex::getVertexDescriptor().descriptors.size() == 2,
			"A position-colour vertex must advertise two attributes");
	}

	std::cout << "Geometry batch tests passed" << std::endl;
}

