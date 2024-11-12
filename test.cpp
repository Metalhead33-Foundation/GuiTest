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
	std::string strToRead(static_cast<char*>(mapped->data()), mapped->size());
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

typedef const char* (*myfunc)();
void testMemoryMappedTempDlopenNoClose()
{
	std::unique_ptr<Euph::Io::MemoryMappedTempFile> tempFile(nullptr);
	{
		//Euph::Io::File dllFile("/usr/lib64/libGL.so", Elv::Io::Mode::READ);
		// libsimple.so
		Euph::Io::File dllFile("/home/legacy/libsimple.so", Elv::Io::Mode::READ);
		tempFile = std::unique_ptr<Euph::Io::MemoryMappedTempFile>(new Euph::Io::MemoryMappedTempFile(dllFile.size()));
		dllFile.read(tempFile->data(),1,tempFile->size());
		tempFile->flushSync();
	}
	std::cout << "Beginning of mapped region: [" << tempFile->data() << ']' << std::endl;
	std::cout << "End of mapped region: [" << static_cast<void*>(static_cast<std::byte*>(tempFile->data()) + tempFile->size()) << ']' << std::endl;

	Elv::Util::DynamicLibrary dynLib(tempFile->getFilePath());
	/*void* glClearFunc = dynLib.link("glClear");
	if(glClearFunc != nullptr) std::cout << "Success! glClear is at address [" << glClearFunc << "]." << std::endl;
	else std::cout << "Failure! glClear is a nullptr! Linking failed!" << std::endl;*/
	void* hello_world = dynLib.link("get_hello_world");
	if(hello_world != nullptr) std::cout << "Success! get_hello_world is at address [" << hello_world << "]." << std::endl;
	else std::cout << "Failure! hello_world is a nullptr! Linking failed!" << std::endl;
	myfunc HelloWorld = reinterpret_cast<myfunc>(hello_world);
	std::cout << HelloWorld() << std::endl;
}

void testMemoryMappedTempDlopenClose()
{
	std::unique_ptr<Elv::Util::DynamicLibrary> dynLib(nullptr);
	{
		std::unique_ptr<Euph::Io::MemoryMappedTempFile> tempFile(nullptr);
		{
			Euph::Io::File dllFile("/home/legacy/libsimple.so", Elv::Io::Mode::READ);
			tempFile = std::unique_ptr<Euph::Io::MemoryMappedTempFile>(new Euph::Io::MemoryMappedTempFile(dllFile.size()));
			dllFile.read(tempFile->data(),1,tempFile->size());
			tempFile->flushSync();
		}
		dynLib = std::unique_ptr<Elv::Util::DynamicLibrary>(new Elv::Util::DynamicLibrary(tempFile->getFilePath()));
	}
	void* hello_world = dynLib->link("get_hello_world");
	if(hello_world != nullptr) std::cout << "Success! get_hello_world is at address [" << hello_world << "]." << std::endl;
	else std::cout << "Failure! hello_world is a nullptr! Linking failed!" << std::endl;
	myfunc HelloWorld = reinterpret_cast<myfunc>(hello_world);
	std::cout << HelloWorld() << std::endl;
}
