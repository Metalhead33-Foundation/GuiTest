#ifndef MHTHREADSAFEQUEUE_HPP
#define MHTHREADSAFEQUEUE_HPP
#include <deque>
#include <mutex>
#include <functional>
#include <condition_variable>

namespace MH33 {
namespace Util {

template<class T, class Allocator = std::allocator<T>> class ThreadsafeQueue {
public:
	typedef T& ref;
	typedef const T& const_ref;
	typedef T&& mov_ref;
	typedef std::deque<T,Allocator> Queue;
	typedef std::function<void(Queue&)> QueueOperator;
	typedef std::function<void(const Queue&)> QueueConstOperator;
	typedef std::mutex Mutex;
	typedef std::lock_guard<Mutex> Lock;
	typedef std::unique_lock<Mutex> UniqueLock;
protected:
	Queue queue;
	mutable Mutex mutex;
	mutable Mutex blocker;
	mutable std::condition_variable cvBlock;
public:
	ThreadsafeQueue() {

	}
	ThreadsafeQueue(const ThreadsafeQueue& cpy) {
		this->queue = cpy.queue;
	}
	ThreadsafeQueue(ThreadsafeQueue&& mov) {
		this->queue = std::move(mov.queue);
	}
	ThreadsafeQueue(const Queue& cpy) {
		this->queue = cpy;
	}
	ThreadsafeQueue(Queue&& mov) {
		this->queue = std::move(mov);
	}
	~ThreadsafeQueue() = default;
	void operate(QueueOperator function) {
		Lock lock(mutex);
		function(queue);
	}
	void operate(QueueConstOperator function) const {
		Lock lock(mutex);
		function(queue);
	}
	size_t size() const {
		Lock lock(mutex);
		return queue.size();
	}
	void clear() {
		Lock lock(mutex);
		queue.clear();
	}
	bool empty() const {
		Lock lock(mutex);
		return queue.empty();
	}
	void wait() {
		while(empty()) {
			UniqueLock lock(blocker);
			cvBlock.wait(lock);
		}
	}
	// Accessors
	ref back() {
		Lock lock(mutex);
		return queue.back();
	}
	const_ref back() const {
		Lock lock(mutex);
		return queue.back();
	}
	ref front() {
		Lock lock(mutex);
		return queue.front();
	}
	const_ref front() const {
		Lock lock(mutex);
		return queue.front();
	}
	// Insertings
	void push_back(const_ref val) {
		Lock lock(mutex);
		queue.push_back(val);
	}
	void push_back(mov_ref val) {
		Lock lock(mutex);
		queue.push_back(std::move(val));
	}
	void push_front(const_ref val) {
		Lock lock(mutex);
		queue.push_front(val);
	}
	void push_front(mov_ref val) {
		Lock lock(mutex);
		queue.push_front(std::move(val));
	}
	// Removal without return
	void delete_back() {
		Lock lock(mutex);
		queue.pop_back();
	}
	void delete_front() {
		Lock lock(mutex);
		queue.pop_front();
	}
	// Removal with return
	void pop_back(ref target) {
		Lock lock(mutex);
		target = std::move(queue.back());
		queue.pop_back();
	}
	void pop_front(ref target) {
		Lock lock(mutex);
		target = std::move(queue.front());
		queue.pop_front();
	}
	T pop_back() {
		T tmp;
		pop_back(tmp);
		return tmp;
	}
	T pop_front() {
		T tmp;
		pop_front(tmp);
		return tmp;
	}
};

}
}
#endif // MHTHREADSAFEQUEUE_HPP
