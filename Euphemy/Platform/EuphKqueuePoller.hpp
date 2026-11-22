#ifndef EUPHKQUEUEPOLLER_HPP
#define EUPHKQUEUEPOLLER_HPP
/**
 * @file EuphKqueuePoller.hpp
 * @brief BSD/macOS kqueue implementation of `Elv::Io::EventPoller`.
 *
 * On non-BSD platforms the class exists but reports `isValid() == false`, which
 * lets `createEventPoller()` fall through to another backend.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvEventPoller.hpp>
#include <memory>

namespace Euph {
namespace Platform {

/**
 * @class KqueuePoller
 * @brief kqueue-backed readiness poller.
 */
class MH_EUPH_API KqueuePoller final : public Elv::Io::EventPoller {
public:
	/** @brief Creates the kqueue. */
	KqueuePoller();

	/** @brief Closes the kqueue. */
	~KqueuePoller() override;

	KqueuePoller(const KqueuePoller&) = delete;
	KqueuePoller& operator=(const KqueuePoller&) = delete;
	KqueuePoller(KqueuePoller&&) = delete;
	KqueuePoller& operator=(KqueuePoller&&) = delete;

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
	struct Impl;                ///< Hidden kqueue state.
	std::unique_ptr<Impl> impl; ///< Pimpl.
};

} // namespace Platform
} // namespace Euph

#endif // EUPHKQUEUEPOLLER_HPP
