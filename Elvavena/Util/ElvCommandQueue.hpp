#ifndef ELVCOMMANDQUEUE_HPP
#define ELVCOMMANDQUEUE_HPP
/**
 * @file ElvCommandQueue.hpp
 * @brief Declares the ElvCommandQueue API in the Elvavena/Util module.
 *
 * This header is part of the public declaration surface for Elvavena/Util.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <vector>
#include <functional>
#include <mutex>
#include <thread>
#include <future>
namespace Elv {
namespace Util {

/**
 * \brief CommandQueue class utilizing the Curiously Recurring Template Pattern (CRTP)
 *
 * This class provides a thread-safe command queue for asynchronous execution of functors
 * (commands) on a derived class instance. It allows for both synchronous and asynchronous
 * task execution or enqueueing.
 *
 * \tparam Derived The derived class type that will execute the commands
 */
template <typename Derived> class CommandQueue {
public:
	/**
	 * \brief Type alias for a functor taking a reference to the derived class
	 */
	typedef std::function<void(Derived&)> Command;

	/**
	 * \brief Type alias for a packaged task executing a void function with a Derived& parameter
	 */
	typedef std::packaged_task<void(Derived&)> Task;

	/**
	 * \brief Constructor
	 *
	 * Initializes the command queue and stores the current thread identifier.
	 */
	CommandQueue() : threadIdentifier(std::this_thread::get_id()) {}

	/**
	 * \brief Destructor (defaulted)
	 */
	~CommandQueue() = default;

	/**
	 * \brief Enqueue a command for execution
	 *
	 * Adds a command to the queue, ensuring thread safety through a mutex lock.
	 *
	 * \param command The command functor to enqueue
	 */
	void enqueue(Command&& command) {
		std::lock_guard<std::mutex> lock(commandsMutex);
		writeBuffer.emplace_back(std::move(command));
		// No need to notify here since the rendering thread never sleeps
	}

	/**
	 * \brief Process all commands in the queue
	 *
	 * Swaps the write and read buffers, executes all commands in the read buffer on the
	 * derived class instance, and clears the read buffer.
	 */
	void processCommands() {
		if(writeBuffer.empty()) return;
		{
			std::lock_guard<std::mutex> lock(commandsMutex);
			std::swap(readBuffer, writeBuffer);
		}

		for(auto& command : readBuffer) {
			command(*static_cast<Derived*>(this));
		}

		readBuffer.clear();
	}

	/**
	 * \brief Get the number of commands in the queue
	 *
	 * Returns the size of the write buffer, which represents the number of pending commands.
	 *
	 * \return The number of commands in the queue
	 */
	size_t sizeOfCommands() const {
		std::lock_guard<std::mutex> lock(commandsMutex);
		return writeBuffer.size();
	}

	/**
	 * \brief Synchronously execute a task or enqueue it if called from a different thread
	 *
	 * If called from the same thread as the queue's initialization, executes the task
	 * immediately. Otherwise, enqueues the task for execution on the derived class instance.
	 *
	 * \tparam Func The task function type
	 * \tparam Args The task function argument types
	 * \param callable The task function to execute or enqueue
	 * \param args The task function arguments
	 * \return The result of the task execution (if synchronous) or the future result
	 */
	template <typename Func, typename... Args>
	auto enqueueSync(Func&& callable, Args&&... args) {
		if (std::this_thread::get_id() == threadIdentifier) {
			return std::invoke(std::forward<Func>(callable), *static_cast<Derived*>(this), std::forward<Args>(args)...);
		}

		using return_type = std::invoke_result_t<Func, Derived&, Args...>;
		using packaged_task_type = std::packaged_task<return_type(Derived&, Args&&...)>;

		packaged_task_type task(std::forward<Func>(callable));
		auto future = task.get_future();

		enqueue([&](Derived& derived) {
			task(derived, std::forward<Args>(args)...);
		});

		return future.get();
	}

	/**
	 * \brief Asynchronously enqueue a task for execution
	 *
	 * Creates a packaged task from the provided function and arguments, enqueues it for
	 * execution on the derived class instance, and returns the future result.
	 *
	 * \tparam Func The task function type
	 * \tparam Args The task function argument types
	 * \param callable The task function to enqueue
	 * \param args The task function arguments
	 * \return The future result of the task execution
	 */
	template <typename Func, typename... Args>
	[[nodiscard]] auto enqueueAsync(Func&& callable, Args&&... args) {
		using return_type = std::invoke_result_t<Func, Derived&, Args...>;
		using packaged_task_type = std::packaged_task<return_type(Derived&, Args&&...)>;

		auto taskPtr = std::make_shared<packaged_task_type>(std::bind(
				std::forward<Func>(callable), std::forward<Args>(args)...));
		auto future = taskPtr->get_future();

		enqueue(std::bind(&packaged_task_type::operator(), taskPtr));

		return future;
	}

private:
	/**
	 * \brief Write buffer for incoming commands
	 */
	std::vector<Command> writeBuffer;

	/**
	 * \brief Read buffer for processing commands
	 */
	std::vector<Command> readBuffer;

	/**
	 * \brief Thread identifier for synchronous execution checks
	 */
	std::thread::id threadIdentifier;

	/**
	 * \brief Mutex for thread-safe access to the command buffers
	 */
	mutable std::mutex commandsMutex;
};

}
}
#endif // ELVCOMMANDQUEUE_HPP
