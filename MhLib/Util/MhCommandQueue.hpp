#ifndef MHCOMMANDQUEUE_H
#define MHCOMMANDQUEUE_H
#include <deque>
#include <functional>
#include <mutex>
namespace MH33 {
namespace Util {
// CRTP: Curiously Recurring Template Pattern
template <typename Derived> class CommandQueue {
public:
	// Type alias for a functor that takes a reference to the derived class
	typedef std::function<void(Derived&)> Command;

	// Constructor
	CommandQueue() = default;

	// Destructor
	~CommandQueue() = default;

	// Method to add a command to the queue
	void enqueue(Command&& command) {
		std::lock_guard<std::mutex> lock(__commandsMutex_);
		__commands_.emplace_back(std::move(command));
	}

	// Method to process all commands in the queue
	void processCommands() {
		std::lock_guard<std::mutex> lock(__commandsMutex_);
		while (!__commands_.empty()) {
			Command command = __commands_.front();
			command(*static_cast<Derived*>(this));  // Execute the command on the derived class
			__commands_.pop_front();
		}
	}

	// Method to get the size of the command queue
	size_t sizeOfCommands() const {
		std::lock_guard<std::mutex> lock(__commandsMutex_);
		return __commands_.size();
	}

private:
	std::deque<Command> __commands_;
	mutable std::mutex __commandsMutex_;
};

}
}
#endif // MHCOMMANDQUEUE_H
