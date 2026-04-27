#include "test.hpp"
#include <Elvavena/Util/ElvHeapArray.hpp>
#include <Elvavena/Util/ElvChunkyArray.hpp>
#include <Elvavena/Util/ElvIntegralIterator.hpp>
#include <Elvavena/Util/ElvFreelist.hpp>
#include <Elvavena/Util/ElvBitmapAllocator.hpp>
#include <vector>
#include <Euphemy/Io/EuphBufferDevice.hpp>
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
#include <Euphemy/Asset/EuphAssetManager.hpp>
#include <Euphemy/Io/EuphFile.hpp>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <thread>

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

static void requireAssetTest(bool condition, const char* message)
{
	if(!condition) {
		throw std::runtime_error(message);
	}
}

static std::string makeAssetTestPath(const char* name)
{
	return (std::filesystem::temp_directory_path() / name).string();
}

static void writeAssetTestFile(const std::string& path, std::span<const std::byte> bytes)
{
	Euph::Io::File file(path.c_str(), Elv::Io::Mode::WRITE);
	requireAssetTest(file.isValid(), "Failed to create asset test file");
	requireAssetTest(file.write(bytes.data(), 1, bytes.size()) == bytes.size(), "Failed to write asset test file");
	file.flush();
}

static void writeAssetTestFile(const std::string& path, std::string_view text)
{
	writeAssetTestFile(path, std::span<const std::byte>(
		reinterpret_cast<const std::byte*>(text.data()),
		text.size()));
}

