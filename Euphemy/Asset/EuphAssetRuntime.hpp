#ifndef EUPHASSETRUNTIME_HPP
#define EUPHASSETRUNTIME_HPP
/**
 * @file EuphAssetRuntime.hpp
 * @brief CPU-side asset catalog, dependency, residency, and streaming runtime.
 *
 * This module deliberately owns genre-independent, non-GPU asset lifetime:
 * catalog metadata, typed handles, dependency ordering, budget accounting,
 * retained CPU payloads, and background I/O/decode scheduling. Renderer upload
 * and GPU residency are integration concerns outside Euphemy.
 */

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

/** @brief Broad CPU-side resource category used for typed handles and placeholders. */
enum class ResourceKind : std::uint8_t {
	Image, /**< Decoded image-like payload. */
	Audio, /**< Decoded audio-like payload. */
	Blob,  /**< Generic byte payload. */
	Count  /**< Sentinel used for fixed-size arrays. */
};

/** @brief Handle tag for image resources. */
struct ImageResource { static constexpr ResourceKind kind = ResourceKind::Image; };
/** @brief Handle tag for audio resources. */
struct AudioResource { static constexpr ResourceKind kind = ResourceKind::Audio; };
/** @brief Handle tag for generic byte/blob resources. */
struct GenericBlobResource { static constexpr ResourceKind kind = ResourceKind::Blob; };
/** @brief Compatibility alias for image resources consumed as textures later. */
using TextureResource = ImageResource;
/** @brief Handle tag for buffer-like byte resources. */
struct BufferResource { static constexpr ResourceKind kind = ResourceKind::Blob; };

/**
 * @brief Typed, generation-checked reference to a registry entry.
 *
 * Handles are cheap values. They become invalid when the registry entry is
 * destroyed and later reused with a new generation.
 */
template<typename TTag> struct ResourceHandle {
	/** @brief Asset id referenced by the handle. */
	AssetId id = 0;
	/** @brief Generation captured when the handle was created. */
	std::uint32_t generation = 0;
	/** @brief Registry slot captured when the handle was created. */
	std::uint32_t slot = 0;

	/** @brief Returns true when the handle has any non-null identity. */
	constexpr explicit operator bool() const noexcept { return id != 0 || generation != 0; }
	/** @brief Compares raw handle identity. */
	constexpr bool operator==(const ResourceHandle&) const = default;
};

/** @brief Result of validating catalog dependency references. */
struct CatalogValidation {
	/** @brief True when no invalid records or dependency errors were found. */
	bool ok = true;
	/** @brief Records with unusable metadata, such as id zero. */
	std::vector<AssetId> invalidRecords;
	/** @brief Dependency ids that do not exist in the catalog. */
	std::vector<AssetId> missingDependencies;
	/** @brief Records that directly depend on themselves. */
	std::vector<AssetId> selfDependencies;
	/** @brief Records that list the same direct dependency more than once. */
	std::vector<AssetId> duplicateDependencies;
};

/**
 * @brief Stores AssetRecord metadata and lightweight lookup indexes.
 *
 * AssetCatalog is authoritative for ids and dependencies. Path/type data are
 * hints for loaders and decoders, not resource lifetime state.
 */
class MH_EUPH_API AssetCatalog {
private:
	std::unordered_map<AssetId, AssetRecord> records;
	std::unordered_map<std::string, AssetId> pathIndex;
	static const std::vector<AssetId>& emptyDependencies();
	void removePathIndexFor(AssetId id);
public:
	/** @brief Adds or replaces one record, updating the optional path index. */
	bool addRecord(AssetRecord record);
	/** @brief Removes a record and its path lookup entry. */
	bool removeRecord(AssetId id);
	/** @brief Removes all catalog records and indexes. */
	void clear();
	/** @brief Returns true when id exists in the catalog. */
	bool contains(AssetId id) const;
	/** @brief Finds a record by id, or nullptr when absent. */
	const AssetRecord* find(AssetId id) const;
	/** @brief Finds a record by path hint, or nullptr when absent. */
	const AssetRecord* findByPath(const std::string& path) const;
	/** @brief Returns direct dependency ids for a record, or an empty span. */
	std::span<const AssetId> dependenciesOf(AssetId id) const;
	/** @brief Returns all asset ids currently stored in the catalog. */
	std::vector<AssetId> assetIds() const;
	/** @brief Checks direct dependency references for missing or malformed data. */
	CatalogValidation validateDependencies() const;
	/** @brief Returns the number of records in the catalog. */
	std::size_t size() const noexcept;
	/** @brief Returns true when the catalog has no records. */
	bool empty() const noexcept;
};

