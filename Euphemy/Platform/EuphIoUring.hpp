#ifndef EUPHIOURING_HPP
#define EUPHIOURING_HPP
/**
 * @file EuphIoUring.hpp
 * @brief Convenience RAII wrapper around Linux io_uring.
 *
 * io_uring is a completion API: submit read/write/poll/timeout operations and
 * wait for completion records. It is not a readiness poller and is not forced
 * into `Elv::Io::EventPoller`.
 *
 * The wrapper is compiled against liburing when it is available. On platforms
 * where io_uring is disabled (or liburing is absent), the class still exists but
 * `isValid()` is false and operations report `-ENOSYS`; callers should fall back
 * to epoll/WSAPoll for readiness or `Elv::Io::Async<ThreadPool>` for file I/O.
 *
 * @note Runtime availability is separate from compile-time availability:
 *       Docker seccomp, Android and ChromeOS can disable `io_uring_setup` even
 *       when liburing is linked. Always check `isValid()` or call
 *       `runtimeAvailable()` before selecting this backend.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvAsyncCompletion.hpp>
#include <Elvavena/Io/ElvEventPoller.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>

namespace Euph {
namespace Platform {

/**
 * @class IoUring
 * @brief RAII io_uring queue with common submission helpers.
 */
class MH_EUPH_API IoUring {
public:
	/**
	 * @brief Initializes an io_uring queue.
	 * @param entries Requested queue depth.
	 * @param flags `io_uring_queue_init` flags; zero for defaults.
	 */
	explicit IoUring(unsigned entries = 256, unsigned flags = 0);

	/** @brief Exits the io_uring queue. */
	~IoUring();

	IoUring(const IoUring&) = delete;
	IoUring& operator=(const IoUring&) = delete;
	IoUring(IoUring&&) = delete;
	IoUring& operator=(IoUring&&) = delete;

	/** @brief Returns whether the queue was created successfully. */
	[[nodiscard]] bool isValid() const;

	/** @brief Returns the last negative errno, or zero. */
	[[nodiscard]] int lastError() const;

	/** @brief Returns the configured queue depth. */
	[[nodiscard]] unsigned queueDepth() const;

	/**
	 * @brief Queues a no-op completion.
	 * @param userData Token returned in the completion.
	 * @return True when an SQE was available.
	 */
	bool submitNop(std::uint64_t userData);

	/**
	 * @brief Queues a read into a caller-owned buffer.
	 * @param fd File descriptor.
	 * @param buffer Destination buffer; must outlive the operation.
	 * @param offset File offset.
	 * @param userData Token returned in the completion.
	 * @return True when an SQE was available.
	 */
	bool submitRead(int fd, std::span<std::byte> buffer, std::uint64_t offset,
		std::uint64_t userData);

	/**
	 * @brief Queues a write from a caller-owned buffer.
	 * @param fd File descriptor.
	 * @param buffer Source buffer; must outlive the operation.
	 * @param offset File offset.
	 * @param userData Token returned in the completion.
	 * @return True when an SQE was available.
	 */
	bool submitWrite(int fd, std::span<const std::byte> buffer, std::uint64_t offset,
		std::uint64_t userData);

	/**
	 * @brief Queues a poll-add operation.
	 * @param fd File descriptor to poll.
	 * @param interest Read/write/error/hangup interest.
	 * @param userData Token returned in the completion.
	 * @return True when an SQE was available.
	 */
	bool submitPollAdd(int fd, Elv::Io::EventMask interest, std::uint64_t userData);

	/**
	 * @brief Queues a relative timeout.
	 * @param timeout Timeout duration.
	 * @param userData Token returned in the completion.
	 * @return True when an SQE was available.
	 */
	bool submitTimeout(Elv::Io::EventDuration timeout, std::uint64_t userData);

	/**
	 * @brief Submits all currently queued SQEs.
	 * @return Number submitted, or a negative errno.
	 */
	int submit();

	/**
	 * @brief Submits pending work and waits for completions.
	 * @param completions Destination span.
	 * @param timeout Empty for infinite, zero for a non-blocking poll.
	 * @return Number of completions written, or a negative errno.
	 */
	int wait(std::span<Elv::Io::AsyncCompletion> completions,
		std::optional<Elv::Io::EventDuration> timeout);

	/**
	 * @brief Pops one completion if one is ready.
	 * @param completion Receives the completion.
	 * @return True when a completion was available.
	 */
	bool tryPop(Elv::Io::AsyncCompletion& completion);

	/**
	 * @brief Probes whether io_uring can be initialized on this machine.
	 * @return True when a temporary queue can be created and destroyed.
	 */
	[[nodiscard]] static bool runtimeAvailable();

private:
	struct Impl;                ///< Hidden liburing state.
	std::unique_ptr<Impl> impl; ///< Pimpl.
};

} // namespace Platform
} // namespace Euph

#endif // EUPHIOURING_HPP
