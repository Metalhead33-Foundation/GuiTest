#ifndef EUPHIOCP_HPP
#define EUPHIOCP_HPP
/**
 * @file EuphIocp.hpp
 * @brief Convenience RAII wrapper around Windows I/O completion ports.
 *
 * IOCP is the Windows completion-based counterpart to io_uring: associate
 * handles with a completion port, submit overlapped operations, and wait for
 * completion packets. On non-Windows builds the class exists but reports
 * `isValid() == false`; portable code can select epoll/io_uring instead.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvEventPoller.hpp>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>

namespace Euph {
namespace Platform {

/**
 * @struct IocpCompletion
 * @brief One Windows completion packet.
 */
struct IocpCompletion {
	std::uint64_t key = 0;       ///< Completion key supplied when associating/posting.
	std::uint32_t bytes = 0;     ///< Transferred bytes.
	void* overlapped = nullptr;  ///< OVERLAPPED pointer, if any.
	int error = 0;               ///< Win32 error for a failed overlapped operation.
};

/**
 * @class Iocp
 * @brief RAII wrapper around a Windows I/O completion port.
 */
class MH_EUPH_API Iocp {
public:
	/**
	 * @brief Creates a completion port.
	 * @param concurrency Maximum concurrent threads; zero lets Windows choose.
	 * @param flags Reserved for `CreateIoCompletionPort` flags; pass zero.
	 */
	explicit Iocp(unsigned concurrency = 0, unsigned flags = 0);

	/** @brief Closes the completion port. */
	~Iocp();

	Iocp(const Iocp&) = delete;
	Iocp& operator=(const Iocp&) = delete;
	Iocp(Iocp&&) = delete;
	Iocp& operator=(Iocp&&) = delete;

	/**
	 * @brief Associates a file/socket handle with the port.
	 * @param handle Native HANDLE/SOCKET.
	 * @param key Completion key returned with packets.
	 * @return True on success.
	 */
	bool associate(Elv::Io::NativeHandle handle, std::uint64_t key);

	/**
	 * @brief Posts a user-defined completion packet.
	 * @param key Completion key.
	 * @param bytes Transferred-byte value.
	 * @param overlapped Optional OVERLAPPED pointer.
	 * @return True on success.
	 */
	bool post(std::uint64_t key, std::uint32_t bytes = 0, void* overlapped = nullptr);

	/**
	 * @brief Waits for completion packets.
	 * @param completions Destination span.
	 * @param timeout Empty for infinite, zero for non-blocking.
	 * @return Number written, or -1 on error.
	 */
	int wait(std::span<IocpCompletion> completions,
		std::optional<Elv::Io::EventDuration> timeout);

	/** @brief Returns whether the port was created. */
	[[nodiscard]] bool isValid() const;

	/** @brief Returns the last Win32 error, or zero. */
	[[nodiscard]] int lastError() const noexcept;

	/** @brief Returns the backend name. */
	[[nodiscard]] const char* backendName() const noexcept;

private:
	struct Impl;                ///< Hidden native state.
	std::unique_ptr<Impl> impl; ///< Pimpl.
};

} // namespace Platform
} // namespace Euph

#endif // EUPHIOCP_HPP
