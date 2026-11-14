#ifndef ELVTHREADSAFEQUEUE_HPP
#define ELVTHREADSAFEQUEUE_HPP
/**
 * @file ElvThreadsafeQueue.hpp
 * @brief Declares the ElvThreadsafeQueue API in the Elvavena/Util module.
 *
 * This header is part of the public declaration surface for Elvavena/Util.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */

#include <chrono>
#include <concepts>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace Elv {
namespace Util {

/**
 * @class ThreadsafeQueue
 * @brief A blocking, thread-safe FIFO/LIFO queue built on top of `std::deque`.
 *
 * @tparam T Element type stored in the queue.
 * @tparam Allocator Allocator type used by the underlying `std::deque`.
 *
 * @note All operations internally synchronize on a single mutex.
 * @note Waiting APIs use a condition variable with predicates (spurious wakeups safe).
 * @note Accessors (`front`, `back`) return values, not references, to avoid lifetime races.
 */
template <class T, class Allocator = std::allocator<T>>
class ThreadsafeQueue {
public:
	/// Reference type for `T`.
	using ref = T&;
	/// Const reference type for `T`.
	using const_ref = const T&;
	/// Rvalue reference type for `T`.
	using mov_ref = T&&;
	/// Underlying queue type.
	using Queue = std::deque<T, Allocator>;
	/// Internal mutex type.
	using Mutex = std::mutex;
	/// Internal unique lock type.
	using UniqueLock = std::unique_lock<Mutex>;

private:
	Queue queue;
	mutable Mutex mutex;
	mutable std::condition_variable cvBlock;

	/**
	 * @brief Throws `std::out_of_range` if the queue is empty.
	 * @param functionName Text used in the exception message.
	 * @throws std::out_of_range If queue has no elements.
	 */
	void throwIfEmpty(const char* functionName) const {
		if(queue.empty()) {
			throw std::out_of_range(functionName);
		}
	}

public:
	/** @brief Constructs an empty queue. */
	ThreadsafeQueue() = default;

	/**
	 * @brief Copy-constructs from another queue.
	 * @param cpy Source queue.
	 *
	 * @note Locks the source queue mutex during the copy.
	 */
	ThreadsafeQueue(const ThreadsafeQueue& cpy) {
		std::scoped_lock lock(cpy.mutex);
		queue = cpy.queue;
	}

	/**
	 * @brief Move-constructs from another queue.
	 * @param mov Source queue.
	 *
	 * @note Locks the source queue mutex during the move.
	 */
	ThreadsafeQueue(ThreadsafeQueue&& mov) noexcept {
		std::scoped_lock lock(mov.mutex);
		queue = std::move(mov.queue);
	}

	/**
	 * @brief Constructs from an existing `Queue` copy.
	 * @param cpy Source container.
	 */
	explicit ThreadsafeQueue(const Queue& cpy) : queue(cpy) {}

	/**
	 * @brief Constructs from an existing `Queue` by move.
	 * @param mov Source container.
	 */
	explicit ThreadsafeQueue(Queue&& mov) : queue(std::move(mov)) {}

	/**
	 * @brief Copy-assigns from another queue.
	 * @param cpy Source queue.
	 * @return `*this`.
	 *
	 * @note Uses a scoped lock over both queue mutexes.
	 */
	ThreadsafeQueue& operator=(const ThreadsafeQueue& cpy) {
		if(this == &cpy) return *this;
		std::scoped_lock lock(mutex, cpy.mutex);
		queue = cpy.queue;
		return *this;
	}

	/**
	 * @brief Move-assigns from another queue.
	 * @param mov Source queue.
	 * @return `*this`.
	 *
	 * @note Uses a scoped lock over both queue mutexes.
	 */
	ThreadsafeQueue& operator=(ThreadsafeQueue&& mov) noexcept {
		if(this == &mov) return *this;
		std::scoped_lock lock(mutex, mov.mutex);
		queue = std::move(mov.queue);
		return *this;
	}