template <typename T>
static bool waitForAssetReady(const Euph::Asset::AssetHandle<T>& handle, std::chrono::milliseconds timeout)
{
	const auto deadline = std::chrono::steady_clock::now() + timeout;
	while(std::chrono::steady_clock::now() < deadline) {
		if(handle.ready() || handle.state() == Euph::Asset::ResidencyState::Failed) {
			return handle.ready();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	return handle.ready();
}

static std::string blobToString(const Euph::Asset::Blob& blob)
{
	return std::string(reinterpret_cast<const char*>(blob.data()), blob.size());
}

void testAssetStreaming()
{
	using namespace Euph::Asset;
	const std::string path = makeAssetTestPath("euphemy_asset_streaming_test.bin");
	writeAssetTestFile(path, "abcdef");

	Euph::Io::Filesystem filesystem;
	{
		AsyncIoScheduler scheduler(filesystem);
		AsyncRead read = scheduler.read(path, 2, 3);
		AsyncReadResult result = read.future().get();
		requireAssetTest(result.ok(), "Async scheduler byte-range read failed");
		requireAssetTest(std::string(reinterpret_cast<const char*>(result.data.data()), result.data.size()) == "cde",
						 "Async scheduler returned the wrong byte range");
	}

	{
		AsyncIoScheduler scheduler(filesystem, 1, nullptr, true);
		std::mutex orderMutex;
		std::vector<std::uint64_t> completionOrder;
		AsyncRead low = scheduler.read(path, 0, 1, StreamPriority{0}, [&](const AsyncReadResult&) {
			std::lock_guard<std::mutex> lock(orderMutex);
			completionOrder.push_back(1);
		});
		AsyncRead high = scheduler.read(path, 1, 1, StreamPriority{10}, [&](const AsyncReadResult&) {
			std::lock_guard<std::mutex> lock(orderMutex);
			completionOrder.push_back(2);
		});
		scheduler.resume();
		(void)low.future().get();
		(void)high.future().get();
		std::lock_guard<std::mutex> lock(orderMutex);
		requireAssetTest(!completionOrder.empty() && completionOrder.front() == 2,
						 "Async scheduler did not service higher-priority work first");
	}

	AssetRecord firstRecord;
	firstRecord.id = 0x10;
	firstRecord.path = path;
	firstRecord.offset = 0;
	firstRecord.storedSize = 3;
	firstRecord.decodedSize = 3;
	firstRecord.compression = Compression::None;
	firstRecord.type = "blob";
	firstRecord.dependencies = { 0x20 };

	Euph::Conf::Configuration config(
		"[asset:0000000000000010]\n"
		"spath=" + path + "\n"
		"uoffset=0\n"
		"ustoredSize=3\n"
		"udecodedSize=3\n"
		"scompression=none\n"
		"stype=blob\n"
		"sdependencies=0000000000000020\n");
	AssetRegistry manualRegistry;
	manualRegistry.registerAsset(firstRecord);
	AssetRegistry configRegistry;
	configRegistry.loadFromConfiguration(config);
	requireAssetTest(configRegistry.get(0x10) == manualRegistry.get(0x10),
					 "Asset registry config loading does not match manual registration");

	AssetRecord secondRecord = firstRecord;
	secondRecord.id = 0x20;
	secondRecord.offset = 3;
	secondRecord.dependencies.clear();
	manualRegistry.registerAsset(secondRecord);

	{
		AsyncIoScheduler scheduler(filesystem);
		AssetManager manager(manualRegistry, scheduler, 64);
		auto handleA = manager.request<Blob>(0x10);
		auto handleB = manager.request<Blob>(0x10);
		requireAssetTest(waitForAssetReady(handleA, std::chrono::seconds(2)), "Asset handle did not become ready");
		requireAssetTest(handleB.ready(), "Coalesced duplicate asset handle did not become ready");
		auto lockA = handleA.lock();
		auto lockB = handleB.lock();
		requireAssetTest(lockA && lockB && lockA.get() == lockB.get(), "Duplicate asset requests were not coalesced");
		requireAssetTest(blobToString(*lockA) == "abc", "Asset manager decoded the wrong blob");

		auto missing = manager.request<Blob>(0x9999);
		requireAssetTest(missing.state() == ResidencyState::Failed && !missing.error().empty(),
						 "Missing asset did not enter Failed state");
	}

	{
		AsyncIoScheduler scheduler(filesystem);
		AssetManager manager(manualRegistry, scheduler, 3);
		auto first = manager.request<Blob>(0x10);
		requireAssetTest(waitForAssetReady(first, std::chrono::seconds(2)), "First eviction-test asset did not load");
		auto pinned = first.lock();
		requireAssetTest(static_cast<bool>(pinned), "Pinned asset lease could not be acquired");
		auto second = manager.request<Blob>(0x20);
		requireAssetTest(waitForAssetReady(second, std::chrono::seconds(2)) || second.state() == ResidencyState::Unloaded,
						 "Second eviction-test asset neither loaded nor evicted");
		manager.collectGarbage();
		requireAssetTest(first.ready() && first.lock(), "Pinned asset was evicted despite an external lease");
		requireAssetTest(manager.getResidentBytes() <= 3 || static_cast<bool>(pinned),
						 "Eviction budget accounting failed");
	}

	const std::string zstdPath = makeAssetTestPath("euphemy_asset_streaming_test.zst");
	const std::string zstdPayload = "hello zstd";
	{
		Euph::Io::File compressedFile(zstdPath.c_str(), Elv::Io::Mode::WRITE);
		Euph::Io::ZstdCompressor compressor(&compressedFile);
		requireAssetTest(compressor.write(zstdPayload.data(), 1, zstdPayload.size()) == zstdPayload.size(),
						 "Failed to write zstd asset payload");
		compressor.flush();
	}

	AssetRecord zstdRecord;
	zstdRecord.id = 0x30;
	zstdRecord.path = zstdPath;
	zstdRecord.offset = 0;
	zstdRecord.storedSize = std::filesystem::file_size(zstdPath);
	zstdRecord.decodedSize = zstdPayload.size();
	zstdRecord.compression = Compression::Zstd;
	zstdRecord.type = "blob";
	manualRegistry.registerAsset(zstdRecord);
	{
		AsyncIoScheduler scheduler(filesystem);
		AssetManager manager(manualRegistry, scheduler, 64);
		auto zstdHandle = manager.request<Blob>(0x30);
		requireAssetTest(waitForAssetReady(zstdHandle, std::chrono::seconds(2)), "Zstd asset did not become ready");
		auto blob = zstdHandle.lock();
		requireAssetTest(blob && blobToString(*blob) == zstdPayload, "Zstd asset round-trip decoded incorrectly");
	}

	std::filesystem::remove(path);
	std::filesystem::remove(zstdPath);
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
