#include "EuphEpollPoller.hpp"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdint>
#ifndef ENOSYS
#define ENOSYS 38
#endif
#include <unordered_map>
#include <vector>

#if defined(__linux__)
#include <sys/epoll.h>
#include <unistd.h>
#endif

namespace Euph {
namespace Platform {
#if defined(__linux__)

namespace {

/// @brief Converts an abstract interest mask to epoll flags.
std::uint32_t toEpollMask(Elv::Io::EventMask interest, Elv::Io::PollFlags flags) {
	std::uint32_t mask = 0;
	if (Elv::Io::hasEvent(interest, Elv::Io::EventMask::Read)) mask |= EPOLLIN;
	if (Elv::Io::hasEvent(interest, Elv::Io::EventMask::Write)) mask |= EPOLLOUT;
	if (Elv::Io::hasFlag(flags, Elv::Io::PollFlags::EdgeTriggered)) mask |= EPOLLET;
	if (Elv::Io::hasFlag(flags, Elv::Io::PollFlags::OneShot)) mask |= EPOLLONESHOT;
#ifdef EPOLLRDHUP
	if (Elv::Io::hasEvent(interest, Elv::Io::EventMask::Hangup)) mask |= EPOLLRDHUP;
#endif
	return mask;
}

/// @brief Converts epoll flags to the abstract event mask.
Elv::Io::EventMask fromEpollMask(std::uint32_t events) {
	Elv::Io::EventMask mask = Elv::Io::EventMask::None;
	if ((events & EPOLLIN) != 0) mask |= Elv::Io::EventMask::Read;
	if ((events & EPOLLOUT) != 0) mask |= Elv::Io::EventMask::Write;
	if ((events & EPOLLERR) != 0) mask |= Elv::Io::EventMask::Error;
	if ((events & EPOLLHUP) != 0
#ifdef EPOLLRDHUP
		|| (events & EPOLLRDHUP) != 0
#endif
	) mask |= Elv::Io::EventMask::Hangup;
	return mask;
}

} // namespace
#endif // __linux__ helper functions

#if defined(__linux__)

struct EpollPoller::Impl {
	int descriptor = -1;                                  ///< epoll file descriptor.
	int error = 0;                                        ///< Last native error.
	std::unordered_map<int, std::uint64_t> tokens;        ///< Handle -> token.
	std::vector<epoll_event> rawEvents;                   ///< Reused kernel buffer.
};

EpollPoller::EpollPoller() : impl(std::make_unique<Impl>()) {
	impl->descriptor = epoll_create1(EPOLL_CLOEXEC);
	if (impl->descriptor < 0) impl->error = errno;
}

EpollPoller::~EpollPoller() {
	if (impl != nullptr && impl->descriptor >= 0) close(impl->descriptor);
}

bool EpollPoller::add(Elv::Io::NativeHandle handle, Elv::Io::EventMask interest,
	std::uint64_t token, Elv::Io::PollFlags flags) {
	if (impl == nullptr || impl->descriptor < 0 || handle < 0) return false;
	epoll_event event {};
	event.events = toEpollMask(interest, flags);
	event.data.fd = static_cast<int>(handle);
	if (epoll_ctl(impl->descriptor, EPOLL_CTL_ADD, static_cast<int>(handle), &event) != 0) {
		impl->error = errno;
		return false;
	}
	impl->tokens[static_cast<int>(handle)] = token;
	return true;
}

bool EpollPoller::modify(Elv::Io::NativeHandle handle, Elv::Io::EventMask interest,
	std::uint64_t token, Elv::Io::PollFlags flags) {
	if (impl == nullptr || impl->descriptor < 0 || handle < 0) return false;
	epoll_event event {};
	event.events = toEpollMask(interest, flags);
	event.data.fd = static_cast<int>(handle);
	if (epoll_ctl(impl->descriptor, EPOLL_CTL_MOD, static_cast<int>(handle), &event) != 0) {
		impl->error = errno;
		return false;
	}
	impl->tokens[static_cast<int>(handle)] = token;
	return true;
}

bool EpollPoller::remove(Elv::Io::NativeHandle handle) {
	if (impl == nullptr || impl->descriptor < 0 || handle < 0) return false;
	if (epoll_ctl(impl->descriptor, EPOLL_CTL_DEL, static_cast<int>(handle), nullptr) != 0) {
		impl->error = errno;
		return false;
	}
	impl->tokens.erase(static_cast<int>(handle));
	return true;
}

int EpollPoller::wait(std::span<Elv::Io::PollEvent> events,
	std::optional<Elv::Io::EventDuration> timeout) {
	if (impl == nullptr || impl->descriptor < 0) return -1;
	if (events.empty()) return 0;

	int timeoutMs = -1;
	if (timeout.has_value()) {
		const auto milliseconds = std::chrono::ceil<std::chrono::milliseconds>(*timeout).count();
		timeoutMs = static_cast<int>(std::clamp<long long>(milliseconds, 0, INT_MAX));
	}

	impl->rawEvents.resize(events.size());
	const int ready = epoll_wait(impl->descriptor, impl->rawEvents.data(),
		static_cast<int>(impl->rawEvents.size()), timeoutMs);
	if (ready < 0) {
		impl->error = errno;
		return -1;
	}
	for (int index = 0; index < ready; ++index) {
		const epoll_event& raw = impl->rawEvents[static_cast<std::size_t>(index)];
		const int handle = raw.data.fd;
		const auto token = impl->tokens.find(handle);
		events[static_cast<std::size_t>(index)] = Elv::Io::PollEvent {
			static_cast<Elv::Io::NativeHandle>(handle),
			token != impl->tokens.end() ? token->second : 0,
			fromEpollMask(raw.events)
		};
	}
	return ready;
}

bool EpollPoller::isValid() const { return impl != nullptr && impl->descriptor >= 0; }

const char* EpollPoller::backendName() const noexcept { return "epoll"; }

int EpollPoller::lastError() const noexcept { return impl != nullptr ? impl->error : 0; }

#else

struct EpollPoller::Impl {
	int error = 0; ///< Always ENOSYS on unsupported platforms.
};

EpollPoller::EpollPoller() : impl(std::make_unique<Impl>()) { impl->error = ENOSYS; }
EpollPoller::~EpollPoller() = default;

bool EpollPoller::add(Elv::Io::NativeHandle, Elv::Io::EventMask, std::uint64_t,
	Elv::Io::PollFlags) { return false; }
bool EpollPoller::modify(Elv::Io::NativeHandle, Elv::Io::EventMask, std::uint64_t,
	Elv::Io::PollFlags) { return false; }
bool EpollPoller::remove(Elv::Io::NativeHandle) { return false; }
int EpollPoller::wait(std::span<Elv::Io::PollEvent>, std::optional<Elv::Io::EventDuration>) {
	return -1;
}
bool EpollPoller::isValid() const { return false; }
const char* EpollPoller::backendName() const noexcept { return "epoll (unavailable)"; }
int EpollPoller::lastError() const noexcept { return impl != nullptr ? impl->error : ENOSYS; }

#endif

} // namespace Platform
} // namespace Euph
