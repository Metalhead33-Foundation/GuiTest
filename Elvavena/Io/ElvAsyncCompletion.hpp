#ifndef ELVASYNCCOMPLETION_HPP
#define ELVASYNCCOMPLETION_HPP
/**
 * @file ElvAsyncCompletion.hpp
 * @brief Backend-neutral completion record for io_uring and IOCP wrappers.
 *
 * Completion-based engines return the caller's user data, a result code and
 * platform flags. The common fields here cover both io_uring's `cqe->res/flags`
 * and IOCP's transferred-byte/overlapped results without exposing either
 * platform header.
 */
#include <cstdint>

namespace Elv {
namespace Io {

/**
 * @struct AsyncCompletion
 * @brief One completed asynchronous operation.
 */
struct AsyncCompletion {
	std::uint64_t userData = 0;       ///< Caller token from the submission.
	int result = 0;                   ///< Bytes/0 on success, negative error otherwise.
	std::uint32_t flags = 0;          ///< Backend-specific completion flags.
	std::uint32_t bytesTransferred = 0; ///< IOCP byte count, or io_uring's positive result.
};

} // namespace Io
} // namespace Elv

#endif // ELVASYNCCOMPLETION_HPP