/** @brief Result of resolving dependency ordering for one asset. */
struct DependencyResult {
	/** @brief True when dependency traversal completed without missing refs or cycles. */
	bool ok = true;
	/** @brief Failure category for the traversal. */
	FailureReason failure = FailureReason::None;
	/** @brief Transitive dependencies in load-before-parent order. */
	std::vector<AssetId> orderedDependencies;
	/** @brief Missing dependency ids found during traversal. */
	std::vector<AssetId> missingDependencies;
	/** @brief Cycle path when traversal detects a dependency loop. */
	std::vector<AssetId> cycle;
};

class ResourceRegistry;

/**
 * @brief Resolves dependency relationships from an AssetCatalog.
 *
 * The graph is a view over the catalog; it does not copy or own records.
 */
class MH_EUPH_API DependencyGraph {
private:
	const AssetCatalog& catalog;
	bool visit(AssetId id, std::unordered_map<AssetId, std::uint8_t>& marks, DependencyResult& result) const;
public:
	/** @brief Creates a dependency graph view over assetCatalog. */
	explicit DependencyGraph(const AssetCatalog& assetCatalog);
	/** @brief Resolves all transitive dependencies for id in load order. */
	DependencyResult resolveTransitive(AssetId id) const;
	/** @brief Returns direct dependencies for id. */
	std::vector<AssetId> directDependencies(AssetId id) const;
	/** @brief Returns catalog records that directly depend on id. */
	std::vector<AssetId> dependentsOf(AssetId id) const;
	/** @brief Returns true when asset transitively depends on dependency. */
	bool hasDependency(AssetId asset, AssetId dependency) const;
	/** @brief Checks whether every direct dependency for id is resident. */
	bool dependenciesResident(AssetId id, const ResourceRegistry& registry, AssetId* blockingDependency = nullptr) const;
};

/** @brief Budget counters maintained by the CPU-side asset runtime. */
enum class BudgetKind : std::uint8_t {
	CpuBytes,            /**< Resident CPU payload bytes. */
	AudioBytes,          /**< Resident audio payload bytes. */
	IoInflightBytes,     /**< Bytes currently being loaded from I/O. */
	DecodeInflightBytes, /**< Bytes currently being decoded. */
	Count                /**< Sentinel used for fixed-size arrays. */
};

/**
 * @brief Type-erased CPU resident payload retained by ResourceRegistry.
 *
 * The type string is intentionally lightweight. Higher layers may agree on
 * strings such as "bytes" or "decode-target" without making Euphemy depend on
 * renderer or backend-specific payload classes.
 */
struct ResidentPayload {
	/** @brief Shared ownership of the payload object. */
	std::shared_ptr<void> data;
	/** @brief Byte cost or payload size used by budgets and upload bridges. */
	std::uint64_t bytes = 0;
	/** @brief Free-form payload type tag. */
	std::string type;

	/** @brief Returns true when data is non-null. */
	explicit operator bool() const noexcept { return data != nullptr; }
};

/** @brief Wraps a byte vector in a ResidentPayload with a type tag. */
MH_EUPH_API ResidentPayload makeBytePayload(std::vector<std::byte> bytes, std::string type = "bytes");

/**
 * @brief Owns resource lifetime state, typed handle validation, costs, and payloads.
 *
 * ResourceRegistry is the central CPU-side table used by the streaming runtime.
 * It does not perform I/O, decoding, or GPU work by itself.
 */
