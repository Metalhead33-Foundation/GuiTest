#include "EuphKqueuePoller.hpp"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <vector>

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) \
	|| defined(__OpenBSD__) || defined(__DragonFly__)
#define EUPH_HAS_KQUEUE 1
#include <sys/event.h>
#include <unistd.h>
#else
#define EUPH_HAS_KQUEUE 0
#endif

#ifndef ENOSYS
#define ENOSYS 38
#endif

namespace Euph {
namespace Platform {

#if EUPH_HAS_KQUEUE

namespace {

/** @brief Returns the kqueue change flags for a registration. */
std::uint16_t toKqueueFlags(Elv::Io::PollFlags flags) {
	std::uint16_t value = EV_ADD | EV_ENABLE;
	if (Elv::Io::hasFlag(flags, Elv::Io::PollFlags::EdgeTriggered)) value |= EV_CLEAR;
	if (Elv::Io::hasFlag(flags, Elv::Io::PollFlags::OneShot)) value |= EV_ONESHOT;
	return value;
}

/** @brief Returns whether a read filter is needed. */
bool needsReadFilter(Elv::Io::EventMask interest) {
	return Elv::Io::hasEvent(interest, Elv::Io::EventMask::Read)
		|| Elv::Io::hasEvent(interest, Elv::Io::EventMask::Error)
		|| Elv::Io::hasEvent(interest, Elv::Io::EventMask::Hangup);
}

/** @brief Converts a kqueue event to the abstract mask. */
Elv::Io::EventMask fromKqueue(const struct kevent& event) {
	Elv::Io::EventMask mask = Elv::Io::EventMask::None;
	if (event.filter == EVFILT_READ) mask |= Elv::Io::EventMask::Read;
	if (event.filter == EVFILT_WRITE) mask |= Elv::Io::EventMask::Write;
	if ((event.flags & EV_EOF) != 0) mask |= Elv::Io::EventMask::Hangup;
	if ((event.flags & EV_ERROR) != 0) mask |= Elv::Io::EventMask::Error;
	return mask;
}

/** @brief Applies one kqueue change and reports failure. */
bool applyChange(int descriptor, int handle, std::int16_t filter, std::uint16_t flags,
	std::uint64_t token, int& error) {
	struct kevent change;
	EV_SET(&change, static_cast<std::uintptr_t>(handle), filter, flags, 0, 0,
		reinterpret_cast<void*>(static_cast<std::uintptr_t>(token)));
	if (kevent(descriptor, &change, 1, nullptr, 0, nullptr) < 0) {
		error = errno;
		return false;
	}
	return true;
}

/** @brief Removes one filter, tolerating an absent filter. */
void removeFilter(int descriptor, int handle, std::int16_t filter) {
	struct kevent change;
	EV_SET(&change, static_cast<std::uintptr_t>(handle), filter, EV_DELETE, 0, 0, nullptr);
	kevent(descriptor, &change, 1, nullptr, 0, nullptr);
}

/** @brief Removes every filter implied by an interest mask. */
void removeFilters(int descriptor, int handle, Elv::Io::EventMask interest) {
	if (needsReadFilter(interest)) removeFilter(descriptor, handle, EVFILT_READ);
	if (Elv::Io::hasEvent(interest, Elv::Io::EventMask::Write)) {
		removeFilter(descriptor, handle, EVFILT_WRITE);
	}
}

} // namespace

struct KqueuePoller::Impl {
	struct Entry {
		std::uint64_t token = 0;
		Elv::Io::EventMask interest = Elv::Io::EventMask::None;
		Elv::Io::PollFlags flags = Elv::Io::PollFlags::None;
	};

