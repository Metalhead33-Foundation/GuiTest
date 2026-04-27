#include "EuphAsyncIoScheduler.hpp"
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <algorithm>
#include <exception>

namespace Euph {
namespace Asset {

AsyncRead::AsyncRead(std::uint64_t id, std::shared_ptr<std::atomic_bool> flag, std::future<AsyncReadResult>&& future)
	: readId(id), cancelFlag(std::move(flag)), resultFuture(std::move(future))
{
}

std::uint64_t AsyncRead::id() const
{
	return readId;
}

bool AsyncRead::valid() const
{
	return resultFuture.valid();
}

void AsyncRead::cancel()
{
	if(cancelFlag) {
		cancelFlag->store(true);
	}
}

std::future<AsyncReadResult>& AsyncRead::future()
{
	return resultFuture;
}

bool AsyncIoScheduler::QueueCompare::operator()(const QueueItem& left, const QueueItem& right) const
{
	if(left.priority.value != right.priority.value) {
		return left.priority.value < right.priority.value;
	}
	return left.sequence > right.sequence;
}

AsyncIoScheduler::AsyncIoScheduler(Elv::Io::System& system,
								   std::size_t workerCount,
								   Elv::Util::EventLoop* completionLoop,
								   bool startPaused)
	: system(system),
	  completionLoop(completionLoop),
	  nextId(1),
	  nextSequence(0),
	  stopping(false),
	  paused(startPaused)
{
	workerCount = std::max<std::size_t>(workerCount, 1);
	workers.reserve(workerCount);
	for(std::size_t i = 0; i < workerCount; ++i) {
		workers.emplace_back(&AsyncIoScheduler::workerLoop, this);
	}
}

AsyncIoScheduler::~AsyncIoScheduler()
{
	stop();
}

AsyncRead AsyncIoScheduler::read(std::string path,
								 std::uint64_t offset,
								 std::uint64_t size,
								 StreamPriority priority,
								 Completion completion)
{
	auto pending = std::make_shared<PendingRead>();
	pending->id = nextId.fetch_add(1);
	pending->generation = 0;
	pending->priority = priority;
	pending->path = std::move(path);
	pending->offset = offset;
	pending->size = size;
	pending->cancelFlag = std::make_shared<std::atomic_bool>(false);
	pending->promise = std::make_shared<std::promise<AsyncReadResult>>();
	pending->completion = std::move(completion);

	auto future = pending->promise->get_future();
	{
		std::lock_guard<std::mutex> lock(mutex);
		pending->sequence = nextSequence++;
		pendingReads.emplace(pending->id, pending);
		queue.push(QueueItem{ priority, pending->sequence, pending->generation, pending });
	}
	cv.notify_one();
	return AsyncRead(pending->id, pending->cancelFlag, std::move(future));
}

bool AsyncIoScheduler::bumpPriority(std::uint64_t readId, StreamPriority priority)
{
	std::lock_guard<std::mutex> lock(mutex);
	auto it = pendingReads.find(readId);
	if(it == pendingReads.end() || !it->second->queued) {
		return false;
	}
	auto& pending = it->second;
	pending->priority = priority;
	++pending->generation;
	queue.push(QueueItem{ priority, pending->sequence, pending->generation, pending });
	cv.notify_one();
	return true;
}

bool AsyncIoScheduler::cancel(std::uint64_t readId)
{
	std::lock_guard<std::mutex> lock(mutex);
	auto it = pendingReads.find(readId);
	if(it == pendingReads.end()) {
		return false;
	}
	it->second->cancelFlag->store(true);
	cv.notify_all();
	return true;
}

void AsyncIoScheduler::resume()
{
	setPaused(false);
}

void AsyncIoScheduler::setPaused(bool value)
{
	{
		std::lock_guard<std::mutex> lock(mutex);
		paused = value;
	}
	cv.notify_all();
}

void AsyncIoScheduler::stop()
{
	{
		std::lock_guard<std::mutex> lock(mutex);
		if(stopping) {
			return;
		}
		stopping = true;
		for(auto& it : pendingReads) {
			it.second->cancelFlag->store(true);
		}
	}
	cv.notify_all();
	for(auto& worker : workers) {
		if(worker.joinable()) {
			worker.join();
		}
	}
	workers.clear();
}

void AsyncIoScheduler::workerLoop()
{
	while(true) {
		std::shared_ptr<PendingRead> pending;
		{
			std::unique_lock<std::mutex> lock(mutex);
			cv.wait(lock, [this] {
				return stopping || (!paused && !queue.empty());
			});
			if(stopping && queue.empty()) {
				return;
			}
			while(!queue.empty()) {
				QueueItem item = queue.top();
				queue.pop();
				if(!item.pending || item.pending->generation != item.generation || !item.pending->queued) {
					continue;
				}
				pending = item.pending;
				pending->queued = false;
				pendingReads.erase(pending->id);
				break;
			}
			if(!pending) {
				continue;
			}
		}

		if(pending->cancelFlag->load()) {
			complete(pending, AsyncReadResult{ {}, true, {} });
			continue;
		}
		complete(pending, performRead(*pending));
	}
}

void AsyncIoScheduler::complete(const std::shared_ptr<PendingRead>& pending, AsyncReadResult&& result)
{
	AsyncReadResult callbackResult = result;
	pending->promise->set_value(std::move(result));
	if(pending->completion) {
		if(completionLoop) {
			auto completion = pending->completion;
			completionLoop->enqueue([completion = std::move(completion), callbackResult = std::move(callbackResult)]() mutable {
				completion(callbackResult);
			});
		} else {
			pending->completion(callbackResult);
		}
	}
}

AsyncReadResult AsyncIoScheduler::performRead(const PendingRead& pending)
{
	try {
		Elv::Io::uDevice device = system.openUnique(pending.path.c_str(), Elv::Io::Mode::READ);
		if(!device || !device->isValid()) {
			return AsyncReadResult{ {}, false, "Failed to open asset path: " + pending.path };
		}
		if(device->seek(static_cast<long>(pending.offset), Elv::Io::SeekOrigin::SET) != 0) {
			return AsyncReadResult{ {}, false, "Failed to seek asset path: " + pending.path };
		}

		std::uint64_t bytesToRead = pending.size;
		if(bytesToRead == 0) {
			const std::uint64_t deviceSize = static_cast<std::uint64_t>(device->size());
			bytesToRead = pending.offset < deviceSize ? deviceSize - pending.offset : 0;
		}

		std::vector<std::byte> data(static_cast<std::size_t>(bytesToRead));
		const size_t readCount = bytesToRead == 0 ? 0 : device->read(data.data(), 1, static_cast<size_t>(bytesToRead));
		data.resize(readCount);
		if(readCount != bytesToRead) {
			return AsyncReadResult{ std::move(data), false, "Short read from asset path: " + pending.path };
		}
		return AsyncReadResult{ std::move(data), false, {} };
	} catch(const std::exception& e) {
		return AsyncReadResult{ {}, false, e.what() };
	} catch(...) {
		return AsyncReadResult{ {}, false, "Unknown async IO failure" };
	}
}

} // namespace Asset
} // namespace Euph
