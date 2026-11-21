#include "EuphIocp.hpp"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdint>
#ifndef ENOSYS
#define ENOSYS 38
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#endif

namespace Euph {
namespace Platform {

#ifdef _WIN32

struct Iocp::Impl {
	HANDLE port = INVALID_HANDLE_VALUE;
	int error = 0;
};

Iocp::Iocp(unsigned concurrency, unsigned) : impl(std::make_unique<Impl>()) {
	impl->port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0,
		static_cast<DWORD>(concurrency));
	if (impl->port == nullptr) impl->error = static_cast<int>(GetLastError());
}

Iocp::~Iocp() {
	if (impl != nullptr && impl->port != nullptr) CloseHandle(impl->port);
}

bool Iocp::associate(Elv::Io::NativeHandle handle, std::uint64_t key) {
	if (impl == nullptr || impl->port == nullptr) return false;
	const HANDLE result = CreateIoCompletionPort(reinterpret_cast<HANDLE>(handle),
		impl->port, static_cast<ULONG_PTR>(key), 0);
	if (result == nullptr) {
		impl->error = static_cast<int>(GetLastError());
		return false;
	}
	return true;
}

bool Iocp::post(std::uint64_t key, std::uint32_t bytes, void* overlapped) {
	if (impl == nullptr || impl->port == nullptr) return false;
	if (!PostQueuedCompletionStatus(impl->port, static_cast<DWORD>(bytes),
		static_cast<ULONG_PTR>(key), static_cast<LPOVERLAPPED>(overlapped))) {
		impl->error = static_cast<int>(GetLastError());
		return false;
	}
	return true;
}

int Iocp::wait(std::span<IocpCompletion> completions,
	std::optional<Elv::Io::EventDuration> timeout) {
	if (impl == nullptr || impl->port == nullptr) return -1;
	if (completions.empty()) return 0;

	DWORD timeoutMs = INFINITE;
	if (timeout.has_value()) {
		const auto milliseconds = std::chrono::ceil<std::chrono::milliseconds>(*timeout).count();
		timeoutMs = static_cast<DWORD>(std::clamp<long long>(milliseconds, 0, INT_MAX));
	}

	std::size_t written = 0;
	for (;;) {
		DWORD bytes = 0;
		ULONG_PTR key = 0;
		LPOVERLAPPED overlapped = nullptr;
		const BOOL ok = GetQueuedCompletionStatus(impl->port, &bytes, &key,
			&overlapped, written == 0 ? timeoutMs : 0);
		if (!ok && overlapped == nullptr) {
			const int error = static_cast<int>(GetLastError());
			if (error == WAIT_TIMEOUT || error == ERROR_ABANDONED_WAIT_0) break;
			impl->error = error;
			return written != 0 ? static_cast<int>(written) : -1;
		}
		completions[written] = IocpCompletion {
			static_cast<std::uint64_t>(key),
			static_cast<std::uint32_t>(bytes),
			overlapped,
			ok ? 0 : static_cast<int>(GetLastError())
		};
		++written;
		if (written >= completions.size()) break;
	}
	return static_cast<int>(written);
}

bool Iocp::isValid() const { return impl != nullptr && impl->port != nullptr; }
int Iocp::lastError() const noexcept { return impl != nullptr ? impl->error : 0; }
const char* Iocp::backendName() const noexcept { return "IOCP"; }

#else

struct Iocp::Impl {
	int error = ENOSYS;
};

Iocp::Iocp(unsigned, unsigned) : impl(std::make_unique<Impl>()) {}
Iocp::~Iocp() = default;
bool Iocp::associate(Elv::Io::NativeHandle, std::uint64_t) { return false; }
bool Iocp::post(std::uint64_t, std::uint32_t, void*) { return false; }
int Iocp::wait(std::span<IocpCompletion>, std::optional<Elv::Io::EventDuration>) { return -1; }
bool Iocp::isValid() const { return false; }
int Iocp::lastError() const noexcept { return impl != nullptr ? impl->error : ENOSYS; }
const char* Iocp::backendName() const noexcept { return "IOCP (unavailable)"; }

#endif

} // namespace Platform
} // namespace Euph
