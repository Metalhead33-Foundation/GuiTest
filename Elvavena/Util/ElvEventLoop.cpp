#include <Elvavena/Util/ElvEventLoop.hpp>
namespace Elv {
namespace Util {

void EventLoop::loopFunction()
{
	std::vector<Command> readBuffer;
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

EventLoop::EventLoop()
	: isRunning(true), loopThread(&EventLoop::loopFunction, this)
{

}

EventLoop::~EventLoop()
{
	enqueue([this]
	{
		isRunning = false;
	});
	loopThread.join();
}

bool EventLoop::running() const
{
	return isRunning;
}

void EventLoop::enqueue(Command&& callable)
{
	{
		std::lock_guard<std::mutex> lockguard(commandsMutex);
		writeBuffer.emplace_back(std::move(callable));
	}
	condVar.notify_one();
}
}
}
