#ifndef EUPHEVENTPOLLERFACTORY_HPP
#define EUPHEVENTPOLLERFACTORY_HPP
/**
 * @file EuphEventPollerFactory.hpp
 * @brief Selects the native readiness poller for the current platform.
 *
 * The factory returns `nullptr` when the requested backend is unavailable. This
 * lets an application probe at runtime instead of assuming epoll, kqueue or
 * WSAPoll exists.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvEventPoller.hpp>
#include <memory>

namespace Euph {
namespace Platform {

class IoUring;

/**
 * @enum PollerBackend
 * @brief Requested readiness backend.
 */
enum class PollerBackend : unsigned char {
	Auto,    ///< Best native backend.
	Epoll,   ///< Linux epoll.
	WSAPoll, ///< Windows socket poll.
};

/**
 * @brief Creates a readiness poller.
 * @param backend Requested backend; `Auto` chooses the platform default.
 * @return Poller on success, null when unsupported or initialization failed.
 */
[[nodiscard]] MH_EUPH_API std::unique_ptr<Elv::Io::EventPoller> createEventPoller(
	PollerBackend backend = PollerBackend::Auto);

/**
 * @brief Returns the backend name Auto would select on this platform.
 * @return Static backend name, or "none".
 */
[[nodiscard]] MH_EUPH_API const char* defaultEventPollerName() noexcept;

/**
 * @brief Creates an io_uring queue if the kernel permits it.
 * @param entries Requested queue depth.
 * @return Valid queue, or null when liburing/io_uring is unavailable at compile
 *         time or `io_uring_setup` is rejected at runtime.
 */
[[nodiscard]] MH_EUPH_API std::unique_ptr<IoUring> createIoUringIfAvailable(
	unsigned entries = 256);

} // namespace Platform
} // namespace Euph

#endif // EUPHEVENTPOLLERFACTORY_HPP