class MH_EUPH_API ResourceRegistry {
public:
	/** @brief Internal resource slot exposed for diagnostics and tests. */
	struct Entry {
		/** @brief Asset id owned by this entry. */
		AssetId id = 0;
		/** @brief Resource kind used for typed handle checks. */
		ResourceKind kind = ResourceKind::Blob;
		/** @brief Generation incremented when a slot is reused. */
		std::uint32_t generation = 1;
		/** @brief True when this slot currently owns a resource. */
		bool active = false;
		/** @brief Current residency state. */
		ResidencyState state = ResidencyState::Missing;
		/** @brief Policy/diagnostic flags. */
		ResidencyFlag flags = ResidencyFlag::None;
		/** @brief Last recorded failure category. */
		FailureReason failureReason = FailureReason::None;
		/** @brief Optional human-readable failure details. */
		std::string failureMessage;
		/** @brief Strong references that block destruction/eviction. */
		std::uint32_t strongRefs = 0;
		/** @brief Weak references recorded for diagnostics/policy. */
		std::uint32_t weakRefs = 0;
		/** @brief Current streaming/eviction priority. */
		StreamPriority priority {};
		/** @brief Last frame index that touched this resource. */
		std::uint64_t lastTouchedFrame = 0;
		/** @brief Per-budget byte costs. */
		std::array<std::uint64_t, static_cast<std::size_t>(BudgetKind::Count)> costs {};
		/** @brief Optional resident CPU payload. */
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
	/** @brief Creates an empty registry. */
	ResourceRegistry();

	/** @brief Creates or finds a resource entry for id and kind. */
	ResourceHandle<GenericBlobResource> create(ResourceKind kind, AssetId id);
	/** @brief Creates or finds a typed resource handle for id. */
	template<typename TTag> ResourceHandle<TTag> createHandle(AssetId id);
	/** @brief Destroys an entry if it is not strongly referenced. */
	bool destroy(AssetId id);

	/** @brief Returns true when a typed handle still matches a live entry. */
	template<typename TTag> bool isAlive(ResourceHandle<TTag> handle) const;
	/** @brief Returns true when id maps to a live entry. */
	bool isAlive(AssetId id) const;
	/** @brief Returns current state for id, or Missing when absent. */
	ResidencyState state(AssetId id) const;
	/** @brief Returns current state for a typed handle, or Missing when invalid. */
	template<typename TTag> ResidencyState state(ResourceHandle<TTag> handle) const;

	/** @brief Applies a legal state transition to a resource. */
	bool setState(AssetId id, ResidencyState newState);
	/** @brief Marks a resource Failed with diagnostic information. */
	bool fail(AssetId id, FailureReason reason, std::string message = {});
	/** @brief Returns the last failure reason for id. */
	FailureReason failureReason(AssetId id) const;
	/** @brief Returns the last failure message for id, or an empty string. */
	const std::string& failureMessage(AssetId id) const;

	/** @brief Updates the last-touched frame through a typed handle. */
	template<typename TTag> bool touch(ResourceHandle<TTag> handle, std::uint64_t frameIndex);
	/** @brief Updates the last-touched frame for id. */
	bool touch(AssetId id, std::uint64_t frameIndex);
	/** @brief Sets the streaming/eviction priority for id. */
	void setPriority(AssetId id, StreamPriority priority);
	/** @brief Returns the priority for id, or default priority when absent. */
	StreamPriority priority(AssetId id) const;

	/** @brief Adds a strong reference that blocks destruction/eviction. */
	bool retain(AssetId id);
	/** @brief Releases one strong reference. */
	bool release(AssetId id);
	/** @brief Adds a weak reference counter for diagnostics/policy. */
	bool retainWeak(AssetId id);
	/** @brief Releases one weak reference counter. */
	bool releaseWeak(AssetId id);
	/** @brief Returns the strong reference count for id. */
	std::uint32_t strongRefs(AssetId id) const;
	/** @brief Returns the weak reference count for id. */
	std::uint32_t weakRefs(AssetId id) const;

	/** @brief Sets one budget cost counter for id. */
	void setCost(AssetId id, BudgetKind kind, std::uint64_t bytes);
	/** @brief Returns one budget cost counter for id. */
	std::uint64_t cost(AssetId id, BudgetKind kind) const;
	/** @brief Returns the CPU byte cost for id. */
	std::uint64_t cpuCost(AssetId id) const;
	/** @brief Returns the audio byte cost for id. */
	std::uint64_t audioCost(AssetId id) const;
	/** @brief Clears all budget cost counters for id. */
	void clearCosts(AssetId id);

