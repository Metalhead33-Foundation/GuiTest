#ifndef ELVTHREADSAFEQUEUE_HPP
#define ELVTHREADSAFEQUEUE_HPP
#include <deque>
#include <mutex>
#include <functional>
#include <condition_variable>
namespace Elv {
namespace Util {
/**
 * @class ThreadsafeQueue
 * @brief A thread-safe queue implementation using a std::deque and mutexes.
 *
 * @tparam T The type of elements stored in the queue.
 * @tparam Allocator The allocator type for the underlying deque (default: std::allocator<T>).
 */
template<class T, class Allocator = std::allocator<T>>
class ThreadsafeQueue {
public:
	/**
	 * @name Type Aliases
	 * @{
	 */
	/// Reference to an element of type T
	typedef T& ref;
	/// Constant reference to an element of type T
	typedef const T& const_ref;
	/// Rvalue reference to an element of type T
	typedef T&& mov_ref;
	/// Type of the underlying queue (std::deque with specified Allocator)
	typedef std::deque<T, Allocator> Queue;
	/// Function type for modifying the queue
	typedef std::function<void(Queue&)> QueueOperator;
	/// Function type for accessing the queue (const)
	typedef std::function<void(const Queue&)> QueueConstOperator;
	/// Mutex type for synchronization
	typedef std::mutex Mutex;
	/// Lock guard type for exclusive access
	typedef std::lock_guard<Mutex> Lock;
	/// Unique lock type for conditional access
	typedef std::unique_lock<Mutex> UniqueLock;
	/// @}

protected:
	/// The underlying queue storing elements of type T
	Queue queue;
	/// Mutex for protecting queue access
	mutable Mutex mutex;
	/// Mutex for blocking operations (e.g., wait)
	mutable Mutex blocker;
	/// Condition variable for signaling blocking operations
	mutable std::condition_variable cvBlock;

public:
	/**
	 * @name Constructors
	 * @{
	 */
	/**
	 * Default constructor.
	 */
	ThreadsafeQueue() {}

	/**
	 * Copy constructor.
	 * @param cpy The ThreadsafeQueue instance to copy from.
	 */
	ThreadsafeQueue(const ThreadsafeQueue& cpy) {
		this->queue = cpy.queue;
	}

	/**
	 * Move constructor.
	 * @param mov The ThreadsafeQueue instance to move from.
	 */
	ThreadsafeQueue(ThreadsafeQueue&& mov) {
		this->queue = std::move(mov.queue);
	}

	/**
	 * Constructor from a const Queue reference.
	 * @param cpy The Queue instance to copy from.
	 */
	ThreadsafeQueue(const Queue& cpy) {
		this->queue = cpy;
	}

	/**
	 * Constructor from a Queue rvalue reference.
	 * @param mov The Queue instance to move from.
	 */
	ThreadsafeQueue(Queue&& mov) {
		this->queue = std::move(mov);
	}

	/**
	 * Destructor (default implementation).
	 */
	~ThreadsafeQueue() = default;
	/// @}

	/**
	 * @name Queue Operations
	 * @{
	 */
	/**
	 * Execute a modifying function on the queue while locked.
	 * @param function The QueueOperator to apply to the queue.
	 */
	void operate(QueueOperator function) {
		Lock lock(mutex);
		function(queue);
	}

	/**
	 * Execute a non-modifying function on the queue while locked (const).
	 * @param function The QueueConstOperator to apply to the queue.
	 */
	void operate(QueueConstOperator function) const {
		Lock lock(mutex);
		function(queue);
	}

	/**
	 * Get the number of elements in the queue.
	 * @return The size of the queue.
	 */
	size_t size() const {
		Lock lock(mutex);
		return queue.size();
	}

	/**
	 * Clear all elements from the queue.
	 */
	void clear() {
		Lock lock(mutex);
		queue.clear();
	}

	/**
	 * Check if the queue is empty.
	 * @return True if the queue is empty, false otherwise.
	 */
	bool empty() const {
		Lock lock(mutex);
		return queue.empty();
	}

	/**
	 * Block until the queue is non-empty.
	 */
	void wait() {
		while(empty()) {
			UniqueLock lock(blocker);
			cvBlock.wait(lock);
		}
	}
	/// @}

	/**
	 * @name Accessors
	 * @{
	 */
	/**
	 * Get a reference to the last element in the queue.
	 * @return A reference to the last element.
	 */
	ref back() {
		Lock lock(mutex);
		return queue.back();
	}

	/**
	 * Get a constant reference to the last element in the queue (const).
	 * @return A constant reference to the last element.
	 */
	const_ref back() const {
		Lock lock(mutex);
		return queue.back();
	}

	/**
	 * Get a reference to the first element in the queue.
	 * @return A reference to the first element.
	 */
	ref front() {
		Lock lock(mutex);
		return queue.front();
	}

	/**
	 * Get a constant reference to the first element in the queue (const).
	 * @return A constant reference to the first element.
	 */
	const_ref front() const {
		Lock lock(mutex);
		return queue.front();
	}
	/// @}

	/**
	 * @name Insertion
	 * @{
	 */
	/**
	 * Add an element to the end of the queue.
	 * @param val The element to add (const reference).
	 */
	void push_back(const_ref val) {
		Lock lock(mutex);
		queue.push_back(val);
	}

	/**
	 * Add an element to the end of the queue (rvalue reference).
	 * @param val The element to add (rvalue reference).
	 */
	void push_back(mov_ref val) {
		Lock lock(mutex);
		queue.push_back(std::move(val));
	}

	/**
	 * Add an element to the front of the queue.
	 * @param val The element to add (const reference).
	 */
	void push_front(const_ref val) {
		Lock lock(mutex);
		queue.push_front(val);
	}

	/**
	 * Add an element to the front of the queue (rvalue reference).
	 * @param val The element to add (rvalue reference).
	 */
	void push_front(mov_ref val) {
		Lock lock(mutex);
		queue.push_front(std::move(val));
	}
	/// @}

	/**
	 * @name Removal (no return)
	 * @{
	 */
	/**
	 * Remove the last element from the queue.
	 */
	void delete_back() {
		Lock lock(mutex);
		queue.pop_back();
	}

	/**
	 * Remove the first element from the queue.
	 */
	void delete_front() {
		Lock lock(mutex);
		queue.pop_front();
	}
	/// @}

	/**
	 * @name Removal (with return)
	 * @{
	 */
	/**
	 * Remove the last element from the queue and store it in the target.
	 * @param target The variable to store the removed element in.
	 */
	void pop_back(ref target) {
		Lock lock(mutex);
		target = std::move(queue.back());
		queue.pop_back();
	}

	/**
	 * Remove the first element from the queue and store it in the target.
	 * @param target The variable to store the removed element in.
	 */
	void pop_front(ref target) {
		Lock lock(mutex);
		target = std::move(queue.front());
		queue.pop_front();
	}

	/**
	 * Remove and return the last element from the queue.
	 * @return The removed element.
	 */
	T pop_back() {
		T tmp;
		pop_back(tmp);
		return tmp;
	}

	/**
	 * Remove and return the first element from the queue.
	 * @return The removed element.
	 */
	T pop_front() {
		T tmp;
		pop_front(tmp);
		return tmp;
	}
	/// @}
};
}
}

#endif // ELVTHREADSAFEQUEUE_HPP
