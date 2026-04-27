#ifndef EUPHASYNCIOSCHEDULER_HPP
#define EUPHASYNCIOSCHEDULER_HPP

#include <Euphemy/Asset/EuphAssetTypes.hpp>
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvIoSystem.hpp>
#include <Elvavena/Util/ElvEventLoop.hpp>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Euph {
namespace Asset {

struct AsyncReadResult {
	std::vector<std::byte> data;
	bool cancelled = false;
	std::string error;

	bool ok() const {
		return !cancelled && error.empty();
	}
};

class MH_EUPH_API AsyncRead {
	friend class AsyncIoScheduler;
private:
	std::uint64_t readId = 0;
	std::shared_ptr<std::atomic_bool> cancelFlag;
	std::future<AsyncReadResult> resultFuture;

	AsyncRead(std::uint64_t id, std::shared_ptr<std::atomic_bool> flag, std::future<AsyncReadResult>&& future);

public:
	AsyncRead() = default;
	AsyncRead(AsyncRead&&) noexcept = default;
	AsyncRead& operator=(AsyncRead&&) noexcept = default;
	AsyncRead(const AsyncRead&) = delete;
	AsyncRead& operator=(const AsyncRead&) = delete;

	std::uint64_t id() const;
	bool valid() const;
	void cancel();
	std::future<AsyncReadResult>& future();
};

class MH_EUPH_API AsyncIoScheduler {
public:
	using Completion = std::function<void(const AsyncReadResult&)>;

private:
	struct PendingRead {
		std::uint64_t id = 0;
		std::uint64_t generation = 0;
		std::uint64_t sequence = 0;
		StreamPriority priority;
		std::string path;
		std::uint64_t offset = 0;
		std::uint64_t size = 0;
		std::shared_ptr<std::atomic_bool> cancelFlag;
		std::shared_ptr<std::promise<AsyncReadResult>> promise;
		Completion completion;
		bool queued = true;
	};

	struct QueueItem {
		StreamPriority priority;
		std::uint64_t sequence = 0;
		std::uint64_t generation = 0;
		std::shared_ptr<PendingRead> pending;
	};

	struct QueueCompare {
		bool operator()(const QueueItem& left, const QueueItem& right) const;
	};

	Elv::Io::System& system;
	Elv::Util::EventLoop* completionLoop;
	std::vector<std::thread> workers;
	std::priority_queue<QueueItem, std::vector<QueueItem>, QueueCompare> queue;
	std::unordered_map<std::uint64_t, std::shared_ptr<PendingRead>> pendingReads;
	std::mutex mutex;
	std::condition_variable cv;
	std::atomic_uint64_t nextId;
	std::uint64_t nextSequence;
	bool stopping;
	bool paused;

	void workerLoop();
	void complete(const std::shared_ptr<PendingRead>& pending, AsyncReadResult&& result);
	AsyncReadResult performRead(const PendingRead& pending);

public:
	AsyncIoScheduler(Elv::Io::System& system,
					 std::size_t workerCount = 1,
					 Elv::Util::EventLoop* completionLoop = nullptr,
					 bool startPaused = false);
	~AsyncIoScheduler();

	AsyncRead read(std::string path,
				   std::uint64_t offset,
				   std::uint64_t size,
				   StreamPriority priority = {},
				   Completion completion = {});
	bool bumpPriority(std::uint64_t readId, StreamPriority priority);
	bool cancel(std::uint64_t readId);
	void resume();
	void setPaused(bool value);
	void stop();
};

} // namespace Asset
} // namespace Euph

#endif // EUPHASYNCIOSCHEDULER_HPP
