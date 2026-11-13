#ifndef EUPHMEMORY_HPP
#define EUPHMEMORY_HPP
/**
 * @file EuphMemory.hpp
 * @brief Optional process-wide PMR arenas for Euphemy's logical subsystems.
 *
 * The arenas are opt-in. Until `MemoryManager::initialize()` succeeds,
 * `MemoryManager::resource()` returns `std::pmr::get_default_resource()`, so
 * callers never depend on a global configuration object or on an
 * application-provided symbol.
 *
 * Arena-backed memory is intended for long-lived, chunky allocations (asset
 * payloads, decode scratch, command buffers), not for many tiny objects: every
 * PMR allocation carries a small bookkeeping header and the underlying freelist
 * allocators take a lock per allocation.
 *
 * @warning Initialize from `main()` (or another post-startup point), before
 * creating objects that allocate from these resources. The arenas are destroyed
 * during static destruction, so do not let arena-backed objects outlive them.
 */
#include <Euphemy/Config/EuphConfiguration.hpp>
#include <Euphemy/Config/EuphLib.hpp>
#include <cstddef>
#include <cstdint>
#include <memory_resource>

namespace Euph {
namespace Memory {

/**
 * @brief Logical memory pools managed by @ref MemoryManager.
 */
enum class AllocatorSubsystem : uint8_t {
	SCRATCH = 0, ///< Short-lived scratch storage.
	AUDIO,       ///< Audio engine buffers and payloads.
	VIDEO,       ///< Image/texture payloads and decode scratch.
	SCRIPT,      ///< Script-engine allocations (for example WasmJIT).
	Count        ///< Sentinel used to size internal arrays.
};

/**
 * @brief Byte sizes used to create the per-subsystem arenas.
 *
 * The defaults preserve the historical pool sizes. They total roughly 512 MiB,
 * so applications that do not need the arenas should simply not call
 * @ref MemoryManager::initialize.
 */
struct PoolSizes {
	std::size_t scratch = 65535u * 2u;           ///< SCRATCH pool size in bytes.
	std::size_t audio = 128u * 1024u * 1024u;    ///< AUDIO pool size in bytes.
	std::size_t video = 256u * 1024u * 1024u;    ///< VIDEO pool size in bytes.
	std::size_t script = 128u * 1024u * 1024u;   ///< SCRIPT pool size in bytes.
};

/**
 * @brief Entry point to the optional process-wide freelist arenas.
 *
 * The manager owns one `Elv::Util::FreelistMemoryManager` and exposes each
 * subsystem's `ContiguousFreeListAllocator` through a
 * `std::pmr::memory_resource` adapter. Nothing is allocated until
 * @ref initialize is called.
 */
struct MH_EUPH_API MemoryManager {
	/**
	 * @brief Returns the compiled-in default pool sizes.
	 * @return A `PoolSizes` value with every field set to its default.
	 */
	static PoolSizes defaultPoolSizes() noexcept;

	/**
	 * @brief Creates the arenas with explicit pool sizes.
	 *
	 * @param sizes Per-subsystem byte sizes. Every field must be greater than zero.
	 * @return `true` when the arenas were created; `false` when they already existed.
	 * @throws std::invalid_argument If any pool size is zero.
	 * @throws std::bad_alloc If the arena cannot be allocated.
	 */
	[[nodiscard]] static bool initialize(const PoolSizes& sizes);

	/**
	 * @brief Creates the arenas from a configuration object.
	 *
	 * Reads `[Scratch]`, `[Audio]`, `[Video]` and `[Script]` sections, key
	 * `uMemoryPoolSize`, falling back to @ref defaultPoolSizes. Note that the
	 * `Configuration` getters insert missing keys, so the passed object is
	 * updated with any defaults that were used.
	 *
	 * @param config Configuration to read; not retained.
	 * @return `true` when the arenas were created; `false` when they already existed.
	 * @throws std::invalid_argument If a configured pool size is zero.
	 * @throws std::bad_alloc If the arena cannot be allocated.
	 */
	[[nodiscard]] static bool initialize(Euph::Conf::Configuration& config);

	/**
	 * @brief Returns whether @ref initialize has succeeded.
	 * @return `true` when the arenas exist.
	 */
	[[nodiscard]] static bool initialized();

	/**
	 * @brief Returns the PMR resource for a subsystem.
	 *
	 * Returns `std::pmr::get_default_resource()` when the arenas have not been
	 * initialized, or when @p subsystem is the `Count` sentinel.
	 *
	 * @param subsystem Subsystem whose arena should be used.
	 * @return A resource that remains valid for the lifetime of the process.
	 */
	[[nodiscard]] static std::pmr::memory_resource& resource(AllocatorSubsystem subsystem);
};

} // namespace Memory
} // namespace Euph
#endif // EUPHMEMORY_HPP
