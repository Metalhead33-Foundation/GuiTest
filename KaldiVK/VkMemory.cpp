/**
 * @file VkMemory.cpp
 * @brief Implements VkMemory.hpp and hosts the single VMA implementation unit.
 *
 * `VMA_IMPLEMENTATION` has to be defined before `<vk_mem_alloc.h>` is included for
 * the *first* time, so it is defined here, above the header that includes it.
 */
#define VMA_IMPLEMENTATION
#include "VkMemory.hpp"
#include <stdexcept>
#include <string>

namespace Vk {

namespace {

/** @brief Builds the exception message for a failed VMA call. */
[[noreturn]] void throwVmaFailure(const char* step, VkResult result)
{
	std::string message = "VkMemory: ";
	message += step;
	message += " failed (";
	message += resultName(result);
	message += ")";
	throw std::runtime_error(message);
}

} // namespace

MemoryAllocator::MemoryAllocator(Context& context)
	: context_(context)
{
	VmaAllocatorCreateInfo info {};
	// VMA 3.x derives the API version from `vulkanApiVersion`; keep it at 1.0 to
	// match `ContextConfig::apiVersion` and the glad entry points that were loaded.
	info.vulkanApiVersion = VK_API_VERSION_1_0;
	info.instance = context_.instance();
	info.physicalDevice = context_.physicalDevice();
	info.device = context_.device();
	// Defaults cover the rest: VMA uses the static (glad-loaded) entry points and
	// its own block/suballocation strategy, and asks for no optional extensions.
	const VkResult result = vmaCreateAllocator(&info, &allocator_);
	if (result != VK_SUCCESS) {
		allocator_ = VK_NULL_HANDLE;
		throwVmaFailure("vmaCreateAllocator", result);
	}
}

MemoryAllocator::~MemoryAllocator()
{
	if (allocator_ != VK_NULL_HANDLE) {
		// Safe even when the backend forgot something: VMA frees leftover
		// allocations strictly before it frees the blocks that hold them.
		vmaDestroyAllocator(allocator_);
		allocator_ = VK_NULL_HANDLE;
	}
}

std::uint64_t MemoryAllocator::blockBytes() const noexcept
{
	if (allocator_ == VK_NULL_HANDLE) {
		return 0;
	}
	VmaTotalStatistics statistics {};
	vmaCalculateStatistics(allocator_, &statistics);
	return statistics.total.statistics.blockBytes;
}

std::uint64_t MemoryAllocator::allocationBytes() const noexcept
{
	if (allocator_ == VK_NULL_HANDLE) {
		return 0;
	}
	VmaTotalStatistics statistics {};
	vmaCalculateStatistics(allocator_, &statistics);
	return statistics.total.statistics.allocationBytes;
}

} // namespace Vk
