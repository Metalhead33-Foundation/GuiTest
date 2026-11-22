#ifndef ELVPOLLINGEVENTLOOP_HPP
#define ELVPOLLINGEVENTLOOP_HPP
/**
 * @file ElvPollingEventLoop.hpp
 * @brief Event loop variant that owns an OS readiness poller and a wakeup handle.
 *
 * `Elv::Util::EventLoop` remains the pure command pump. This class is the second
 * variant: commands still run sequentially on one thread, but the loop blocks in
 * an `Elv::Io::EventPoller` and dispatches ready handles to registered callbacks.
 * Enqueuing a command notifies an `Elv::Io::EventLoopWakeup`, so the poller wakes
 * immediately instead of relying on a short polling timeout.
 *
 * Registration and event callbacks run on the loop thread. `add`/`modify`/`remove`
 * from another thread marshal through the command queue, which keeps all access
 * to the non-thread-safe poller on one thread.
 */
#include <Elvavena/Io/ElvEventLoopWakeup.hpp>
#include <Elvavena/Io/ElvEventPoller.hpp>
#include <Elvavena/Util/ElvAsyncExecutor.hpp>
#include <Elvavena/Util/ElvUtilGlobals.hpp>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Elv {
namespace Util {

/**
 * @class PollingEventLoop
 * @brief Single-threaded poller/reactor plus sequential command executor.
 */
class MH_UTIL_API PollingEventLoop {
public:
	/** @brief Command type, matching `EventLoop::Command`. */
	using Command = std::function<void()>;

	/** @brief Readiness callback invoked on the loop thread. */
	using EventCallback = void (*)(void* userData, const Elv::Io::PollEvent& event);

	/** @struct EventHandler
	 *  @brief Callback and opaque user data for one registration.
	 */
	struct EventHandler {
		EventCallback callback = nullptr; ///< Callback, or null to ignore events.
		void* userData = nullptr;         ///< Opaque callback context.
	};

	/**
	 * @brief Creates a polling loop from an owned poller and wakeup handle.
	 * @param poller Readiness poller; must be valid.
	 * @param wakeup Wakeup primitive; must be valid.
	 * @param memoryResource PMR resource for commands/tasks; may be null.
	 *
	 * If either dependency is invalid, or the wakeup cannot be registered, the
	 * loop reports `isValid() == false` and starts no thread.
	 */
	PollingEventLoop(std::unique_ptr<Elv::Io::EventPoller> poller,
		std::unique_ptr<Elv::Io::EventLoopWakeup> wakeup,
		std::pmr::memory_resource* memoryResource = std::pmr::get_default_resource());

	/** @brief Stops the loop and joins its thread. */
	~PollingEventLoop();

	PollingEventLoop(const PollingEventLoop&) = delete;
	PollingEventLoop& operator=(const PollingEventLoop&) = delete;
	PollingEventLoop(PollingEventLoop&&) = delete;
	PollingEventLoop& operator=(PollingEventLoop&&) = delete;

	/** @brief Returns whether the loop owns a usable poller/wakeup pair. */
	[[nodiscard]] bool isValid() const noexcept;

	/** @brief Returns whether the loop thread is running. */
	[[nodiscard]] bool running() const noexcept;

	/** @brief Returns the last poller error code. */
	[[nodiscard]] int lastError() const noexcept;

	/**
	 * @brief Registers a handle with the poller and an event callback.
	 * @param handle Native handle.
	 * @param interest Readiness interest.
	 * @param token Caller token, also used to find the callback.
	 * @param handler Callback/user-data pair.
	 * @param flags Optional edge/one-shot flags.
	 * @return True on success.
	 */
	bool add(Elv::Io::NativeHandle handle, Elv::Io::EventMask interest,
		std::uint64_t token, EventHandler handler,
		Elv::Io::PollFlags flags = Elv::Io::PollFlags::None);

	/** @brief Changes an existing registration. */
	bool modify(Elv::Io::NativeHandle handle, Elv::Io::EventMask interest,
		std::uint64_t token, EventHandler handler,
		Elv::Io::PollFlags flags = Elv::Io::PollFlags::None);

	/** @brief Removes a registration and its callback. */
	bool remove(Elv::Io::NativeHandle handle);

	/** @brief Enqueues a fire-and-forget command. */
	void enqueue(Command&& callable);

	/** @brief Enqueues a callable and waits for its result. */
	template<typename Func, typename... Args> inline auto enqueueSync(Func&& callable, Args&&... args)
	{
		if (!isRunning.load(std::memory_order_acquire)
			|| std::this_thread::get_id() == loopThread.get_id())
		{
			return std::invoke(std::forward<Func>(callable), std::forward<Args>(args)...);
		}
		return enqueueAsync(std::forward<Func>(callable), std::forward<Args>(args)...).get();
	}

	/** @brief Enqueues a callable and returns a future for its result. */
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
			});
		auto future = taskPtr->get_future();
		enqueue([taskPtr]() mutable { (*taskPtr)(); });
		return future;
	}

	/** @brief Stops the loop and wakes a blocked poller. */
	void stop();

private:
	/** @brief Registration record keyed by handle. */
	struct Registration {
		std::uint64_t token = 0;
		Elv::Io::EventMask interest = Elv::Io::EventMask::None;
		Elv::Io::PollFlags flags = Elv::Io::PollFlags::None;
	};

	/** @brief Internal wakeup token; rejected for user registrations. */
	static constexpr std::uint64_t WakeupToken = ~std::uint64_t(0);

	std::unique_ptr<Elv::Io::EventPoller> poller;
	std::unique_ptr<Elv::Io::EventLoopWakeup> wakeup;
	std::pmr::vector<Command> writeBuffer;
	mutable std::mutex commandsMutex;
	std::pmr::memory_resource* memoryResource = std::pmr::get_default_resource();
	std::atomic<bool> isRunning { false };
	std::thread loopThread;
	std::unordered_map<Elv::Io::NativeHandle, Registration> registrations;
	std::unordered_map<std::uint64_t, EventHandler> handlers;
	Elv::Io::NativeHandle wakeupHandle = Elv::Io::InvalidNativeHandle;
	bool valid = false;
	std::atomic<int> lastErrorMessage { 0 };

	void loopFunction();
	bool addImpl(Elv::Io::NativeHandle handle, Elv::Io::EventMask interest,
		std::uint64_t token, EventHandler handler, Elv::Io::PollFlags flags);
	bool modifyImpl(Elv::Io::NativeHandle handle, Elv::Io::EventMask interest,
		std::uint64_t token, EventHandler handler, Elv::Io::PollFlags flags);
	bool removeImpl(Elv::Io::NativeHandle handle);
};

static_assert(AsyncExecutor<PollingEventLoop>);

} // namespace Util
} // namespace Elv

#endif // ELVPOLLINGEVENTLOOP_HPP