	/** @brief Enables or disables one ResidencyFlag. */
	void setFlag(AssetId id, ResidencyFlag flag, bool enabled);
	/** @brief Returns true when a ResidencyFlag is set for id. */
	bool hasFlag(AssetId id, ResidencyFlag flag) const;

	/** @brief Stores a resident CPU payload for id. */
	void setPayload(AssetId id, ResidentPayload payload);
	/** @brief Clears the resident CPU payload for id. */
	void clearPayload(AssetId id);
	/** @brief Returns the resident payload for id, or nullptr when absent. */
	const ResidentPayload* payload(AssetId id) const;

	/** @brief Registers a typed fallback handle for nonresident resources. */
	template<typename TTag> void setPlaceholder(ResourceHandle<TTag> handle);
	/** @brief Returns the typed fallback handle, if one is registered and live. */
	template<typename TTag> std::optional<ResourceHandle<TTag> > placeholder() const;
	/** @brief Returns handle when resident, otherwise a live typed placeholder. */
	template<typename TTag> ResourceHandle<TTag> resolveOrPlaceholder(ResourceHandle<TTag> handle) const;

	/** @brief Returns all live asset ids currently known to the registry. */
	std::vector<AssetId> activeAssetIds() const;
	/** @brief Returns a diagnostic view of an entry, or nullptr when absent. */
	const Entry* inspect(AssetId id) const;
};

/**
 * @brief RAII strong reference for a ResourceRegistry entry.
 *
 * The lease retains a resource on construction and releases it on destruction,
 * making short-lived CPU-side use explicit.
 */
template<typename TTag> class ResourceLease {
private:
	ResourceRegistry* registry = nullptr;
	ResourceHandle<TTag> handle {};
public:
	/** @brief Creates an empty lease. */
	ResourceLease() = default;
	/** @brief Attempts to retain resourceHandle in resourceRegistry. */
	ResourceLease(ResourceRegistry& resourceRegistry, ResourceHandle<TTag> resourceHandle)
		: registry(&resourceRegistry), handle(resourceHandle)
	{
		if (!registry->retain(handle.id))
			registry = nullptr;
	}
	ResourceLease(const ResourceLease&) = delete;
	ResourceLease& operator=(const ResourceLease&) = delete;
	/** @brief Moves a lease without changing the reference count. */
	ResourceLease(ResourceLease&& other) noexcept
		: registry(other.registry), handle(other.handle)
	{
		other.registry = nullptr;
		other.handle = {};
	}
	/** @brief Releases any current lease, then moves another lease into this one. */
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
	/** @brief Releases the retained resource, if any. */
	~ResourceLease() { reset(); }

	/** @brief Releases the retained resource and clears this lease. */
	void reset()
	{
		if (registry) {
			registry->release(handle.id);
			registry = nullptr;
			handle = {};
		}
	}
	/** @brief Returns the handle retained by this lease. */
	ResourceHandle<TTag> get() const noexcept { return handle; }
	/** @brief Returns true when this lease successfully retained a resource. */
	explicit operator bool() const noexcept { return registry != nullptr; }
};

/** @brief Detailed result from budget eviction. */
struct EvictionResult {
	/** @brief Asset ids actually evicted. */
	std::vector<AssetId> evicted;
	/** @brief Asset ids considered but protected by policy or references. */
	std::vector<AssetId> blocked;
	/** @brief True when all configured budgets are satisfied after eviction. */
	bool withinBudget = true;
};

/**
 * @brief Applies CPU-side budget policy and conservative eviction.
 *
 * The manager evicts only resources that have been explicitly marked Evictable,
 * and it respects pins, strong references, and resident dependents.
 */
class MH_EUPH_API ResidencyManager {
private:
	ResourceRegistry& registry;
	const AssetCatalog* catalog = nullptr;
	std::array<std::uint64_t, static_cast<std::size_t>(BudgetKind::Count)> budgets;

