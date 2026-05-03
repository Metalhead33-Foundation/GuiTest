#include "EuphAssetRuntime.hpp"

#include <Elvavena/Io/ElvIoAsync.hpp>
#include <algorithm>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <utility>

namespace Euph {
namespace Asset {

namespace {

using AsyncIo = Elv::Io::Async<Elv::Util::ThreadPool>;

constexpr std::size_t budgetIndex(BudgetKind kind)
{
	return static_cast<std::size_t>(kind);
}

bool futureReady(std::future_status status)
{
	return status == std::future_status::ready;
}

} // namespace

const std::vector<AssetId>& AssetCatalog::emptyDependencies()
{
	static const std::vector<AssetId> empty;
	return empty;
}

bool AssetCatalog::addRecord(AssetRecord record)
{
	if (record.id == 0)
		return false;

	const AssetId id = record.id;
	const std::string path = record.path;
	records[id] = std::move(record);
	if (!path.empty())
		pathIndex[path] = id;
	return true;
}

const AssetRecord* AssetCatalog::find(AssetId id) const
{
	const auto it = records.find(id);
	return it == records.end() ? nullptr : &it->second;
}

const AssetRecord* AssetCatalog::findByPath(const std::string& path) const
{
	const auto it = pathIndex.find(path);
	return it == pathIndex.end() ? nullptr : find(it->second);
}

std::span<const AssetId> AssetCatalog::dependenciesOf(AssetId id) const
{
	const AssetRecord* record = find(id);
	const std::vector<AssetId>& deps = record ? record->dependencies : emptyDependencies();
	return std::span<const AssetId>(deps.data(), deps.size());
}

CatalogValidation AssetCatalog::validateDependencies() const
{
	CatalogValidation result;
	for (const auto& pair : records) {
		for (AssetId dependency : pair.second.dependencies) {
			if (!find(dependency)) {
				result.ok = false;
				result.missingDependencies.push_back(dependency);
			}
		}
	}
	std::sort(result.missingDependencies.begin(), result.missingDependencies.end());
	result.missingDependencies.erase(std::unique(result.missingDependencies.begin(), result.missingDependencies.end()), result.missingDependencies.end());
	return result;
}

std::size_t AssetCatalog::size() const noexcept
{
	return records.size();
}

DependencyGraph::DependencyGraph(const AssetCatalog& assetCatalog)
	: catalog(assetCatalog)
{
}

bool DependencyGraph::visit(AssetId id, std::unordered_map<AssetId, std::uint8_t>& marks, DependencyResult& result) const
{
	const AssetRecord* record = catalog.find(id);
	if (!record) {
		result.ok = false;
		result.failure = FailureReason::MissingDependency;
		result.missingDependencies.push_back(id);
		return false;
	}

	std::uint8_t& mark = marks[id];
	if (mark == 1) {
		result.ok = false;
		result.failure = FailureReason::DependencyCycle;
		result.cycle.push_back(id);
		return false;
	}
	if (mark == 2)
		return true;

	mark = 1;
	for (AssetId dependency : record->dependencies) {
		if (!visit(dependency, marks, result)) {
			if (result.failure == FailureReason::DependencyCycle)
				result.cycle.push_back(id);
			return false;
		}
	}
	mark = 2;
	result.orderedDependencies.push_back(id);
	return true;
}

DependencyResult DependencyGraph::resolveTransitive(AssetId id) const
{
	DependencyResult result;
	std::unordered_map<AssetId, std::uint8_t> marks;
	if (!visit(id, marks, result))
		return result;
	result.orderedDependencies.erase(std::remove(result.orderedDependencies.begin(), result.orderedDependencies.end(), id), result.orderedDependencies.end());
	return result;
}

bool DependencyGraph::dependenciesResident(AssetId id, const ResourceRegistry& registry, AssetId* blockingDependency) const
{
	const DependencyResult deps = resolveTransitive(id);
	if (!deps.ok) {
		if (blockingDependency) {
			if (!deps.missingDependencies.empty())
				*blockingDependency = deps.missingDependencies.front();
			else if (!deps.cycle.empty())
				*blockingDependency = deps.cycle.front();
		}
		return false;
	}

	for (AssetId dependency : deps.orderedDependencies) {
		const ResidencyState depState = registry.state(dependency);
		if (depState != ResidencyState::Resident && depState != ResidencyState::Evictable) {
			if (blockingDependency)
				*blockingDependency = dependency;
			return false;
		}
	}
	return true;
}

std::size_t ResourceRegistry::kindIndex(ResourceKind kind)
{
	return static_cast<std::size_t>(kind);
}

ResourceRegistry::Entry* ResourceRegistry::entryFor(AssetId id)
{
	const auto it = slotByAsset.find(id);
	if (it == slotByAsset.end() || it->second >= entries.size())
		return nullptr;
	Entry& entry = entries[it->second];
	return entry.active ? &entry : nullptr;
}

const ResourceRegistry::Entry* ResourceRegistry::entryFor(AssetId id) const
{
	return const_cast<ResourceRegistry*>(this)->entryFor(id);
}

bool ResourceRegistry::legalTransition(ResidencyState from, ResidencyState to)
{
	if (from == to)
		return true;
	if (to == ResidencyState::Failed)
		return true;

	switch (from) {
		case ResidencyState::Missing:
			return to == ResidencyState::Known;
		case ResidencyState::Known:
			return to == ResidencyState::Requested || to == ResidencyState::Missing;
		case ResidencyState::Requested:
			return to == ResidencyState::WaitingDependencies || to == ResidencyState::LoadingIO || to == ResidencyState::Known;
		case ResidencyState::WaitingDependencies:
			return to == ResidencyState::Requested || to == ResidencyState::LoadingIO || to == ResidencyState::Known;
		case ResidencyState::LoadingIO:
			return to == ResidencyState::Decoding || to == ResidencyState::Known;
		case ResidencyState::Decoding:
			return to == ResidencyState::WaitingUpload || to == ResidencyState::Resident || to == ResidencyState::Known;
		case ResidencyState::WaitingUpload:
			return to == ResidencyState::Uploading || to == ResidencyState::Resident || to == ResidencyState::Known;
		case ResidencyState::Uploading:
			return to == ResidencyState::Resident || to == ResidencyState::Known;
		case ResidencyState::Resident:
			return to == ResidencyState::Evictable || to == ResidencyState::Evicting;
		case ResidencyState::Evictable:
			return to == ResidencyState::Resident || to == ResidencyState::Evicting || to == ResidencyState::Requested;
		case ResidencyState::Evicting:
			return to == ResidencyState::Known || to == ResidencyState::Missing;
		case ResidencyState::Failed:
			return to == ResidencyState::Requested || to == ResidencyState::Known || to == ResidencyState::Missing;
	}
	return false;
}

ResourceRegistry::ResourceRegistry()
{
	placeholders.fill(std::nullopt);
}

bool ResourceRegistry::destroy(AssetId id)
{
	const auto it = slotByAsset.find(id);
	if (it == slotByAsset.end() || it->second >= entries.size())
		return false;

	Entry& entry = entries[it->second];
	if (!entry.active)
		return false;

	entry.active = false;
	entry.generation += 1;
	entry.state = ResidencyState::Missing;
	entry.flags = ResidencyFlag::None;
	entry.kaldiHandle.reset();
	entry.failureReason = FailureReason::None;
	entry.failureMessage.clear();
	entry.costs.fill(0);

	for (auto& placeholderSlot : placeholders) {
		if (placeholderSlot && *placeholderSlot == it->second)
			placeholderSlot.reset();
	}

	freeSlots.push_back(it->second);
	slotByAsset.erase(it);
	return true;
}

bool ResourceRegistry::isAlive(AssetId id) const
{
	return entryFor(id) != nullptr;
}

ResidencyState ResourceRegistry::state(AssetId id) const
{
	const Entry* entry = entryFor(id);
	return entry ? entry->state : ResidencyState::Missing;
}

bool ResourceRegistry::setState(AssetId id, ResidencyState newState)
{
	Entry* entry = entryFor(id);
	if (!entry)
		return false;
	if (!legalTransition(entry->state, newState)) {
		entry->failureReason = FailureReason::InvalidTransition;
		return false;
	}

	entry->state = newState;
	if (newState != ResidencyState::Failed) {
		entry->failureReason = FailureReason::None;
		entry->failureMessage.clear();
	}
	return true;
}

bool ResourceRegistry::fail(AssetId id, FailureReason reason, std::string message)
{
	Entry* entry = entryFor(id);
	if (!entry)
		return false;
	entry->state = ResidencyState::Failed;
	entry->failureReason = reason == FailureReason::None ? FailureReason::Unknown : reason;
	entry->failureMessage = std::move(message);
	return true;
}

FailureReason ResourceRegistry::failureReason(AssetId id) const
{
	const Entry* entry = entryFor(id);
	return entry ? entry->failureReason : FailureReason::AssetNotFound;
}

const std::string& ResourceRegistry::failureMessage(AssetId id) const
{
	static const std::string empty;
	const Entry* entry = entryFor(id);
	return entry ? entry->failureMessage : empty;
}

bool ResourceRegistry::touch(AssetId id, std::uint64_t frameIndex)
{
	Entry* entry = entryFor(id);
	if (!entry)
		return false;
	entry->lastTouchedFrame = frameIndex;
	return true;
}

void ResourceRegistry::setPriority(AssetId id, StreamPriority newPriority)
{
	if (Entry* entry = entryFor(id))
		entry->priority = newPriority;
}

StreamPriority ResourceRegistry::priority(AssetId id) const
{
	const Entry* entry = entryFor(id);
	return entry ? entry->priority : StreamPriority {};
}

void ResourceRegistry::setCost(AssetId id, BudgetKind kind, std::uint64_t bytes)
{
	if (Entry* entry = entryFor(id))
		entry->costs[budgetIndex(kind)] = bytes;
}

std::uint64_t ResourceRegistry::cost(AssetId id, BudgetKind kind) const
{
	const Entry* entry = entryFor(id);
	return entry ? entry->costs[budgetIndex(kind)] : 0;
}

std::uint64_t ResourceRegistry::cpuCost(AssetId id) const
{
	return cost(id, BudgetKind::CpuBytes);
}

std::uint64_t ResourceRegistry::gpuCost(AssetId id) const
{
	return cost(id, BudgetKind::GpuBytes);
}

void ResourceRegistry::clearCosts(AssetId id)
{
	if (Entry* entry = entryFor(id))
		entry->costs.fill(0);
}

void ResourceRegistry::setFlag(AssetId id, ResidencyFlag flag, bool enabled)
{
	if (Entry* entry = entryFor(id)) {
		if (enabled)
			entry->flags |= flag;
		else
			entry->flags = static_cast<ResidencyFlag>(static_cast<std::uint32_t>(entry->flags) & ~static_cast<std::uint32_t>(flag));
	}
}

bool ResourceRegistry::hasFlag(AssetId id, ResidencyFlag flag) const
{
	const Entry* entry = entryFor(id);
	return entry ? Asset::hasFlag(entry->flags, flag) : false;
}

void ResourceRegistry::setKaldiHandle(AssetId id, Kld::HandleId handle)
{
	if (Entry* entry = entryFor(id)) {
		entry->kaldiHandle = handle;
		setFlag(id, ResidencyFlag::GpuResident, true);
	}
}

void ResourceRegistry::clearKaldiHandle(AssetId id)
{
	if (Entry* entry = entryFor(id)) {
		entry->kaldiHandle.reset();
		setFlag(id, ResidencyFlag::GpuResident, false);
	}
}

std::optional<Kld::HandleId> ResourceRegistry::kaldiHandle(AssetId id) const
{
	const Entry* entry = entryFor(id);
	return entry ? entry->kaldiHandle : std::nullopt;
}

std::vector<AssetId> ResourceRegistry::activeAssetIds() const
{
	std::vector<AssetId> result;
	result.reserve(slotByAsset.size());
	for (const Entry& entry : entries) {
		if (entry.active)
			result.push_back(entry.id);
	}
	return result;
}

const ResourceRegistry::Entry* ResourceRegistry::inspect(AssetId id) const
{
	return entryFor(id);
}

ResidencyManager::ResidencyManager(ResourceRegistry& resourceRegistry)
	: registry(resourceRegistry)
{
	budgets.fill(std::numeric_limits<std::uint64_t>::max());
}

void ResidencyManager::setBudget(BudgetKind kind, std::uint64_t bytes)
{
	budgets[budgetIndex(kind)] = bytes;
}

std::uint64_t ResidencyManager::budget(BudgetKind kind) const
{
	return budgets[budgetIndex(kind)];
}

std::uint64_t ResidencyManager::currentUsage(BudgetKind kind) const
{
	std::uint64_t total = 0;
	for (AssetId id : registry.activeAssetIds())
		total += registry.cost(id, kind);
	return total;
}

bool ResidencyManager::makeEvictable(AssetId id)
{
	return registry.setState(id, ResidencyState::Evictable);
}

void ResidencyManager::pin(AssetId id)
{
	registry.setFlag(id, ResidencyFlag::Pinned, true);
}

void ResidencyManager::unpin(AssetId id)
{
	registry.setFlag(id, ResidencyFlag::Pinned, false);
}

bool ResidencyManager::withinBudgets() const
{
	for (std::size_t i = 0; i < budgetIndex(BudgetKind::Count); ++i) {
		if (currentUsage(static_cast<BudgetKind>(i)) > budgets[i])
			return false;
	}
	return true;
}

std::uint64_t ResidencyManager::totalReleasableCost(const ResourceRegistry::Entry& entry)
{
	std::uint64_t total = 0;
	for (std::uint64_t cost : entry.costs)
		total += cost;
	return total;
}

void ResidencyManager::recordDestroy(Kld::CommandBuffer& commandBuffer, const ResourceRegistry::Entry& entry)
{
	if (!entry.kaldiHandle)
		return;

	switch (entry.kind) {
		case ResourceKind::Texture: commandBuffer.destroyTexture(*entry.kaldiHandle); break;
		case ResourceKind::Buffer: commandBuffer.destroyBuffer(*entry.kaldiHandle); break;
		case ResourceKind::Audio: break;
		case ResourceKind::GenericBlob: break;
	}
}

std::vector<AssetId> ResidencyManager::evictUntilWithinBudget(Kld::CommandBuffer* commandBuffer)
{
	std::vector<AssetId> evicted;

	while (!withinBudgets()) {
		std::vector<const ResourceRegistry::Entry*> candidates;
		for (AssetId id : registry.activeAssetIds()) {
			const ResourceRegistry::Entry* entry = registry.inspect(id);
			if (!entry || entry->state != ResidencyState::Evictable || Asset::hasFlag(entry->flags, ResidencyFlag::Pinned))
				continue;
			candidates.push_back(entry);
		}

		if (candidates.empty())
			break;

		std::sort(candidates.begin(), candidates.end(), [](const ResourceRegistry::Entry* left, const ResourceRegistry::Entry* right) {
			if (left->priority.value != right->priority.value)
				return left->priority.value < right->priority.value;
			if (left->lastTouchedFrame != right->lastTouchedFrame)
				return left->lastTouchedFrame < right->lastTouchedFrame;
			const std::uint64_t leftCost = totalReleasableCost(*left);
			const std::uint64_t rightCost = totalReleasableCost(*right);
			if (leftCost != rightCost)
				return leftCost > rightCost;
			return left->id < right->id;
		});

		const ResourceRegistry::Entry* victim = candidates.front();
		const AssetId victimId = victim->id;
		if (commandBuffer)
			recordDestroy(*commandBuffer, *victim);
		registry.setState(victimId, ResidencyState::Evicting);
		registry.clearCosts(victimId);
		registry.clearKaldiHandle(victimId);
		registry.setFlag(victimId, ResidencyFlag::CpuResident, false);
		registry.setFlag(victimId, ResidencyFlag::EvictionRequested, false);
		registry.setState(victimId, ResidencyState::Known);
		evicted.push_back(victimId);
	}

	return evicted;
}

UploadQueue::UploadQueue(ResourceRegistry& resourceRegistry)
	: registry(resourceRegistry)
{
}

void UploadQueue::enqueue(UploadItem item, RecordCallback record)
{
	if (!item.payload)
		item.payload = std::make_shared<std::vector<std::byte> >();
	registry.setCost(item.id, BudgetKind::GpuUploadInflightBytes, item.gpuBytes);
	registry.setState(item.id, ResidencyState::WaitingUpload);
	pending.push_back({ std::move(item), std::move(record) });
}

std::uint64_t UploadQueue::recordInto(Kld::CommandBuffer& commandBuffer, std::uint64_t maxUploadBytes)
{
	std::uint64_t recordedBytes = 0;
	std::vector<PendingUpload> remaining;
	remaining.reserve(pending.size());

	for (PendingUpload& upload : pending) {
		const std::uint64_t uploadBytes = upload.item.gpuBytes;
		if (recordedBytes != 0 && recordedBytes + uploadBytes > maxUploadBytes) {
			remaining.push_back(std::move(upload));
			continue;
		}
		if (recordedBytes == 0 && uploadBytes > maxUploadBytes && maxUploadBytes != 0) {
			remaining.push_back(std::move(upload));
			continue;
		}

		registry.setState(upload.item.id, ResidencyState::Uploading);
		if (upload.record)
			upload.record(commandBuffer, upload.item);
		recordedBytes += uploadBytes;
		recorded.push_back(std::move(upload));
	}

	pending = std::move(remaining);
	return recordedBytes;
}

void UploadQueue::commitRecorded()
{
	for (const PendingUpload& upload : recorded) {
		registry.setCost(upload.item.id, BudgetKind::GpuBytes, upload.item.gpuBytes);
		registry.setCost(upload.item.id, BudgetKind::GpuUploadInflightBytes, 0);
		if (upload.item.kaldiHandle)
			registry.setKaldiHandle(upload.item.id, *upload.item.kaldiHandle);
		registry.setFlag(upload.item.id, ResidencyFlag::GpuResident, true);
		registry.setState(upload.item.id, ResidencyState::Resident);
	}
	recorded.clear();
}

void UploadQueue::failRecorded(FailureReason reason, const std::string& message)
{
	for (const PendingUpload& upload : recorded) {
		registry.setCost(upload.item.id, BudgetKind::GpuUploadInflightBytes, 0);
		registry.fail(upload.item.id, reason, message);
	}
	recorded.clear();
}

std::size_t UploadQueue::pendingCount() const noexcept
{
	return pending.size();
}

std::size_t UploadQueue::recordedCount() const noexcept
{
	return recorded.size();
}

struct StreamingScheduler::Job {
	enum class Stage {
		Io,
		Decode
	};

