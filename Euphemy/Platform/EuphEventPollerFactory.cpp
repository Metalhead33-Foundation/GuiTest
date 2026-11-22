#include "EuphEventPollerFactory.hpp"
#include "EuphEpollPoller.hpp"
#include "EuphEventLoopWakeup.hpp"
#include "EuphIoUring.hpp"
#include "EuphKqueuePoller.hpp"
#include "EuphWSAPollPoller.hpp"
#include <Elvavena/Util/ElvPollingEventLoop.hpp>
#include <memory>

namespace Euph {
namespace Platform {

std::unique_ptr<Elv::Io::EventPoller> createEventPoller(PollerBackend backend) {
	if (backend == PollerBackend::Auto) {
#if defined(__linux__)
		backend = PollerBackend::Epoll;
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) \
	|| defined(__OpenBSD__) || defined(__DragonFly__)
		backend = PollerBackend::Kqueue;
#elif defined(_WIN32)
		backend = PollerBackend::WSAPoll;
#else
		return nullptr;
#endif
	}

	if (backend == PollerBackend::Epoll) {
		auto poller = std::make_unique<EpollPoller>();
		if (poller->isValid()) return poller;
		return nullptr;
	}
	if (backend == PollerBackend::Kqueue) {
		auto poller = std::make_unique<KqueuePoller>();
		if (poller->isValid()) return poller;
		return nullptr;
	}
	if (backend == PollerBackend::WSAPoll) {
		auto poller = std::make_unique<WSAPollPoller>();
		if (poller->isValid()) return poller;
		return nullptr;
	}
	return nullptr;
}

const char* defaultEventPollerName() noexcept {
#if defined(__linux__)
	return "epoll";
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) \
	|| defined(__OpenBSD__) || defined(__DragonFly__)
	return "kqueue";
#elif defined(_WIN32)
	return "WSAPoll";
#else
	return "none";
#endif
}

std::unique_ptr<IoUring> createIoUringIfAvailable(unsigned entries) {
	auto ring = std::make_unique<IoUring>(entries);
	if (!ring->isValid()) return nullptr;
	return ring;
}

std::unique_ptr<Elv::Util::PollingEventLoop> createPollingEventLoop(
	std::pmr::memory_resource* memoryResource) {
	auto poller = createEventPoller();
	auto wakeup = createWakeupEvent();
	if (!poller || !wakeup) return nullptr;
	auto loop = std::make_unique<Elv::Util::PollingEventLoop>(
		std::move(poller), std::move(wakeup), memoryResource);
	if (!loop->isValid()) return nullptr;
	return loop;
}

} // namespace Platform
} // namespace Euph
