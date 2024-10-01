#ifndef ELVCOMMANDQUEUE_HPP
#define ELVCOMMANDQUEUE_HPP
#include <vector>
#include <functional>
#include <mutex>
#include <thread>
#include <future>
namespace Elv {
namespace Util {

// CRTP: Curiously Recurring Template Pattern
template <typename Derived> class CommandQueue {
public:
	// Type alias for a functor that takes a reference to the derived class
	typedef std::function<void(Derived&)> Command;
	typedef std::packaged_task<void(Derived&)> Task;

	CommandQueue() : threadIdentifier(std::this_thread::get_id()) {

	}

	// Destructor
	~CommandQueue() = default;

	// Method to add a command to the queue
	void enqueue(Command&& command) {
		std::lock_guard<std::mutex> lock(commandsMutex);
		writeBuffer.emplace_back(std::move(command));
		// No need to notify here since the rendering thread never sleeps
	}

	// Method to process all commands in the queue
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

	// Method to get the size of the command queue
	size_t sizeOfCommands() const {
		std::lock_guard<std::mutex> lock(commandsMutex);
		return writeBuffer.size();
	}

	// Synchronous task execution or enqueueing
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

	// Asynchronous task enqueueing
	template <typename Func, typename... Args>
	[[nodiscard]] auto enqueueAsync(Func&& callable, Args&&... args) {
		using return_type = std::invoke_result_t<Func, Derived&, Args...>;
		using packaged_task_type = std::packaged_task<return_type(Derived&, Args&&...)>;

		//auto taskPtr = std::make_shared<packaged_task_type>(std::forward<Func>(callable));
		auto taskPtr = std::make_shared<packaged_task_type>(std::bind(
				std::forward<Func>(callable), std::forward<Args>(args)...));
		auto future = taskPtr->get_future();

		/*enqueue([taskPtr](Derived& derived) {
			(*taskPtr)(derived, std::forward<Args>(args)...);
		});*/
		enqueue(std::bind(&packaged_task_type::operator(), taskPtr));

		return future;
	}

private:
	std::vector<Command> writeBuffer;
	std::vector<Command> readBuffer;
	std::thread::id threadIdentifier;
	mutable std::mutex commandsMutex;
};

}
}
#endif // ELVCOMMANDQUEUE_HPP
