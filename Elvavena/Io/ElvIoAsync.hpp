#ifndef ELVIOASYNC_HPP
#define ELVIOASYNC_HPP
/**
 * @file ElvIoAsync.hpp
 * @brief Provides asynchronous I/O helpers for Elv::Io::Device objects.
 *
 * The helpers in this header adapt synchronous Device operations to any
 * Elv::Util::AsyncExecutor implementation, returning the executor's future-like
 * result for each submitted operation.
 */
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Elvavena/Util/ElvAsyncExecutor.hpp>
#include <functional>
#include <memory_resource>
namespace Elv {
namespace Io {

/**
 * @typedef ByteVector
 * @brief PMR-backed byte buffer used by asynchronous read helpers.
 */
using ByteVector = std::pmr::vector<std::byte>;

/**
 * @typedef ByteVectorPtr
 * @brief Shared pointer to a PMR-backed byte buffer.
 */
using ByteVectorPtr = std::shared_ptr<ByteVector>;

/**
 * @struct Async
 * @brief Static asynchronous wrappers for Device read and write operations.
 *
 * Async submits synchronous Device work to the provided executor. The caller is
 * responsible for keeping referenced Device objects, buffers, callbacks, and
 * memory resources alive until the returned asynchronous operation has
 * completed.
 *
 * @tparam Executor Executor type satisfying Util::AsyncExecutor.
 */
template<Util::AsyncExecutor Executor> struct Async {
	/**
	 * @brief Asynchronously reads elements from a Device into a caller-owned buffer.
	 *
	 * @param executor Executor used to run the operation.
	 * @param device Device to read from.
	 * @param buffer Destination buffer. Must remain valid until the task completes.
	 * @param size Size of each element to read.
	 * @param count Maximum number of elements to read.
	 * @return Executor-specific future-like object containing the Device::read result.
	 */
	[[nodiscard]] static auto read(Executor& executor, Device& device, void* buffer, size_t size, size_t count)
	{
		return executor.enqueueAsync( [&device, buffer, size, count] () {
				return device.read(buffer, size, count);
		});
	}

	/**
	 * @brief Asynchronously writes elements from a caller-owned buffer to a Device.
	 *
	 * @param executor Executor used to run the operation.
	 * @param device Device to write to.
	 * @param buffer Source buffer. Must remain valid until the task completes.
	 * @param size Size of each element to write.
	 * @param count Maximum number of elements to write.
	 * @return Executor-specific future-like object containing the Device::write result.
	 */
	[[nodiscard]] static auto write(Executor& executor, Device& device, const void* buffer, size_t size, size_t count)
	{
		return executor.enqueueAsync( [&device, buffer, size, count] () {
				return device.write(buffer, size, count);
		});
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a Device.
	 *
	 * Reading starts at the Device's current position and continues to the
	 * reported end of the Device.
	 *
	 * @param executor Executor used to run the operation.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param memRes Memory resource used by the returned ByteVector. The resource
	 * must outlive both the task and the returned vector.
	 * @return Executor-specific future-like object containing a ByteVector.
	 */
	[[nodiscard]] static auto readAll(Executor& executor, Device& device, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
	{
		return executor.enqueueAsync([&device, memRes]() {
			const auto pos = static_cast<size_t>(device.tell());
			const auto total = device.size();
			const auto remaining = total > pos ? total - pos : 0;

			std::pmr::vector<std::byte> result(memRes);
			result.resize(remaining);

			if (!result.empty())
				device.read(result.data(), 1, result.size());

			return result;
		});
	}

	/**
	 * @brief Asynchronously reads all remaining bytes into a shared PMR buffer.
	 *
	 * Reading starts at the Device's current position and continues to the
	 * reported end of the Device. The byte vector itself is allocated with the
	 * supplied memory resource and returned in a shared pointer.
	 *
	 * @param executor Executor used to run the operation.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param memRes Memory resource used for the shared object and byte storage.
	 * The resource must outlive both the task and the returned shared buffer.
	 * @return Executor-specific future-like object containing a ByteVectorPtr.
	 */
	[[nodiscard]] static auto readAllShared(Executor& executor, Device& device, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
	{
		return executor.enqueueAsync([&device, memRes]() {
			auto result = std::allocate_shared<ByteVector>(
				std::pmr::polymorphic_allocator<ByteVector>(memRes),
				memRes
				);

			const auto pos = static_cast<size_t>(device.tell());
			const auto total = device.size();
			const auto remaining = total > pos ? total - pos : 0;

			result->resize(remaining);

			if (!result->empty())
				device.read(result->data(), 1, result->size());

			return result;
		});
	}

	/**
	 * @brief Asynchronously reads a Device in allocated chunks and invokes a callback.
	 *
	 * The callback is called once per non-empty chunk with a span covering only
	 * the bytes read in that iteration. The span is valid only for the duration
	 * of the callback call.
	 *
	 * @tparam Callback Callable accepting std::span<const std::byte>.
	 * @param executor Executor used to run the operation.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param chunkSize Size of the temporary chunk buffer.
	 * @param callback Callback invoked for each chunk.
	 * @param memRes Memory resource used for the temporary chunk buffer. The
	 * resource must outlive the asynchronous task.
	 * @return Executor-specific future-like object for completion of the chunk read.
	 */
	template<typename Callback> [[nodiscard]] static auto readChunks(Executor& executor, Device& device, size_t chunkSize, Callback&& callback, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
	{
		return executor.enqueueAsync( [&device, memRes, chunkSize, cb = std::forward<Callback>(callback)]() mutable {
			std::pmr::vector<std::byte> chunk(memRes);
			chunk.resize(chunkSize);

			while (!device.eof()) {
				const size_t got = device.read(chunk.data(), 1, chunk.size());
				if (got == 0)
					break;
				cb(std::span<const std::byte>(chunk.data(), got));
			}
		});
	}

	/**
	 * @brief Asynchronously reads a Device in chunks using caller-owned storage.
	 *
	 * The callback is called once per non-empty chunk with a span covering only
	 * the bytes read in that iteration. The span is valid only for the duration
	 * of the callback call.
	 *
	 * @tparam Callback Callable accepting std::span<const std::byte>.
	 * @param executor Executor used to run the operation.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param chunk Caller-owned temporary buffer. Must remain valid until the task completes.
	 * @param chunkSize Size of the caller-owned temporary buffer.
	 * @param callback Callback invoked for each chunk.
	 * @return Executor-specific future-like object for completion of the chunk read.
	 */
	template<typename Callback> [[nodiscard]] static auto readChunks(Executor& executor, Device& device, void* chunk, size_t chunkSize, Callback&& callback)
	{
		return executor.enqueueAsync( [&device, chunk, chunkSize, cb = std::forward<Callback>(callback)]() mutable {
			while (!device.eof()) {
				const size_t got = device.read(chunk, 1, chunkSize);
				if (got == 0)
					break;
				cb(std::span<const std::byte>(static_cast<std::byte*>(chunk), got));
			}
		});
	}
};

} // namespace Io
} // namespace Elv
#endif // ELVIOASYNC_HPP
