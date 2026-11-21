#include "EuphEventPollerFactory.hpp"
#include "EuphEpollPoller.hpp"
#include "EuphIoUring.hpp"
#include "EuphWSAPollPoller.hpp"
#include <memory>

namespace Euph {
namespace Platform {

std::unique_ptr<Elv::Io::EventPoller> createEventPoller(PollerBackend backend) {
	if (backend == PollerBackend::Auto) {
#if defined(__linux__)
		backend = PollerBackend::Epoll;
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

} // namespace Platform
} // namespace Euph
