#ifndef EUPHMEMORY_HPP
#define EUPHMEMORY_HPP
/**
 * @file EuphMemory.hpp
 * @brief Declares the EuphMemory API in the Euphemy/Memory module.
 *
 * This header is part of the public declaration surface for Euphemy/Memory.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Util/ElvAllocatorBasic.hpp>
#include <Elvavena/Util/ElvFreelist.hpp>
namespace Euph {
namespace Memory {

/**
 * @brief Logical memory pools used by the global Euph allocator manager.
 */
enum class AllocatorSubsystem : uint8_t {
	SCRATCH = 0,
	AUDIO,
	VIDEO,
	SCRIPT
};

/**
 * @brief Entry point to the process-wide freelist memory manager.
 */
struct MH_EUPH_API MemoryManager {
	/// Owning pointer type for the global freelist manager instance.
	typedef std::unique_ptr<Elv::Util::FreelistMemoryManager> uManager;
private:
	static uManager manager;
	static std::mutex managerMutex;
public:
	/**
	 * @brief Returns the singleton freelist memory manager.
	 * @return Reference to the process-wide memory manager.
	 */
	static Elv::Util::FreelistMemoryManager& getStaticManager();
};

/**
 * @brief Adapter exposing one allocator subsystem as an Alexandrescu allocator backend.
 * @tparam subsysId Target subsystem ID.
 */
template <AllocatorSubsystem subsysId> struct MemoryManagerSubsystem {
private:
	static Elv::Util::ContiguousFreeListAllocator* alloc;
public:
	/**
	 * @brief Initializes the subsystem allocator pointer on first use.
	 */
	MemoryManagerSubsystem() {
		if(!alloc) {
			alloc = MemoryManager::getStaticManager().getAllocator(static_cast<size_t>(subsysId));
		}
	}
	/**
	 * @brief Allocates a contiguous block.
	 * @param n Number of bytes to allocate.
	 * @return Allocated block descriptor.
	 */
	Elv::Util::Blk allocateBlock(std::size_t n) noexcept {
		return alloc->allocateBlock(n);
	}
	/**
	 * @brief Releases a previously allocated block.
	 * @param blk Block descriptor to deallocate.
	 */
	void deallocateBlock(const Elv::Util::Blk& blk) noexcept {
		alloc->deallocateBlock(blk);
	}
	/**
	 * @brief Checks whether this subsystem owns a block.
	 * @param blk Block descriptor to test.
	 * @return `true` when the block belongs to this subsystem.
	 */
	bool ownsBlock(const Elv::Util::Blk& blk) const noexcept {
		return alloc->ownsBlock(blk);
	}
};
/** @brief Documents the alloc declaration. */
template <AllocatorSubsystem subsysId> Elv::Util::ContiguousFreeListAllocator* MemoryManagerSubsystem<subsysId>::alloc = nullptr;
/** @brief Documents the SubsystemAllocator type or declaration. */
template <typename T, AllocatorSubsystem subsysId> using SubsystemAllocator = Elv::Util::AlexandrescuAllocatorAdapter<MemoryManagerSubsystem<subsysId>,T>;

/** @brief Documents the ScratchpadAllocator__ type or declaration. */
typedef MemoryManagerSubsystem<AllocatorSubsystem::SCRATCH> ScratchpadAllocator__;
/** @brief Documents the ScratchpadAllocator type or declaration. */
template <typename T> using ScratchpadAllocator = Elv::Util::AlexandrescuAllocatorAdapter<ScratchpadAllocator__, T>;
/** @brief Documents the AudioAllocator__ type or declaration. */
typedef MemoryManagerSubsystem<AllocatorSubsystem::AUDIO> AudioAllocator__;
/** @brief Documents the AudioAllocator type or declaration. */
template <typename T> using AudioAllocator = Elv::Util::AlexandrescuAllocatorAdapter<AudioAllocator__, T>;
/** @brief Documents the VideoAllocator__ type or declaration. */
typedef MemoryManagerSubsystem<AllocatorSubsystem::VIDEO> VideoAllocator__;
/** @brief Documents the VideoAllocator type or declaration. */
template <typename T> using VideoAllocator = Elv::Util::AlexandrescuAllocatorAdapter<VideoAllocator__, T>;
/** @brief Documents the ScriptAllocator__ type or declaration. */
typedef MemoryManagerSubsystem<AllocatorSubsystem::SCRIPT> ScriptAllocator__;
/** @brief Documents the ScriptAllocator type or declaration. */
template <typename T> using ScriptAllocator = Elv::Util::AlexandrescuAllocatorAdapter<ScriptAllocator__, T>;

}
}
#endif // EUPHMEMORY_HPP
