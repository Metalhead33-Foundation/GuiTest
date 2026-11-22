#ifndef EUPHEPOLLPOLLER_HPP
#define EUPHEPOLLPOLLER_HPP
/**
 * @file EuphEpollPoller.hpp
 * @brief Linux epoll implementation of `Elv::Io::EventPoller`.
 *
 * The class hides epoll headers and the kernel event buffer behind a pimpl so
 * portable code can hold an `EventPoller` without including platform headers.
 * On non-Linux builds the class exists but reports `isValid() == false`; a
 * factory can then select another backend.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvEventPoller.hpp>
#include <memory>

namespace Euph {
namespace Platform {

/**
 * @class EpollPoller
 * @brief epoll-backed readiness poller.
 */
class MH_EUPH_API EpollPoller final : public Elv::Io::EventPoller {
public:
	/** @brief Creates the epoll queue. */
	EpollPoller();

	/** @brief Closes the epoll queue. */
	~EpollPoller() override;

	EpollPoller(const EpollPoller&) = delete;
	EpollPoller& operator=(const EpollPoller&) = delete;
	EpollPoller(EpollPoller&&) = delete;
	EpollPoller& operator=(EpollPoller&&) = delete;

	bool add(Elv::Io::NativeHandle handle, Elv::Io::EventMask interest,
		std::uint64_t token, Elv::Io::PollFlags flags = Elv::Io::PollFlags::None) override;

	bool modify(Elv::Io::NativeHandle handle, Elv::Io::EventMask interest,
		std::uint64_t token, Elv::Io::PollFlags flags = Elv::Io::PollFlags::None) override;

	bool remove(Elv::Io::NativeHandle handle) override;

	int wait(std::span<Elv::Io::PollEvent> events,
		std::optional<Elv::Io::EventDuration> timeout) override;

	[[nodiscard]] bool isValid() const override;
	[[nodiscard]] const char* backendName() const noexcept override;
	[[nodiscard]] int lastError() const noexcept override;

private:
	struct Impl;                ///< Hidden epoll state.
	std::unique_ptr<Impl> impl; ///< Pimpl.
};

} // namespace Platform
} // namespace Euph

#endif // EUPHEPOLLPOLLER_HPP
