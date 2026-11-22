#ifndef KALDIVKMEMORY_HPP
#define KALDIVKMEMORY_HPP
/**
 * @file VkMemory.hpp
 * @brief VulkanMemoryAllocator bootstrap for the Vulkan backend.
 *
 * Vulkan makes the application choose a memory type, honour the alignment and
 * `memoryTypeBits` of every resource, and hand out `VkDeviceMemory` blocks by
 * hand. Doing that per resource either wastes a page per object (dedicated
 * allocation) or needs a suballocator of its own - which is exactly what
 * [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
 * is: one implementation, battle-tested against every desktop and mobile driver.
 * This backend therefore allocates through VMA and keeps only the *policy*
 * ("device local, or host visible?") in its own code.
 *
 * @section kaldivk_memory_config VMA configuration
 *
 * The macros below are set before `<vk_mem_alloc.h>` is pulled in, and every
 * translation unit that touches VMA must include *this* header rather than
 * `<vk_mem_alloc.h>` directly:
 *
 * - `VMA_VULKAN_VERSION 1000000` - `ContextConfig::apiVersion` is
 *   `VK_API_VERSION_1_0`, so the instance and device are created against Vulkan
 *   1.0 and glad only loads 1.0 core entry points. Telling VMA 1.0 keeps it off
 *   `vkBindBufferMemory2`, `vkGetBufferMemoryRequirements2` and the dedicated
 *   allocation structure, all of which would otherwise resolve to null pointers.
 * - `VMA_STATIC_VULKAN_FUNCTIONS 1` / `VMA_DYNAMIC_VULKAN_FUNCTIONS 0` - glad
 *   already owns function loading (`gladLoaderLoadVulkan`), so VMA uses the same
 *   global pointers instead of loading a second copy behind our back. Static mode
 *   is only safe because of that: it never null-checks the pointers it calls.
 *
 * glad's header defines the `VULKAN_H_` include guard, so `VkContext.hpp` must be
 * included before `<vk_mem_alloc.h>` - otherwise VMA would include the *system*
 * Vulkan header and the two sets of declarations would collide.
 *
 * @section kaldivk_memory_implementation Implementation unit
 *
 * `vk_mem_alloc.h` is a single-header library: the declarations come from this
 * header, and `VkMemory.cpp` is the one translation unit that defines
 * `VMA_IMPLEMENTATION` before including it.
 */
#include "VkContext.hpp"
#include <cstdint>

#ifndef VMA_VULKAN_VERSION
#define VMA_VULKAN_VERSION 1000000
#endif
#ifndef VMA_STATIC_VULKAN_FUNCTIONS
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#endif
#ifndef VMA_DYNAMIC_VULKAN_FUNCTIONS
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#endif

#include <vk_mem_alloc.h>

namespace Vk {

/**
 * @brief Owns the `VmaAllocator` that serves one `Vk::Context`.
 *
 * Lifetime is the whole point of this wrapper: VMA requires every allocation to
 * be freed *before* the allocator is destroyed and the allocator to be destroyed
 * before the `VkDevice`. The executor keeps this object as its first member, so
 * C++ destroys it after every resource container it owns.
 *
 * The wrapper is deliberately thin - it does not wrap `vmaCreateBuffer` or
 * `vmaCreateImage`. The backend calls those directly, the same way it calls
 * `vkCreateBuffer`, and keeps the resulting `VmaAllocation` next to the Vulkan
 * handle in its resource structs.
 *
 * @note Not movable and not copyable: two owners of one allocator would destroy
 * the device's allocations twice.
 */
class MemoryAllocator final {
public:
	/**
	 * @brief Creates the allocator for @p context.
	 * @throws std::runtime_error When VMA cannot initialise; the message names the
	 *         failing call and carries `resultName()`.
	 */
	explicit MemoryAllocator(Context& context);

	/** @brief Destroys the allocator; every allocation must be gone by now. */
	~MemoryAllocator();

	MemoryAllocator(const MemoryAllocator&) = delete;
	MemoryAllocator& operator=(const MemoryAllocator&) = delete;
	MemoryAllocator(MemoryAllocator&&) = delete;
	MemoryAllocator& operator=(MemoryAllocator&&) = delete;

	/** @brief The raw allocator handle, for `vmaCreateBuffer` and friends. */
	VmaAllocator handle() const noexcept { return allocator_; }

	/** @brief True when the allocator was created successfully. */
	bool isValid() const noexcept { return allocator_ != VK_NULL_HANDLE; }

	/**
	 * @brief Bytes currently committed to `VkDeviceMemory` blocks.
	 *
	 * Reports VMA's own accounting (block size, not sum of allocation sizes), which
	 * is what a device actually pays for. Used by diagnostics and tests.
	 */
	std::uint64_t blockBytes() const noexcept;

	/** @brief Bytes handed out to allocations, rounding and unused space excluded. */
	std::uint64_t allocationBytes() const noexcept;

private:
	/** @brief The context the allocator was created for; only read for lifetime. */
	Context& context_;
	/** @brief The allocator; `VK_NULL_HANDLE` after a failed construction. */
	VmaAllocator allocator_ = VK_NULL_HANDLE;
};

}

#endif // KALDIVKMEMORY_HPP
