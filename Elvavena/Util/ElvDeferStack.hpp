#ifndef ELVDEFERSTACK_HPP
#define ELVDEFERSTACK_HPP
/**
 * @file ElvDeferStack.hpp
 * @brief A fixed-capacity, non-allocating LIFO cleanup stack.
 *
 * Declarative control flow for state that is *not* owned by an object: an OS
 * handle released through a C API, a lock taken through a C interface, a
 * callback registration that has no RAII wrapper. The stack never allocates and
 * never throws, so it is safe on a failure path (including a path that is
 * already reporting an error through some other channel).
 *
 * Typical use:
 * @code
 * Elv::Util::DeferStack::Record storage[8];
 * Elv::Util::DeferStack stack(storage, 8);
 *
 * auto token = stack.defer(closeHandle, handle);   // records closeHandle(handle)
 * ... work that may fail ...
 * stack.dismiss(token);                            // normal path
 *
 * // failure path: run every record, most recent first
 * stack.runAll();
 * @endcode
 *
 * This header is part of the public declaration surface for Elvavena/Util.
 */
#include <cassert>
#include <cstdint>

namespace Elv {
namespace Util {

/**
 * @brief A fixed-capacity LIFO stack of cleanup thunks.
 *
 * Records are stored in caller-provided storage, so registration cannot fail by
 * allocation. `defer()` returns an invalid token when the storage is full; the
 * caller decides whether that is fatal for its use case.
 *
 * Tokens are indices, and dismissal is LIFO: dismissing a record that is not the
 * most recent one is a contract violation, because running the remaining thunks
 * out of order is how cleanup bugs become use-after-free bugs.
 */
class DeferStack
{
public:
	/// A cleanup thunk. It must not throw and must not re-enter the stack.
	using Thunk = void (*)(void*) noexcept;

	/// One record. Trivially copyable on purpose: the stack lives on a failure path.
	struct Record {
		Thunk fn = nullptr;
		void* arg = nullptr;
	};

	/// A registration token; `kInvalidIndex` means "not registered".
	struct Token {
		std::uint32_t index = kInvalidIndex;

		bool valid() const noexcept { return index != kInvalidIndex; }
	};

	static constexpr std::uint32_t kInvalidIndex = 0xFFFFFFFFu;

	/**
	 * @brief Builds a stack over `storage`.
	 *
	 * @param storage Caller-owned array of at least `capacity` records.
	 * @param capacity Number of records the stack may hold.
	 */
	DeferStack(Record* storage, std::uint32_t capacity) noexcept
		: records_(storage), capacity_(storage != nullptr ? capacity : 0)
	{
	}

	DeferStack(const DeferStack&) = delete;
	DeferStack& operator=(const DeferStack&) = delete;

	/**
	 * @brief Registers `fn(arg)`.
	 *
	 * Refused (invalid token) when the stack is full, when `fn` is null, or while
	 * a drain is in progress (a thunk that registers another thunk would make the
	 * drain unbounded).
	 */
	Token defer(Thunk fn, void* arg) noexcept
	{
		if (fn == nullptr || running_ || top_ >= capacity_)
			return Token{};
		records_[top_] = Record{fn, arg};
		return Token{top_++};
	}

	/**
	 * @brief Removes the most recent record.
	 *
	 * Returns false for an invalid, stale or non-top token. After `runAll()` the
	 * stack is empty, so a dismiss on the normal path that was already handled by
	 * the trap path is a harmless false.
	 */
	bool dismiss(Token token) noexcept
	{
		if (!token.valid() || top_ == 0 || token.index != top_ - 1)
			return false;
		--top_;
		return true;
	}

	/**
	 * @brief Runs every live record, most recent first, and empties the stack.
	 *
	 * Re-entrant calls are no-ops: the drain that is already running owns the
	 * stack. A thunk that throws terminates the process (the thunk is `noexcept`);
	 * a thunk that registers another record is refused by `defer()`.
	 */
	void runAll() noexcept
	{
		if (running_)
			return;
		running_ = true;
		while (top_ > 0) {
			const Record record = records_[--top_];
			records_[top_] = Record{};
			record.fn(record.arg);
		}
		running_ = false;
	}

	std::uint32_t size() const noexcept { return top_; }
	bool empty() const noexcept { return top_ == 0; }
	std::uint32_t capacity() const noexcept { return capacity_; }

private:
	Record* records_;
	std::uint32_t capacity_;
	std::uint32_t top_ = 0;
	bool running_ = false;
};

} // namespace Util
} // namespace Elv

#endif // ELVDEFERSTACK_HPP