	/** @brief Destructor. */
	~ThreadsafeQueue() = default;

	/**
	 * @brief Executes a callable while holding the queue lock (mutable access).
	 * @tparam F Callable type.
	 * @param function Callable invoked as `function(Queue&)`.
	 * @return Whatever `function` returns.
	 */
	template <typename F>
	requires std::invocable<F&, Queue&>
	decltype(auto) operate(F&& function) {
		std::scoped_lock lock(mutex);
		return std::forward<F>(function)(queue);
	}

	/**
	 * @brief Executes a callable while holding the queue lock (const access).
	 * @tparam F Callable type.
	 * @param function Callable invoked as `function(const Queue&)`.
	 * @return Whatever `function` returns.
	 */
	template <typename F>
	requires std::invocable<F&, const Queue&>
	decltype(auto) operate(F&& function) const {
		std::scoped_lock lock(mutex);
		return std::forward<F>(function)(queue);
	}

	/**
	 * @brief Returns number of elements currently stored.
	 * @return Queue size.
	 */
	[[nodiscard]] size_t size() const {
		std::scoped_lock lock(mutex);
		return queue.size();
	}

	/** @brief Removes all elements. */
	void clear() {
		std::scoped_lock lock(mutex);
		queue.clear();
	}

	/**
	 * @brief Checks whether the queue is empty.
	 * @return `true` if empty, otherwise `false`.
	 */
	[[nodiscard]] bool empty() const {
		std::scoped_lock lock(mutex);
		return queue.empty();
	}

	/**
	 * @brief Blocks until the queue becomes non-empty.
	 */
	void wait() {
		UniqueLock lock(mutex);
		cvBlock.wait(lock, [this] { return !queue.empty(); });
	}

	/**
	 * @brief Waits until non-empty or timeout.
	 * @tparam Rep Duration representation type.
	 * @tparam Period Duration period type.
	 * @param timeout Max duration to wait.
	 * @return `true` if the queue became non-empty, `false` on timeout.
	 */
	template <class Rep, class Period>
	[[nodiscard]] bool wait_for(const std::chrono::duration<Rep, Period>& timeout) {
		UniqueLock lock(mutex);
		return cvBlock.wait_for(lock, timeout, [this] { return !queue.empty(); });
	}

	/**
	 * @brief Returns a copy of the last element.
	 * @return Last element.
	 * @throws std::out_of_range If queue is empty.
	 */
	[[nodiscard]] T back() {
		std::scoped_lock lock(mutex);
		throwIfEmpty("ThreadsafeQueue::back - queue is empty");
		return queue.back();
	}

	/**
	 * @brief Returns a copy of the last element.
	 * @return Last element.
	 * @throws std::out_of_range If queue is empty.
	 */
	[[nodiscard]] T back() const {
		std::scoped_lock lock(mutex);
		throwIfEmpty("ThreadsafeQueue::back - queue is empty");
		return queue.back();
	}

	/**
	 * @brief Returns a copy of the first element.
	 * @return First element.
	 * @throws std::out_of_range If queue is empty.
	 */
	[[nodiscard]] T front() {
		std::scoped_lock lock(mutex);
		throwIfEmpty("ThreadsafeQueue::front - queue is empty");
		return queue.front();
	}

	/**
	 * @brief Returns a copy of the first element.
	 * @return First element.
	 * @throws std::out_of_range If queue is empty.
	 */
	[[nodiscard]] T front() const {
		std::scoped_lock lock(mutex);
		throwIfEmpty("ThreadsafeQueue::front - queue is empty");
		return queue.front();
	}

	/**
	 * @brief Appends an element to the back.
	 * @tparam U Value type forwarded into `T`.
	 * @param val Value to insert.
	 */
	template <typename U>
	requires std::constructible_from<T, U&&>
	void push_back(U&& val) {
		{
			std::scoped_lock lock(mutex);
			queue.emplace_back(std::forward<U>(val));
		}
		cvBlock.notify_one();
	}

