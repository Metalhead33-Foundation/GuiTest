#ifndef EUPHWSAPOLLPOLLER_HPP
#define EUPHWSAPOLLPOLLER_HPP
/**
 * @file EuphWSAPollPoller.hpp
 * @brief Windows `WSAPoll` implementation of the readiness poller.
 *
 * `WSAPoll` is the Windows equivalent of `poll(2)` for sockets. It does not
 * support arbitrary file/pipe HANDLEs; use the IOCP wrapper for those. On
 * non-Windows builds the class exists but reports `isValid() == false`.
 *
 * Edge-triggered and one-shot flags are accepted for API portability but are
 * ignored by this backend.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvEventPoller.hpp>
#include <memory>

namespace Euph {
namespace Platform {

/**
 * @class WSAPollPoller
 * @brief Winsock socket readiness poller.
 */
class MH_EUPH_API WSAPollPoller final : public Elv::Io::EventPoller {
public:
	/** @brief Initializes Winsock and the registration table. */
	WSAPollPoller();

	/** @brief Releases Winsock. */
	~WSAPollPoller() override;

	WSAPollPoller(const WSAPollPoller&) = delete;
	WSAPollPoller& operator=(const WSAPollPoller&) = delete;
	WSAPollPoller(WSAPollPoller&&) = delete;
	WSAPollPoller& operator=(WSAPollPoller&&) = delete;

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
	struct Impl;                ///< Hidden Winsock state.
	std::unique_ptr<Impl> impl; ///< Pimpl.
};

} // namespace Platform
} // namespace Euph

#endif // EUPHWSAPOLLPOLLER_HPP
