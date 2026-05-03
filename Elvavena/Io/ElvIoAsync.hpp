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
	AsyncOperationCanceled();
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

template<typename Callback, typename T>
concept AsyncResultCallback =
	std::invocable<Callback&, const AsyncResult<T>&> &&
	std::same_as<std::invoke_result_t<Callback&, const AsyncResult<T>&>, void>;

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
 *
 * @tparam Executor Executor type satisfying Util::AsyncExecutor.
 */
template<Util::AsyncExecutor Executor> struct Async {
private:
	static constexpr size_t ReadAllChunkSize = 64 * 1024;

	static std::pmr::memory_resource* memoryResourceOrDefault(std::pmr::memory_resource* memRes)
	{
		return memRes ? memRes : std::pmr::get_default_resource();
	}

	static Device& requireDevice(const sDevice& device)
	{
		if (!device)
			throw std::runtime_error("Async I/O operation received a null Device shared_ptr");
		return *device;
	}

	static void throwIfStopped(std::stop_token stopToken)
	{
		if (stopToken.stop_requested())
			throw AsyncOperationCanceled();
	}

	static void validateReadable(Device& device)
	{
		if (!device.isValid())
			throw std::runtime_error("Async read attempted on an invalid Device");
	}

	static size_t remainingBytes(Device& device)
	{
		const long pos = device.tell();
		if (pos < 0)
			throw std::runtime_error("Device::tell failed during async read");

		const size_t total = device.size();
		const size_t current = static_cast<size_t>(pos);
		return total > current ? total - current : 0;
	}

	static void readRemainingInto(Device& device, ByteVector& result, std::stop_token stopToken)
	{
		throwIfStopped(stopToken);
		validateReadable(device);

		const size_t remaining = remainingBytes(device);
		result.resize(remaining);

		size_t totalRead = 0;
		while (totalRead < remaining) {
			throwIfStopped(stopToken);
			const size_t requested = std::min(ReadAllChunkSize, remaining - totalRead);
			const size_t got = device.read(result.data() + totalRead, 1, requested);
			if (got == 0)
				throw std::runtime_error("Device::read ended before the expected byte count");
			totalRead += got;
		}

		throwIfStopped(stopToken);
	}

	template<typename Callback>
	requires detail::AsyncChunkCallback<Callback>
	static void readAllocatedChunks(Device& device, std::stop_token stopToken, size_t chunkSize, std::pmr::memory_resource* memRes, Callback& callback)
	{
		if (chunkSize == 0)
			throw std::invalid_argument("Async::readChunks requires a non-zero chunk size");

		throwIfStopped(stopToken);
		validateReadable(device);

		ByteVector chunk(memoryResourceOrDefault(memRes));
		chunk.resize(chunkSize);

		while (!device.eof()) {
			throwIfStopped(stopToken);
			const size_t got = device.read(chunk.data(), 1, chunk.size());
			if (got == 0)
				break;
			std::invoke(callback, std::span<const std::byte>(chunk.data(), got));
		}

		throwIfStopped(stopToken);
	}

	template<typename Callback>
	requires detail::AsyncChunkCallback<Callback>
	static void readBorrowedChunks(Device& device, std::stop_token stopToken, void* chunk, size_t chunkSize, Callback& callback)
	{
		if (chunkSize == 0)
			throw std::invalid_argument("Async::readChunks requires a non-zero chunk size");
		if (!chunk)
			throw std::invalid_argument("Async::readChunks requires non-null caller-owned storage");

		throwIfStopped(stopToken);
		validateReadable(device);

		while (!device.eof()) {
			throwIfStopped(stopToken);
			const size_t got = device.read(chunk, 1, chunkSize);
			if (got == 0)
				break;
			std::invoke(callback, std::span<const std::byte>(static_cast<const std::byte*>(chunk), got));
		}

		throwIfStopped(stopToken);
	}

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
	 * Device::tell and Device::size, and reads in bounded chunks.
	 *
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param memRes Memory resource used by the returned ByteVector.
	 * @return Executor-specific future-like object containing the read ByteVector.
	 * @throws std::runtime_error Through the future if the Device is invalid or ends early.
	 */
	[[nodiscard]] static auto readAll(Executor& executor, Device& device, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
	{
		return readAll(executor, device, std::stop_token(), memRes);
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
	 * @return Executor-specific future-like object containing the read ByteVector.
	 * @throws AsyncOperationCanceled Through the future when @p stopToken is requested.
	 * @throws std::runtime_error Through the future if the Device is invalid or ends early.
	 */
	[[nodiscard]] static auto readAll(Executor& executor, Device& device, std::stop_token stopToken, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
	{
		return executor.enqueueAsync([&device, stopToken, memRes]() {
			ByteVector result(memoryResourceOrDefault(memRes));
			readRemainingInto(device, result, stopToken);
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
	 * @return Executor-specific future-like object containing the read ByteVector.
	 */
	[[nodiscard]] static auto readAll(Executor& executor, sDevice device, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
	{
		return readAll(executor, std::move(device), std::stop_token(), memRes);
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
	 * @return Executor-specific future-like object containing the read ByteVector.
	 * @throws AsyncOperationCanceled Through the future when @p stopToken is requested.
	 */
	[[nodiscard]] static auto readAll(Executor& executor, sDevice device, std::stop_token stopToken, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
	{
		return executor.enqueueAsync([device = std::move(device), stopToken, memRes]() {
			ByteVector result(memoryResourceOrDefault(memRes));
			readRemainingInto(requireDevice(device), result, stopToken);
			return result;
		});
	}

	/**
	 * @brief Asynchronously reads all remaining bytes into a shared PMR buffer.
	 *
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param memRes Memory resource used for the shared object and byte storage.
	 * @return Executor-specific future-like object containing a ByteVectorPtr.
	 */
	[[nodiscard]] static auto readAllShared(Executor& executor, Device& device, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
	{
		return readAllShared(executor, device, std::stop_token(), memRes);
	}

	/**
	 * @brief Asynchronously reads all remaining bytes into a shared PMR buffer with cooperative cancellation.
	 *
	 * @param executor Executor used to run the blocking read loop.
	 * @param device Device to read from. Must remain valid until the task completes.
	 * @param stopToken Cooperative cancellation token.
	 * @param memRes Memory resource used for the shared object and byte storage.
	 * @return Executor-specific future-like object containing a ByteVectorPtr.
	 * @throws AsyncOperationCanceled Through the future when @p stopToken is requested.
	 */
	[[nodiscard]] static auto readAllShared(Executor& executor, Device& device, std::stop_token stopToken, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
	{
		return executor.enqueueAsync([&device, stopToken, memRes]() {
			auto resource = memoryResourceOrDefault(memRes);
			auto result = std::allocate_shared<ByteVector>(
				std::pmr::polymorphic_allocator<ByteVector>(resource),
				resource
				);
			readRemainingInto(device, *result, stopToken);
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
	 * @return Executor-specific future-like object containing a ByteVectorPtr.
	 */
	[[nodiscard]] static auto readAllShared(Executor& executor, sDevice device, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
	{
		return readAllShared(executor, std::move(device), std::stop_token(), memRes);
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
	 * @return Executor-specific future-like object containing a ByteVectorPtr.
	 * @throws AsyncOperationCanceled Through the future when @p stopToken is requested.
	 */
	[[nodiscard]] static auto readAllShared(Executor& executor, sDevice device, std::stop_token stopToken, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
	{
		return executor.enqueueAsync([device = std::move(device), stopToken, memRes]() {
			auto resource = memoryResourceOrDefault(memRes);
			auto result = std::allocate_shared<ByteVector>(
				std::pmr::polymorphic_allocator<ByteVector>(resource),
				resource
				);
			readRemainingInto(requireDevice(device), *result, stopToken);
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
		return readAllThen(executor, device, std::stop_token(), std::pmr::get_default_resource(), std::forward<Callback>(callback));
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
		return readAllThen(executor, device, std::stop_token(), memRes, std::forward<Callback>(callback));
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
		return readAllThen(executor, device, stopToken, std::pmr::get_default_resource(), std::forward<Callback>(callback));
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
		return executor.enqueueAsync([&device, stopToken, memRes, cb = std::forward<Callback>(callback)]() mutable {
			return runReadAllThen([&device, stopToken, memRes]() {
				ByteVector result(memoryResourceOrDefault(memRes));
				readRemainingInto(device, result, stopToken);
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
		return readAllThen(executor, std::move(device), std::stop_token(), std::pmr::get_default_resource(), std::forward<Callback>(callback));
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
		return readAllThen(executor, std::move(device), std::stop_token(), memRes, std::forward<Callback>(callback));
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
		return readAllThen(executor, std::move(device), stopToken, std::pmr::get_default_resource(), std::forward<Callback>(callback));
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
		return executor.enqueueAsync([device = std::move(device), stopToken, memRes, cb = std::forward<Callback>(callback)]() mutable {
			return runReadAllThen([&device, stopToken, memRes]() {
				ByteVector result(memoryResourceOrDefault(memRes));
				readRemainingInto(requireDevice(device), result, stopToken);
				return result;
			}, cb);
		});
	}
};

} // namespace Io
} // namespace Elv
#endif // ELVIOASYNC_HPP
