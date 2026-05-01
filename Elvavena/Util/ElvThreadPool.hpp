#ifndef ELVTHREADPOOL_HPP
#define ELVTHREADPOOL_HPP

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <Elvavena/Util/ElvUtilGlobals.hpp>

namespace Elv {
namespace Util {

/**
 * @class ThreadPool
 * @brief Simple fixed-size thread pool for executing queued tasks.
 *
 * The ThreadPool owns a set of worker threads. Tasks can be submitted either
 * as fire-and-forget jobs using enqueue(), as blocking jobs using
 * enqueueSync(), or as asynchronous jobs returning std::future using
 * enqueueAsync().
 *
 * The pool automatically stops and joins all worker threads when destroyed.
 */
class MH_UTIL_API ThreadPool {
private:
	/**
	 * @brief Worker threads owned by the pool.
	 */
	std::vector<std::thread> threads_;

	/**
	 * @brief Queue storing pending tasks.
	 *
	 * Each task is represented as a void callable and is executed by one of the
	 * worker threads.
	 */
	std::queue<std::function<void()> > tasks_;

	/**
	 * @brief Mutex protecting access to the task queue and stop flag.
	 */
	std::mutex queue_mutex_;

	/**
	 * @brief Condition variable used to wake worker threads when tasks arrive
	 * or when the pool is stopping.
	 */
	std::condition_variable cv_;

	/**
	 * @brief Indicates whether the thread pool is shutting down.
	 */
	bool stop_ = false;

public:
	/**
	 * @brief Constructs a thread pool with the given number of worker threads.
	 *
	 * If no thread count is provided, std::thread::hardware_concurrency() is
	 * used as the default.
	 *
	 * @param num_threads Number of worker threads to create.
	 */
	ThreadPool(size_t num_threads = std::thread::hardware_concurrency());

	/**
	 * @brief Destroys the thread pool.
	 *
	 * Signals all worker threads to stop, wakes them, and joins them before
	 * returning.
	 */
	~ThreadPool();

	/**
	 * @brief Enqueues a fire-and-forget task for execution.
	 *
	 * The task is moved into the internal queue and executed by one of the
	 * worker threads when available.
	 *
	 * @param task Task to enqueue.
	 */
	void enqueue(std::function<void()>&& task);

	/**
	 * @brief Enqueues a callable with arguments and waits for its result.
	 *
	 * This function submits the callable to the pool and blocks until the task
	 * has completed. The callable's return value is then returned to the caller.
	 *
	 * @tparam Func Type of the callable object.
	 * @tparam Args Types of the callable arguments.
	 *
	 * @param callable Callable object to execute.
	 * @param args Arguments to pass to the callable.
	 *
	 * @return The result returned by the callable.
	 */
	template<typename Func, typename... Args>
	inline auto enqueueSync(Func&& callable, Args&&... args)
	{
		using return_type = std::invoke_result_t<Func, Args...>;
		using packaged_task_type =
			std::packaged_task<return_type(Args&&...)>;

		packaged_task_type task(std::forward<Func>(callable));

		enqueue([&]
				{
					task(std::forward<Args>(args)...);
				});

		return task.get_future().get();
	}

	/**
	 * @brief Enqueues a callable with arguments and returns a future.
	 *
	 * This function submits the callable to the pool and immediately returns a
	 * std::future that can later be used to retrieve the callable's result.
	 *
	 * @tparam Func Type of the callable object.
	 * @tparam Args Types of the callable arguments.
	 *
	 * @param callable Callable object to execute.
	 * @param args Arguments to pass to the callable.
	 *
	 * @return std::future containing the eventual result of the callable.
	 */
	template<typename Func, typename... Args>
	[[nodiscard]] inline auto enqueueAsync(Func&& callable, Args&&... args)
	{
		using return_type = std::invoke_result_t<Func, Args...>;
		using packaged_task_type = std::packaged_task<return_type()>;

		auto taskPtr = std::make_shared<packaged_task_type>(
			[func = std::forward<Func>(callable),
			 argTuple = std::make_tuple(std::forward<Args>(args)...)]() mutable -> return_type
			{
				return std::apply(
					[&func](auto&&... unpackedArgs) mutable -> return_type
					{
						return std::invoke(
							std::move(func),
							std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
					},
					std::move(argTuple));
			});

		enqueue([taskPtr]() mutable
				{
					(*taskPtr)();
				});

		return taskPtr->get_future();
	}
};

}
}

#endif // ELVTHREADPOOL_HPP