#ifndef ELVEVENTLOOPWAKEUP_HPP
#define ELVEVENTLOOPWAKEUP_HPP
/**
 * @file ElvEventLoopWakeup.hpp
 * @brief Portable wakeup handle used by `Elv::Util::PollingEventLoop`.
 *
 * A polling event loop blocks in the OS poller. Commands enqueued from another
 * thread must wake that wait without polling with a short timeout. Concrete
 * implementations live in Euphemy: eventfd/pipe on POSIX and a loopback socket
 * pair on Windows (WSAPoll only accepts sockets).
 */
#include <Elvavena/Io/ElvEventPoller.hpp>
#include <cstdint>

namespace Elv {
namespace Io {

/**
 * @class EventLoopWakeup
 * @brief Thread-safe wakeup primitive exported to an `EventPoller`.
 */
class EventLoopWakeup {
public:
	/** @brief Virtual destructor. */
	virtual ~EventLoopWakeup() = default;

	/** @brief Returns the handle the poller should watch for `Read`. */
	[[nodiscard]] virtual NativeHandle nativeHandle() const noexcept = 0;

	/** @brief Signals the loop thread. May be called from any thread. */
	virtual bool notify() noexcept = 0;

	/** @brief Clears pending wakeup signals. Called on the loop thread. */
	virtual bool drain() noexcept = 0;

	/** @brief Returns whether the wakeup primitive was created successfully. */
	[[nodiscard]] virtual bool isValid() const noexcept = 0;

	/** @brief Returns the last native error code. */
	[[nodiscard]] virtual int lastError() const noexcept = 0;
};

} // namespace Io
} // namespace Elv

#endif // ELVEVENTLOOPWAKEUP_HPP
