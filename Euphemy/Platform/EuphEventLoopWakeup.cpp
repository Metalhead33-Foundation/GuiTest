#include "EuphEventLoopWakeup.hpp"
#include <cerrno>
#include <cstdint>
#include <cstring>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/eventfd.h>
#endif
#endif

#ifndef ENOSYS
#define ENOSYS 38
#endif

namespace Euph {
namespace Platform {

#if defined(_WIN32)

struct WakeupEvent::Impl {
	SOCKET listener = INVALID_SOCKET;
	SOCKET sender = INVALID_SOCKET;
	bool started = false;
	int error = 0;

	~Impl() {
		if (listener != INVALID_SOCKET) closesocket(listener);
		if (sender != INVALID_SOCKET) closesocket(sender);
		if (started) WSACleanup();
	}
};

WakeupEvent::WakeupEvent() : impl(std::make_unique<Impl>()) {
	WSADATA winsock {};
	if (WSAStartup(MAKEWORD(2, 2), &winsock) != 0) {
		impl->error = WSAGetLastError();
		return;
	}
	impl->started = true;
	impl->listener = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	impl->sender = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (impl->listener == INVALID_SOCKET || impl->sender == INVALID_SOCKET) {
		impl->error = WSAGetLastError();
		return;
	}
	sockaddr_in address {};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = 0;
	if (bind(impl->listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
		impl->error = WSAGetLastError();
		return;
	}
	int addressLength = sizeof(address);
	if (getsockname(impl->listener, reinterpret_cast<sockaddr*>(&address), &addressLength) == SOCKET_ERROR) {
		impl->error = WSAGetLastError();
		return;
	}
	if (connect(impl->sender, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
		impl->error = WSAGetLastError();
		return;
	}
	u_long nonBlocking = 1;
	ioctlsocket(impl->listener, FIONBIO, &nonBlocking);
	ioctlsocket(impl->sender, FIONBIO, &nonBlocking);
}

WakeupEvent::~WakeupEvent() = default;

Elv::Io::NativeHandle WakeupEvent::nativeHandle() const noexcept {
	return isValid() ? static_cast<Elv::Io::NativeHandle>(impl->listener)
		: Elv::Io::InvalidNativeHandle;
}

bool WakeupEvent::notify() noexcept {
	if (!isValid()) return false;
	const char byte = 1;
	const int sent = send(impl->sender, &byte, 1, 0);
	if (sent == 1) return true;
	const int error = WSAGetLastError();
	if (error == WSAEWOULDBLOCK) return true; // Already signalled.
	impl->error = error;
	return false;
}

bool WakeupEvent::drain() noexcept {
	if (!isValid()) return false;
	char buffer[64];
	for (;;) {
		const int received = recv(impl->listener, buffer, sizeof(buffer), 0);
		if (received > 0) continue;
		if (received == 0) return true;
		const int error = WSAGetLastError();
		if (error == WSAEWOULDBLOCK) return true;
		impl->error = error;
		return false;
	}
}

bool WakeupEvent::isValid() const noexcept {
	return impl != nullptr && impl->started
		&& impl->listener != INVALID_SOCKET && impl->sender != INVALID_SOCKET;
}

int WakeupEvent::lastError() const noexcept { return impl != nullptr ? impl->error : 0; }

#elif defined(__unix__) || defined(__APPLE__)

namespace {

/**
 * @brief Sets non-blocking and close-on-exec flags.
 * @param descriptor File descriptor.
 * @return True on success.
 */
bool configureDescriptor(int descriptor) {
	const int flags = fcntl(descriptor, F_GETFL, 0);
	if (flags < 0 || fcntl(descriptor, F_SETFL, flags | O_NONBLOCK) < 0) return false;
	const int descriptorFlags = fcntl(descriptor, F_GETFD, 0);
	if (descriptorFlags < 0) return false;
	return fcntl(descriptor, F_SETFD, descriptorFlags | FD_CLOEXEC) == 0;
}

} // namespace

struct WakeupEvent::Impl {
	int readFd = -1;
	int writeFd = -1;
	bool eventFd = false;
	int error = 0;

	~Impl() {
		if (readFd >= 0) close(readFd);
		if (writeFd >= 0 && writeFd != readFd) close(writeFd);
	}
};

WakeupEvent::WakeupEvent() : impl(std::make_unique<Impl>()) {
#if defined(__linux__)
	impl->readFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (impl->readFd >= 0) {
		impl->writeFd = impl->readFd;
		impl->eventFd = true;
		return;
	}
#endif
	int descriptors[2] { -1, -1 };
	if (pipe(descriptors) != 0) {
		impl->error = errno;
		return;
	}
	if (!configureDescriptor(descriptors[0]) || !configureDescriptor(descriptors[1])) {
		impl->error = errno;
		close(descriptors[0]);
		close(descriptors[1]);
		return;
	}
	impl->readFd = descriptors[0];
	impl->writeFd = descriptors[1];
}

WakeupEvent::~WakeupEvent() = default;

Elv::Io::NativeHandle WakeupEvent::nativeHandle() const noexcept {
	return isValid() ? static_cast<Elv::Io::NativeHandle>(impl->readFd)
		: Elv::Io::InvalidNativeHandle;
}

bool WakeupEvent::notify() noexcept {
	if (!isValid()) return false;
	if (impl->eventFd) {
		const std::uint64_t one = 1;
		const ssize_t written = write(impl->writeFd, &one, sizeof(one));
		if (written == static_cast<ssize_t>(sizeof(one))) return true;
		if (written < 0 && errno == EAGAIN) return true;
		impl->error = errno;
		return false;
	}
	const char byte = 1;
	const ssize_t written = write(impl->writeFd, &byte, 1);
	if (written == 1) return true;
	if (written < 0 && errno == EAGAIN) return true;
	impl->error = errno;
	return false;
}

bool WakeupEvent::drain() noexcept {
	if (!isValid()) return false;
	if (impl->eventFd) {
		std::uint64_t value = 0;
		for (;;) {
			const ssize_t readCount = read(impl->readFd, &value, sizeof(value));
			if (readCount == static_cast<ssize_t>(sizeof(value))) continue;
			if (readCount == 0) return true;
			if (errno == EINTR) continue;
			if (errno == EAGAIN) return true;
			impl->error = errno;
			return false;
		}
	}
	char buffer[256];
	for (;;) {
		const ssize_t readCount = read(impl->readFd, buffer, sizeof(buffer));
		if (readCount > 0) continue;
		if (readCount == 0) return true;
		if (errno == EINTR) continue;
		if (errno == EAGAIN) return true;
		impl->error = errno;
		return false;
	}
}

bool WakeupEvent::isValid() const noexcept {
	return impl != nullptr && impl->readFd >= 0 && impl->writeFd >= 0;
}

int WakeupEvent::lastError() const noexcept { return impl != nullptr ? impl->error : 0; }

#else

struct WakeupEvent::Impl {
	int error = ENOSYS;
};

WakeupEvent::WakeupEvent() : impl(std::make_unique<Impl>()) {}
WakeupEvent::~WakeupEvent() = default;
Elv::Io::NativeHandle WakeupEvent::nativeHandle() const noexcept { return Elv::Io::InvalidNativeHandle; }
bool WakeupEvent::notify() noexcept { return false; }
bool WakeupEvent::drain() noexcept { return false; }
bool WakeupEvent::isValid() const noexcept { return false; }
int WakeupEvent::lastError() const noexcept { return impl != nullptr ? impl->error : ENOSYS; }

#endif

std::unique_ptr<Elv::Io::EventLoopWakeup> createWakeupEvent()
{
	auto wakeup = std::make_unique<WakeupEvent>();
	if (!wakeup->isValid()) return nullptr;
	return wakeup;
}

} // namespace Platform
} // namespace Euph

