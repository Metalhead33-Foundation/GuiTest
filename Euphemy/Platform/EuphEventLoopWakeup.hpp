#ifndef EUPHEVENTLOOPWAKEUP_HPP
#define EUPHEVENTLOOPWAKEUP_HPP
/**
 * @file EuphEventLoopWakeup.hpp
 * @brief Platform wakeup primitive for `Elv::Util::PollingEventLoop`.
 *
 * POSIX uses eventfd when available and falls back to a non-blocking pipe.
 * Windows uses a loopback UDP socket pair because WSAPoll only accepts sockets.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvEventLoopWakeup.hpp>
#include <memory>

namespace Euph {
namespace Platform {

/**
 * @class WakeupEvent
 * @brief Native wakeup handle suitable for an `Elv::Io::EventPoller`.
 */
class MH_EUPH_API WakeupEvent final : public Elv::Io::EventLoopWakeup {
public:
	/** @brief Creates the native wakeup primitive. */
	WakeupEvent();

	/** @brief Releases the native wakeup primitive. */
	~WakeupEvent() override;

	WakeupEvent(const WakeupEvent&) = delete;
	WakeupEvent& operator=(const WakeupEvent&) = delete;
	WakeupEvent(WakeupEvent&&) = delete;
	WakeupEvent& operator=(WakeupEvent&&) = delete;

	[[nodiscard]] Elv::Io::NativeHandle nativeHandle() const noexcept override;
	bool notify() noexcept override;
	bool drain() noexcept override;
	[[nodiscard]] bool isValid() const noexcept override;
	[[nodiscard]] int lastError() const noexcept override;

private:
	struct Impl;                ///< Hidden native state.
	std::unique_ptr<Impl> impl; ///< Pimpl.
};

/**
 * @brief Creates a valid wakeup primitive, or null when unsupported.
 * @return Wakeup handle or null.
 */
[[nodiscard]] MH_EUPH_API std::unique_ptr<Elv::Io::EventLoopWakeup> createWakeupEvent();

} // namespace Platform
} // namespace Euph

#endif // EUPHEVENTLOOPWAKEUP_HPP
