#ifndef ELVEVENTLOOP_HPP
#define ELVEVENTLOOP_HPP
#include <functional>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <future>
#include <thread>
#include <Elvavena/Util/ElvUtilGlobals.hpp>
namespace Elv {
namespace Util {

DEFINE_CLASS(EventLoop)
class MH_UTIL_API EventLoop
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
	template<typename Func, typename... Args> inline auto enqueueSync(Func&& callable, Args&& ...args)
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
	template<typename Func, typename... Args> [[nodiscard]] inline auto enqueueAsync(Func&& callable, Args&& ...args)
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
#endif // ELVEVENTLOOP_HPP
