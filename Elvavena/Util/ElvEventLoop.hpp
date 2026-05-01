#ifndef ELVEVENTLOOP_HPP
#define ELVEVENTLOOP_HPP
/**
 * @file ElvEventLoop.hpp
 * @brief Declares the ElvEventLoop API in the Elvavena/Util module.
 *
 * This header is part of the public declaration surface for Elvavena/Util.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <functional>
#include <condition_variable>
#include <future>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <Elvavena/Util/ElvUtilGlobals.hpp>
#include <Elvavena/Util/ElvAsyncExecutor.hpp>
namespace Elv {
namespace Util {

/**
 * @class EventLoop
 * @brief Manages a thread for executing commands (functions) in a sequential manner.
 *
 * This class provides a thread-safe way to enqueue and execute commands from multiple threads.
 * It ensures that all commands are executed in the order they were enqueued, within a dedicated thread.
 *
 * @note This class is non-copyable and non-movable to prevent unintended behavior.
 */
class MH_UTIL_API EventLoop
{
public:
	/**
	 * @typedef Command
	 * @brief Type alias for a callable function with no arguments and no return value.
	 */
	typedef std::function<void()> Command;

private:
	/**
	 * @var writeBuffer
	 * @brief Buffer to store commands waiting to be executed.
	 */
	std::pmr::vector<Command> writeBuffer;

	/**
	 * @var commandsMutex
	 * @brief Mutex to synchronize access to the command buffer.
	 */
	mutable std::mutex commandsMutex;

	/**
	 * @var condVar
	 * @brief Condition variable to signal when new commands are available.
	 */
	std::condition_variable condVar;

	/**
	 * @var memoryResource
	 * @brief PMR memory resource used for container/task allocations.
	 *
	 * The pointed-to resource is not owned by EventLoop and must outlive the
	 * EventLoop and any futures/tasks that allocate through it.
	 */
	std::pmr::memory_resource* memoryResource = std::pmr::get_default_resource();

	/**
	 * @var isRunning
	 * @brief Flag indicating whether the event loop is currently running.
	 */
	bool isRunning;

	/**
	 * @var loopThread
	 * @brief Dedicated thread for executing the event loop.
	 */
	std::thread loopThread;

	/**
	 * @fn loopFunction
	 * @brief Internal function executed by the dedicated thread to process commands.
	 *
	 * This function runs in an infinite loop until the event loop is stopped.
	 * It executes commands from the buffer in the order they were enqueued.
	 */
	void loopFunction();

	/**
	 * @brief Copy construction is disabled because EventLoop owns a live thread.
	 */
	EventLoop(const EventLoop&) = delete;

	/**
	 * @brief Move construction is disabled to keep the worker thread bound to a stable object address.
	 */
	EventLoop(EventLoop&&) noexcept = delete;

	/**
	 * @brief Copy assignment is disabled because EventLoop owns synchronization primitives and a live thread.
	 */
	EventLoop& operator=(const EventLoop&) = delete;

	/**
	 * @brief Move assignment is disabled to keep synchronization state and worker thread ownership stable.
	 */
	EventLoop& operator=(EventLoop&&) noexcept = delete;

public:
	/**
	 * @fn EventLoop
	 * @brief Constructor, initializes the event loop and starts the dedicated thread.
	 *
	 * @param memory_resource Memory resource used for command and task
	 * allocations. If nullptr is passed, std::pmr::get_default_resource() is
	 * used instead. The resource must outlive the EventLoop and any returned
	 * asynchronous results that allocated through it.
	 */
	EventLoop(std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource());

	/**
	 * @fn ~EventLoop
	 * @brief Destructor, stops the event loop and joins the dedicated thread.
	 */
	~EventLoop();

	/**
	 * @fn running
	 * @brief Checks whether the event loop is currently running.
	 *
	 * @return True if the event loop is running, false otherwise.
	 */
	bool running() const;

	/**
	 * @fn enqueue
	 * @brief Enqueues a command to be executed by the event loop.
	 *
	 * The command will be executed in the order it was received, within the dedicated thread.
	 *
	 * @param callable Command to be executed (rvalue reference to allow for temporary objects).
	 */
	void enqueue(Command&& callable);

	/**
	 * @fn enqueueSync
	 * @brief Enqueues a command with arguments and waits for its completion.
	 *
	 * If called from the same thread as the event loop, the command is executed immediately.
	 * Otherwise, the command is enqueued and the function waits for its completion.
	 *
	 * @tparam Func Type of the callable function.
	 * @tparam Args Types of the function arguments.
	 * @param callable Callable function to be executed (forwarding reference).
	 * @param args Function arguments (forwarding references).
	 * @return The result of the executed function.
	 */
	template<typename Func, typename... Args> inline auto enqueueSync(Func&& callable, Args&&...args)
	{
		if (std::this_thread::get_id() == loopThread.get_id())
		{
			return std::invoke(
				std::forward<Func>(callable),
				std::forward<Args>(args)...);
		}
		else return enqueueAsync(
				   std::forward<Func>(callable),
				   std::forward<Args>(args)...
				   ).get();
	}

	/**
	 * @fn enqueueAsync
	 * @brief Enqueues a command with arguments and returns a future for its result.
	 *
	 * The command is executed asynchronously, and the returned future can be used to retrieve the result.
	 *
	 * @tparam Func Type of the callable function.
	 * @tparam Args Types of the function arguments.
	 * @param callable Callable function to be executed (forwarding reference).
	 * @param args Function arguments (forwarding references).
	 * @return A future representing the result of the executed function.
	 */
	template<typename Func, typename... Args> [[nodiscard]] auto enqueueAsync(Func&& callable, Args&&... args)
	{
		using return_type = std::invoke_result_t<Func, Args...>;

		auto taskAllocator = std::pmr::polymorphic_allocator<std::packaged_task<return_type()> >(memoryResource);
		auto taskPtr = std::allocate_shared<std::packaged_task<return_type()> >(
			taskAllocator,
			[func = std::forward<Func>(callable),
			 argsTuple = std::make_tuple(std::forward<Args>(args)...)]() mutable -> return_type
			{
				return std::apply(std::move(func), std::move(argsTuple));
			}
			);

		auto fut = taskPtr->get_future();

		enqueue([taskPtr]() mutable {
			(*taskPtr)();
		});

		return fut;
	}
};

/**
 * @brief Verifies that EventLoop satisfies the generic asynchronous executor contract.
 */
static_assert(AsyncExecutor<EventLoop>);

} // namespace Util
} // namespace Elv
#endif // ELVEVENTLOOP_HPP