	int descriptor = -1;
	int error = 0;
	std::unordered_map<int, Entry> entries;
};

KqueuePoller::KqueuePoller() : impl(std::make_unique<Impl>()) {
	impl->descriptor = kqueue();
	if (impl->descriptor < 0) impl->error = errno;
}

KqueuePoller::~KqueuePoller() {
	if (impl != nullptr && impl->descriptor >= 0) close(impl->descriptor);
}

bool KqueuePoller::add(Elv::Io::NativeHandle handle, Elv::Io::EventMask interest,
	std::uint64_t token, Elv::Io::PollFlags flags) {
	if (impl == nullptr || impl->descriptor < 0 || handle < 0
		|| interest == Elv::Io::EventMask::None) {
		return false;
	}
	if (impl->entries.find(static_cast<int>(handle)) != impl->entries.end()) return false;
	const std::uint16_t changeFlags = toKqueueFlags(flags);
	if (needsReadFilter(interest)
		&& !applyChange(impl->descriptor, static_cast<int>(handle), EVFILT_READ,
			changeFlags, token, impl->error)) {
		return false;
	}
	if (Elv::Io::hasEvent(interest, Elv::Io::EventMask::Write)
		&& !applyChange(impl->descriptor, static_cast<int>(handle), EVFILT_WRITE,
			changeFlags, token, impl->error)) {
		removeFilters(impl->descriptor, static_cast<int>(handle), interest);
		return false;
	}
	impl->entries[static_cast<int>(handle)] = Impl::Entry { token, interest, flags };
	return true;
}

bool KqueuePoller::modify(Elv::Io::NativeHandle handle, Elv::Io::EventMask interest,
	std::uint64_t token, Elv::Io::PollFlags flags) {
	if (impl == nullptr || impl->descriptor < 0 || handle < 0) return false;
	const auto entry = impl->entries.find(static_cast<int>(handle));
	if (entry == impl->entries.end()) return false;
	removeFilters(impl->descriptor, static_cast<int>(handle), entry->second.interest);
	const std::uint16_t changeFlags = toKqueueFlags(flags);
	if (needsReadFilter(interest)
		&& !applyChange(impl->descriptor, static_cast<int>(handle), EVFILT_READ,
			changeFlags, token, impl->error)) {
		impl->entries.erase(entry);
		return false;
	}
	if (Elv::Io::hasEvent(interest, Elv::Io::EventMask::Write)
		&& !applyChange(impl->descriptor, static_cast<int>(handle), EVFILT_WRITE,
			changeFlags, token, impl->error)) {
		removeFilters(impl->descriptor, static_cast<int>(handle), interest);
		impl->entries.erase(entry);
		return false;
	}
	entry->second = Impl::Entry { token, interest, flags };
	return true;
}

bool KqueuePoller::remove(Elv::Io::NativeHandle handle) {
	if (impl == nullptr || impl->descriptor < 0 || handle < 0) return false;
	const auto entry = impl->entries.find(static_cast<int>(handle));
	if (entry == impl->entries.end()) return false;
	removeFilters(impl->descriptor, static_cast<int>(handle), entry->second.interest);
	impl->entries.erase(entry);
	return true;
}


int KqueuePoller::wait(std::span<Elv::Io::PollEvent> events,
	std::optional<Elv::Io::EventDuration> timeout) {
	if (impl == nullptr || impl->descriptor < 0) return -1;
	if (events.empty()) return 0;

	timespec specification {};
	timespec* specificationPtr = nullptr;
	if (timeout.has_value()) {
		const auto nanoseconds = std::chrono::ceil<std::chrono::nanoseconds>(*timeout).count();
		const auto safe = std::max<std::int64_t>(nanoseconds, 0);
		specification.tv_sec = static_cast<time_t>(safe / 1000000000LL);
		specification.tv_nsec = static_cast<long>(safe % 1000000000LL);
		specificationPtr = &specification;
	}

	std::vector<struct kevent> rawEvents(events.size());
	const int ready = kevent(impl->descriptor, nullptr, 0, rawEvents.data(),
		static_cast<int>(rawEvents.size()), specificationPtr);
	if (ready < 0) {
		impl->error = errno;
		return -1;
	}
	for (int index = 0; index < ready; ++index) {
		const struct kevent& raw = rawEvents[static_cast<std::size_t>(index)];
		const int handle = static_cast<int>(raw.ident);
		Elv::Io::EventMask mask = fromKqueue(raw);
		if ((raw.flags & EV_ERROR) != 0) impl->error = static_cast<int>(raw.data);
		const auto entry = impl->entries.find(handle);
		events[static_cast<std::size_t>(index)] = Elv::Io::PollEvent {
			static_cast<Elv::Io::NativeHandle>(handle),
			entry != impl->entries.end() ? entry->second.token
				: static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(raw.udata)),
			mask
		};
		if (entry != impl->entries.end()
			&& Elv::Io::hasFlag(entry->second.flags, Elv::Io::PollFlags::OneShot)) {
			impl->entries.erase(entry);
		}
	}
	return ready;
}

bool KqueuePoller::isValid() const { return impl != nullptr && impl->descriptor >= 0; }
const char* KqueuePoller::backendName() const noexcept { return "kqueue"; }
int KqueuePoller::lastError() const noexcept { return impl != nullptr ? impl->error : 0; }

#else

struct KqueuePoller::Impl {
	int error = ENOSYS;
};

KqueuePoller::KqueuePoller() : impl(std::make_unique<Impl>()) {}
KqueuePoller::~KqueuePoller() = default;
bool KqueuePoller::add(Elv::Io::NativeHandle, Elv::Io::EventMask, std::uint64_t,
	Elv::Io::PollFlags) { return false; }
bool KqueuePoller::modify(Elv::Io::NativeHandle, Elv::Io::EventMask, std::uint64_t,
	Elv::Io::PollFlags) { return false; }
bool KqueuePoller::remove(Elv::Io::NativeHandle) { return false; }
int KqueuePoller::wait(std::span<Elv::Io::PollEvent>,
	std::optional<Elv::Io::EventDuration>) { return -1; }
bool KqueuePoller::isValid() const { return false; }
const char* KqueuePoller::backendName() const noexcept { return "kqueue (unavailable)"; }
int KqueuePoller::lastError() const noexcept { return impl != nullptr ? impl->error : ENOSYS; }

#endif

} // namespace Platform
} // namespace Euph

