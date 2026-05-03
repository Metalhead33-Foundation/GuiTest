#ifndef ELVIOASYNC_HPP
#define ELVIOASYNC_HPP
/**
 * @file ElvIoAsync.hpp
 * @brief Provides asynchronous I/O helpers for Elv::Io::Device objects.
 *
 * The helpers in this header adapt synchronous, blocking Device operations to
 * any Elv::Util::AsyncExecutor implementation. They schedule blocking work on
 * the executor and return the executor's future-like result. They do not
 * provide OS-level non-blocking I/O such as epoll, io_uring, or IOCP.
 */
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Elvavena/Util/ElvAsyncExecutor.hpp>
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <memory_resource>
#include <optional>
#include <span>
#include <stdexcept>
#include <stop_token>
#include <type_traits>
#include <utility>
#include <vector>

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
 * @brief Exception thrown by cancellable async helpers when their stop token is requested.
 */
class AsyncOperationCanceled : public std::runtime_error {
public:
	/**
	 * @brief Constructs the cancellation exception with a stable diagnostic message.
	 */
	AsyncOperationCanceled()
		: std::runtime_error("Async I/O operation canceled")
	{
	}
};

/**
 * @brief C++20 result object passed to async completion callbacks.
 *
 * The object contains either a value or an exception captured from the async
 * operation. Calling value() on an error result rethrows the stored exception.
 *
 * @tparam T Successful operation value type.
 */
template<typename T> class AsyncResult {
private:
	std::optional<T> storedValue;
	std::exception_ptr storedError;

	[[noreturn]] void rethrowStoredError() const
	{
		if (storedError)
			std::rethrow_exception(storedError);
		throw std::runtime_error("AsyncResult has no value");
	}

public:
	/**
	 * @brief Constructs a successful result by copying a value.
	 * @param value Value produced by the async operation.
	 */
	AsyncResult(const T& value)
		: storedValue(value)
		, storedError(nullptr)
	{
	}

	/**
	 * @brief Constructs a successful result by moving a value.
	 * @param value Value produced by the async operation.
	 */
	AsyncResult(T&& value)
		: storedValue(std::move(value))
		, storedError(nullptr)
	{
	}

	/**
	 * @brief Constructs a failed result from a captured exception.
	 * @param error Exception captured from the async operation.
	 */
	explicit AsyncResult(std::exception_ptr error)
		: storedValue(std::nullopt)
		, storedError(error ? error : std::make_exception_ptr(std::runtime_error("Unknown async I/O error")))
	{
	}

	/**
	 * @brief Returns true when this result contains a value.
	 * @return True for success, false when error() contains an exception.
	 */
	[[nodiscard]] bool has_value() const noexcept
	{
		return storedValue.has_value();
	}

	/**
	 * @brief Returns true when this result contains a value.
	 * @return True for success, false when error() contains an exception.
	 */
	[[nodiscard]] explicit operator bool() const noexcept
	{
		return has_value();
	}

	/**
	 * @brief Returns the stored value, or rethrows the stored exception.
	 * @return Mutable reference to the stored value.
	 * @throws Any exception stored in error().
	 */
	[[nodiscard]] T& value() &
	{
		if (!storedValue)
			rethrowStoredError();
		return *storedValue;
	}

	/**
	 * @brief Returns the stored value, or rethrows the stored exception.
	 * @return Const reference to the stored value.
	 * @throws Any exception stored in error().
	 */
	[[nodiscard]] const T& value() const &
	{
		if (!storedValue)
			rethrowStoredError();
		return *storedValue;
	}

	/**
	 * @brief Moves the stored value out, or rethrows the stored exception.
	 * @return Stored value moved out of this result.
	 * @throws Any exception stored in error().
	 */
	[[nodiscard]] T value() &&
	{
		if (!storedValue)
			rethrowStoredError();
		return std::move(*storedValue);
	}

	/**
	 * @brief Returns the captured exception, or nullptr for successful results.
	 * @return Stored exception pointer, or nullptr when has_value() is true.
	 */
	[[nodiscard]] std::exception_ptr error() const noexcept
	{
		return storedError;
	}
};

namespace detail {

/**
 * @concept AsyncResultCallback
 * @brief Requires a completion callback that consumes an AsyncResult<T> and returns void.
 *
 * The async *Then helpers use this concept so callback failures are simple to
 * reason about: operation results are delivered as a const AsyncResult<T>&, and
 * any callback exception propagates through the returned future.
 *
 * @tparam Callback Callback type being constrained.
 * @tparam T Successful operation value type carried by AsyncResult.
 */
template<typename Callback, typename T>
concept AsyncResultCallback =
	std::invocable<Callback&, const AsyncResult<T>&> &&
	std::same_as<std::invoke_result_t<Callback&, const AsyncResult<T>&>, void>;

/**
 * @concept AsyncChunkCallback
 * @brief Requires a per-chunk callback that consumes a byte span and returns void.
 *
 * The span passed to this callback is valid only for the duration of the
 * callback invocation. Returning void keeps accidental asynchronous retention of
 * temporary chunk spans less likely.
 *
 * @tparam Callback Callback type being constrained.
 */
template<typename Callback>
concept AsyncChunkCallback =
	std::invocable<Callback&, std::span<const std::byte> > &&
	std::same_as<std::invoke_result_t<Callback&, std::span<const std::byte> >, void>;

} // namespace detail

/**
 * @struct Async
 * @brief Static asynchronous wrappers for Device operations.
 *
 * Async submits synchronous Device work to the provided executor. Raw-reference
 * overloads require the caller to keep referenced Device objects, buffers,
 * callbacks, and memory resources alive until the returned asynchronous
 * operation has completed. Shared-device overloads keep the Device alive for
 * the task duration, but caller-owned buffers still must outlive the task.
 * The sDevice alias is generated by DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(Device)
 * in ElvIoDevice.hpp and is this module's shared Device pointer type.
 *
 * Aggregate read helpers are intentionally strict and file-like: Device::tell()
 * and Device::size() are treated as a byte-count promise, and Device::read()
 * returning 0 before that promised byte count is consumed is an error.
 * Tune aggregate read polling granularity by passing both a memory resource and
 * chunkSize; there are no bare chunk-size overloads to keep overload resolution
 * predictable.
 *
 * @warning All *Then callbacks run in the executor context before the returned
 * future is made ready. A *Then callback must not wait on that same future,
 * especially when using a single-threaded executor such as EventLoop.
 *
 * @tparam Executor Executor type satisfying Util::AsyncExecutor.
 */
