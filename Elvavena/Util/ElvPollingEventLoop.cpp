#include <Elvavena/Util/ElvPollingEventLoop.hpp>
#include <array>
#include <chrono>

namespace Elv {
namespace Util {

PollingEventLoop::PollingEventLoop(std::unique_ptr<Elv::Io::EventPoller> poller,
	std::unique_ptr<Elv::Io::EventLoopWakeup> wakeup,
	std::pmr::memory_resource* memoryResource)
	: poller(std::move(poller))
	, wakeup(std::move(wakeup))
	, writeBuffer(memoryResource != nullptr ? memoryResource : std::pmr::get_default_resource())
	, memoryResource(memoryResource != nullptr ? memoryResource : std::pmr::get_default_resource())
{
	if (!this->poller || !this->wakeup || !this->poller->isValid() || !this->wakeup->isValid()) return;
	wakeupHandle = this->wakeup->nativeHandle();
	if (wakeupHandle == Elv::Io::InvalidNativeHandle) return;
	if (!this->poller->add(wakeupHandle, Elv::Io::EventMask::Read, WakeupToken)) return;
	valid = true;
	isRunning.store(true, std::memory_order_release);
	loopThread = std::thread(&PollingEventLoop::loopFunction, this);
}

PollingEventLoop::~PollingEventLoop()
{
	stop();
}

bool PollingEventLoop::isValid() const noexcept { return valid; }

bool PollingEventLoop::running() const noexcept
{
	return isRunning.load(std::memory_order_acquire);
}

int PollingEventLoop::lastError() const noexcept
{
	return lastErrorMessage.load(std::memory_order_acquire);
}

void PollingEventLoop::enqueue(Command&& callable)
{
	if (!isRunning.load(std::memory_order_acquire)) return;
	{
		std::scoped_lock lock(commandsMutex);
		writeBuffer.emplace_back(std::move(callable));
	}
	if (wakeup) wakeup->notify();
}

void PollingEventLoop::stop()
{
	isRunning.store(false, std::memory_order_release);
	if (wakeup) wakeup->notify();
	if (loopThread.joinable() && std::this_thread::get_id() != loopThread.get_id()) {
		loopThread.join();
	}
}

void PollingEventLoop::loopFunction()
{
	std::pmr::vector<Command> readBuffer(memoryResource);
	std::array<Elv::Io::PollEvent, 64> events {};
	while (isRunning.load(std::memory_order_acquire)) {
		{
			std::scoped_lock lock(commandsMutex);
			std::swap(readBuffer, writeBuffer);
		}
		for (Command& command : readBuffer) {
			if (command) command();
		}
		readBuffer.clear();
		if (!isRunning.load(std::memory_order_acquire)) break;

		const int ready = poller->wait(events, std::nullopt);
		if (ready < 0) {
			const int error = poller->lastError();
			lastErrorMessage.store(error, std::memory_order_release);
			if (error == EINTR) continue;
			break;
		}
		for (int index = 0; index < ready; ++index) {
			const Elv::Io::PollEvent& event = events[static_cast<std::size_t>(index)];
			if (event.handle == wakeupHandle && event.token == WakeupToken) {
				wakeup->drain();
				continue;
			}
			const auto handler = handlers.find(event.token);
			if (handler != handlers.end() && handler->second.callback != nullptr) {
				handler->second.callback(handler->second.userData, event);
			}
		}
	}
}

bool PollingEventLoop::add(Elv::Io::NativeHandle handle, Elv::Io::EventMask interest,
	std::uint64_t token, EventHandler handler, Elv::Io::PollFlags flags)
{
	if (!valid) return false;
	if (std::this_thread::get_id() == loopThread.get_id()) {
		return addImpl(handle, interest, token, handler, flags);
	}
	return enqueueSync([this, handle, interest, token, handler, flags]() {
		return addImpl(handle, interest, token, handler, flags);
	});
}

bool PollingEventLoop::modify(Elv::Io::NativeHandle handle, Elv::Io::EventMask interest,
	std::uint64_t token, EventHandler handler, Elv::Io::PollFlags flags)
{
	if (!valid) return false;
	if (std::this_thread::get_id() == loopThread.get_id()) {
		return modifyImpl(handle, interest, token, handler, flags);
	}
	return enqueueSync([this, handle, interest, token, handler, flags]() {
		return modifyImpl(handle, interest, token, handler, flags);
	});
}

bool PollingEventLoop::remove(Elv::Io::NativeHandle handle)
{
	if (!valid) return false;
	if (std::this_thread::get_id() == loopThread.get_id()) {
		return removeImpl(handle);
	}
	return enqueueSync([this, handle]() { return removeImpl(handle); });
}

bool PollingEventLoop::addImpl(Elv::Io::NativeHandle handle, Elv::Io::EventMask interest,
	std::uint64_t token, EventHandler handler, Elv::Io::PollFlags flags)
{
	if (handle == Elv::Io::InvalidNativeHandle || token == WakeupToken) return false;
	if (registrations.find(handle) != registrations.end()) return false;
	if (handlers.find(token) != handlers.end()) return false;
	if (!poller->add(handle, interest, token, flags)) return false;
	try {
		registrations.emplace(handle, Registration { token, interest, flags });
		handlers.emplace(token, handler);
	} catch (...) {
		poller->remove(handle);
		registrations.erase(handle);
		handlers.erase(token);
		return false;
	}
	return true;
}

bool PollingEventLoop::modifyImpl(Elv::Io::NativeHandle handle, Elv::Io::EventMask interest,
	std::uint64_t token, EventHandler handler, Elv::Io::PollFlags flags)
{
	const auto registration = registrations.find(handle);
	if (registration == registrations.end()) return false;
	if (token != registration->second.token && handlers.find(token) != handlers.end()) {
		return false;
	}
	if (!poller->modify(handle, interest, token, flags)) return false;
	const std::uint64_t oldToken = registration->second.token;
	registration->second = Registration { token, interest, flags };
	handlers.erase(oldToken);
	handlers[token] = handler;
	return true;
}

bool PollingEventLoop::removeImpl(Elv::Io::NativeHandle handle)
{
	const auto registration = registrations.find(handle);
	if (registration == registrations.end()) return false;
	if (!poller->remove(handle)) return false;
	handlers.erase(registration->second.token);
	registrations.erase(registration);
	return true;
}

} // namespace Util
} // namespace Elv

