/**
 * @file EuphMemory.cpp
 * @brief Implementation of the optional PMR arena manager.
 */
#include <Euphemy/Memory/EuphMemory.hpp>
#include <Elvavena/Util/ElvAlexandrescuResource.hpp>
#include <Elvavena/Util/ElvFreelist.hpp>
#include <array>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

namespace Euph {
namespace Memory {

namespace {

/** @brief Number of pools addressed by @ref AllocatorSubsystem. */
constexpr std::size_t kSubsystemCount = static_cast<std::size_t>(AllocatorSubsystem::Count);

/**
 * @brief Alexandrescu-style adapter over one freelist allocator instance.
 *
 * The manager owns the allocators; this adapter is a cheap handle stored inside
 * the PMR resource. It satisfies `Elv::Util::AlexandrescuAllocator`.
 */
class SubsystemAllocator {
private:
	Elv::Util::ContiguousFreeListAllocator* allocator_ = nullptr;

public:
	/**
	 * @brief Wraps an allocator owned by the freelist manager.
	 * @param allocator Allocator instance; must outlive this adapter.
	 */
	explicit SubsystemAllocator(Elv::Util::ContiguousFreeListAllocator* allocator) noexcept
		: allocator_(allocator)
	{
	}

	/**
	 * @brief Allocates a block.
	 * @param bytes Number of bytes requested.
	 * @return The allocated block, or `{nullptr, 0}` on failure.
	 */
	Elv::Util::Blk allocateBlock(std::size_t bytes) noexcept
	{
		return allocator_->allocateBlock(bytes);
	}

	/**
	 * @brief Returns a block to the allocator.
	 * @param block Block previously returned by @ref allocateBlock.
	 */
	void deallocateBlock(const Elv::Util::Blk& block) noexcept
	{
		allocator_->deallocateBlock(block);
	}

	/**
	 * @brief Checks whether a block belongs to this allocator.
	 * @param block Block to test.
	 * @return `true` when the block is inside this allocator's arena.
	 */
	bool ownsBlock(const Elv::Util::Blk& block) const noexcept
	{
		return allocator_->ownsBlock(block);
	}
};

/** @brief PMR resource backed by one subsystem allocator. */
using SubsystemResource = Elv::Util::AlexandrescuResource<SubsystemAllocator>;

/**
 * @brief Process-wide state for the optional arenas.
 */
struct ArenaState {
	std::mutex mutex;
	std::unique_ptr<Elv::Util::FreelistMemoryManager> manager;
	std::array<std::unique_ptr<SubsystemResource>, kSubsystemCount> resources {};
	bool initialized = false;
};

/**
 * @brief Returns the lazily created process-wide arena state.
 * @return Reference to the singleton state.
 */
ArenaState& arenaState()
{
	static ArenaState state;
	return state;
}

/**
 * @brief Validates one configured pool size.
 * @param value Configured size.
 * @param name Field name used in the diagnostic.
 * @throws std::invalid_argument When @p value is zero.
 */
void requireNonZero(std::size_t value, const char* name)
{
	if (value == 0)
		throw std::invalid_argument(std::string("Euph::Memory::PoolSizes::") + name + " must be greater than zero");
}

} // namespace

PoolSizes MemoryManager::defaultPoolSizes() noexcept
{
	return PoolSizes{};
}

bool MemoryManager::initialize(const PoolSizes& sizes)
{
	requireNonZero(sizes.scratch, "scratch");
	requireNonZero(sizes.audio, "audio");
	requireNonZero(sizes.video, "video");
	requireNonZero(sizes.script, "script");

	ArenaState& state = arenaState();
	std::scoped_lock lock(state.mutex);
	if (state.initialized)
		return false;

	auto manager = std::make_unique<Elv::Util::FreelistMemoryManager>(
		std::initializer_list<std::size_t>{sizes.scratch, sizes.audio, sizes.video, sizes.script});

	std::array<std::unique_ptr<SubsystemResource>, kSubsystemCount> resources;
	for (std::size_t index = 0; index < kSubsystemCount; ++index) {
		resources[index] = std::make_unique<SubsystemResource>(
			SubsystemAllocator{manager->getAllocator(index)});
	}

	state.manager = std::move(manager);
	state.resources = std::move(resources);
	state.initialized = true;
	return true;
}

bool MemoryManager::initialize(Euph::Conf::Configuration& config)
{
	PoolSizes sizes = defaultPoolSizes();
	const auto read = [&config](const char* section, std::size_t fallback) {
		return static_cast<std::size_t>(config.getUInteger(
			section, "uMemoryPoolSize", static_cast<Euph::Conf::ConfUint>(fallback)));
	};

	sizes.scratch = read("Scratch", sizes.scratch);
	sizes.audio = read("Audio", sizes.audio);
	sizes.video = read("Video", sizes.video);
	sizes.script = read("Script", sizes.script);
	return initialize(sizes);
}

bool MemoryManager::initialized()
{
	ArenaState& state = arenaState();
	std::scoped_lock lock(state.mutex);
	return state.initialized;
}

std::pmr::memory_resource& MemoryManager::resource(AllocatorSubsystem subsystem)
{
	ArenaState& state = arenaState();
	std::scoped_lock lock(state.mutex);
	const std::size_t index = static_cast<std::size_t>(subsystem);
	if (state.initialized && index < kSubsystemCount && state.resources[index])
		return *state.resources[index];
	return *std::pmr::get_default_resource();
}

} // namespace Memory
} // namespace Euph
