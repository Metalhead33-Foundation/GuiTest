#ifndef EVENTLOOP_HPP
#define EVENTLOOP_HPP
#include <functional>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <future>
#include <thread>

namespace MH33 {
namespace Util {
class EventLoop
{
public:
	typedef std::function<void()> Command;
private:
	std::vector<Command> writeBuffer;
	mutable std::mutex commandsMutex;
	std::condition_variable condVar;
	bool isRunning;
	std::thread loopThread;
	void loopFunction();
	EventLoop(const EventLoop&) = delete;
	EventLoop(EventLoop&&) noexcept = delete;
	EventLoop& operator= (const EventLoop&) = delete;
	EventLoop& operator= (EventLoop&&) noexcept = delete;
public:
	EventLoop();
	~EventLoop();
	bool running() const;
	void enqueue(Command&& callable);
	template<typename Func, typename... Args> auto enqueueSync(Func&& callable, Args&& ...args)
	{
		if (std::this_thread::get_id() == loopThread.get_id())
		{
			return std::invoke(
				std::forward<Func>(callable),
				std::forward<Args>(args)...);
		}

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
	template<typename Func, typename... Args> [[nodiscard]] auto enqueueAsync(Func&& callable, Args&& ...args)
	{
		using return_type = std::invoke_result_t<Func, Args...>;
		using packaged_task_type = std::packaged_task<return_type()>;

		auto taskPtr = std::make_shared<packaged_task_type>(std::bind(
			std::forward<Func>(callable), std::forward<Args>(args)...));

		enqueue(std::bind(&packaged_task_type::operator(), taskPtr));

		return taskPtr->get_future();
	}
};
}
}

#endif // EVENTLOOP_HPP
