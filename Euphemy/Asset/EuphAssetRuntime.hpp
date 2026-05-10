#ifndef EUPHASSETRUNTIME_HPP
#define EUPHASSETRUNTIME_HPP

#include <Euphemy/Asset/EuphAssetTypes.hpp>
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Elvavena/Util/ElvThreadPool.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace Euph {
namespace Asset {

enum class ResourceKind : std::uint8_t {
	Image,
	Audio,
	Blob,
	Count
};

struct ImageResource { static constexpr ResourceKind kind = ResourceKind::Image; };
struct AudioResource { static constexpr ResourceKind kind = ResourceKind::Audio; };
struct GenericBlobResource { static constexpr ResourceKind kind = ResourceKind::Blob; };
using TextureResource = ImageResource;
struct BufferResource { static constexpr ResourceKind kind = ResourceKind::Blob; };

template<typename TTag> struct ResourceHandle {
	AssetId id = 0;
	std::uint32_t generation = 0;
	std::uint32_t slot = 0;

	constexpr explicit operator bool() const noexcept { return id != 0 || generation != 0; }
	constexpr bool operator==(const ResourceHandle&) const = default;
};

struct CatalogValidation {
	bool ok = true;
	std::vector<AssetId> invalidRecords;
	std::vector<AssetId> missingDependencies;
	std::vector<AssetId> selfDependencies;
	std::vector<AssetId> duplicateDependencies;
};

class MH_EUPH_API AssetCatalog {
private:
	std::unordered_map<AssetId, AssetRecord> records;
	std::unordered_map<std::string, AssetId> pathIndex;
	static const std::vector<AssetId>& emptyDependencies();
	void removePathIndexFor(AssetId id);
public:
	bool addRecord(AssetRecord record);
	bool removeRecord(AssetId id);
	void clear();
	bool contains(AssetId id) const;
	const AssetRecord* find(AssetId id) const;
	const AssetRecord* findByPath(const std::string& path) const;
	std::span<const AssetId> dependenciesOf(AssetId id) const;
	std::vector<AssetId> assetIds() const;
	CatalogValidation validateDependencies() const;
	std::size_t size() const noexcept;
	bool empty() const noexcept;
};

struct DependencyResult {
	bool ok = true;
	FailureReason failure = FailureReason::None;
	std::vector<AssetId> orderedDependencies;
	std::vector<AssetId> missingDependencies;
	std::vector<AssetId> cycle;
};

class ResourceRegistry;

class MH_EUPH_API DependencyGraph {
private:
	const AssetCatalog& catalog;
	bool visit(AssetId id, std::unordered_map<AssetId, std::uint8_t>& marks, DependencyResult& result) const;
public:
	explicit DependencyGraph(const AssetCatalog& assetCatalog);
	DependencyResult resolveTransitive(AssetId id) const;
	std::vector<AssetId> directDependencies(AssetId id) const;
	std::vector<AssetId> dependentsOf(AssetId id) const;
	bool hasDependency(AssetId asset, AssetId dependency) const;
	bool dependenciesResident(AssetId id, const ResourceRegistry& registry, AssetId* blockingDependency = nullptr) const;
};

enum class BudgetKind : std::uint8_t {
	CpuBytes,
	AudioBytes,
	IoInflightBytes,
	DecodeInflightBytes,
	Count
};

struct ResidentPayload {
	std::shared_ptr<void> data;
	std::uint64_t bytes = 0;
	std::string type;

	explicit operator bool() const noexcept { return data != nullptr; }
};

MH_EUPH_API ResidentPayload makeBytePayload(std::vector<std::byte> bytes, std::string type = "bytes");

class MH_EUPH_API ResourceRegistry {
public:
	struct Entry {
		AssetId id = 0;
		ResourceKind kind = ResourceKind::Blob;
		std::uint32_t generation = 1;
		bool active = false;
		ResidencyState state = ResidencyState::Missing;
		ResidencyFlag flags = ResidencyFlag::None;
		FailureReason failureReason = FailureReason::None;
		std::string failureMessage;
		std::uint32_t strongRefs = 0;
		std::uint32_t weakRefs = 0;
		StreamPriority priority {};
		std::uint64_t lastTouchedFrame = 0;
		std::array<std::uint64_t, static_cast<std::size_t>(BudgetKind::Count)> costs {};
		ResidentPayload payload;
	};
private:
	std::vector<Entry> entries;
	std::vector<std::uint32_t> freeSlots;
	std::unordered_map<AssetId, std::uint32_t> slotByAsset;
	std::array<std::optional<std::uint32_t>, static_cast<std::size_t>(ResourceKind::Count)> placeholders;

	static std::size_t kindIndex(ResourceKind kind);
	Entry* entryFor(AssetId id);
	const Entry* entryFor(AssetId id) const;
	template<typename TTag> const Entry* entryFor(ResourceHandle<TTag> handle) const;
	template<typename TTag> Entry* entryFor(ResourceHandle<TTag> handle);
	static bool legalTransition(ResidencyState from, ResidencyState to);
	ResourceHandle<GenericBlobResource> createBlobHandle(AssetId id);
public:
	ResourceRegistry();

	ResourceHandle<GenericBlobResource> create(ResourceKind kind, AssetId id);
	template<typename TTag> ResourceHandle<TTag> createHandle(AssetId id);
	bool destroy(AssetId id);

	template<typename TTag> bool isAlive(ResourceHandle<TTag> handle) const;
	bool isAlive(AssetId id) const;
	ResidencyState state(AssetId id) const;
	template<typename TTag> ResidencyState state(ResourceHandle<TTag> handle) const;

	bool setState(AssetId id, ResidencyState newState);
	bool fail(AssetId id, FailureReason reason, std::string message = {});
	FailureReason failureReason(AssetId id) const;
	const std::string& failureMessage(AssetId id) const;

	template<typename TTag> bool touch(ResourceHandle<TTag> handle, std::uint64_t frameIndex);
	bool touch(AssetId id, std::uint64_t frameIndex);
	void setPriority(AssetId id, StreamPriority priority);
	StreamPriority priority(AssetId id) const;

	bool retain(AssetId id);
	bool release(AssetId id);
	bool retainWeak(AssetId id);
	bool releaseWeak(AssetId id);
	std::uint32_t strongRefs(AssetId id) const;
	std::uint32_t weakRefs(AssetId id) const;

	void setCost(AssetId id, BudgetKind kind, std::uint64_t bytes);
	std::uint64_t cost(AssetId id, BudgetKind kind) const;
	std::uint64_t cpuCost(AssetId id) const;
	std::uint64_t audioCost(AssetId id) const;
	void clearCosts(AssetId id);

	void setFlag(AssetId id, ResidencyFlag flag, bool enabled);
	bool hasFlag(AssetId id, ResidencyFlag flag) const;

	void setPayload(AssetId id, ResidentPayload payload);
	void clearPayload(AssetId id);
	const ResidentPayload* payload(AssetId id) const;

	template<typename TTag> void setPlaceholder(ResourceHandle<TTag> handle);
	template<typename TTag> std::optional<ResourceHandle<TTag> > placeholder() const;
	template<typename TTag> ResourceHandle<TTag> resolveOrPlaceholder(ResourceHandle<TTag> handle) const;

	std::vector<AssetId> activeAssetIds() const;
	const Entry* inspect(AssetId id) const;
};

template<typename TTag> class ResourceLease {
private:
	ResourceRegistry* registry = nullptr;
	ResourceHandle<TTag> handle {};
public:
	ResourceLease() = default;
	ResourceLease(ResourceRegistry& resourceRegistry, ResourceHandle<TTag> resourceHandle)
		: registry(&resourceRegistry), handle(resourceHandle)
	{
		if (!registry->retain(handle.id))
			registry = nullptr;
	}
	ResourceLease(const ResourceLease&) = delete;
	ResourceLease& operator=(const ResourceLease&) = delete;
	ResourceLease(ResourceLease&& other) noexcept
		: registry(other.registry), handle(other.handle)
	{
		other.registry = nullptr;
		other.handle = {};
	}
	ResourceLease& operator=(ResourceLease&& other) noexcept
	{
		if (this != &other) {
			reset();
			registry = other.registry;
			handle = other.handle;
			other.registry = nullptr;
			other.handle = {};
		}
		return *this;
	}
	~ResourceLease() { reset(); }

	void reset()
	{
		if (registry) {
			registry->release(handle.id);
			registry = nullptr;
			handle = {};
		}
	}
	ResourceHandle<TTag> get() const noexcept { return handle; }
	explicit operator bool() const noexcept { return registry != nullptr; }
};

struct EvictionResult {
	std::vector<AssetId> evicted;
	std::vector<AssetId> blocked;
	bool withinBudget = true;
};

class MH_EUPH_API ResidencyManager {
private:
	ResourceRegistry& registry;
	const AssetCatalog* catalog = nullptr;
	std::array<std::uint64_t, static_cast<std::size_t>(BudgetKind::Count)> budgets;

	bool withinBudgets() const;
	static std::uint64_t totalReleasableCost(const ResourceRegistry::Entry& entry);
	bool hasResidentDependent(AssetId id) const;
public:
	explicit ResidencyManager(ResourceRegistry& resourceRegistry, const AssetCatalog* assetCatalog = nullptr);
	void setCatalog(const AssetCatalog* assetCatalog);
	void setBudget(BudgetKind kind, std::uint64_t bytes);
	std::uint64_t budget(BudgetKind kind) const;
	std::uint64_t currentUsage(BudgetKind kind) const;
	bool makeEvictable(AssetId id);
	void pin(AssetId id);
	void unpin(AssetId id);
	EvictionResult evictUntilWithinBudgetDetailed();
	std::vector<AssetId> evictUntilWithinBudget();
};

struct DecodedAsset {
	ResidentPayload payload;
	std::uint64_t cpuBytes = 0;
	std::uint64_t audioBytes = 0;
	ResourceKind kind = ResourceKind::Blob;
};

struct StreamingSchedulerOptions {
	std::size_t maxIoJobs = 2;
	bool autoRequestDependencies = true;
	bool retryFailedRequests = false;
};

class MH_EUPH_API StreamingScheduler {
public:
	using Loader = std::function<Elv::Io::sDevice(const AssetRecord&)>;
	using Decoder = std::function<DecodedAsset(const AssetRecord&, std::span<const std::byte>)>;
private:
	struct Job;

	const AssetCatalog& catalog;
	ResourceRegistry& registry;
	Elv::Util::ThreadPool& executor;
	Loader loader;
	Decoder decoder;
	StreamingSchedulerOptions options;
	std::deque<AssetId> queued;
	std::vector<AssetId> waiting;
	std::vector<Job> jobs;

	bool requestInternal(AssetId id, StreamPriority priority, bool dependencyRequest);
	bool queueReadyRequest(AssetId id, StreamPriority priority);
	void startQueuedRequests();
	void wakeWaitingRequests();
	bool hasQueued(AssetId id) const;
	bool hasWaiting(AssetId id) const;
	bool hasJob(AssetId id) const;
	void completeIo(Job& job);
	void completeDecode(Job& job);
	void failJob(Job& job, FailureReason reason, const std::string& message);
public:
	StreamingScheduler(const AssetCatalog& assetCatalog, ResourceRegistry& resourceRegistry, Elv::Util::ThreadPool& threadPool, Loader loaderCallback, Decoder decoderCallback, StreamingSchedulerOptions schedulerOptions = {});
	~StreamingScheduler();
	bool request(AssetId id, StreamPriority priority = {});
	bool cancel(AssetId id);
	void pollCompletions();
	ResidencyState state(AssetId id) const;
	std::size_t pendingJobs() const noexcept;
	std::size_t queuedRequests() const noexcept;
	std::size_t waitingRequests() const noexcept;
};

template<typename TTag>
const ResourceRegistry::Entry* ResourceRegistry::entryFor(ResourceHandle<TTag> handle) const
{
	if (handle.slot >= entries.size())
		return nullptr;
	const Entry& entry = entries[handle.slot];
	if (!entry.active || entry.id != handle.id || entry.generation != handle.generation || entry.kind != TTag::kind)
		return nullptr;
	return &entry;
}

template<typename TTag>
ResourceRegistry::Entry* ResourceRegistry::entryFor(ResourceHandle<TTag> handle)
{
	return const_cast<Entry*>(static_cast<const ResourceRegistry&>(*this).entryFor(handle));
}

template<typename TTag>
ResourceHandle<TTag> ResourceRegistry::createHandle(AssetId id)
{
	ResourceHandle<GenericBlobResource> generic = create(TTag::kind, id);
	const Entry* entry = inspect(id);
	if (!entry || entry->kind != TTag::kind)
		return {};
	return { id, generic.generation, generic.slot };
}

template<typename TTag>
bool ResourceRegistry::isAlive(ResourceHandle<TTag> handle) const
{
	return entryFor(handle) != nullptr;
}

template<typename TTag>
ResidencyState ResourceRegistry::state(ResourceHandle<TTag> handle) const
{
	const Entry* entry = entryFor(handle);
	return entry ? entry->state : ResidencyState::Missing;
}

template<typename TTag>
bool ResourceRegistry::touch(ResourceHandle<TTag> handle, std::uint64_t frameIndex)
{
	Entry* entry = entryFor(handle);
	if (!entry)
		return false;
	entry->lastTouchedFrame = frameIndex;
	return true;
}

template<typename TTag>
void ResourceRegistry::setPlaceholder(ResourceHandle<TTag> handle)
{
	if (isAlive(handle))
		placeholders[kindIndex(TTag::kind)] = handle.slot;
}

template<typename TTag>
std::optional<ResourceHandle<TTag> > ResourceRegistry::placeholder() const
{
	const auto slot = placeholders[kindIndex(TTag::kind)];
	if (!slot || *slot >= entries.size())
		return std::nullopt;
	const Entry& entry = entries[*slot];
	if (!entry.active || entry.kind != TTag::kind)
		return std::nullopt;
	return ResourceHandle<TTag> { entry.id, entry.generation, *slot };
}

template<typename TTag>
ResourceHandle<TTag> ResourceRegistry::resolveOrPlaceholder(ResourceHandle<TTag> handle) const
{
	if (isAlive(handle) && state(handle) == ResidencyState::Resident)
		return handle;
	const auto fallback = placeholder<TTag>();
	return fallback ? *fallback : ResourceHandle<TTag> {};
}

} // namespace Asset
} // namespace Euph

#endif // EUPHASSETRUNTIME_HPP
