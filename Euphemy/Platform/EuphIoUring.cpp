#include "EuphIoUring.hpp"
#include <cerrno>
#include <chrono>
#include <cstdint>
#ifndef ENOSYS
#define ENOSYS 38
#endif
#include <deque>
#include <limits>

#if defined(MH_HAS_IO_URING)
#include <liburing.h>
#include <poll.h>
#endif

namespace Euph {
namespace Platform {

#if defined(MH_HAS_IO_URING)

namespace {

/**
 * @brief Converts an abstract event mask to poll flags.
 * @param interest Read/write/error/hangup interest.
 * @return Poll mask.
 */
unsigned toPollMask(Elv::Io::EventMask interest) {
	unsigned mask = 0;
	if (Elv::Io::hasEvent(interest, Elv::Io::EventMask::Read)) mask |= POLLIN;
	if (Elv::Io::hasEvent(interest, Elv::Io::EventMask::Write)) mask |= POLLOUT;
	if (Elv::Io::hasEvent(interest, Elv::Io::EventMask::Error)) mask |= POLLERR;
	if (Elv::Io::hasEvent(interest, Elv::Io::EventMask::Hangup)) mask |= POLLHUP;
	return mask;
}

/**
 * @brief Converts a duration to a relative kernel timespec.
 * @param timeout Duration.
 * @return Timespec.
 */
__kernel_timespec toTimespec(Elv::Io::EventDuration timeout) {
	using namespace std::chrono;
	const auto safe = timeout > Elv::Io::EventDuration::zero()
		? timeout : Elv::Io::EventDuration::zero();
	const auto secondsPart = duration_cast<seconds>(safe);
	const auto nanosPart = duration_cast<nanoseconds>(safe - secondsPart);
	__kernel_timespec spec {};
	spec.tv_sec = static_cast<std::int64_t>(secondsPart.count());
	spec.tv_nsec = static_cast<std::int64_t>(nanosPart.count());
	return spec;
}

/**
 * @brief Copies a completion queue entry into the public record.
 * @param cqe Kernel completion.
 * @param completion Destination.
 */
void fillCompletion(const io_uring_cqe* cqe, Elv::Io::AsyncCompletion& completion) {
	completion.userData = cqe->user_data;
	completion.result = cqe->res;
	completion.flags = cqe->flags;
	completion.bytesTransferred = cqe->res > 0
		? static_cast<std::uint32_t>(cqe->res) : 0;
}

} // namespace

struct IoUring::Impl {
	io_uring ring {};
	bool initialized = false;
	int error = 0;
	std::deque<__kernel_timespec> timeouts; ///< Stable storage for timeout SQEs.
};

IoUring::IoUring(unsigned entries, unsigned flags) : impl(std::make_unique<Impl>()) {
	const int result = io_uring_queue_init(entries, &impl->ring, flags);
	if (result < 0) {
		impl->error = result;
		return;
	}
	impl->initialized = true;
}

IoUring::~IoUring() {
	if (impl != nullptr && impl->initialized) io_uring_queue_exit(&impl->ring);
}

bool IoUring::isValid() const { return impl != nullptr && impl->initialized; }

int IoUring::lastError() const { return impl != nullptr ? impl->error : -ENOSYS; }

unsigned IoUring::queueDepth() const {
	return impl != nullptr && impl->initialized ? impl->ring.sq.ring_entries : 0;
}

bool IoUring::submitNop(std::uint64_t userData) {
	if (!isValid()) return false;
	io_uring_sqe* sqe = io_uring_get_sqe(&impl->ring);
	if (sqe == nullptr) {
		impl->error = -EBUSY;
		return false;
	}
	io_uring_prep_nop(sqe);
	sqe->user_data = userData;
	return true;
}

bool IoUring::submitRead(int fd, std::span<std::byte> buffer, std::uint64_t offset,
	std::uint64_t userData) {
	if (!isValid() || buffer.size() > std::numeric_limits<unsigned>::max()) return false;
	io_uring_sqe* sqe = io_uring_get_sqe(&impl->ring);
	if (sqe == nullptr) {
		impl->error = -EBUSY;
		return false;
	}
	io_uring_prep_read(sqe, fd, buffer.data(), static_cast<unsigned>(buffer.size()), offset);
	sqe->user_data = userData;
	return true;
}

bool IoUring::submitWrite(int fd, std::span<const std::byte> buffer, std::uint64_t offset,
	std::uint64_t userData) {
	if (!isValid() || buffer.size() > std::numeric_limits<unsigned>::max()) return false;
	io_uring_sqe* sqe = io_uring_get_sqe(&impl->ring);
	if (sqe == nullptr) {
		impl->error = -EBUSY;
		return false;
	}
	io_uring_prep_write(sqe, fd, buffer.data(), static_cast<unsigned>(buffer.size()), offset);
	sqe->user_data = userData;
	return true;
}

bool IoUring::submitPollAdd(int fd, Elv::Io::EventMask interest, std::uint64_t userData) {
	if (!isValid()) return false;
	io_uring_sqe* sqe = io_uring_get_sqe(&impl->ring);
	if (sqe == nullptr) {
		impl->error = -EBUSY;
		return false;
	}
	io_uring_prep_poll_add(sqe, fd, toPollMask(interest));
	sqe->user_data = userData;
	return true;
}

bool IoUring::submitTimeout(Elv::Io::EventDuration timeout, std::uint64_t userData) {
	if (!isValid()) return false;
	io_uring_sqe* sqe = io_uring_get_sqe(&impl->ring);
	if (sqe == nullptr) {
		impl->error = -EBUSY;
		return false;
	}
	impl->timeouts.push_back(toTimespec(timeout));
	io_uring_prep_timeout(sqe, &impl->timeouts.back(), 1, 0);
	sqe->user_data = userData;
	return true;
}

int IoUring::submit() {
	if (!isValid()) return -ENOSYS;
	const int submitted = io_uring_submit(&impl->ring);
	if (submitted < 0) impl->error = submitted;
	return submitted;
}

int IoUring::wait(std::span<Elv::Io::AsyncCompletion> completions,
	std::optional<Elv::Io::EventDuration> timeout) {
	if (!isValid()) return -ENOSYS;
	if (completions.empty()) return 0;

	const int submitted = io_uring_submit(&impl->ring);
	if (submitted < 0) {
		impl->error = submitted;
		return submitted;
	}

	io_uring_cqe* cqe = nullptr;
	int result = 0;
	if (!timeout.has_value()) {
		result = io_uring_wait_cqe(&impl->ring, &cqe);
	} else if (*timeout <= Elv::Io::EventDuration::zero()) {
		result = io_uring_peek_cqe(&impl->ring, &cqe);
	} else {
		__kernel_timespec spec = toTimespec(*timeout);
		result = io_uring_wait_cqe_timeout(&impl->ring, &cqe, &spec);
	}
	if (result == -ETIME || result == -EAGAIN) return 0;
	if (result < 0) {
		impl->error = result;
		return result;
	}

	std::size_t written = 0;
	while (cqe != nullptr && written < completions.size()) {
		fillCompletion(cqe, completions[written]);
		io_uring_cqe_seen(&impl->ring, cqe);
		++written;
		if (written >= completions.size()) break;
		result = io_uring_peek_cqe(&impl->ring, &cqe);
		if (result < 0) break;
	}
	return static_cast<int>(written);
}

bool IoUring::tryPop(Elv::Io::AsyncCompletion& completion) {
	if (!isValid()) return false;
	io_uring_cqe* cqe = nullptr;
	const int result = io_uring_peek_cqe(&impl->ring, &cqe);
	if (result < 0 || cqe == nullptr) return false;
	fillCompletion(cqe, completion);
	io_uring_cqe_seen(&impl->ring, cqe);
	return true;
}

bool IoUring::runtimeAvailable() {
	io_uring ring {};
	const int result = io_uring_queue_init(8, &ring, 0);
	if (result < 0) return false;
	io_uring_queue_exit(&ring);
	return true;
}

#else

struct IoUring::Impl {
	int error = -ENOSYS;
};

IoUring::IoUring(unsigned, unsigned) : impl(std::make_unique<Impl>()) {}
IoUring::~IoUring() = default;
bool IoUring::isValid() const { return false; }
int IoUring::lastError() const { return impl != nullptr ? impl->error : -ENOSYS; }
unsigned IoUring::queueDepth() const { return 0; }
bool IoUring::submitNop(std::uint64_t) { return false; }
bool IoUring::submitRead(int, std::span<std::byte>, std::uint64_t, std::uint64_t) { return false; }
bool IoUring::submitWrite(int, std::span<const std::byte>, std::uint64_t, std::uint64_t) { return false; }
bool IoUring::submitPollAdd(int, Elv::Io::EventMask, std::uint64_t) { return false; }
bool IoUring::submitTimeout(Elv::Io::EventDuration, std::uint64_t) { return false; }
int IoUring::submit() { return -ENOSYS; }
int IoUring::wait(std::span<Elv::Io::AsyncCompletion>,
	std::optional<Elv::Io::EventDuration>) { return -ENOSYS; }
bool IoUring::tryPop(Elv::Io::AsyncCompletion&) { return false; }
bool IoUring::runtimeAvailable() { return false; }

#endif

} // namespace Platform
} // namespace Euph

