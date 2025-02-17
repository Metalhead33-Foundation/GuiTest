#ifndef EUPHMEMORY_HPP
#define EUPHMEMORY_HPP
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Util/ElvAllocatorBasic.hpp>
#include <Elvavena/Util/ElvFreelist.hpp>
namespace Euph {
namespace Memory {

enum class AllocatorSubsystem : uint8_t {
	SCRATCH = 0,
	AUDIO,
	VIDEO,
	SCRIPT
};

struct MH_EUPH_API MemoryManager {
	typedef std::unique_ptr<Elv::Util::FreelistMemoryManager> uManager;
private:
	static uManager manager;
	static std::mutex managerMutex;
public:
	static Elv::Util::FreelistMemoryManager& getStaticManager();
};

template <AllocatorSubsystem subsysId> struct MemoryManagerSubsystem {
private:
	static Elv::Util::ContiguousFreeListAllocator* alloc;
public:
	MemoryManagerSubsystem() {
		if(!alloc) {
			alloc = MemoryManager::getStaticManager().getAllocator(static_cast<size_t>(subsysId));
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
template <AllocatorSubsystem subsysId> Elv::Util::ContiguousFreeListAllocator* MemoryManagerSubsystem<subsysId>::alloc = nullptr;
template <typename T, AllocatorSubsystem subsysId> using SubsystemAllocator = Elv::Util::AlexandrescuAllocatorAdapter<MemoryManagerSubsystem<subsysId>,T>;

typedef MemoryManagerSubsystem<AllocatorSubsystem::SCRATCH> ScratchpadAllocator__;
template <typename T> using ScratchpadAllocator = Elv::Util::AlexandrescuAllocatorAdapter<ScratchpadAllocator__, T>;
typedef MemoryManagerSubsystem<AllocatorSubsystem::AUDIO> AudioAllocator__;
template <typename T> using AudioAllocator = Elv::Util::AlexandrescuAllocatorAdapter<AudioAllocator__, T>;
typedef MemoryManagerSubsystem<AllocatorSubsystem::VIDEO> VideoAllocator__;
template <typename T> using VideoAllocator = Elv::Util::AlexandrescuAllocatorAdapter<VideoAllocator__, T>;
typedef MemoryManagerSubsystem<AllocatorSubsystem::SCRIPT> ScriptAllocator__;
template <typename T> using ScriptAllocator = Elv::Util::AlexandrescuAllocatorAdapter<ScriptAllocator__, T>;

}
}
#endif // EUPHMEMORY_HPP
