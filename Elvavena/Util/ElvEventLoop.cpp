/**
 * @file ElvEventLoop.cpp
 * @brief Implementation of the `Elv::Util::EventLoop` command execution thread.
 */

#include <Elvavena/Util/ElvEventLoop.hpp>

namespace Elv {
namespace Util {

/**
 * @brief Main loop that drains pending commands and executes them in order.
 */
void EventLoop::loopFunction()
{
	std::pmr::vector<Command> readBuffer(memoryResource);
	while (isRunning)
	{
		{
			std::unique_lock<std::mutex> lock(commandsMutex);
			condVar.wait(lock, [this]
			{
				return !writeBuffer.empty();
			});
			std::swap(readBuffer, writeBuffer);
		}
		for (Command& func : readBuffer)
		{
			func();
		}
		readBuffer.clear();
	}
}

/**
 * @brief Starts the event loop thread.
 */
EventLoop::EventLoop(std::pmr::memory_resource* memory_resource)
	: writeBuffer(memory_resource != nullptr ? memory_resource : std::pmr::get_default_resource())
	, memoryResource(memory_resource != nullptr ? memory_resource : std::pmr::get_default_resource())
	, isRunning(true)
	, loopThread(&EventLoop::loopFunction, this)
{
}

/**
 * @brief Stops the event loop and joins the worker thread.
 */
EventLoop::~EventLoop()
{
	enqueue([this]
	{
		isRunning = false;
	});
	loopThread.join();
}

/**
 * @brief Returns whether the loop is still running.
 * @return `true` if running, otherwise `false`.
 */
bool EventLoop::running() const
{
	return isRunning;
}

/**
 * @brief Enqueues a command for execution by the loop thread.
 * @param callable Command object to enqueue.
 */
void EventLoop::enqueue(Command&& callable)
{
	{
		std::scoped_lock lockguard(commandsMutex);
		writeBuffer.emplace_back(std::move(callable));
	}
	condVar.notify_one();
}

} // namespace Util
} // namespace Elv
