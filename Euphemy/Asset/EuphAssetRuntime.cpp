#include "EuphAssetRuntime.hpp"
/**
 * @file EuphAssetRuntime.cpp
 * @brief Implements Euphemy's CPU-side asset streaming runtime.
 */

#include <algorithm>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace Euph {
namespace Asset {

namespace {

constexpr std::size_t budgetIndex(BudgetKind kind)
{
	return static_cast<std::size_t>(kind);
}

bool futureReady(std::future_status status)
{
	return status == std::future_status::ready;
}

ResourceKind kindFromRecord(const AssetRecord& record)
{
	if (record.type == "image" || record.type == "texture" || record.type == "png" || record.type == "jpg" || record.type == "jpeg" || record.type == "tga" || record.type == "dds")
		return ResourceKind::Image;
	if (record.type == "audio" || record.type == "sound" || record.type == "music" || record.type == "wav" || record.type == "ogg" || record.type == "module")
		return ResourceKind::Audio;
	return ResourceKind::Blob;
}

std::vector<std::byte> readRecordBytes(Elv::Io::Device& device, const AssetRecord& record, std::stop_token stopToken)
{
	if (stopToken.stop_requested())
		throw std::runtime_error("Streaming read canceled");
	if (!device.isValid())
		throw std::runtime_error("Streaming read attempted on an invalid device");

	const size_t deviceSize = device.size();
	if (record.offset > deviceSize)
		throw std::runtime_error("Asset offset is beyond device size");
	if (device.seek(static_cast<long>(record.offset), Elv::Io::SeekOrigin::SET) != 0)
		throw std::runtime_error("Asset read seek failed");

	const size_t available = deviceSize - static_cast<size_t>(record.offset);
	const size_t requested = record.storedSize == 0 ? available : static_cast<size_t>(record.storedSize);
	if (requested > available)
		throw std::runtime_error("Asset stored size exceeds device size");

	std::vector<std::byte> bytes;
	bytes.resize(requested);
	size_t totalRead = 0;
	while (totalRead < requested) {
		if (stopToken.stop_requested())
			throw std::runtime_error("Streaming read canceled");
		const size_t got = device.read(bytes.data() + totalRead, 1, requested - totalRead);
		if (got == 0)
			throw std::runtime_error("Asset read ended before stored size");
		totalRead += got;
	}
	return bytes;
}

} // namespace

ResidentPayload makeBytePayload(std::vector<std::byte> bytes, std::string type)
{
	const std::uint64_t byteCount = bytes.size();
	auto payload = std::make_shared<std::vector<std::byte> >(std::move(bytes));
	return { payload, byteCount, std::move(type) };
}

const std::vector<AssetId>& AssetCatalog::emptyDependencies()
{
	static const std::vector<AssetId> empty;
	return empty;
}

void AssetCatalog::removePathIndexFor(AssetId id)
{
	for (auto it = pathIndex.begin(); it != pathIndex.end();) {
		if (it->second == id)
			it = pathIndex.erase(it);
		else
			++it;
	}
}

bool AssetCatalog::addRecord(AssetRecord record)
{
	if (record.id == 0 || record.type.empty())
		return false;

	if (!record.path.empty()) {
		const auto pathIt = pathIndex.find(record.path);
		if (pathIt != pathIndex.end() && pathIt->second != record.id)
			return false;
	}

	removePathIndexFor(record.id);
	const AssetId id = record.id;
	const std::string path = record.path;
	records[id] = std::move(record);
	if (!path.empty())
		pathIndex[path] = id;
	return true;
}

bool AssetCatalog::removeRecord(AssetId id)
{
	const auto removed = records.erase(id);
	if (removed == 0)
		return false;
	removePathIndexFor(id);
	return true;
}

void AssetCatalog::clear()
{
	records.clear();
	pathIndex.clear();
}

bool AssetCatalog::contains(AssetId id) const
{
	return records.find(id) != records.end();
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

std::vector<AssetId> AssetCatalog::assetIds() const
{
	std::vector<AssetId> ids;
	ids.reserve(records.size());
	for (const auto& pair : records)
		ids.push_back(pair.first);
	std::sort(ids.begin(), ids.end());
	return ids;
}

CatalogValidation AssetCatalog::validateDependencies() const
{
	CatalogValidation result;
	for (const auto& pair : records) {
		const AssetRecord& record = pair.second;
		if (record.id == 0 || record.type.empty())
			result.invalidRecords.push_back(pair.first);

		std::unordered_set<AssetId> seen;
		for (AssetId dependency : record.dependencies) {
			if (dependency == record.id)
				result.selfDependencies.push_back(record.id);
			if (!seen.insert(dependency).second)
				result.duplicateDependencies.push_back(record.id);
			if (!find(dependency))
				result.missingDependencies.push_back(dependency);
		}
	}

	auto normalize = [](std::vector<AssetId>& values) {
		std::sort(values.begin(), values.end());
		values.erase(std::unique(values.begin(), values.end()), values.end());
	};
	normalize(result.invalidRecords);
	normalize(result.missingDependencies);
	normalize(result.selfDependencies);
	normalize(result.duplicateDependencies);
	result.ok = result.invalidRecords.empty() && result.missingDependencies.empty() && result.selfDependencies.empty() && result.duplicateDependencies.empty();
	return result;
}

std::size_t AssetCatalog::size() const noexcept
{
	return records.size();
}

bool AssetCatalog::empty() const noexcept
{
	return records.empty();
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

std::vector<AssetId> DependencyGraph::directDependencies(AssetId id) const
{
	std::span<const AssetId> deps = catalog.dependenciesOf(id);
	return std::vector<AssetId>(deps.begin(), deps.end());
}

std::vector<AssetId> DependencyGraph::dependentsOf(AssetId id) const
{
	std::vector<AssetId> dependents;
	for (AssetId candidate : catalog.assetIds()) {
		if (hasDependency(candidate, id))
			dependents.push_back(candidate);
	}
	return dependents;
}

bool DependencyGraph::hasDependency(AssetId asset, AssetId dependency) const
{
	const DependencyResult deps = resolveTransitive(asset);
	if (!deps.ok)
		return false;
	return std::find(deps.orderedDependencies.begin(), deps.orderedDependencies.end(), dependency) != deps.orderedDependencies.end();
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
			return to == ResidencyState::Requested || to == ResidencyState::WaitingDependencies || to == ResidencyState::Missing;
		case ResidencyState::Requested:
			return to == ResidencyState::WaitingDependencies || to == ResidencyState::LoadingIO || to == ResidencyState::Known;
		case ResidencyState::WaitingDependencies:
			return to == ResidencyState::Requested || to == ResidencyState::LoadingIO || to == ResidencyState::Known;
		case ResidencyState::LoadingIO:
			return to == ResidencyState::Decoding || to == ResidencyState::Known;
		case ResidencyState::Decoding:
			return to == ResidencyState::WaitingCommit || to == ResidencyState::Resident || to == ResidencyState::Known;
		case ResidencyState::WaitingCommit:
			return to == ResidencyState::Committing || to == ResidencyState::Resident || to == ResidencyState::Known;
		case ResidencyState::Committing:
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

ResourceHandle<GenericBlobResource> ResourceRegistry::createBlobHandle(AssetId id)
{
	Entry* existing = entryFor(id);
	if (existing)
		return { id, existing->generation, slotByAsset[id] };

	std::uint32_t slot = 0;
	if (!freeSlots.empty()) {
		slot = freeSlots.back();
		freeSlots.pop_back();
		Entry& entry = entries[slot];
		const std::uint32_t generation = entry.generation;
		entry = Entry {};
		entry.generation = generation == 0 ? 1 : generation;
		entry.id = id;
		entry.active = true;
		entry.state = ResidencyState::Known;
	} else {
		slot = static_cast<std::uint32_t>(entries.size());
		entries.push_back(Entry {});
		entries.back().id = id;
		entries.back().active = true;
		entries.back().state = ResidencyState::Known;
	}
	slotByAsset[id] = slot;
	return { id, entries[slot].generation, slot };
}

ResourceHandle<GenericBlobResource> ResourceRegistry::create(ResourceKind kind, AssetId id)
{
	ResourceHandle<GenericBlobResource> handle = createBlobHandle(id);
	if (Entry* entry = entryFor(id))
		entry->kind = kind;
	return handle;
}

bool ResourceRegistry::destroy(AssetId id)
{
	const auto it = slotByAsset.find(id);
	if (it == slotByAsset.end() || it->second >= entries.size())
		return false;

	Entry& entry = entries[it->second];
	if (!entry.active || entry.strongRefs != 0)
		return false;

	entry.active = false;
	entry.generation += 1;
	entry.state = ResidencyState::Missing;
	entry.flags = ResidencyFlag::None;
	entry.failureReason = FailureReason::None;
	entry.failureMessage.clear();
	entry.costs.fill(0);
	entry.payload = {};

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
	entry->costs[budgetIndex(BudgetKind::IoInflightBytes)] = 0;
	entry->costs[budgetIndex(BudgetKind::DecodeInflightBytes)] = 0;
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

bool ResourceRegistry::retain(AssetId id)
{
	Entry* entry = entryFor(id);
	if (!entry)
		return false;
	++entry->strongRefs;
	return true;
}

bool ResourceRegistry::release(AssetId id)
{
	Entry* entry = entryFor(id);
	if (!entry || entry->strongRefs == 0)
		return false;
	--entry->strongRefs;
	return true;
}

bool ResourceRegistry::retainWeak(AssetId id)
{
	Entry* entry = entryFor(id);
	if (!entry)
		return false;
	++entry->weakRefs;
	return true;
}

bool ResourceRegistry::releaseWeak(AssetId id)
{
	Entry* entry = entryFor(id);
	if (!entry || entry->weakRefs == 0)
		return false;
	--entry->weakRefs;
	return true;
}

std::uint32_t ResourceRegistry::strongRefs(AssetId id) const
{
	const Entry* entry = entryFor(id);
	return entry ? entry->strongRefs : 0;
}

std::uint32_t ResourceRegistry::weakRefs(AssetId id) const
{
	const Entry* entry = entryFor(id);
	return entry ? entry->weakRefs : 0;
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

std::uint64_t ResourceRegistry::audioCost(AssetId id) const
{
	return cost(id, BudgetKind::AudioBytes);
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

void ResourceRegistry::setPayload(AssetId id, ResidentPayload payload)
{
	if (Entry* entry = entryFor(id)) {
		entry->payload = std::move(payload);
		entry->costs[budgetIndex(BudgetKind::CpuBytes)] = entry->payload.bytes;
		setFlag(id, ResidencyFlag::CpuResident, static_cast<bool>(entry->payload));
	}
}

void ResourceRegistry::clearPayload(AssetId id)
{
	if (Entry* entry = entryFor(id)) {
		entry->payload = {};
		entry->costs[budgetIndex(BudgetKind::CpuBytes)] = 0;
		setFlag(id, ResidencyFlag::CpuResident, false);
	}
}

const ResidentPayload* ResourceRegistry::payload(AssetId id) const
{
	const Entry* entry = entryFor(id);
	return entry ? &entry->payload : nullptr;
}

std::vector<AssetId> ResourceRegistry::activeAssetIds() const
{
	std::vector<AssetId> result;
	result.reserve(slotByAsset.size());
	for (const Entry& entry : entries) {
		if (entry.active)
			result.push_back(entry.id);
	}
	std::sort(result.begin(), result.end());
	return result;
}

const ResourceRegistry::Entry* ResourceRegistry::inspect(AssetId id) const
{
	return entryFor(id);
}

ResidencyManager::ResidencyManager(ResourceRegistry& resourceRegistry, const AssetCatalog* assetCatalog)
	: registry(resourceRegistry)
	, catalog(assetCatalog)
{
	budgets.fill(std::numeric_limits<std::uint64_t>::max());
}

void ResidencyManager::setCatalog(const AssetCatalog* assetCatalog)
{
	catalog = assetCatalog;
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

bool ResidencyManager::hasResidentDependent(AssetId id) const
{
	if (!catalog)
		return false;

	DependencyGraph graph(*catalog);
	for (AssetId other : registry.activeAssetIds()) {
		if (other == id)
			continue;
		const ResourceRegistry::Entry* entry = registry.inspect(other);
		if (!entry)
			continue;
		if ((entry->state == ResidencyState::Resident || entry->state == ResidencyState::Evictable) && graph.hasDependency(other, id))
			return true;
	}
	return false;
}

EvictionResult ResidencyManager::evictUntilWithinBudgetDetailed()
{
	EvictionResult result;

	while (!withinBudgets()) {
		std::vector<const ResourceRegistry::Entry*> candidates;
		for (AssetId id : registry.activeAssetIds()) {
			const ResourceRegistry::Entry* entry = registry.inspect(id);
			if (!entry || entry->state != ResidencyState::Evictable)
				continue;
			if (Asset::hasFlag(entry->flags, ResidencyFlag::Pinned) || entry->strongRefs != 0 || hasResidentDependent(id)) {
				result.blocked.push_back(id);
				continue;
			}
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

		const AssetId victimId = candidates.front()->id;
		registry.setState(victimId, ResidencyState::Evicting);
		registry.clearPayload(victimId);
		registry.clearCosts(victimId);
		registry.setFlag(victimId, ResidencyFlag::EvictionRequested, false);
		registry.setState(victimId, ResidencyState::Known);
		result.evicted.push_back(victimId);
	}

	std::sort(result.blocked.begin(), result.blocked.end());
	result.blocked.erase(std::unique(result.blocked.begin(), result.blocked.end()), result.blocked.end());
	result.withinBudget = withinBudgets();
	return result;
}

std::vector<AssetId> ResidencyManager::evictUntilWithinBudget()
{
	return evictUntilWithinBudgetDetailed().evicted;
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
	std::future<std::vector<std::byte> > ioFuture;
	std::future<DecodedAsset> decodeFuture;
};

StreamingScheduler::StreamingScheduler(const AssetCatalog& assetCatalog, ResourceRegistry& resourceRegistry, Elv::Util::ThreadPool& threadPool, Loader loaderCallback, Decoder decoderCallback, StreamingSchedulerOptions schedulerOptions)
	: catalog(assetCatalog)
	, registry(resourceRegistry)
	, executor(threadPool)
	, loader(std::move(loaderCallback))
	, decoder(std::move(decoderCallback))
	, options(schedulerOptions)
{
	if (options.maxIoJobs == 0)
		options.maxIoJobs = 1;
}

StreamingScheduler::~StreamingScheduler() = default;

bool StreamingScheduler::hasQueued(AssetId id) const
{
	return std::find(queued.begin(), queued.end(), id) != queued.end();
}

bool StreamingScheduler::hasWaiting(AssetId id) const
{
	return std::find(waiting.begin(), waiting.end(), id) != waiting.end();
}

bool StreamingScheduler::hasJob(AssetId id) const
{
	return std::any_of(jobs.begin(), jobs.end(), [id](const Job& job) { return job.id == id; });
}

bool StreamingScheduler::queueReadyRequest(AssetId id, StreamPriority priority)
{
	registry.setPriority(id, priority);
	const ResidencyState current = registry.state(id);
	if (current == ResidencyState::Resident || current == ResidencyState::Evictable)
		return true;
	if (hasJob(id) || hasQueued(id))
		return true;
	if (!registry.setState(id, ResidencyState::Requested) && registry.state(id) != ResidencyState::Requested)
		return false;
	queued.push_back(id);
	return true;
}

bool StreamingScheduler::requestInternal(AssetId id, StreamPriority requestPriority, bool dependencyRequest)
{
	const AssetRecord* record = catalog.find(id);
	if (!record) {
		registry.create(ResourceKind::Blob, id);
		return registry.fail(id, FailureReason::AssetNotFound, "Asset record was not found");
	}

	if (!registry.isAlive(id))
		registry.create(kindFromRecord(*record), id);
	else if (registry.state(id) == ResidencyState::Failed && options.retryFailedRequests)
		registry.setState(id, ResidencyState::Known);

	registry.setPriority(id, requestPriority);

	DependencyGraph graph(catalog);
	const DependencyResult deps = graph.resolveTransitive(id);
	if (!deps.ok)
		return registry.fail(id, deps.failure, deps.failure == FailureReason::DependencyCycle ? "Dependency cycle detected" : "Missing dependency");

	bool dependenciesReady = true;
	for (AssetId dependency : deps.orderedDependencies) {
		const ResidencyState dependencyState = registry.state(dependency);
		if (dependencyState != ResidencyState::Resident && dependencyState != ResidencyState::Evictable) {
			dependenciesReady = false;
			if (options.autoRequestDependencies)
				requestInternal(dependency, requestPriority, true);
		}
	}

	if (!dependenciesReady) {
		registry.setState(id, ResidencyState::WaitingDependencies);
		if (!hasWaiting(id))
			waiting.push_back(id);
		return true;
	}

	(void)dependencyRequest;
	return queueReadyRequest(id, requestPriority);
}

bool StreamingScheduler::request(AssetId id, StreamPriority priority)
{
	return requestInternal(id, priority, false);
}

bool StreamingScheduler::cancel(AssetId id)
{
	bool canceled = false;
	auto removeId = [id, &canceled](auto& container) {
		const auto oldSize = container.size();
		container.erase(std::remove(container.begin(), container.end(), id), container.end());
		canceled = canceled || container.size() != oldSize;
	};
	removeId(queued);
	removeId(waiting);

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

void StreamingScheduler::startQueuedRequests()
{
	while (jobs.size() < options.maxIoJobs && !queued.empty()) {
		auto best = std::max_element(queued.begin(), queued.end(), [this](AssetId left, AssetId right) {
			const StreamPriority leftPriority = registry.priority(left);
			const StreamPriority rightPriority = registry.priority(right);
			if (leftPriority.value != rightPriority.value)
				return leftPriority.value < rightPriority.value;
			return left > right;
		});

		const AssetId id = *best;
		queued.erase(best);
		const AssetRecord* record = catalog.find(id);
		if (!record) {
			registry.fail(id, FailureReason::AssetNotFound, "Asset record disappeared before load");
			continue;
		}
		if (!loader) {
			registry.fail(id, FailureReason::IoError, "Streaming scheduler has no loader callback");
			continue;
		}

		Elv::Io::sDevice device;
		try {
			device = loader(*record);
		} catch (const std::exception& err) {
			registry.fail(id, FailureReason::IoError, err.what());
			continue;
		} catch (...) {
			registry.fail(id, FailureReason::IoError, "Loader failed");
			continue;
		}
		if (!device) {
			registry.fail(id, FailureReason::IoError, "Loader returned a null device");
			continue;
		}

		registry.setState(id, ResidencyState::LoadingIO);
		registry.setCost(id, BudgetKind::IoInflightBytes, record->storedSize);
		std::stop_source stopSource;
		auto future = executor.enqueueAsync([device = std::move(device), record = *record, token = stopSource.get_token()]() mutable {
			return readRecordBytes(*device, record, token);
		});
		jobs.push_back(Job {
			id,
			*record,
			Job::Stage::Io,
			std::move(stopSource),
			std::move(future),
			std::future<DecodedAsset>()
		});
	}
}

void StreamingScheduler::wakeWaitingRequests()
{
	std::vector<AssetId> stillWaiting;
	for (AssetId id : waiting) {
		DependencyGraph graph(catalog);
		AssetId blockingDependency = 0;
		if (graph.dependenciesResident(id, registry, &blockingDependency)) {
			queueReadyRequest(id, registry.priority(id));
		} else {
			stillWaiting.push_back(id);
		}
	}
	waiting = std::move(stillWaiting);
}

void StreamingScheduler::completeIo(Job& job)
{
	try {
		std::vector<std::byte> bytes = job.ioFuture.get();
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
		registry.setPayload(job.id, std::move(decoded.payload));
		if (decoded.cpuBytes != 0)
			registry.setCost(job.id, BudgetKind::CpuBytes, decoded.cpuBytes);
		registry.setCost(job.id, BudgetKind::AudioBytes, decoded.audioBytes);
		if (ResourceRegistry::Entry const* entry = registry.inspect(job.id)) {
			if (entry->kind != decoded.kind && decoded.kind != ResourceKind::Blob)
				registry.create(decoded.kind, job.id);
		}
		registry.setState(job.id, ResidencyState::Resident);
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
		return current == ResidencyState::Resident || current == ResidencyState::Failed;
	}), jobs.end());

	wakeWaitingRequests();
	startQueuedRequests();
}

ResidencyState StreamingScheduler::state(AssetId id) const
{
	return registry.state(id);
}

std::size_t StreamingScheduler::pendingJobs() const noexcept
{
	return jobs.size();
}

std::size_t StreamingScheduler::queuedRequests() const noexcept
{
	return queued.size();
}

std::size_t StreamingScheduler::waitingRequests() const noexcept
{
	return waiting.size();
}

} // namespace Asset
} // namespace Euph
