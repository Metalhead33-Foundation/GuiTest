#ifndef ELVASYNCEXECUTOR_HPP
#define ELVASYNCEXECUTOR_HPP
/**
 * @file ElvAsyncExecutor.hpp
 * @brief Defines the common executor concept used by asynchronous utilities.
 *
 * This header describes the minimal task-submission interface shared by
 * Elvavena executor types such as EventLoop and ThreadPool.
 */
#include <concepts>
#include <functional>
namespace Elv {
namespace Util {

/**
 * @concept AsyncExecutor
 * @brief Requires the task-submission API used by asynchronous helper code.
 *
 * A conforming executor accepts fire-and-forget commands through enqueue(),
 * can submit asynchronous work through enqueueAsync(), and can submit blocking
 * work through enqueueSync(). The concept intentionally checks only the common
 * call shapes needed by generic helper code; it does not constrain threading,
 * ordering, or scheduling semantics.
 *
 * @tparam T Executor type to validate.
 */
template<typename T> concept AsyncExecutor =
	requires(T& executor, std::function<void()> task)
{
	/**
	 * @brief Enqueues a void task without returning a result.
	 */
	{ executor.enqueue(std::move(task)) } -> std::same_as<void>;

	/**
	 * @brief Enqueues a callable and returns an asynchronous result handle.
	 */
	{ executor.enqueueAsync([] {}) };

	/**
	 * @brief Enqueues a callable and waits for it to complete.
	 */
	{ executor.enqueueSync([] {}) };
};

} // namespace Util
} // namespace Elv
#endif // ELVASYNCEXECUTOR_HPP