	AssetId id = 0;
	AssetRecord record;
	Stage stage = Stage::Io;
	std::stop_source stopSource;
	std::future<Elv::Io::ByteVector> ioFuture;
	std::future<DecodedAsset> decodeFuture;
};

StreamingScheduler::StreamingScheduler(const AssetCatalog& assetCatalog, ResourceRegistry& resourceRegistry, Elv::Util::ThreadPool& threadPool, Loader loaderCallback, Decoder decoderCallback, UploadQueue* uploads)
	: catalog(assetCatalog)
	, registry(resourceRegistry)
	, executor(threadPool)
	, loader(std::move(loaderCallback))
	, decoder(std::move(decoderCallback))
	, uploadQueue(uploads)
{
}

StreamingScheduler::~StreamingScheduler() = default;

bool StreamingScheduler::request(AssetId id, StreamPriority requestPriority)
{
	if (!registry.isAlive(id))
		registry.createHandle<GenericBlobResource>(id);

	const AssetRecord* record = catalog.find(id);
	if (!record)
		return registry.fail(id, FailureReason::AssetNotFound, "Asset record was not found");

	registry.setPriority(id, requestPriority);
	if (!registry.setState(id, ResidencyState::Requested) && registry.state(id) != ResidencyState::Requested)
		return false;

	DependencyGraph graph(catalog);
	const DependencyResult deps = graph.resolveTransitive(id);
	if (!deps.ok) {
		return registry.fail(id, deps.failure, deps.failure == FailureReason::DependencyCycle ? "Dependency cycle detected" : "Missing dependency");
	}

	AssetId blockingDependency = 0;
	if (!graph.dependenciesResident(id, registry, &blockingDependency)) {
		registry.setState(id, ResidencyState::WaitingDependencies);
		return true;
	}

	if (!loader)
		return registry.fail(id, FailureReason::IoError, "Streaming scheduler has no loader callback");

	Elv::Io::sDevice device;
	try {
		device = loader(*record);
	} catch (const std::exception& err) {
		return registry.fail(id, FailureReason::IoError, err.what());
	} catch (...) {
		return registry.fail(id, FailureReason::IoError, "Loader failed");
	}
	if (!device)
		return registry.fail(id, FailureReason::IoError, "Loader returned a null device");

	registry.setState(id, ResidencyState::LoadingIO);
	registry.setCost(id, BudgetKind::IoInflightBytes, record->storedSize);
	std::stop_source stopSource;
	auto future = AsyncIo::readAll(executor, std::move(device), stopSource.get_token());
	jobs.push_back(Job {
		id,
		*record,
		Job::Stage::Io,
		std::move(stopSource),
		std::move(future),
		std::future<DecodedAsset>()
	});
	return true;
}

bool StreamingScheduler::cancel(AssetId id)
{
	bool canceled = false;
	for (Job& job : jobs) {
		if (job.id == id) {
			job.stopSource.request_stop();
			canceled = true;
		}
	}
	if (canceled)
		registry.fail(id, FailureReason::Canceled, "Streaming request canceled");
	return canceled;
}

void StreamingScheduler::completeIo(Job& job)
{
	try {
		Elv::Io::ByteVector bytes = job.ioFuture.get();
		registry.setCost(job.id, BudgetKind::IoInflightBytes, 0);
		registry.setCost(job.id, BudgetKind::DecodeInflightBytes, bytes.size());
		registry.setState(job.id, ResidencyState::Decoding);

		if (!decoder)
			throw std::runtime_error("Streaming scheduler has no decoder callback");

		job.decodeFuture = executor.enqueueAsync([record = job.record, decoder = decoder, bytes = std::move(bytes)]() mutable {
			return decoder(record, std::span<const std::byte>(bytes.data(), bytes.size()));
		});
		job.stage = Job::Stage::Decode;
	} catch (const std::exception& err) {
		failJob(job, FailureReason::IoError, err.what());
	} catch (...) {
		failJob(job, FailureReason::IoError, "I/O failed");
	}
}

void StreamingScheduler::completeDecode(Job& job)
{
	try {
		DecodedAsset decoded = job.decodeFuture.get();
		registry.setCost(job.id, BudgetKind::DecodeInflightBytes, 0);
		registry.setCost(job.id, BudgetKind::CpuBytes, decoded.cpuBytes != 0 ? decoded.cpuBytes : decoded.cpuPayload.size());
		registry.setCost(job.id, BudgetKind::AudioBytes, decoded.audioBytes);
		registry.setFlag(job.id, ResidencyFlag::CpuResident, !decoded.cpuPayload.empty() || decoded.cpuBytes != 0);

		if (decoded.requiresUpload) {
			if (!uploadQueue)
				throw std::runtime_error("Decoded asset requires upload but no UploadQueue was provided");
			if (!decoded.uploadPayload)
				decoded.uploadPayload = std::make_shared<std::vector<std::byte> >();
			UploadItem item;
			item.id = job.id;
			item.payload = std::move(decoded.uploadPayload);
			item.gpuBytes = decoded.gpuBytes;
			item.kaldiHandle = decoded.kaldiHandle;
			uploadQueue->enqueue(std::move(item), std::move(decoded.uploadRecord));
		} else {
			registry.setState(job.id, ResidencyState::Resident);
		}
	} catch (const std::exception& err) {
		failJob(job, FailureReason::DecodeError, err.what());
	} catch (...) {
		failJob(job, FailureReason::DecodeError, "Decode failed");
	}
}

void StreamingScheduler::failJob(Job& job, FailureReason reason, const std::string& message)
{
	registry.setCost(job.id, BudgetKind::IoInflightBytes, 0);
	registry.setCost(job.id, BudgetKind::DecodeInflightBytes, 0);
	registry.fail(job.id, reason, message);
}

void StreamingScheduler::pollCompletions()
{
	for (Job& job : jobs) {
		if (job.stage == Job::Stage::Io && futureReady(job.ioFuture.wait_for(std::chrono::seconds(0))))
			completeIo(job);
		if (job.stage == Job::Stage::Decode && futureReady(job.decodeFuture.wait_for(std::chrono::seconds(0))))
			completeDecode(job);
	}

	jobs.erase(std::remove_if(jobs.begin(), jobs.end(), [this](const Job& job) {
		const ResidencyState current = registry.state(job.id);
		return current == ResidencyState::Resident || current == ResidencyState::WaitingUpload || current == ResidencyState::Failed;
	}), jobs.end());
}

std::uint64_t StreamingScheduler::enqueueUploads(Kld::CommandBuffer& commandBuffer, std::uint64_t maxUploadBytes)
{
	return uploadQueue ? uploadQueue->recordInto(commandBuffer, maxUploadBytes) : 0;
}

ResidencyState StreamingScheduler::state(AssetId id) const
{
	return registry.state(id);
}

std::size_t StreamingScheduler::pendingJobs() const noexcept
{
	return jobs.size();
}

} // namespace Asset
} // namespace Euph