template<Util::AsyncExecutor Executor> struct Async {
public:
	/**
	 * @brief Default byte chunk size used by aggregate read helpers.
	 *
	 * Pass both a memory resource and an explicit chunk size to readAll(),
	 * readAllShared(), or readAllThen() to tune cancellation polling and read
	 * request size without introducing ambiguous bare-size overloads.
	 */
	inline static constexpr size_t DefaultReadAllChunkSize = 64 * 1024;

private:
	/**
	 * @brief Returns a usable PMR memory resource.
	 *
	 * @param memRes Optional caller-provided memory resource.
	 * @return @p memRes when non-null, otherwise std::pmr::get_default_resource().
	 */
	static std::pmr::memory_resource* memoryResourceOrDefault(std::pmr::memory_resource* memRes)
	{
		return memRes ? memRes : std::pmr::get_default_resource();
	}

	/**
	 * @brief Converts a shared Device pointer to a checked Device reference.
	 *
	 * @param device Shared Device pointer to validate.
	 * @return Reference to the pointed-to Device.
	 * @throws std::runtime_error If @p device is null.
	 */
	static Device& requireDevice(const sDevice& device)
	{
		if (!device)
			throw std::runtime_error("Async I/O operation received a null Device shared_ptr");
		return *device;
	}

	/**
	 * @brief Throws the async cancellation exception when a stop request exists.
	 *
	 * @param stopToken Cooperative cancellation token to inspect.
	 * @throws AsyncOperationCanceled If @p stopToken has been requested.
	 */
	static void throwIfStopped(std::stop_token stopToken)
	{
		if (stopToken.stop_requested())
			throw AsyncOperationCanceled();
	}

	/**
	 * @brief Validates that a Device can be used for async read helpers.
	 *
	 * @param device Device to validate.
	 * @throws std::runtime_error If Device::isValid() reports false.
	 */
	static void validateReadable(Device& device)
	{
		if (!device.isValid())
			throw std::runtime_error("Async read attempted on an invalid Device");
	}

	/**
	 * @brief Computes remaining bytes from the current Device position.
	 *
	 * Device::tell() and Device::size() are treated as a file-like byte-count
	 * contract for aggregate reads.
	 *
	 * @param device Device to query.
	 * @return Number of bytes between Device::tell() and Device::size().
	 * @throws std::runtime_error If Device::tell() returns a negative value.
	 */
	static size_t remainingBytes(Device& device)
	{
		const long pos = device.tell();
		if (pos < 0)
			throw std::runtime_error("Device::tell failed during async read");

		const size_t total = device.size();
		const size_t current = static_cast<size_t>(pos);
		return total > current ? total - current : 0;
	}

	/**
	 * @brief Reads the remaining Device bytes into an owning PMR vector.
	 *
	 * This helper implements strict aggregate-read semantics: the byte count
	 * promised by remainingBytes() must be consumed completely, and a zero-byte
	 * Device::read() before completion is treated as an error.
	 *
	 * @param device Device to read from.
	 * @param result Destination vector whose contents are replaced with the remaining bytes.
	 * @param stopToken Cooperative cancellation token checked before and between reads.
	 * @param chunkSize Maximum byte count requested from each Device::read call.
	 * @throws AsyncOperationCanceled If @p stopToken is requested.
	 * @throws std::invalid_argument If @p chunkSize is 0.
	 * @throws std::runtime_error If the Device is invalid, tell() fails, or read() ends early.
	 */
	static void readRemainingInto(Device& device, ByteVector& result, std::stop_token stopToken, size_t chunkSize)
	{
		if (chunkSize == 0)
			throw std::invalid_argument("Async aggregate reads require a non-zero chunk size");

		throwIfStopped(stopToken);
		validateReadable(device);

		const size_t remaining = remainingBytes(device);
		result.resize(remaining);

		size_t totalRead = 0;
		while (totalRead < remaining) {
			throwIfStopped(stopToken);
			const size_t requested = std::min(chunkSize, remaining - totalRead);
			const size_t got = device.read(result.data() + totalRead, 1, requested);
			if (got == 0)
				throw std::runtime_error("Device::read ended before the expected byte count");
			totalRead += got;
		}

		throwIfStopped(stopToken);
	}

	/**
	 * @brief Reads a Device in PMR-allocated chunks and returns delivered byte count.
	 *
	 * @tparam Callback Per-chunk callback type constrained by AsyncChunkCallback.
	 * @param device Device to read from.
	 * @param stopToken Cooperative cancellation token checked before and between reads.
	 * @param chunkSize Size in bytes of the temporary chunk buffer.
	 * @param memRes Memory resource used by the temporary chunk buffer.
	 * @param callback Callback invoked once per non-empty chunk.
	 * @return Total bytes delivered to @p callback.
	 * @throws AsyncOperationCanceled If @p stopToken is requested.
	 * @throws std::invalid_argument If @p chunkSize is 0.
	 * @throws std::runtime_error If the Device is invalid.
	 */
	template<typename Callback>
	requires detail::AsyncChunkCallback<Callback>
	static size_t readAllocatedChunks(Device& device, std::stop_token stopToken, size_t chunkSize, std::pmr::memory_resource* memRes, Callback& callback)
	{
		if (chunkSize == 0)
			throw std::invalid_argument("Async::readChunks requires a non-zero chunk size");

		throwIfStopped(stopToken);
		validateReadable(device);

		ByteVector chunk(memoryResourceOrDefault(memRes));
		chunk.resize(chunkSize);

		size_t totalRead = 0;
		while (!device.eof()) {
			throwIfStopped(stopToken);
			const size_t got = device.read(chunk.data(), 1, chunk.size());
			if (got == 0)
				break;
			std::invoke(callback, std::span<const std::byte>(chunk.data(), got));
			totalRead += got;
		}

		throwIfStopped(stopToken);
		return totalRead;
	}

	/**
	 * @brief Reads a Device into caller-owned chunks and returns delivered byte count.
	 *
	 * @tparam Callback Per-chunk callback type constrained by AsyncChunkCallback.
	 * @param device Device to read from.
	 * @param stopToken Cooperative cancellation token checked before and between reads.
	 * @param chunk Caller-owned temporary storage used for each read.
	 * @param chunkSize Size in bytes of @p chunk.
	 * @param callback Callback invoked once per non-empty chunk.
	 * @return Total bytes delivered to @p callback.
	 * @throws AsyncOperationCanceled If @p stopToken is requested.
	 * @throws std::invalid_argument If @p chunkSize is 0 or @p chunk is null.
	 * @throws std::runtime_error If the Device is invalid.
	 */
	template<typename Callback>
	requires detail::AsyncChunkCallback<Callback>
	static size_t readBorrowedChunks(Device& device, std::stop_token stopToken, void* chunk, size_t chunkSize, Callback& callback)
	{
		if (chunkSize == 0)
			throw std::invalid_argument("Async::readChunks requires a non-zero chunk size");
		if (!chunk)
			throw std::invalid_argument("Async::readChunks requires non-null caller-owned storage");

		throwIfStopped(stopToken);
		validateReadable(device);

		size_t totalRead = 0;
		while (!device.eof()) {
			throwIfStopped(stopToken);
			const size_t got = device.read(chunk, 1, chunkSize);
			if (got == 0)
				break;
			std::invoke(callback, std::span<const std::byte>(static_cast<const std::byte*>(chunk), got));
			totalRead += got;
		}

		throwIfStopped(stopToken);
		return totalRead;
	}

	/**
	 * @brief Runs an operation and reports success or failure to a result callback.
	 *
	 * The callback is invoked before this helper returns or rethrows. Operation
	 * exceptions are delivered as AsyncResult<T> errors and then rethrown so the
	 * executor future observes the same failure. Callback exceptions are allowed
	 * to propagate through the executor future.
	 *
	 * @tparam T Successful operation value type.
	 * @tparam Operation Callable operation returning T.
	 * @tparam Callback Result callback type constrained by AsyncResultCallback.
	 * @param operation Operation to invoke.
	 * @param callback Callback to receive success or failure.
	 * @return Successful operation value.
	 * @throws Any exception thrown by @p operation or @p callback.
	 */
	template<typename T, typename Operation, typename Callback>
	requires detail::AsyncResultCallback<Callback, T>
	static T runThen(Operation&& operation, Callback& callback)
	{
		T value = [&]() -> T {
			try {
				return std::invoke(std::forward<Operation>(operation));
			} catch (...) {
				const std::exception_ptr error = std::current_exception();
				AsyncResult<T> callbackResult(error);
				std::invoke(callback, callbackResult);
				std::rethrow_exception(error);
			}
		}();

		AsyncResult<T> callbackResult(value);
		std::invoke(callback, callbackResult);
		return value;
	}

	/**
	 * @brief Runs an aggregate read operation and reports a temporary span result.
	 *
	 * The returned future owns the ByteVector, while the callback receives an
	 * AsyncResult containing a span into that vector. The span is valid only for
	 * the duration of the callback call.
	 *
	 * @tparam Operation Callable operation returning ByteVector.
	 * @tparam Callback Result callback type constrained for std::span<const std::byte>.
	 * @param operation Operation to invoke.
	 * @param callback Callback to receive success or failure.
	 * @return Owning ByteVector produced by @p operation.
	 * @throws Any exception thrown by @p operation or @p callback.
	 */
	template<typename Operation, typename Callback>
	requires detail::AsyncResultCallback<Callback, std::span<const std::byte> >
	static ByteVector runReadAllThen(Operation&& operation, Callback& callback)
	{
		ByteVector result = [&]() -> ByteVector {
			try {
				return std::invoke(std::forward<Operation>(operation));
			} catch (...) {
				const std::exception_ptr error = std::current_exception();
				AsyncResult<std::span<const std::byte> > callbackResult(error);
				std::invoke(callback, callbackResult);
				std::rethrow_exception(error);
			}
		}();

		AsyncResult<std::span<const std::byte> > callbackResult(std::span<const std::byte>(result.data(), result.size()));
		std::invoke(callback, callbackResult);
		return result;
	}

public:
	/**
	 * @brief Asynchronously reads elements from a Device into a caller-owned buffer.
	 *
	 * @param executor Executor used to run the blocking Device::read call.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param buffer Destination buffer. Must remain valid until the task completes.
	 * @param size Size of each element to read.
	 * @param count Maximum number of elements to read.
	 * @return Executor-specific future-like object containing Device::read's size_t result.
	 */
	[[nodiscard]] static auto read(Executor& executor, Device& device, void* buffer, size_t size, size_t count)
	{
		return executor.enqueueAsync([&device, buffer, size, count]() {
			return device.read(buffer, size, count);
		});
	}

	/**
	 * @brief Asynchronously reads elements from a shared Device into a caller-owned buffer.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @param executor Executor used to run the blocking Device::read call.
	 * @param device Shared Device to read from. Must not be null.
	 * @param buffer Destination buffer. Must remain valid until the task completes.
	 * @param size Size of each element to read.
	 * @param count Maximum number of elements to read.
	 * @return Executor-specific future-like object containing Device::read's size_t result.
	 */
	[[nodiscard]] static auto read(Executor& executor, sDevice device, void* buffer, size_t size, size_t count)
	{
		return executor.enqueueAsync([device = std::move(device), buffer, size, count]() {
			return requireDevice(device).read(buffer, size, count);
		});
	}

	/**
	 * @brief Asynchronously writes elements from a caller-owned buffer to a Device.
	 *
	 * @param executor Executor used to run the blocking Device::write call.
	 * @param device Device to write to. Must remain valid until the task completes.
	 * @param buffer Source buffer. Must remain valid until the task completes.
	 * @param size Size of each element to write.
	 * @param count Maximum number of elements to write.
	 * @return Executor-specific future-like object containing Device::write's size_t result.
	 */
	[[nodiscard]] static auto write(Executor& executor, Device& device, const void* buffer, size_t size, size_t count)
	{
		return executor.enqueueAsync([&device, buffer, size, count]() {
			return device.write(buffer, size, count);
		});
	}

	/**
	 * @brief Asynchronously writes elements from a caller-owned buffer to a shared Device.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @param executor Executor used to run the blocking Device::write call.
	 * @param device Shared Device to write to. Must not be null.
	 * @param buffer Source buffer. Must remain valid until the task completes.
	 * @param size Size of each element to write.
	 * @param count Maximum number of elements to write.
	 * @return Executor-specific future-like object containing Device::write's size_t result.
	 */
	[[nodiscard]] static auto write(Executor& executor, sDevice device, const void* buffer, size_t size, size_t count)
	{
		return executor.enqueueAsync([device = std::move(device), buffer, size, count]() {
			return requireDevice(device).write(buffer, size, count);
		});
	}

	/**
	 * @brief Asynchronously seeks within a Device.
	 *
	 * @param executor Executor used to run the blocking Device::seek call.
	 * @param device Device to seek. Must remain valid until the task completes.
	 * @param offset Byte offset relative to @p whence.
	 * @param whence Origin used to interpret @p offset.
	 * @return Executor-specific future-like object containing Device::seek's int result.
	 */
	[[nodiscard]] static auto seek(Executor& executor, Device& device, long offset, SeekOrigin whence)
	{
		return executor.enqueueAsync([&device, offset, whence]() {
			return device.seek(offset, whence);
		});
	}

	/**
	 * @brief Asynchronously seeks within a shared Device.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @param executor Executor used to run the blocking Device::seek call.
	 * @param device Shared Device to seek. Must not be null.
	 * @param offset Byte offset relative to @p whence.
	 * @param whence Origin used to interpret @p offset.
	 * @return Executor-specific future-like object containing Device::seek's int result.
	 */
	[[nodiscard]] static auto seek(Executor& executor, sDevice device, long offset, SeekOrigin whence)
	{
		return executor.enqueueAsync([device = std::move(device), offset, whence]() {
			return requireDevice(device).seek(offset, whence);
		});
	}

	/**
	 * @brief Asynchronously queries the current Device position.
	 *
	 * @param executor Executor used to run the blocking Device::tell call.
	 * @param device Device to query. Must remain valid until the task completes.
	 * @return Executor-specific future-like object containing Device::tell's long result.
	 */
	[[nodiscard]] static auto tell(Executor& executor, Device& device)
	{
		return executor.enqueueAsync([&device]() {
			return device.tell();
		});
	}

	/**
	 * @brief Asynchronously queries the current shared Device position.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @param executor Executor used to run the blocking Device::tell call.
	 * @param device Shared Device to query. Must not be null.
	 * @return Executor-specific future-like object containing Device::tell's long result.
	 */
	[[nodiscard]] static auto tell(Executor& executor, sDevice device)
	{
		return executor.enqueueAsync([device = std::move(device)]() {
			return requireDevice(device).tell();
		});
	}

	/**
	 * @brief Asynchronously queries the complete Device size.
	 *
	 * @param executor Executor used to run the blocking Device::size call.
	 * @param device Device to query. Must remain valid until the task completes.
	 * @return Executor-specific future-like object containing Device::size's size_t result.
	 */
	[[nodiscard]] static auto size(Executor& executor, Device& device)
	{
		return executor.enqueueAsync([&device]() {
			return device.size();
		});
	}

	/**
	 * @brief Asynchronously queries the complete shared Device size.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @param executor Executor used to run the blocking Device::size call.
	 * @param device Shared Device to query. Must not be null.
	 * @return Executor-specific future-like object containing Device::size's size_t result.
	 */
	[[nodiscard]] static auto size(Executor& executor, sDevice device)
	{
		return executor.enqueueAsync([device = std::move(device)]() {
			return requireDevice(device).size();
		});
	}

	/**
	 * @brief Asynchronously checks whether a Device is at EOF.
	 *
	 * @param executor Executor used to run the blocking Device::eof call.
	 * @param device Device to query. Must remain valid until the task completes.
	 * @return Executor-specific future-like object containing Device::eof's bool result.
	 */
	[[nodiscard]] static auto eof(Executor& executor, Device& device)
	{
		return executor.enqueueAsync([&device]() {
			return device.eof();
		});
	}

	/**
	 * @brief Asynchronously checks whether a shared Device is at EOF.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @param executor Executor used to run the blocking Device::eof call.
	 * @param device Shared Device to query. Must not be null.
	 * @return Executor-specific future-like object containing Device::eof's bool result.
	 */
	[[nodiscard]] static auto eof(Executor& executor, sDevice device)
	{
		return executor.enqueueAsync([device = std::move(device)]() {
			return requireDevice(device).eof();
		});
	}

	/**
	 * @brief Asynchronously flushes a Device.
	 *
	 * @param executor Executor used to run the blocking Device::flush call.
	 * @param device Device to flush. Must remain valid until the task completes.
	 * @return Executor-specific future-like object containing Device::flush's bool result.
	 */
	[[nodiscard]] static auto flush(Executor& executor, Device& device)
	{
		return executor.enqueueAsync([&device]() {
			return device.flush();
		});
	}

	/**
	 * @brief Asynchronously flushes a shared Device.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @param executor Executor used to run the blocking Device::flush call.
	 * @param device Shared Device to flush. Must not be null.
	 * @return Executor-specific future-like object containing Device::flush's bool result.
	 */
	[[nodiscard]] static auto flush(Executor& executor, sDevice device)
	{
		return executor.enqueueAsync([device = std::move(device)]() {
			return requireDevice(device).flush();
		});
	}

	/**
	 * @brief Asynchronously checks whether a Device is valid.
	 *
	 * @param executor Executor used to run the blocking Device::isValid call.
	 * @param device Device to query. Must remain valid until the task completes.
	 * @return Executor-specific future-like object containing Device::isValid's bool result.
	 */
	[[nodiscard]] static auto isValid(Executor& executor, Device& device)
	{
		return executor.enqueueAsync([&device]() {
			return device.isValid();
		});
	}

	/**
	 * @brief Asynchronously checks whether a shared Device is valid.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @param executor Executor used to run the blocking Device::isValid call.
	 * @param device Shared Device to query. Must not be null.
	 * @return Executor-specific future-like object containing Device::isValid's bool result.
	 */
	[[nodiscard]] static auto isValid(Executor& executor, sDevice device)
	{
		return executor.enqueueAsync([device = std::move(device)]() {
			return requireDevice(device).isValid();
		});
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a Device.
	 *
	 * The operation validates the Device, computes bytes remaining from
	 * Device::tell and Device::size, and reads in bounded chunks. Device::tell()
	 * and Device::size() are treated as a byte-count promise; Device::read()
	 * returning 0 before the expected count is consumed is an error.
	 *
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param memRes Memory resource used by the returned ByteVector.
	 * @param chunkSize Maximum byte count requested from each Device::read call.
	 * @return Executor-specific future-like object containing the read ByteVector.
	 * @throws std::invalid_argument Through the future if @p chunkSize is 0.
	 * @throws std::runtime_error Through the future if the Device is invalid or ends early.
	 */
	[[nodiscard]] static auto readAll(Executor& executor, Device& device, std::pmr::memory_resource* memRes = std::pmr::get_default_resource(), size_t chunkSize = DefaultReadAllChunkSize)
	{
		return readAll(executor, device, std::stop_token(), memRes, chunkSize);
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a Device with cooperative cancellation.
	 *
	 * The stop token is checked before reading, between bounded reads, and after
	 * the loop finishes. A stop request fails the returned future with
	 * AsyncOperationCanceled.
	 *
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param stopToken Cooperative cancellation token.
	 * @param memRes Memory resource used by the returned ByteVector.
	 * @param chunkSize Maximum byte count requested from each Device::read call.
	 * @return Executor-specific future-like object containing the read ByteVector.
	 * @throws AsyncOperationCanceled Through the future when @p stopToken is requested.
	 * @throws std::invalid_argument Through the future if @p chunkSize is 0.
	 * @throws std::runtime_error Through the future if the Device is invalid or ends early.
	 */
	[[nodiscard]] static auto readAll(Executor& executor, Device& device, std::stop_token stopToken, std::pmr::memory_resource* memRes = std::pmr::get_default_resource(), size_t chunkSize = DefaultReadAllChunkSize)
	{
		return executor.enqueueAsync([&device, stopToken, memRes, chunkSize]() {
			ByteVector result(memoryResourceOrDefault(memRes));
			readRemainingInto(device, result, stopToken, chunkSize);
			return result;
		});
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a shared Device.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param memRes Memory resource used by the returned ByteVector.
	 * @param chunkSize Maximum byte count requested from each Device::read call.
	 * @return Executor-specific future-like object containing the read ByteVector.
	 */
	[[nodiscard]] static auto readAll(Executor& executor, sDevice device, std::pmr::memory_resource* memRes = std::pmr::get_default_resource(), size_t chunkSize = DefaultReadAllChunkSize)
	{
		return readAll(executor, std::move(device), std::stop_token(), memRes, chunkSize);
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a shared Device with cooperative cancellation.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 * A stop request fails the returned future with AsyncOperationCanceled.
	 *
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param stopToken Cooperative cancellation token.
	 * @param memRes Memory resource used by the returned ByteVector.
	 * @param chunkSize Maximum byte count requested from each Device::read call.
	 * @return Executor-specific future-like object containing the read ByteVector.
	 * @throws AsyncOperationCanceled Through the future when @p stopToken is requested.
	 */
	[[nodiscard]] static auto readAll(Executor& executor, sDevice device, std::stop_token stopToken, std::pmr::memory_resource* memRes = std::pmr::get_default_resource(), size_t chunkSize = DefaultReadAllChunkSize)
	{
		return executor.enqueueAsync([device = std::move(device), stopToken, memRes, chunkSize]() {
			ByteVector result(memoryResourceOrDefault(memRes));
			readRemainingInto(requireDevice(device), result, stopToken, chunkSize);
			return result;
		});
	}

	/**
	 * @brief Asynchronously reads all remaining bytes into a shared PMR buffer.
	 *
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param memRes Memory resource used for the shared object and byte storage.
	 * @param chunkSize Maximum byte count requested from each Device::read call.
	 * @return Executor-specific future-like object containing a ByteVectorPtr.
	 */
	[[nodiscard]] static auto readAllShared(Executor& executor, Device& device, std::pmr::memory_resource* memRes = std::pmr::get_default_resource(), size_t chunkSize = DefaultReadAllChunkSize)
	{
		return readAllShared(executor, device, std::stop_token(), memRes, chunkSize);
	}

	/**
	 * @brief Asynchronously reads all remaining bytes into a shared PMR buffer with cooperative cancellation.
	 *
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param stopToken Cooperative cancellation token.
	 * @param memRes Memory resource used for the shared object and byte storage.
	 * @param chunkSize Maximum byte count requested from each Device::read call.
	 * @return Executor-specific future-like object containing a ByteVectorPtr.
	 * @throws AsyncOperationCanceled Through the future when @p stopToken is requested.
	 */
	[[nodiscard]] static auto readAllShared(Executor& executor, Device& device, std::stop_token stopToken, std::pmr::memory_resource* memRes = std::pmr::get_default_resource(), size_t chunkSize = DefaultReadAllChunkSize)
	{
		return executor.enqueueAsync([&device, stopToken, memRes, chunkSize]() {
			auto resource = memoryResourceOrDefault(memRes);
			auto result = std::allocate_shared<ByteVector>(
				std::pmr::polymorphic_allocator<ByteVector>(resource)
				);
			readRemainingInto(device, *result, stopToken, chunkSize);
			return result;
		});
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a shared Device into a shared PMR buffer.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param memRes Memory resource used for the shared object and byte storage.
	 * @param chunkSize Maximum byte count requested from each Device::read call.
	 * @return Executor-specific future-like object containing a ByteVectorPtr.
	 */
	[[nodiscard]] static auto readAllShared(Executor& executor, sDevice device, std::pmr::memory_resource* memRes = std::pmr::get_default_resource(), size_t chunkSize = DefaultReadAllChunkSize)
	{
		return readAllShared(executor, std::move(device), std::stop_token(), memRes, chunkSize);
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a shared Device into a shared PMR buffer with cooperative cancellation.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param stopToken Cooperative cancellation token.
	 * @param memRes Memory resource used for the shared object and byte storage.
	 * @param chunkSize Maximum byte count requested from each Device::read call.
	 * @return Executor-specific future-like object containing a ByteVectorPtr.
	 * @throws AsyncOperationCanceled Through the future when @p stopToken is requested.
	 */
	[[nodiscard]] static auto readAllShared(Executor& executor, sDevice device, std::stop_token stopToken, std::pmr::memory_resource* memRes = std::pmr::get_default_resource(), size_t chunkSize = DefaultReadAllChunkSize)
	{
		return executor.enqueueAsync([device = std::move(device), stopToken, memRes, chunkSize]() {
			auto resource = memoryResourceOrDefault(memRes);
			auto result = std::allocate_shared<ByteVector>(
				std::pmr::polymorphic_allocator<ByteVector>(resource)
				);
			readRemainingInto(requireDevice(device), *result, stopToken, chunkSize);
			return result;
		});
	}

	/**
	 * @brief Asynchronously reads a Device in allocated chunks and invokes a callback.
	 *
	 * The callback is called once per non-empty chunk with a span covering only
	 * the bytes read in that iteration. The span points into temporary storage
	 * and is valid only for the duration of the callback call.
	 *
	 * @tparam Callback Callable invoked as void(std::span<const std::byte>).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param chunkSize Size in bytes of the temporary chunk buffer.
	 * @param callback Callback invoked once per non-empty chunk.
	 * @return Executor-specific future-like object for completion of the read loop.
	 */
	template<typename Callback>
	requires detail::AsyncChunkCallback<Callback>
	[[nodiscard]] static auto readChunks(Executor& executor, Device& device, size_t chunkSize, Callback&& callback)
	{
		return readChunks(executor, device, std::stop_token(), chunkSize, std::pmr::get_default_resource(), std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads a Device in allocated chunks with custom temporary storage allocation.
	 *
	 * @tparam Callback Callable invoked as void(std::span<const std::byte>).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param chunkSize Size in bytes of the temporary chunk buffer.
	 * @param memRes Memory resource used by the temporary chunk buffer.
	 * @param callback Callback invoked once per non-empty chunk.
	 * @return Executor-specific future-like object for completion of the read loop.
	 */
	template<typename Callback>
	requires detail::AsyncChunkCallback<Callback>
	[[nodiscard]] static auto readChunks(Executor& executor, Device& device, size_t chunkSize, std::pmr::memory_resource* memRes, Callback&& callback)
	{
		return readChunks(executor, device, std::stop_token(), chunkSize, memRes, std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads a Device in allocated chunks with cooperative cancellation.
	 *
	 * @tparam Callback Callable invoked as void(std::span<const std::byte>).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param stopToken Cooperative cancellation token checked between reads.
	 * @param chunkSize Size in bytes of the temporary chunk buffer.
	 * @param callback Callback invoked once per non-empty chunk.
	 * @return Executor-specific future-like object for completion of the read loop.
	 * @throws AsyncOperationCanceled Through the future when @p stopToken is requested.
	 */
	template<typename Callback>
	requires detail::AsyncChunkCallback<Callback>
	[[nodiscard]] static auto readChunks(Executor& executor, Device& device, std::stop_token stopToken, size_t chunkSize, Callback&& callback)
	{
		return readChunks(executor, device, stopToken, chunkSize, std::pmr::get_default_resource(), std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads a Device in allocated chunks with custom allocation and cooperative cancellation.
	 *
	 * @tparam Callback Callable invoked as void(std::span<const std::byte>).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param stopToken Cooperative cancellation token checked between reads.
	 * @param chunkSize Size in bytes of the temporary chunk buffer.
	 * @param memRes Memory resource used by the temporary chunk buffer.
	 * @param callback Callback invoked once per non-empty chunk.
	 * @return Executor-specific future-like object for completion of the read loop.
	 * @throws AsyncOperationCanceled Through the future when @p stopToken is requested.
	 */
	template<typename Callback>
	requires detail::AsyncChunkCallback<Callback>
	[[nodiscard]] static auto readChunks(Executor& executor, Device& device, std::stop_token stopToken, size_t chunkSize, std::pmr::memory_resource* memRes, Callback&& callback)
	{
		return executor.enqueueAsync([&device, stopToken, chunkSize, memRes, cb = std::forward<Callback>(callback)]() mutable {
			readAllocatedChunks(device, stopToken, chunkSize, memRes, cb);
		});
	}

	/**
	 * @brief Asynchronously reads a shared Device in allocated chunks and invokes a callback.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @tparam Callback Callable invoked as void(std::span<const std::byte>).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param chunkSize Size in bytes of the temporary chunk buffer.
	 * @param callback Callback invoked once per non-empty chunk.
	 * @return Executor-specific future-like object for completion of the read loop.
	 */
	template<typename Callback>
	requires detail::AsyncChunkCallback<Callback>
	[[nodiscard]] static auto readChunks(Executor& executor, sDevice device, size_t chunkSize, Callback&& callback)
	{
		return readChunks(executor, std::move(device), std::stop_token(), chunkSize, std::pmr::get_default_resource(), std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads a shared Device in allocated chunks with custom temporary storage allocation.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @tparam Callback Callable invoked as void(std::span<const std::byte>).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param chunkSize Size in bytes of the temporary chunk buffer.
	 * @param memRes Memory resource used by the temporary chunk buffer.
	 * @param callback Callback invoked once per non-empty chunk.
	 * @return Executor-specific future-like object for completion of the read loop.
	 */
	template<typename Callback>
	requires detail::AsyncChunkCallback<Callback>
	[[nodiscard]] static auto readChunks(Executor& executor, sDevice device, size_t chunkSize, std::pmr::memory_resource* memRes, Callback&& callback)
	{
		return readChunks(executor, std::move(device), std::stop_token(), chunkSize, memRes, std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads a shared Device in allocated chunks with cooperative cancellation.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @tparam Callback Callable invoked as void(std::span<const std::byte>).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param stopToken Cooperative cancellation token checked between reads.
	 * @param chunkSize Size in bytes of the temporary chunk buffer.
	 * @param callback Callback invoked once per non-empty chunk.
	 * @return Executor-specific future-like object for completion of the read loop.
	 * @throws AsyncOperationCanceled Through the future when @p stopToken is requested.
	 */
	template<typename Callback>
	requires detail::AsyncChunkCallback<Callback>
	[[nodiscard]] static auto readChunks(Executor& executor, sDevice device, std::stop_token stopToken, size_t chunkSize, Callback&& callback)
	{
		return readChunks(executor, std::move(device), stopToken, chunkSize, std::pmr::get_default_resource(), std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads a shared Device in allocated chunks with custom allocation and cooperative cancellation.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @tparam Callback Callable invoked as void(std::span<const std::byte>).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param stopToken Cooperative cancellation token checked between reads.
	 * @param chunkSize Size in bytes of the temporary chunk buffer.
	 * @param memRes Memory resource used by the temporary chunk buffer.
	 * @param callback Callback invoked once per non-empty chunk.
	 * @return Executor-specific future-like object for completion of the read loop.
	 * @throws AsyncOperationCanceled Through the future when @p stopToken is requested.
	 */
	template<typename Callback>
	requires detail::AsyncChunkCallback<Callback>
	[[nodiscard]] static auto readChunks(Executor& executor, sDevice device, std::stop_token stopToken, size_t chunkSize, std::pmr::memory_resource* memRes, Callback&& callback)
	{
		return executor.enqueueAsync([device = std::move(device), stopToken, chunkSize, memRes, cb = std::forward<Callback>(callback)]() mutable {
			readAllocatedChunks(requireDevice(device), stopToken, chunkSize, memRes, cb);
		});
	}

	/**
	 * @brief Asynchronously reads a Device in chunks using caller-owned storage.
	 *
	 * The callback is called once per non-empty chunk with a span covering only
	 * the bytes read in that iteration. The span points into caller-owned
	 * scratch storage and is valid only for the duration of the callback call.
	 *
	 * @tparam Callback Callable invoked as void(std::span<const std::byte>).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param chunk Caller-owned temporary storage. Must remain valid until the task completes.
	 * @param chunkSize Size in bytes of @p chunk.
	 * @param callback Callback invoked once per non-empty chunk.
	 * @return Executor-specific future-like object for completion of the read loop.
	 */
	template<typename Callback>
	requires detail::AsyncChunkCallback<Callback>
	[[nodiscard]] static auto readChunks(Executor& executor, Device& device, void* chunk, size_t chunkSize, Callback&& callback)
	{
		return readChunks(executor, device, std::stop_token(), chunk, chunkSize, std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads a Device in caller-owned chunks with cooperative cancellation.
	 *
	 * @tparam Callback Callable invoked as void(std::span<const std::byte>).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param stopToken Cooperative cancellation token checked between reads.
	 * @param chunk Caller-owned temporary storage. Must remain valid until the task completes.
	 * @param chunkSize Size in bytes of @p chunk.
	 * @param callback Callback invoked once per non-empty chunk.
	 * @return Executor-specific future-like object for completion of the read loop.
	 * @throws AsyncOperationCanceled Through the future when @p stopToken is requested.
	 */
	template<typename Callback>
	requires detail::AsyncChunkCallback<Callback>
	[[nodiscard]] static auto readChunks(Executor& executor, Device& device, std::stop_token stopToken, void* chunk, size_t chunkSize, Callback&& callback)
	{
		return executor.enqueueAsync([&device, stopToken, chunk, chunkSize, cb = std::forward<Callback>(callback)]() mutable {
			readBorrowedChunks(device, stopToken, chunk, chunkSize, cb);
		});
	}

	/**
	 * @brief Asynchronously reads a shared Device in chunks using caller-owned storage.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @tparam Callback Callable invoked as void(std::span<const std::byte>).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param chunk Caller-owned temporary storage. Must remain valid until the task completes.
	 * @param chunkSize Size in bytes of @p chunk.
	 * @param callback Callback invoked once per non-empty chunk.
	 * @return Executor-specific future-like object for completion of the read loop.
	 */
	template<typename Callback>
	requires detail::AsyncChunkCallback<Callback>
	[[nodiscard]] static auto readChunks(Executor& executor, sDevice device, void* chunk, size_t chunkSize, Callback&& callback)
	{
		return readChunks(executor, std::move(device), std::stop_token(), chunk, chunkSize, std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads a shared Device in caller-owned chunks with cooperative cancellation.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @tparam Callback Callable invoked as void(std::span<const std::byte>).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param stopToken Cooperative cancellation token checked between reads.
	 * @param chunk Caller-owned temporary storage. Must remain valid until the task completes.
	 * @param chunkSize Size in bytes of @p chunk.
	 * @param callback Callback invoked once per non-empty chunk.
	 * @return Executor-specific future-like object for completion of the read loop.
	 * @throws AsyncOperationCanceled Through the future when @p stopToken is requested.
	 */
	template<typename Callback>
	requires detail::AsyncChunkCallback<Callback>
	[[nodiscard]] static auto readChunks(Executor& executor, sDevice device, std::stop_token stopToken, void* chunk, size_t chunkSize, Callback&& callback)
	{
		return executor.enqueueAsync([device = std::move(device), stopToken, chunk, chunkSize, cb = std::forward<Callback>(callback)]() mutable {
			readBorrowedChunks(requireDevice(device), stopToken, chunk, chunkSize, cb);
		});
	}

	/**
	 * @brief Asynchronously reads a Device in allocated chunks and invokes a completion callback.
	 *
	 * The per-chunk callback receives a span pointing into temporary storage and
	 * valid only for that callback call. The completion callback receives an
	 * AsyncResult<size_t> whose value is the total bytes delivered to per-chunk
	 * callbacks.
	 *
	 * @warning The completion callback runs in the executor context before the
	 * returned future is ready. It must not wait on that same future.
	 *
	 * @tparam ChunkCallback Callable invoked as void(std::span<const std::byte>).
	 * @tparam CompletionCallback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param chunkSize Size in bytes of the temporary chunk buffer.
	 * @param chunkCallback Callback invoked once per non-empty chunk.
	 * @param completionCallback Result-aware completion callback invoked once after success or failure.
	 * @return Executor-specific future-like object containing the total delivered byte count.
	 */
	template<typename ChunkCallback, typename CompletionCallback>
	requires detail::AsyncChunkCallback<ChunkCallback> && detail::AsyncResultCallback<CompletionCallback, size_t>
	[[nodiscard]] static auto readChunksThen(Executor& executor, Device& device, size_t chunkSize, ChunkCallback&& chunkCallback, CompletionCallback&& completionCallback)
	{
		return readChunksThen(executor, device, std::stop_token(), chunkSize, std::pmr::get_default_resource(), std::forward<ChunkCallback>(chunkCallback), std::forward<CompletionCallback>(completionCallback));
	}

	/**
	 * @brief Asynchronously reads a Device in allocated chunks with custom allocation and completion callback.
	 *
	 * @warning The completion callback runs in the executor context before the
	 * returned future is ready. It must not wait on that same future.
	 *
	 * @tparam ChunkCallback Callable invoked as void(std::span<const std::byte>).
	 * @tparam CompletionCallback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param chunkSize Size in bytes of the temporary chunk buffer.
	 * @param memRes Memory resource used by the temporary chunk buffer.
	 * @param chunkCallback Callback invoked once per non-empty chunk.
	 * @param completionCallback Result-aware completion callback invoked once after success or failure.
	 * @return Executor-specific future-like object containing the total delivered byte count.
	 */
	template<typename ChunkCallback, typename CompletionCallback>
	requires detail::AsyncChunkCallback<ChunkCallback> && detail::AsyncResultCallback<CompletionCallback, size_t>
	[[nodiscard]] static auto readChunksThen(Executor& executor, Device& device, size_t chunkSize, std::pmr::memory_resource* memRes, ChunkCallback&& chunkCallback, CompletionCallback&& completionCallback)
	{
		return readChunksThen(executor, device, std::stop_token(), chunkSize, memRes, std::forward<ChunkCallback>(chunkCallback), std::forward<CompletionCallback>(completionCallback));
	}

	/**
	 * @brief Asynchronously reads a Device in allocated chunks with cancellation and completion callback.
	 *
	 * @warning The completion callback runs in the executor context before the
	 * returned future is ready. It must not wait on that same future.
	 *
	 * @tparam ChunkCallback Callable invoked as void(std::span<const std::byte>).
	 * @tparam CompletionCallback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param stopToken Cooperative cancellation token checked between reads.
	 * @param chunkSize Size in bytes of the temporary chunk buffer.
	 * @param chunkCallback Callback invoked once per non-empty chunk.
	 * @param completionCallback Result-aware completion callback invoked once after success or failure.
	 * @return Executor-specific future-like object containing the total delivered byte count.
	 * @throws AsyncOperationCanceled Through the future and completion callback when @p stopToken is requested.
	 */
	template<typename ChunkCallback, typename CompletionCallback>
	requires detail::AsyncChunkCallback<ChunkCallback> && detail::AsyncResultCallback<CompletionCallback, size_t>
	[[nodiscard]] static auto readChunksThen(Executor& executor, Device& device, std::stop_token stopToken, size_t chunkSize, ChunkCallback&& chunkCallback, CompletionCallback&& completionCallback)
	{
		return readChunksThen(executor, device, stopToken, chunkSize, std::pmr::get_default_resource(), std::forward<ChunkCallback>(chunkCallback), std::forward<CompletionCallback>(completionCallback));
	}

	/**
	 * @brief Asynchronously reads a Device in allocated chunks with custom allocation, cancellation, and completion callback.
	 *
	 * @warning The completion callback runs in the executor context before the
	 * returned future is ready. It must not wait on that same future.
	 *
	 * @tparam ChunkCallback Callable invoked as void(std::span<const std::byte>).
	 * @tparam CompletionCallback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param stopToken Cooperative cancellation token checked between reads.
	 * @param chunkSize Size in bytes of the temporary chunk buffer.
	 * @param memRes Memory resource used by the temporary chunk buffer.
	 * @param chunkCallback Callback invoked once per non-empty chunk.
	 * @param completionCallback Result-aware completion callback invoked once after success or failure.
	 * @return Executor-specific future-like object containing the total delivered byte count.
	 * @throws AsyncOperationCanceled Through the future and completion callback when @p stopToken is requested.
	 */
	template<typename ChunkCallback, typename CompletionCallback>
	requires detail::AsyncChunkCallback<ChunkCallback> && detail::AsyncResultCallback<CompletionCallback, size_t>
	[[nodiscard]] static auto readChunksThen(Executor& executor, Device& device, std::stop_token stopToken, size_t chunkSize, std::pmr::memory_resource* memRes, ChunkCallback&& chunkCallback, CompletionCallback&& completionCallback)
	{
		return executor.enqueueAsync([&device, stopToken, chunkSize, memRes,
									  chunkCb = std::forward<ChunkCallback>(chunkCallback),
									  completionCb = std::forward<CompletionCallback>(completionCallback)]() mutable {
			return runThen<size_t>([&device, stopToken, chunkSize, memRes, &chunkCb]() {
				return readAllocatedChunks(device, stopToken, chunkSize, memRes, chunkCb);
			}, completionCb);
		});
	}

	/**
	 * @brief Asynchronously reads a shared Device in allocated chunks and invokes a completion callback.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 * The per-chunk span points into temporary storage and is valid only for
	 * that callback call.
	 *
	 * @warning The completion callback runs in the executor context before the
	 * returned future is ready. It must not wait on that same future.
	 *
	 * @tparam ChunkCallback Callable invoked as void(std::span<const std::byte>).
	 * @tparam CompletionCallback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param chunkSize Size in bytes of the temporary chunk buffer.
	 * @param chunkCallback Callback invoked once per non-empty chunk.
	 * @param completionCallback Result-aware completion callback invoked once after success or failure.
	 * @return Executor-specific future-like object containing the total delivered byte count.
	 */
	template<typename ChunkCallback, typename CompletionCallback>
	requires detail::AsyncChunkCallback<ChunkCallback> && detail::AsyncResultCallback<CompletionCallback, size_t>
	[[nodiscard]] static auto readChunksThen(Executor& executor, sDevice device, size_t chunkSize, ChunkCallback&& chunkCallback, CompletionCallback&& completionCallback)
	{
		return readChunksThen(executor, std::move(device), std::stop_token(), chunkSize, std::pmr::get_default_resource(), std::forward<ChunkCallback>(chunkCallback), std::forward<CompletionCallback>(completionCallback));
	}

	/**
	 * @brief Asynchronously reads a shared Device in allocated chunks with custom allocation and completion callback.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @warning The completion callback runs in the executor context before the
	 * returned future is ready. It must not wait on that same future.
	 *
	 * @tparam ChunkCallback Callable invoked as void(std::span<const std::byte>).
	 * @tparam CompletionCallback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param chunkSize Size in bytes of the temporary chunk buffer.
	 * @param memRes Memory resource used by the temporary chunk buffer.
	 * @param chunkCallback Callback invoked once per non-empty chunk.
	 * @param completionCallback Result-aware completion callback invoked once after success or failure.
	 * @return Executor-specific future-like object containing the total delivered byte count.
	 */
	template<typename ChunkCallback, typename CompletionCallback>
	requires detail::AsyncChunkCallback<ChunkCallback> && detail::AsyncResultCallback<CompletionCallback, size_t>
	[[nodiscard]] static auto readChunksThen(Executor& executor, sDevice device, size_t chunkSize, std::pmr::memory_resource* memRes, ChunkCallback&& chunkCallback, CompletionCallback&& completionCallback)
	{
		return readChunksThen(executor, std::move(device), std::stop_token(), chunkSize, memRes, std::forward<ChunkCallback>(chunkCallback), std::forward<CompletionCallback>(completionCallback));
	}

	/**
	 * @brief Asynchronously reads a shared Device in allocated chunks with cancellation and completion callback.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @warning The completion callback runs in the executor context before the
	 * returned future is ready. It must not wait on that same future.
	 *
	 * @tparam ChunkCallback Callable invoked as void(std::span<const std::byte>).
	 * @tparam CompletionCallback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param stopToken Cooperative cancellation token checked between reads.
	 * @param chunkSize Size in bytes of the temporary chunk buffer.
	 * @param chunkCallback Callback invoked once per non-empty chunk.
	 * @param completionCallback Result-aware completion callback invoked once after success or failure.
	 * @return Executor-specific future-like object containing the total delivered byte count.
	 * @throws AsyncOperationCanceled Through the future and completion callback when @p stopToken is requested.
	 */
	template<typename ChunkCallback, typename CompletionCallback>
	requires detail::AsyncChunkCallback<ChunkCallback> && detail::AsyncResultCallback<CompletionCallback, size_t>
	[[nodiscard]] static auto readChunksThen(Executor& executor, sDevice device, std::stop_token stopToken, size_t chunkSize, ChunkCallback&& chunkCallback, CompletionCallback&& completionCallback)
	{
		return readChunksThen(executor, std::move(device), stopToken, chunkSize, std::pmr::get_default_resource(), std::forward<ChunkCallback>(chunkCallback), std::forward<CompletionCallback>(completionCallback));
	}

	/**
	 * @brief Asynchronously reads a shared Device in allocated chunks with custom allocation, cancellation, and completion callback.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @warning The completion callback runs in the executor context before the
	 * returned future is ready. It must not wait on that same future.
	 *
	 * @tparam ChunkCallback Callable invoked as void(std::span<const std::byte>).
	 * @tparam CompletionCallback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param stopToken Cooperative cancellation token checked between reads.
	 * @param chunkSize Size in bytes of the temporary chunk buffer.
	 * @param memRes Memory resource used by the temporary chunk buffer.
	 * @param chunkCallback Callback invoked once per non-empty chunk.
	 * @param completionCallback Result-aware completion callback invoked once after success or failure.
	 * @return Executor-specific future-like object containing the total delivered byte count.
	 * @throws AsyncOperationCanceled Through the future and completion callback when @p stopToken is requested.
	 */
	template<typename ChunkCallback, typename CompletionCallback>
	requires detail::AsyncChunkCallback<ChunkCallback> && detail::AsyncResultCallback<CompletionCallback, size_t>
	[[nodiscard]] static auto readChunksThen(Executor& executor, sDevice device, std::stop_token stopToken, size_t chunkSize, std::pmr::memory_resource* memRes, ChunkCallback&& chunkCallback, CompletionCallback&& completionCallback)
	{
		return executor.enqueueAsync([device = std::move(device), stopToken, chunkSize, memRes,
									  chunkCb = std::forward<ChunkCallback>(chunkCallback),
									  completionCb = std::forward<CompletionCallback>(completionCallback)]() mutable {
			return runThen<size_t>([&device, stopToken, chunkSize, memRes, &chunkCb]() {
				return readAllocatedChunks(requireDevice(device), stopToken, chunkSize, memRes, chunkCb);
			}, completionCb);
		});
	}

	/**
	 * @brief Asynchronously reads a Device in caller-owned chunks and invokes a completion callback.
	 *
	 * The per-chunk span points into @p chunk and is valid only for that
	 * callback call.
	 *
	 * @warning The completion callback runs in the executor context before the
	 * returned future is ready. It must not wait on that same future.
	 *
	 * @tparam ChunkCallback Callable invoked as void(std::span<const std::byte>).
	 * @tparam CompletionCallback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param chunk Caller-owned temporary storage. Must remain valid until the task completes.
	 * @param chunkSize Size in bytes of @p chunk.
	 * @param chunkCallback Callback invoked once per non-empty chunk.
	 * @param completionCallback Result-aware completion callback invoked once after success or failure.
	 * @return Executor-specific future-like object containing the total delivered byte count.
	 */
	template<typename ChunkCallback, typename CompletionCallback>
	requires detail::AsyncChunkCallback<ChunkCallback> && detail::AsyncResultCallback<CompletionCallback, size_t>
	[[nodiscard]] static auto readChunksThen(Executor& executor, Device& device, void* chunk, size_t chunkSize, ChunkCallback&& chunkCallback, CompletionCallback&& completionCallback)
	{
		return readChunksThen(executor, device, std::stop_token(), chunk, chunkSize, std::forward<ChunkCallback>(chunkCallback), std::forward<CompletionCallback>(completionCallback));
	}

	/**
	 * @brief Asynchronously reads a Device in caller-owned chunks with cancellation and completion callback.
	 *
	 * @warning The completion callback runs in the executor context before the
	 * returned future is ready. It must not wait on that same future.
	 *
	 * @tparam ChunkCallback Callable invoked as void(std::span<const std::byte>).
	 * @tparam CompletionCallback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param stopToken Cooperative cancellation token checked between reads.
	 * @param chunk Caller-owned temporary storage. Must remain valid until the task completes.
	 * @param chunkSize Size in bytes of @p chunk.
	 * @param chunkCallback Callback invoked once per non-empty chunk.
	 * @param completionCallback Result-aware completion callback invoked once after success or failure.
	 * @return Executor-specific future-like object containing the total delivered byte count.
	 * @throws AsyncOperationCanceled Through the future and completion callback when @p stopToken is requested.
	 */
	template<typename ChunkCallback, typename CompletionCallback>
	requires detail::AsyncChunkCallback<ChunkCallback> && detail::AsyncResultCallback<CompletionCallback, size_t>
	[[nodiscard]] static auto readChunksThen(Executor& executor, Device& device, std::stop_token stopToken, void* chunk, size_t chunkSize, ChunkCallback&& chunkCallback, CompletionCallback&& completionCallback)
	{
		return executor.enqueueAsync([&device, stopToken, chunk, chunkSize,
									  chunkCb = std::forward<ChunkCallback>(chunkCallback),
									  completionCb = std::forward<CompletionCallback>(completionCallback)]() mutable {
			return runThen<size_t>([&device, stopToken, chunk, chunkSize, &chunkCb]() {
				return readBorrowedChunks(device, stopToken, chunk, chunkSize, chunkCb);
			}, completionCb);
		});
	}

	/**
	 * @brief Asynchronously reads a shared Device in caller-owned chunks and invokes a completion callback.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 * The per-chunk span points into @p chunk and is valid only for that
	 * callback call.
	 *
	 * @warning The completion callback runs in the executor context before the
	 * returned future is ready. It must not wait on that same future.
	 *
	 * @tparam ChunkCallback Callable invoked as void(std::span<const std::byte>).
	 * @tparam CompletionCallback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param chunk Caller-owned temporary storage. Must remain valid until the task completes.
	 * @param chunkSize Size in bytes of @p chunk.
	 * @param chunkCallback Callback invoked once per non-empty chunk.
	 * @param completionCallback Result-aware completion callback invoked once after success or failure.
	 * @return Executor-specific future-like object containing the total delivered byte count.
	 */
	template<typename ChunkCallback, typename CompletionCallback>
	requires detail::AsyncChunkCallback<ChunkCallback> && detail::AsyncResultCallback<CompletionCallback, size_t>
	[[nodiscard]] static auto readChunksThen(Executor& executor, sDevice device, void* chunk, size_t chunkSize, ChunkCallback&& chunkCallback, CompletionCallback&& completionCallback)
	{
		return readChunksThen(executor, std::move(device), std::stop_token(), chunk, chunkSize, std::forward<ChunkCallback>(chunkCallback), std::forward<CompletionCallback>(completionCallback));
	}

	/**
	 * @brief Asynchronously reads a shared Device in caller-owned chunks with cancellation and completion callback.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @warning The completion callback runs in the executor context before the
	 * returned future is ready. It must not wait on that same future.
	 *
	 * @tparam ChunkCallback Callable invoked as void(std::span<const std::byte>).
	 * @tparam CompletionCallback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking chunk-read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param stopToken Cooperative cancellation token checked between reads.
	 * @param chunk Caller-owned temporary storage. Must remain valid until the task completes.
	 * @param chunkSize Size in bytes of @p chunk.
	 * @param chunkCallback Callback invoked once per non-empty chunk.
	 * @param completionCallback Result-aware completion callback invoked once after success or failure.
	 * @return Executor-specific future-like object containing the total delivered byte count.
	 * @throws AsyncOperationCanceled Through the future and completion callback when @p stopToken is requested.
	 */
	template<typename ChunkCallback, typename CompletionCallback>
	requires detail::AsyncChunkCallback<ChunkCallback> && detail::AsyncResultCallback<CompletionCallback, size_t>
	[[nodiscard]] static auto readChunksThen(Executor& executor, sDevice device, std::stop_token stopToken, void* chunk, size_t chunkSize, ChunkCallback&& chunkCallback, CompletionCallback&& completionCallback)
	{
		return executor.enqueueAsync([device = std::move(device), stopToken, chunk, chunkSize,
									  chunkCb = std::forward<ChunkCallback>(chunkCallback),
									  completionCb = std::forward<CompletionCallback>(completionCallback)]() mutable {
			return runThen<size_t>([&device, stopToken, chunk, chunkSize, &chunkCb]() {
				return readBorrowedChunks(requireDevice(device), stopToken, chunk, chunkSize, chunkCb);
			}, completionCb);
		});
	}

	/**
	 * @brief Asynchronously reads elements from a Device and invokes a result-aware callback.
	 *
	 * The callback runs in the executor context before the future is made ready.
	 * Operation failures are delivered to the callback as AsyncResult errors and
	 * are also rethrown by the returned future.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking Device::read call.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param buffer Destination buffer. Must remain valid until the task completes.
	 * @param size Size of each element to read.
	 * @param count Maximum number of elements to read.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing Device::read's size_t result.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, size_t>
	[[nodiscard]] static auto readThen(Executor& executor, Device& device, void* buffer, size_t size, size_t count, Callback&& callback)
	{
		return executor.enqueueAsync([&device, buffer, size, count, cb = std::forward<Callback>(callback)]() mutable {
			return runThen<size_t>([&device, buffer, size, count]() {
				return device.read(buffer, size, count);
			}, cb);
		});
	}

	/**
	 * @brief Asynchronously reads elements from a shared Device and invokes a result-aware callback.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking Device::read call.
	 * @param device Shared Device to read from. Must not be null.
	 * @param buffer Destination buffer. Must remain valid until the task completes.
	 * @param size Size of each element to read.
	 * @param count Maximum number of elements to read.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing Device::read's size_t result.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, size_t>
	[[nodiscard]] static auto readThen(Executor& executor, sDevice device, void* buffer, size_t size, size_t count, Callback&& callback)
	{
		return executor.enqueueAsync([device = std::move(device), buffer, size, count, cb = std::forward<Callback>(callback)]() mutable {
			return runThen<size_t>([&device, buffer, size, count]() {
				return requireDevice(device).read(buffer, size, count);
			}, cb);
		});
	}

	/**
	 * @brief Asynchronously writes elements to a Device and invokes a result-aware callback.
	 *
	 * Operation failures are delivered to the callback as AsyncResult errors and
	 * are also rethrown by the returned future.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking Device::write call.
	 * @param device Device to write to. Must remain valid until the task completes.
	 * @param buffer Source buffer. Must remain valid until the task completes.
	 * @param size Size of each element to write.
	 * @param count Maximum number of elements to write.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing Device::write's size_t result.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, size_t>
	[[nodiscard]] static auto writeThen(Executor& executor, Device& device, const void* buffer, size_t size, size_t count, Callback&& callback)
	{
		return executor.enqueueAsync([&device, buffer, size, count, cb = std::forward<Callback>(callback)]() mutable {
			return runThen<size_t>([&device, buffer, size, count]() {
				return device.write(buffer, size, count);
			}, cb);
		});
	}

	/**
	 * @brief Asynchronously writes elements to a shared Device and invokes a result-aware callback.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking Device::write call.
	 * @param device Shared Device to write to. Must not be null.
	 * @param buffer Source buffer. Must remain valid until the task completes.
	 * @param size Size of each element to write.
	 * @param count Maximum number of elements to write.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing Device::write's size_t result.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, size_t>
	[[nodiscard]] static auto writeThen(Executor& executor, sDevice device, const void* buffer, size_t size, size_t count, Callback&& callback)
	{
		return executor.enqueueAsync([device = std::move(device), buffer, size, count, cb = std::forward<Callback>(callback)]() mutable {
			return runThen<size_t>([&device, buffer, size, count]() {
				return requireDevice(device).write(buffer, size, count);
			}, cb);
		});
	}

	/**
	 * @brief Asynchronously seeks within a Device and invokes a result-aware callback.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<int>&).
	 * @param executor Executor used to run the blocking Device::seek call.
	 * @param device Device to seek. Must remain valid until the task completes.
	 * @param offset Byte offset relative to @p whence.
	 * @param whence Origin used to interpret @p offset.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing Device::seek's int result.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, int>
	[[nodiscard]] static auto seekThen(Executor& executor, Device& device, long offset, SeekOrigin whence, Callback&& callback)
	{
		return executor.enqueueAsync([&device, offset, whence, cb = std::forward<Callback>(callback)]() mutable {
			return runThen<int>([&device, offset, whence]() {
				return device.seek(offset, whence);
			}, cb);
		});
	}

	/**
	 * @brief Asynchronously seeks within a shared Device and invokes a result-aware callback.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<int>&).
	 * @param executor Executor used to run the blocking Device::seek call.
	 * @param device Shared Device to seek. Must not be null.
	 * @param offset Byte offset relative to @p whence.
	 * @param whence Origin used to interpret @p offset.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing Device::seek's int result.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, int>
	[[nodiscard]] static auto seekThen(Executor& executor, sDevice device, long offset, SeekOrigin whence, Callback&& callback)
	{
		return executor.enqueueAsync([device = std::move(device), offset, whence, cb = std::forward<Callback>(callback)]() mutable {
			return runThen<int>([&device, offset, whence]() {
				return requireDevice(device).seek(offset, whence);
			}, cb);
		});
	}

	/**
	 * @brief Asynchronously queries the Device position and invokes a result-aware callback.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<long>&).
	 * @param executor Executor used to run the blocking Device::tell call.
	 * @param device Device to query. Must remain valid until the task completes.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing Device::tell's long result.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, long>
	[[nodiscard]] static auto tellThen(Executor& executor, Device& device, Callback&& callback)
	{
		return executor.enqueueAsync([&device, cb = std::forward<Callback>(callback)]() mutable {
			return runThen<long>([&device]() {
				return device.tell();
			}, cb);
		});
	}

	/**
	 * @brief Asynchronously queries the shared Device position and invokes a result-aware callback.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<long>&).
	 * @param executor Executor used to run the blocking Device::tell call.
	 * @param device Shared Device to query. Must not be null.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing Device::tell's long result.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, long>
	[[nodiscard]] static auto tellThen(Executor& executor, sDevice device, Callback&& callback)
	{
		return executor.enqueueAsync([device = std::move(device), cb = std::forward<Callback>(callback)]() mutable {
			return runThen<long>([&device]() {
				return requireDevice(device).tell();
			}, cb);
		});
	}

	/**
	 * @brief Asynchronously queries Device size and invokes a result-aware callback.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking Device::size call.
	 * @param device Device to query. Must remain valid until the task completes.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing Device::size's size_t result.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, size_t>
	[[nodiscard]] static auto sizeThen(Executor& executor, Device& device, Callback&& callback)
	{
		return executor.enqueueAsync([&device, cb = std::forward<Callback>(callback)]() mutable {
			return runThen<size_t>([&device]() {
				return device.size();
			}, cb);
		});
	}

	/**
	 * @brief Asynchronously queries shared Device size and invokes a result-aware callback.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<size_t>&).
	 * @param executor Executor used to run the blocking Device::size call.
	 * @param device Shared Device to query. Must not be null.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing Device::size's size_t result.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, size_t>
	[[nodiscard]] static auto sizeThen(Executor& executor, sDevice device, Callback&& callback)
	{
		return executor.enqueueAsync([device = std::move(device), cb = std::forward<Callback>(callback)]() mutable {
			return runThen<size_t>([&device]() {
				return requireDevice(device).size();
			}, cb);
		});
	}

	/**
	 * @brief Asynchronously checks Device EOF and invokes a result-aware callback.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<bool>&).
	 * @param executor Executor used to run the blocking Device::eof call.
	 * @param device Device to query. Must remain valid until the task completes.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing Device::eof's bool result.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, bool>
	[[nodiscard]] static auto eofThen(Executor& executor, Device& device, Callback&& callback)
	{
		return executor.enqueueAsync([&device, cb = std::forward<Callback>(callback)]() mutable {
			return runThen<bool>([&device]() {
				return device.eof();
			}, cb);
		});
	}

	/**
	 * @brief Asynchronously checks shared Device EOF and invokes a result-aware callback.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<bool>&).
	 * @param executor Executor used to run the blocking Device::eof call.
	 * @param device Shared Device to query. Must not be null.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing Device::eof's bool result.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, bool>
	[[nodiscard]] static auto eofThen(Executor& executor, sDevice device, Callback&& callback)
	{
		return executor.enqueueAsync([device = std::move(device), cb = std::forward<Callback>(callback)]() mutable {
			return runThen<bool>([&device]() {
				return requireDevice(device).eof();
			}, cb);
		});
	}

	/**
	 * @brief Asynchronously flushes a Device and invokes a result-aware callback.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<bool>&).
	 * @param executor Executor used to run the blocking Device::flush call.
	 * @param device Device to flush. Must remain valid until the task completes.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing Device::flush's bool result.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, bool>
	[[nodiscard]] static auto flushThen(Executor& executor, Device& device, Callback&& callback)
	{
		return executor.enqueueAsync([&device, cb = std::forward<Callback>(callback)]() mutable {
			return runThen<bool>([&device]() {
				return device.flush();
			}, cb);
		});
	}

	/**
	 * @brief Asynchronously flushes a shared Device and invokes a result-aware callback.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<bool>&).
	 * @param executor Executor used to run the blocking Device::flush call.
	 * @param device Shared Device to flush. Must not be null.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing Device::flush's bool result.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, bool>
	[[nodiscard]] static auto flushThen(Executor& executor, sDevice device, Callback&& callback)
	{
		return executor.enqueueAsync([device = std::move(device), cb = std::forward<Callback>(callback)]() mutable {
			return runThen<bool>([&device]() {
				return requireDevice(device).flush();
			}, cb);
		});
	}

	/**
	 * @brief Asynchronously checks Device validity and invokes a result-aware callback.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<bool>&).
	 * @param executor Executor used to run the blocking Device::isValid call.
	 * @param device Device to query. Must remain valid until the task completes.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing Device::isValid's bool result.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, bool>
	[[nodiscard]] static auto isValidThen(Executor& executor, Device& device, Callback&& callback)
	{
		return executor.enqueueAsync([&device, cb = std::forward<Callback>(callback)]() mutable {
			return runThen<bool>([&device]() {
				return device.isValid();
			}, cb);
		});
	}

	/**
	 * @brief Asynchronously checks shared Device validity and invokes a result-aware callback.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<bool>&).
	 * @param executor Executor used to run the blocking Device::isValid call.
	 * @param device Shared Device to query. Must not be null.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing Device::isValid's bool result.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, bool>
	[[nodiscard]] static auto isValidThen(Executor& executor, sDevice device, Callback&& callback)
	{
		return executor.enqueueAsync([device = std::move(device), cb = std::forward<Callback>(callback)]() mutable {
			return runThen<bool>([&device]() {
				return requireDevice(device).isValid();
			}, cb);
		});
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a Device and invokes a result-aware callback.
	 *
	 * The callback receives a span result whose span is valid only for the
	 * duration of the callback call. The returned future still contains the
	 * owning ByteVector.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<std::span<const std::byte>>&).
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing the owning ByteVector.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, std::span<const std::byte> >
	[[nodiscard]] static auto readAllThen(Executor& executor, Device& device, Callback&& callback)
	{
		return readAllThen(executor, device, std::stop_token(), std::pmr::get_default_resource(), DefaultReadAllChunkSize, std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a Device with custom allocation and invokes a result-aware callback.
	 *
	 * The callback span is valid only for the duration of the callback call.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<std::span<const std::byte>>&).
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param memRes Memory resource used by the returned ByteVector.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing the owning ByteVector.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, std::span<const std::byte> >
	[[nodiscard]] static auto readAllThen(Executor& executor, Device& device, std::pmr::memory_resource* memRes, Callback&& callback)
	{
		return readAllThen(executor, device, std::stop_token(), memRes, DefaultReadAllChunkSize, std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a Device with custom allocation, chunk size, and callback.
	 *
	 * The callback receives a temporary span result that is valid only for the
	 * duration of the callback call; copy bytes inside the callback if retaining
	 * them. Device::tell and Device::size are treated as a byte-count promise.
	 *
	 * @warning The callback runs in the executor context before the returned
	 * future is ready. It must not wait on that same future.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<std::span<const std::byte>>&).
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param memRes Memory resource used by the returned ByteVector.
	 * @param chunkSize Maximum byte count requested from each Device::read call.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing the owning ByteVector.
	 * @throws std::invalid_argument Through the future and callback result if @p chunkSize is 0.
	 * @throws std::runtime_error Through the future and callback result if the Device is invalid or ends early.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, std::span<const std::byte> >
	[[nodiscard]] static auto readAllThen(Executor& executor, Device& device, std::pmr::memory_resource* memRes, size_t chunkSize, Callback&& callback)
	{
		return readAllThen(executor, device, std::stop_token(), memRes, chunkSize, std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a Device with cooperative cancellation and invokes a result-aware callback.
	 *
	 * The callback span is valid only for the duration of the callback call.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<std::span<const std::byte>>&).
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param stopToken Cooperative cancellation token.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing the owning ByteVector.
	 * @throws AsyncOperationCanceled Through the future and callback result when @p stopToken is requested.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, std::span<const std::byte> >
	[[nodiscard]] static auto readAllThen(Executor& executor, Device& device, std::stop_token stopToken, Callback&& callback)
	{
		return readAllThen(executor, device, stopToken, std::pmr::get_default_resource(), DefaultReadAllChunkSize, std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a Device with custom allocation and cooperative cancellation.
	 *
	 * The callback span is valid only for the duration of the callback call.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<std::span<const std::byte>>&).
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param stopToken Cooperative cancellation token.
	 * @param memRes Memory resource used by the returned ByteVector.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing the owning ByteVector.
	 * @throws AsyncOperationCanceled Through the future and callback result when @p stopToken is requested.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, std::span<const std::byte> >
	[[nodiscard]] static auto readAllThen(Executor& executor, Device& device, std::stop_token stopToken, std::pmr::memory_resource* memRes, Callback&& callback)
	{
		return readAllThen(executor, device, stopToken, memRes, DefaultReadAllChunkSize, std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a Device with custom allocation, chunk size, cancellation, and callback.
	 *
	 * The callback receives a temporary span result that is valid only for the
	 * duration of the callback call; copy bytes inside the callback if retaining
	 * them. Device::tell and Device::size are treated as a byte-count promise.
	 *
	 * @warning The callback runs in the executor context before the returned
	 * future is ready. It must not wait on that same future.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<std::span<const std::byte>>&).
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param stopToken Cooperative cancellation token.
	 * @param memRes Memory resource used by the returned ByteVector.
	 * @param chunkSize Maximum byte count requested from each Device::read call.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing the owning ByteVector.
	 * @throws AsyncOperationCanceled Through the future and callback result when @p stopToken is requested.
	 * @throws std::invalid_argument Through the future and callback result if @p chunkSize is 0.
	 * @throws std::runtime_error Through the future and callback result if the Device is invalid or ends early.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, std::span<const std::byte> >
	[[nodiscard]] static auto readAllThen(Executor& executor, Device& device, std::stop_token stopToken, std::pmr::memory_resource* memRes, size_t chunkSize, Callback&& callback)
	{
		return executor.enqueueAsync([&device, stopToken, memRes, chunkSize, cb = std::forward<Callback>(callback)]() mutable {
			return runReadAllThen([&device, stopToken, memRes, chunkSize]() {
				ByteVector result(memoryResourceOrDefault(memRes));
				readRemainingInto(device, result, stopToken, chunkSize);
				return result;
			}, cb);
		});
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a shared Device and invokes a result-aware callback.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 * The callback span is valid only for the duration of the callback call.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<std::span<const std::byte>>&).
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing the owning ByteVector.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, std::span<const std::byte> >
	[[nodiscard]] static auto readAllThen(Executor& executor, sDevice device, Callback&& callback)
	{
		return readAllThen(executor, std::move(device), std::stop_token(), std::pmr::get_default_resource(), DefaultReadAllChunkSize, std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a shared Device with custom allocation and invokes a result-aware callback.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 * The callback span is valid only for the duration of the callback call.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<std::span<const std::byte>>&).
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param memRes Memory resource used by the returned ByteVector.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing the owning ByteVector.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, std::span<const std::byte> >
	[[nodiscard]] static auto readAllThen(Executor& executor, sDevice device, std::pmr::memory_resource* memRes, Callback&& callback)
	{
		return readAllThen(executor, std::move(device), std::stop_token(), memRes, DefaultReadAllChunkSize, std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a shared Device with custom allocation, chunk size, and callback.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 * The callback receives a temporary span result that is valid only for the
	 * duration of the callback call; copy bytes inside the callback if retaining
	 * them. Device::tell and Device::size are treated as a byte-count promise.
	 *
	 * @warning The callback runs in the executor context before the returned
	 * future is ready. It must not wait on that same future.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<std::span<const std::byte>>&).
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param memRes Memory resource used by the returned ByteVector.
	 * @param chunkSize Maximum byte count requested from each Device::read call.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing the owning ByteVector.
	 * @throws std::invalid_argument Through the future and callback result if @p chunkSize is 0.
	 * @throws std::runtime_error Through the future and callback result if the Device is invalid or ends early.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, std::span<const std::byte> >
	[[nodiscard]] static auto readAllThen(Executor& executor, sDevice device, std::pmr::memory_resource* memRes, size_t chunkSize, Callback&& callback)
	{
		return readAllThen(executor, std::move(device), std::stop_token(), memRes, chunkSize, std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a shared Device with cooperative cancellation and invokes a result-aware callback.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 * The callback span is valid only for the duration of the callback call.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<std::span<const std::byte>>&).
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param stopToken Cooperative cancellation token.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing the owning ByteVector.
	 * @throws AsyncOperationCanceled Through the future and callback result when @p stopToken is requested.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, std::span<const std::byte> >
	[[nodiscard]] static auto readAllThen(Executor& executor, sDevice device, std::stop_token stopToken, Callback&& callback)
	{
		return readAllThen(executor, std::move(device), stopToken, std::pmr::get_default_resource(), DefaultReadAllChunkSize, std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a shared Device with custom allocation and cooperative cancellation.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 * The callback span is valid only for the duration of the callback call.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<std::span<const std::byte>>&).
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param stopToken Cooperative cancellation token.
	 * @param memRes Memory resource used by the returned ByteVector.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing the owning ByteVector.
	 * @throws AsyncOperationCanceled Through the future and callback result when @p stopToken is requested.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, std::span<const std::byte> >
	[[nodiscard]] static auto readAllThen(Executor& executor, sDevice device, std::stop_token stopToken, std::pmr::memory_resource* memRes, Callback&& callback)
	{
		return readAllThen(executor, std::move(device), stopToken, memRes, DefaultReadAllChunkSize, std::forward<Callback>(callback));
	}

	/**
	 * @brief Asynchronously reads all remaining bytes from a shared Device with custom allocation, chunk size, cancellation, and callback.
	 *
	 * Capturing the shared pointer keeps the Device alive until the task exits.
	 * The callback receives a temporary span result that is valid only for the
	 * duration of the callback call; copy bytes inside the callback if retaining
	 * them. Device::tell and Device::size are treated as a byte-count promise.
	 *
	 * @warning The callback runs in the executor context before the returned
	 * future is ready. It must not wait on that same future.
	 *
	 * @tparam Callback Callable invoked as void(const AsyncResult<std::span<const std::byte>>&).
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Shared Device to read from. Must not be null.
	 * @param stopToken Cooperative cancellation token.
	 * @param memRes Memory resource used by the returned ByteVector.
	 * @param chunkSize Maximum byte count requested from each Device::read call.
	 * @param callback Result-aware completion callback.
	 * @return Executor-specific future-like object containing the owning ByteVector.
	 * @throws AsyncOperationCanceled Through the future and callback result when @p stopToken is requested.
	 * @throws std::invalid_argument Through the future and callback result if @p chunkSize is 0.
	 * @throws std::runtime_error Through the future and callback result if the Device is invalid or ends early.
	 */
	template<typename Callback>
	requires detail::AsyncResultCallback<Callback, std::span<const std::byte> >
	[[nodiscard]] static auto readAllThen(Executor& executor, sDevice device, std::stop_token stopToken, std::pmr::memory_resource* memRes, size_t chunkSize, Callback&& callback)
	{
		return executor.enqueueAsync([device = std::move(device), stopToken, memRes, chunkSize, cb = std::forward<Callback>(callback)]() mutable {
			return runReadAllThen([&device, stopToken, memRes, chunkSize]() {
				ByteVector result(memoryResourceOrDefault(memRes));
				readRemainingInto(requireDevice(device), result, stopToken, chunkSize);
				return result;
			}, cb);
		});
	}
};

} // namespace Io
} // namespace Elv
#endif // ELVIOASYNC_HPP