	/**
	 * @brief Prepends an element to the front.
	 * @tparam U Value type forwarded into `T`.
	 * @param val Value to insert.
	 */
	template <typename U>
	requires std::constructible_from<T, U&&>
	void push_front(U&& val) {
		{
			std::scoped_lock lock(mutex);
			queue.emplace_front(std::forward<U>(val));
		}
		cvBlock.notify_one();
	}

	/**
	 * @brief Constructs an element in-place at the back.
	 * @tparam Args Constructor argument types.
	 * @param args Arguments forwarded to `T` constructor.
	 */
	template <typename... Args>
	requires std::constructible_from<T, Args&&...>
	void emplace_back(Args&&... args) {
		{
			std::scoped_lock lock(mutex);
			queue.emplace_back(std::forward<Args>(args)...);
		}
		cvBlock.notify_one();
	}

	/**
	 * @brief Constructs an element in-place at the front.
	 * @tparam Args Constructor argument types.
	 * @param args Arguments forwarded to `T` constructor.
	 */
	template <typename... Args>
	requires std::constructible_from<T, Args&&...>
	void emplace_front(Args&&... args) {
		{
			std::scoped_lock lock(mutex);
			queue.emplace_front(std::forward<Args>(args)...);
		}
		cvBlock.notify_one();
	}

	/**
	 * @brief Removes last element.
	 * @throws std::out_of_range If queue is empty.
	 */
	void delete_back() {
		std::scoped_lock lock(mutex);
		throwIfEmpty("ThreadsafeQueue::delete_back - queue is empty");
		queue.pop_back();
	}

	/**
	 * @brief Removes first element.
	 * @throws std::out_of_range If queue is empty.
	 */
	void delete_front() {
		std::scoped_lock lock(mutex);
		throwIfEmpty("ThreadsafeQueue::delete_front - queue is empty");
		queue.pop_front();
	}

	/**
	 * @brief Tries to remove the last element.
	 * @return `true` on success, `false` if empty.
	 */
	[[nodiscard]] bool try_delete_back() {
		std::scoped_lock lock(mutex);
		if(queue.empty()) return false;
		queue.pop_back();
		return true;
	}

	/**
	 * @brief Tries to remove the first element.
	 * @return `true` on success, `false` if empty.
	 */
	[[nodiscard]] bool try_delete_front() {
		std::scoped_lock lock(mutex);
		if(queue.empty()) return false;
		queue.pop_front();
		return true;
	}

	/**
	 * @brief Moves the last element into `target` and removes it.
	 * @param target Destination object.
	 * @throws std::out_of_range If queue is empty.
	 */
	void pop_back(ref target) {
		std::scoped_lock lock(mutex);
		throwIfEmpty("ThreadsafeQueue::pop_back - queue is empty");
		target = std::move(queue.back());
		queue.pop_back();
	}

	/**
	 * @brief Moves the first element into `target` and removes it.
	 * @param target Destination object.
	 * @throws std::out_of_range If queue is empty.
	 */
	void pop_front(ref target) {
		std::scoped_lock lock(mutex);
		throwIfEmpty("ThreadsafeQueue::pop_front - queue is empty");
		target = std::move(queue.front());
		queue.pop_front();
	}

	/**
	 * @brief Tries to move last element into `target`.
	 * @param target Destination object.
	 * @return `true` on success, `false` if empty.
	 */
	[[nodiscard]] bool try_pop_back(ref target) {
		std::scoped_lock lock(mutex);
		if(queue.empty()) return false;
		target = std::move(queue.back());
		queue.pop_back();
		return true;
	}

	/**
	 * @brief Tries to move first element into `target`.
	 * @param target Destination object.
	 * @return `true` on success, `false` if empty.
	 */
	[[nodiscard]] bool try_pop_front(ref target) {
		std::scoped_lock lock(mutex);
		if(queue.empty()) return false;
		target = std::move(queue.front());
		queue.pop_front();
		return true;
	}

