#include <Euphemy/Memory/EuphMemory.hpp>
#include <Euphemy/Config/GlobalConfig.hpp>
#include <cstdint>
#include <climits>
namespace Euph {
namespace Memory {

/*
enum AllocatorSubsystem {
	SCRATCH = 0,
	AUDIO,
	VIDEO,
	SCRIPT
};
*/

Elv::Util::FreelistMemoryManager& MemoryManager::getStaticManager() {
	std::lock_guard<std::mutex> lock(managerMutex);
	if(!manager) {
		manager = uManager(new Elv::Util::FreelistMemoryManager(
							   {
								   // SCRATCH
								   static_cast<size_t>(std::numeric_limits<std::uint16_t>::max()) * 2,
								   // AUDIO
								   GLOBAL_CONFIGURATION.getUInteger("Audio","uMemoryPoolSize", 134217728),
								   // VIDEO
								   GLOBAL_CONFIGURATION.getUInteger("Video","uMemoryPoolSize", 268435456),
								   // SCRIPT
								   GLOBAL_CONFIGURATION.getUInteger("Script","uMemoryPoolSize", 134217728),
							   }
							   ));
	}
	return *manager;
}
MemoryManager::uManager MemoryManager::manager = nullptr;
std::mutex MemoryManager::managerMutex;

}
}
