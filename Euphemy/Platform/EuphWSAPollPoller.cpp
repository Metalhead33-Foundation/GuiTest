#include "EuphWSAPollPoller.hpp"
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

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace Euph {
namespace Platform {

#ifdef _WIN32

namespace {

/// @brief Converts an abstract interest mask to WSAPoll events.
short toPollMask(Elv::Io::EventMask interest) {
	short mask = 0;
	if (Elv::Io::hasEvent(interest, Elv::Io::EventMask::Read)) mask |= POLLRDNORM;
	if (Elv::Io::hasEvent(interest, Elv::Io::EventMask::Write)) mask |= POLLWRNORM;
	return mask;
}

/// @brief Converts WSAPoll revents to the abstract mask.
Elv::Io::EventMask fromPollMask(short events) {
	Elv::Io::EventMask mask = Elv::Io::EventMask::None;
	if ((events & (POLLRDNORM | POLLIN)) != 0) mask |= Elv::Io::EventMask::Read;
	if ((events & (POLLWRNORM | POLLOUT)) != 0) mask |= Elv::Io::EventMask::Write;
	if ((events & POLLERR) != 0) mask |= Elv::Io::EventMask::Error;
	if ((events & POLLHUP) != 0) mask |= Elv::Io::EventMask::Hangup;
	return mask;
}

} // namespace

struct WSAPollPoller::Impl {
	struct Entry {
		std::uint64_t token = 0;
		Elv::Io::EventMask interest = Elv::Io::EventMask::None;
	};

	WSADATA winsock {};
	bool started = false;
	int error = 0;
	std::unordered_map<std::uintptr_t, Entry> entries;

	~Impl() {
		if (started) WSACleanup();
	}
};

WSAPollPoller::WSAPollPoller() : impl(std::make_unique<Impl>()) {
	impl->started = WSAStartup(MAKEWORD(2, 2), &impl->winsock) == 0;
	if (!impl->started) impl->error = WSAGetLastError();
}

WSAPollPoller::~WSAPollPoller() = default;

bool WSAPollPoller::add(Elv::Io::NativeHandle handle, Elv::Io::EventMask interest,
	std::uint64_t token, Elv::Io::PollFlags) {
	if (impl == nullptr || !impl->started || handle == static_cast<Elv::Io::NativeHandle>(INVALID_SOCKET)) {
		return false;
	}
	impl->entries[static_cast<std::uintptr_t>(handle)] = Impl::Entry { token, interest };
	return true;
}

bool WSAPollPoller::modify(Elv::Io::NativeHandle handle, Elv::Io::EventMask interest,
	std::uint64_t token, Elv::Io::PollFlags) {
	if (impl == nullptr) return false;
	auto iterator = impl->entries.find(static_cast<std::uintptr_t>(handle));
	if (iterator == impl->entries.end()) return false;
	iterator->second.token = token;
	iterator->second.interest = interest;
	return true;
}

bool WSAPollPoller::remove(Elv::Io::NativeHandle handle) {
	if (impl == nullptr) return false;
	return impl->entries.erase(static_cast<std::uintptr_t>(handle)) != 0;
}

int WSAPollPoller::wait(std::span<Elv::Io::PollEvent> events,
	std::optional<Elv::Io::EventDuration> timeout) {
	if (impl == nullptr || !impl->started || events.empty()) return -1;

	int timeoutMs = -1;
	if (timeout.has_value()) {
		const auto milliseconds = std::chrono::ceil<std::chrono::milliseconds>(*timeout).count();
		timeoutMs = static_cast<int>(std::clamp<long long>(milliseconds, 0, INT_MAX));
	}

	std::vector<WSAPOLLFD> descriptors;
	descriptors.reserve(impl->entries.size());
	for (const auto& entry : impl->entries) {
		WSAPOLLFD descriptor {};
		descriptor.fd = static_cast<SOCKET>(entry.first);
		descriptor.events = toPollMask(entry.second.interest);
		descriptors.push_back(descriptor);
	}
	const int ready = WSAPoll(descriptors.data(),
		static_cast<ULONG>(descriptors.size()), timeoutMs);
	if (ready == SOCKET_ERROR) {
		impl->error = WSAGetLastError();
		return -1;
	}

	int written = 0;
	for (const WSAPOLLFD& descriptor : descriptors) {
		if (descriptor.revents == 0) continue;
		if (static_cast<std::size_t>(written) >= events.size()) break;
		const auto entry = impl->entries.find(static_cast<std::uintptr_t>(descriptor.fd));
		events[static_cast<std::size_t>(written)] = Elv::Io::PollEvent {
			static_cast<Elv::Io::NativeHandle>(descriptor.fd),
			entry != impl->entries.end() ? entry->second.token : 0,
			fromPollMask(descriptor.revents)
		};
		++written;
	}
	return written;
}

bool WSAPollPoller::isValid() const { return impl != nullptr && impl->started; }
const char* WSAPollPoller::backendName() const noexcept { return "WSAPoll"; }
int WSAPollPoller::lastError() const noexcept { return impl != nullptr ? impl->error : 0; }

#else

struct WSAPollPoller::Impl {
	int error = ENOSYS;
};

WSAPollPoller::WSAPollPoller() : impl(std::make_unique<Impl>()) {}
WSAPollPoller::~WSAPollPoller() = default;

bool WSAPollPoller::add(Elv::Io::NativeHandle, Elv::Io::EventMask, std::uint64_t,
	Elv::Io::PollFlags) { return false; }
bool WSAPollPoller::modify(Elv::Io::NativeHandle, Elv::Io::EventMask, std::uint64_t,
	Elv::Io::PollFlags) { return false; }
bool WSAPollPoller::remove(Elv::Io::NativeHandle) { return false; }
int WSAPollPoller::wait(std::span<Elv::Io::PollEvent>,
	std::optional<Elv::Io::EventDuration>) { return -1; }
bool WSAPollPoller::isValid() const { return false; }
const char* WSAPollPoller::backendName() const noexcept { return "WSAPoll (unavailable)"; }
int WSAPollPoller::lastError() const noexcept { return impl != nullptr ? impl->error : ENOSYS; }

#endif

} // namespace Platform
} // namespace Euph