	/**
	 * @brief Tries to pop and return last element.
	 * @return Popped value or `std::nullopt` if empty.
	 */
	[[nodiscard]] std::optional<T> try_pop_back() {
		std::scoped_lock lock(mutex);
		if(queue.empty()) return std::nullopt;
		T tmp = std::move(queue.back());
		queue.pop_back();
		return tmp;
	}

	/**
	 * @brief Tries to pop and return first element.
	 * @return Popped value or `std::nullopt` if empty.
	 */
	[[nodiscard]] std::optional<T> try_pop_front() {
		std::scoped_lock lock(mutex);
		if(queue.empty()) return std::nullopt;
		T tmp = std::move(queue.front());
		queue.pop_front();
		return tmp;
	}

	/**
	 * @brief Blocks until non-empty, then pops first element.
	 * @return Popped element.
	 */
	[[nodiscard]] T wait_pop_front() {
		UniqueLock lock(mutex);
		cvBlock.wait(lock, [this] { return !queue.empty(); });
		T tmp = std::move(queue.front());
		queue.pop_front();
		return tmp;
	}

	/**
	 * @brief Blocks until non-empty, then pops last element.
	 * @return Popped element.
	 */
	[[nodiscard]] T wait_pop_back() {
		UniqueLock lock(mutex);
		cvBlock.wait(lock, [this] { return !queue.empty(); });
		T tmp = std::move(queue.back());
		queue.pop_back();
		return tmp;
	}

	/**
	 * @brief Waits up to `timeout` for an element, then pops first.
	 * @tparam Rep Duration representation type.
	 * @tparam Period Duration period type.
	 * @param timeout Max duration to wait.
	 * @return Popped value or `std::nullopt` on timeout.
	 */
	template <class Rep, class Period>
	[[nodiscard]] std::optional<T> wait_pop_front_for(const std::chrono::duration<Rep, Period>& timeout) {
		UniqueLock lock(mutex);
		if(!cvBlock.wait_for(lock, timeout, [this] { return !queue.empty(); })) {
			return std::nullopt;
		}
		T tmp = std::move(queue.front());
		queue.pop_front();
		return tmp;
	}

	/**
	 * @brief Waits up to `timeout` for an element, then pops last.
	 * @tparam Rep Duration representation type.
	 * @tparam Period Duration period type.
	 * @param timeout Max duration to wait.
	 * @return Popped value or `std::nullopt` on timeout.
	 */
	template <class Rep, class Period>
	[[nodiscard]] std::optional<T> wait_pop_back_for(const std::chrono::duration<Rep, Period>& timeout) {
		UniqueLock lock(mutex);
		if(!cvBlock.wait_for(lock, timeout, [this] { return !queue.empty(); })) {
			return std::nullopt;
		}
		T tmp = std::move(queue.back());
		queue.pop_back();
		return tmp;
	}

	/**
	 * @brief Pops and returns last element.
	 * @return Popped value.
	 * @throws std::out_of_range If queue is empty.
	 */
	[[nodiscard]] T pop_back() {
		std::scoped_lock lock(mutex);
		throwIfEmpty("ThreadsafeQueue::pop_back - queue is empty");
		T tmp = std::move(queue.back());
		queue.pop_back();
		return tmp;
	}

	/**
	 * @brief Pops and returns first element.
	 * @return Popped value.
	 * @throws std::out_of_range If queue is empty.
	 */
	[[nodiscard]] T pop_front() {
		std::scoped_lock lock(mutex);
		throwIfEmpty("ThreadsafeQueue::pop_front - queue is empty");
		T tmp = std::move(queue.front());
		queue.pop_front();
		return tmp;
	}
};

} // namespace Util
} // namespace Elv

#endif // ELVTHREADSAFEQUEUE_HPP