	bool withinBudgets() const;
	static std::uint64_t totalReleasableCost(const ResourceRegistry::Entry& entry);
	bool hasResidentDependent(AssetId id) const;
public:
	/** @brief Creates a manager for a registry and optional dependency catalog. */
	explicit ResidencyManager(ResourceRegistry& resourceRegistry, const AssetCatalog* assetCatalog = nullptr);
	/** @brief Updates the dependency catalog used for eviction protection. */
	void setCatalog(const AssetCatalog* assetCatalog);
	/** @brief Sets the byte budget for one budget kind. */
	void setBudget(BudgetKind kind, std::uint64_t bytes);
	/** @brief Returns the configured byte budget for one kind. */
	std::uint64_t budget(BudgetKind kind) const;
	/** @brief Returns current registry usage for one budget kind. */
	std::uint64_t currentUsage(BudgetKind kind) const;
	/** @brief Marks a resident resource as eligible for eviction. */
	bool makeEvictable(AssetId id);
	/** @brief Pins a resource so budget eviction will not remove it. */
	void pin(AssetId id);
	/** @brief Clears a resource pin. */
	void unpin(AssetId id);
	/** @brief Evicts deterministic candidates until budgets are satisfied or blocked. */
	EvictionResult evictUntilWithinBudgetDetailed();
	/** @brief Evicts candidates and returns only the evicted ids. */
	std::vector<AssetId> evictUntilWithinBudget();
};

/** @brief Output produced by a StreamingScheduler decoder callback. */
struct DecodedAsset {
	/** @brief Optional resident CPU payload to store in ResourceRegistry. */
	ResidentPayload payload;
	/** @brief CPU byte cost to account after decode. */
	std::uint64_t cpuBytes = 0;
	/** @brief Audio byte cost to account after decode. */
	std::uint64_t audioBytes = 0;
	/** @brief Resource kind produced by the decode step. */
	ResourceKind kind = ResourceKind::Blob;
};

/** @brief Tunable behavior for StreamingScheduler. */
struct StreamingSchedulerOptions {
	/** @brief Maximum number of I/O jobs running at once. */
	std::size_t maxIoJobs = 2;
	/** @brief When true, requests dependency assets before the parent asset. */
	bool autoRequestDependencies = true;
	/** @brief When true, failed assets may be requested again. */
	bool retryFailedRequests = false;
};

/**
 * @brief Coordinates dependency checks, async byte reads, decode callbacks, and commits.
 *
 * StreamingScheduler uses the caller-provided ThreadPool and loader/decoder
 * callbacks. It never talks to Kaldi or renderer APIs; decoded payloads remain
 * CPU-side in ResourceRegistry.
 */
class MH_EUPH_API StreamingScheduler {
public:
	/** @brief Opens an I/O device for one asset record. */
	using Loader = std::function<Elv::Io::sDevice(const AssetRecord&)>;
	/** @brief Converts stored bytes into a decoded asset payload and costs. */
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
	/** @brief Creates a scheduler over existing catalog, registry, executor, and callbacks. */
	StreamingScheduler(const AssetCatalog& assetCatalog, ResourceRegistry& resourceRegistry, Elv::Util::ThreadPool& threadPool, Loader loaderCallback, Decoder decoderCallback, StreamingSchedulerOptions schedulerOptions = {});
	/** @brief Waits for owned background job futures to settle. */
	~StreamingScheduler();
	/** @brief Requests an asset and optionally its dependencies. */
	bool request(AssetId id, StreamPriority priority = {});
	/** @brief Cancels a queued or waiting request that has not started I/O. */
	bool cancel(AssetId id);
	/** @brief Polls I/O/decode futures and advances completed jobs. */
	void pollCompletions();
	/** @brief Returns the current registry state for id. */
	ResidencyState state(AssetId id) const;
	/** @brief Returns the number of active background jobs. */
	std::size_t pendingJobs() const noexcept;
	/** @brief Returns the number of requests waiting to start I/O. */
	std::size_t queuedRequests() const noexcept;
	/** @brief Returns the number of parent requests waiting on dependencies. */
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
