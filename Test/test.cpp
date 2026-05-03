#include "test.hpp"
#include <algorithm>
#include <array>
#include <Elvavena/Util/ElvHeapArray.hpp>
#include <Elvavena/Util/ElvChunkyArray.hpp>
#include <Elvavena/Util/ElvIntegralIterator.hpp>
#include <Elvavena/Util/ElvFreelist.hpp>
#include <Elvavena/Util/ElvBitmapAllocator.hpp>
#include <Elvavena/Util/ElvThreadPool.hpp>
#include <Elvavena/Io/ElvIoAsync.hpp>
#include <Euphemy/Asset/EuphAssetRuntime.hpp>
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
#include <Euphemy/Config/GlobalConfig.hpp>
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
		std::lock_guard<std::mutex> lock(managerMutex);
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

	const auto texture = registry.createHandle<TextureResource>(1);
	makeResident(registry, 1);
	requireCondition(registry.isAlive(texture), "Fresh texture handle should be alive");
	requireCondition(registry.destroy(1), "Destroying a live resource should succeed");
	const auto textureAgain = registry.createHandle<TextureResource>(1);
	requireCondition(!registry.isAlive(texture), "Old handle generation should be invalid after destroy/recreate");
	requireCondition(registry.isAlive(textureAgain), "Recreated handle should be alive");
	requireCondition(texture.generation != textureAgain.generation, "Recreated handle should have a different generation");
	requireCondition(!registry.setState(1, ResidencyState::LoadingIO), "Resident-like illegal state transition should be rejected");

	AssetCatalog catalog;
	requireCondition(catalog.addRecord({ 2, "dep.bin", 0, 4, 4, Compression::None, "blob", {} }), "Dependency record should be accepted");
	requireCondition(catalog.addRecord({ 3, "parent.bin", 0, 4, 4, Compression::None, "blob", { 2 } }), "Parent record should be accepted");
	requireCondition(catalog.validateDependencies().ok, "Catalog dependencies should validate");
	requireCondition(catalog.dependenciesOf(3).size() == 1, "Catalog should expose dependency spans");

	DependencyGraph graph(catalog);
	const DependencyResult deps = graph.resolveTransitive(3);
	requireCondition(deps.ok && deps.orderedDependencies.size() == 1 && deps.orderedDependencies.front() == 2, "Dependency graph should resolve transitive dependencies");

	registry.createHandle<GenericBlobResource>(2);
	registry.createHandle<GenericBlobResource>(3);

	Elv::Util::ThreadPool pool(2);
	auto loader = [](const AssetRecord& record) -> Elv::Io::sDevice {
		std::vector<std::byte> data(static_cast<std::size_t>(record.storedSize), std::byte { 0x42 });
		return std::make_shared<Euph::Io::MemoryDevice>(Elv::Io::Mode::READ, std::move(data));
	};
	auto decoder = [](const AssetRecord&, std::span<const std::byte> bytes) {
		DecodedAsset decoded;
		decoded.cpuPayload.assign(bytes.begin(), bytes.end());
		decoded.cpuBytes = decoded.cpuPayload.size();
		return decoded;
	};

	StreamingScheduler scheduler(catalog, registry, pool, loader, decoder);
	requireCondition(scheduler.request(3, { 5 }), "Parent request should be accepted while waiting on dependencies");
	requireCondition(registry.state(3) == ResidencyState::WaitingDependencies, "Parent should wait until dependency is resident");
	makeResident(registry, 2);
	requireCondition(scheduler.request(3, { 5 }), "Parent request should start once dependency is resident");
	pumpScheduler(scheduler, 3, ResidencyState::Resident);
	requireCondition(registry.cpuCost(3) == 4, "Decoded resource should account CPU cost");

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

	ResourceRegistry budgetRegistry;
	ResidencyManager residency(budgetRegistry);
	for (AssetId id : { AssetId(10), AssetId(11), AssetId(12) }) {
		budgetRegistry.createHandle<GenericBlobResource>(id);
		makeResident(budgetRegistry, id);
		budgetRegistry.setCost(id, BudgetKind::CpuBytes, 100);
		requireCondition(residency.makeEvictable(id), "Resident resource should become evictable");
	}
	budgetRegistry.setPriority(10, { 0 });
	budgetRegistry.setPriority(11, { 0 });
	budgetRegistry.setPriority(12, { 10 });
	budgetRegistry.touch(10, 1);
	budgetRegistry.touch(11, 2);
	budgetRegistry.touch(12, 0);
	residency.pin(10);
	residency.setBudget(BudgetKind::CpuBytes, 150);
	const std::vector<AssetId> evicted = residency.evictUntilWithinBudget();
	requireCondition(evicted.size() == 2, "Residency manager should evict enough resources to satisfy budget");
	requireCondition(evicted[0] == 11 && evicted[1] == 12, "Eviction should respect pinning, priority, touch age, and deterministic ordering");
	requireCondition(budgetRegistry.state(10) == ResidencyState::Evictable, "Pinned resource should not be evicted");

	ResourceRegistry uploadRegistry;
	const auto buffer = uploadRegistry.createHandle<BufferResource>(20);
	(void)buffer;
	makeResident(uploadRegistry, 20);
	uploadRegistry.setState(20, ResidencyState::Evictable);
	uploadRegistry.setState(20, ResidencyState::Requested);
	uploadRegistry.setState(20, ResidencyState::LoadingIO);
	uploadRegistry.setState(20, ResidencyState::Decoding);
	UploadQueue uploads(uploadRegistry);
	auto uploadPayload = std::make_shared<std::vector<std::byte> >(8, std::byte { 0x7f });
	std::weak_ptr<std::vector<std::byte> > weakPayload = uploadPayload;
	uploads.enqueue({ 20, uploadPayload, 8, Kld::HandleId(77) }, [](Kld::CommandBuffer&, const UploadItem& item) {
		requireCondition(item.payload && item.payload->size() == 8, "Upload payload should stay alive through recording");
	});
	uploadPayload.reset();
	requireCondition(!weakPayload.expired(), "Upload queue should own payload before recording");
	Kld::CommandBuffer commands;
	requireCondition(uploads.recordInto(commands, 64) == 8, "Upload queue should record within byte budget");
	requireCondition(!weakPayload.expired(), "Recorded upload should still own payload before commit");
	uploads.commitRecorded();
	requireCondition(uploadRegistry.state(20) == ResidencyState::Resident && uploadRegistry.gpuCost(20) == 8, "Committed upload should mark resource resident and account GPU bytes");
	requireCondition(uploadRegistry.kaldiHandle(20).has_value() && *uploadRegistry.kaldiHandle(20) == 77, "Committed upload should publish Kaldi handle");
	requireCondition(weakPayload.expired(), "Upload payload should be released after commit");

	ResourceRegistry placeholderRegistry;
	const auto fallback = placeholderRegistry.createHandle<TextureResource>(30);
	const auto wanted = placeholderRegistry.createHandle<TextureResource>(31);
	makeResident(placeholderRegistry, 30);
	placeholderRegistry.setPlaceholder<TextureResource>(fallback);
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
void testGlobalConfigFile()
{
	std::cout << GLOBAL_CONFIGURATION;
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
