#ifndef ELVEVENTPOLLER_HPP
#define ELVEVENTPOLLER_HPP
/**
 * @file ElvEventPoller.hpp
 * @brief OS-independent readiness-event poller interface.
 *
 * epoll, kqueue and WSAPoll answer the same question: which registered handles
 * are ready for read/write/error. This interface is the common seam. Concrete
 * implementations live in Euphemy because they need platform headers.
 *
 * io_uring and IOCP are completion APIs, not readiness pollers. They get a
 * separate wrapper and are not forced into this interface.
 *
 * Handles are opaque `intptr_t` values: a file descriptor on POSIX, a SOCKET on
 * Windows. Tokens are caller-owned and are returned unchanged in `PollEvent`.
 */
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace Elv {
namespace Io {

/// @brief Platform-native handle: file descriptor or socket.
using NativeHandle = std::intptr_t;

/// @brief Invalid native handle sentinel.
inline constexpr NativeHandle InvalidNativeHandle = -1;

/// @brief Duration used by event waits.
using EventDuration = std::chrono::steady_clock::duration;

/**
 * @enum EventMask
 * @brief Readiness interests and reported conditions.
 */
enum class EventMask : std::uint8_t {
	None = 0,   ///< No interest.
	Read = 1,   ///< Readable / incoming connection.
	Write = 2,  ///< Writable.
	Error = 4,  ///< Error condition.
	Hangup = 8, ///< Peer closed or handle hung up.
};

/** @brief Bitwise OR for event masks. */
constexpr EventMask operator|(EventMask lhs, EventMask rhs) noexcept {
	return static_cast<EventMask>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}

/** @brief Bitwise AND for event masks. */
constexpr EventMask operator&(EventMask lhs, EventMask rhs) noexcept {
	return static_cast<EventMask>(static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs));
}

/** @brief Bitwise OR-assignment for event masks. */
constexpr EventMask& operator|=(EventMask& lhs, EventMask rhs) noexcept {
	lhs = lhs | rhs;
	return lhs;
}

/** @brief Returns whether a mask contains a condition. */
constexpr bool hasEvent(EventMask mask, EventMask flag) noexcept {
	return (mask & flag) != EventMask::None;
}

/**
 * @enum PollFlags
 * @brief Optional poller behaviour flags.
 */
enum class PollFlags : std::uint8_t {
	None = 0,          ///< Level-triggered one-shot-free registration.
	EdgeTriggered = 1, ///< Report only state transitions (where supported).
	OneShot = 2,       ///< Disable the registration after one event.
};

/** @brief Bitwise OR for poll flags. */
constexpr PollFlags operator|(PollFlags lhs, PollFlags rhs) noexcept {
	return static_cast<PollFlags>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}

/** @brief Returns whether a flag set contains a flag. */
constexpr bool hasFlag(PollFlags value, PollFlags flag) noexcept {
	return (static_cast<std::uint8_t>(value) & static_cast<std::uint8_t>(flag)) != 0;
}

/**
 * @struct PollEvent
 * @brief One ready handle returned by @ref EventPoller::wait.
 */
struct PollEvent {
	NativeHandle handle = InvalidNativeHandle; ///< Ready handle.
	std::uint64_t token = 0;                   ///< Caller token from add/modify.
	EventMask events = EventMask::None;        ///< Conditions reported.
};

/**
 * @class EventPoller
 * @brief Portable readiness poller.
 *
 * A poller owns its kernel queue. It is not thread-safe; one thread should call
 * `add`/`modify`/`remove`/`wait`. Implementations may not support every handle
 * type (WSAPoll only polls sockets) or edge-triggered flags; consult the
 * implementation documentation.
 */
class EventPoller {
public:
	/** @brief Virtual destructor. */
	virtual ~EventPoller() = default;

	/**
	 * @brief Registers a handle.
	 * @param handle Native file descriptor or socket.
	 * @param interest Requested readiness conditions.
	 * @param token Caller token returned with events.
	 * @param flags Optional edge/one-shot behaviour.
	 * @return True on success.
	 */
	virtual bool add(NativeHandle handle, EventMask interest, std::uint64_t token,
		PollFlags flags = PollFlags::None) = 0;

	/**
	 * @brief Changes an existing registration.
	 * @param handle Registered handle.
	 * @param interest New readiness conditions.
	 * @param token New caller token.
	 * @param flags New behaviour flags.
	 * @return True on success.
	 */
	virtual bool modify(NativeHandle handle, EventMask interest, std::uint64_t token,
		PollFlags flags = PollFlags::None) = 0;

	/**
	 * @brief Removes a registration.
	 * @param handle Registered handle.
	 * @return True on success.
	 */
	virtual bool remove(NativeHandle handle) = 0;

	/**
	 * @brief Waits for readiness events.
	 * @param events Destination span.
	 * @param timeout Empty for infinite, zero for a non-blocking poll.
	 * @return Number of events written, or -1 on error.
	 */
	virtual int wait(std::span<PollEvent> events,
		std::optional<EventDuration> timeout) = 0;

	/** @brief Returns whether the poller is usable. */
	[[nodiscard]] virtual bool isValid() const = 0;

	/** @brief Returns the implementation name ("epoll", "kqueue", "WSAPoll", ...). */
	[[nodiscard]] virtual const char* backendName() const noexcept = 0;

	/** @brief Returns the native error code from the last failed operation. */
	[[nodiscard]] virtual int lastError() const noexcept = 0;
};

} // namespace Io
} // namespace Elv

#endif // ELVEVENTPOLLER_HPP
