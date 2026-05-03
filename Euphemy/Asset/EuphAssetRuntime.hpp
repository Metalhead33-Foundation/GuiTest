#ifndef EUPHASSETRUNTIME_HPP
#define EUPHASSETRUNTIME_HPP

#include <Euphemy/Asset/EuphAssetTypes.hpp>
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Elvavena/Util/ElvThreadPool.hpp>
#include <Kaldi/KldCommandBuffer.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
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
	Texture,
	Buffer,
	Audio,
	GenericBlob
};

struct TextureResource { static constexpr ResourceKind kind = ResourceKind::Texture; };
struct BufferResource { static constexpr ResourceKind kind = ResourceKind::Buffer; };
struct AudioResource { static constexpr ResourceKind kind = ResourceKind::Audio; };
struct GenericBlobResource { static constexpr ResourceKind kind = ResourceKind::GenericBlob; };

template<typename TTag> struct ResourceHandle {
	AssetId id = 0;
	std::uint32_t generation = 0;
	std::uint32_t slot = 0;

	constexpr explicit operator bool() const noexcept { return id != 0 || generation != 0; }
	constexpr bool operator==(const ResourceHandle&) const = default;
};

struct CatalogValidation {
	bool ok = true;
	std::vector<AssetId> missingDependencies;
};

class MH_EUPH_API AssetCatalog {
private:
	std::unordered_map<AssetId, AssetRecord> records;
	std::unordered_map<std::string, AssetId> pathIndex;
	static const std::vector<AssetId>& emptyDependencies();
public:
	bool addRecord(AssetRecord record);
	const AssetRecord* find(AssetId id) const;
	const AssetRecord* findByPath(const std::string& path) const;
	std::span<const AssetId> dependenciesOf(AssetId id) const;
	CatalogValidation validateDependencies() const;
	std::size_t size() const noexcept;
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
	bool dependenciesResident(AssetId id, const ResourceRegistry& registry, AssetId* blockingDependency = nullptr) const;
};

enum class BudgetKind : std::uint8_t {
	CpuBytes,
	GpuBytes,
	AudioBytes,
	IoInflightBytes,
	DecodeInflightBytes,
	GpuUploadInflightBytes,
	Count
};

class MH_EUPH_API ResourceRegistry {
public:
	struct Entry {
		AssetId id = 0;
		ResourceKind kind = ResourceKind::GenericBlob;
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
		std::optional<Kld::HandleId> kaldiHandle;
	};
private:
	std::vector<Entry> entries;
	std::vector<std::uint32_t> freeSlots;
	std::unordered_map<AssetId, std::uint32_t> slotByAsset;
	std::array<std::optional<std::uint32_t>, 4> placeholders;

	static std::size_t kindIndex(ResourceKind kind);
	Entry* entryFor(AssetId id);
	const Entry* entryFor(AssetId id) const;
	template<typename TTag> const Entry* entryFor(ResourceHandle<TTag> handle) const;
	template<typename TTag> Entry* entryFor(ResourceHandle<TTag> handle);
	static bool legalTransition(ResidencyState from, ResidencyState to);
public:
	ResourceRegistry();

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

	void setCost(AssetId id, BudgetKind kind, std::uint64_t bytes);
	std::uint64_t cost(AssetId id, BudgetKind kind) const;
	std::uint64_t cpuCost(AssetId id) const;
	std::uint64_t gpuCost(AssetId id) const;
	void clearCosts(AssetId id);

	void setFlag(AssetId id, ResidencyFlag flag, bool enabled);
	bool hasFlag(AssetId id, ResidencyFlag flag) const;

	void setKaldiHandle(AssetId id, Kld::HandleId handle);
	void clearKaldiHandle(AssetId id);
	std::optional<Kld::HandleId> kaldiHandle(AssetId id) const;
	template<typename TTag> std::optional<Kld::HandleId> kaldiHandle(ResourceHandle<TTag> handle) const;

	template<typename TTag> void setPlaceholder(ResourceHandle<TTag> handle);
	template<typename TTag> std::optional<ResourceHandle<TTag> > placeholder() const;
	template<typename TTag> ResourceHandle<TTag> resolveOrPlaceholder(ResourceHandle<TTag> handle) const;

	std::vector<AssetId> activeAssetIds() const;
	const Entry* inspect(AssetId id) const;
};

class MH_EUPH_API ResidencyManager {
private:
	ResourceRegistry& registry;
	std::array<std::uint64_t, static_cast<std::size_t>(BudgetKind::Count)> budgets;

	bool withinBudgets() const;
	static std::uint64_t totalReleasableCost(const ResourceRegistry::Entry& entry);
	static void recordDestroy(Kld::CommandBuffer& commandBuffer, const ResourceRegistry::Entry& entry);
public:
	explicit ResidencyManager(ResourceRegistry& resourceRegistry);
	void setBudget(BudgetKind kind, std::uint64_t bytes);
	std::uint64_t budget(BudgetKind kind) const;
	std::uint64_t currentUsage(BudgetKind kind) const;
	bool makeEvictable(AssetId id);
	void pin(AssetId id);
	void unpin(AssetId id);
	std::vector<AssetId> evictUntilWithinBudget(Kld::CommandBuffer* commandBuffer = nullptr);
};

struct UploadItem {
	AssetId id = 0;
	std::shared_ptr<std::vector<std::byte> > payload;
	std::uint64_t gpuBytes = 0;
	std::optional<Kld::HandleId> kaldiHandle;
};

class MH_EUPH_API UploadQueue {
public:
	using RecordCallback = std::function<void(Kld::CommandBuffer&, const UploadItem&)>;
private:
	struct PendingUpload {
		UploadItem item;
		RecordCallback record;
	};
	ResourceRegistry& registry;
	std::vector<PendingUpload> pending;
	std::vector<PendingUpload> recorded;
public:
	explicit UploadQueue(ResourceRegistry& resourceRegistry);
	void enqueue(UploadItem item, RecordCallback record);
	std::uint64_t recordInto(Kld::CommandBuffer& commandBuffer, std::uint64_t maxUploadBytes);
	void commitRecorded();
	void failRecorded(FailureReason reason, const std::string& message);
	std::size_t pendingCount() const noexcept;
	std::size_t recordedCount() const noexcept;
};

struct DecodedAsset {
	std::vector<std::byte> cpuPayload;
	std::uint64_t cpuBytes = 0;
	std::uint64_t gpuBytes = 0;
	std::uint64_t audioBytes = 0;
	bool requiresUpload = false;
	std::shared_ptr<std::vector<std::byte> > uploadPayload;
	UploadQueue::RecordCallback uploadRecord;
	std::optional<Kld::HandleId> kaldiHandle;
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
	UploadQueue* uploadQueue = nullptr;
	std::vector<Job> jobs;

	void completeIo(Job& job);
	void completeDecode(Job& job);
	void failJob(Job& job, FailureReason reason, const std::string& message);
public:
	StreamingScheduler(const AssetCatalog& assetCatalog, ResourceRegistry& resourceRegistry, Elv::Util::ThreadPool& threadPool, Loader loaderCallback, Decoder decoderCallback, UploadQueue* uploads = nullptr);
	~StreamingScheduler();
	bool request(AssetId id, StreamPriority priority = {});
	bool cancel(AssetId id);
	void pollCompletions();
	std::uint64_t enqueueUploads(Kld::CommandBuffer& commandBuffer, std::uint64_t maxUploadBytes);
	ResidencyState state(AssetId id) const;
	std::size_t pendingJobs() const noexcept;
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
	Entry* existing = entryFor(id);
	if (existing) {
		if (existing->kind != TTag::kind)
			return {};
		return { id, existing->generation, slotByAsset[id] };
	}

	std::uint32_t slot = 0;
	if (!freeSlots.empty()) {
		slot = freeSlots.back();
		freeSlots.pop_back();
		Entry& entry = entries[slot];
		const std::uint32_t generation = entry.generation;
		entry = Entry {};
		entry.generation = generation == 0 ? 1 : generation;
		entry.id = id;
		entry.kind = TTag::kind;
		entry.active = true;
		entry.state = ResidencyState::Known;
	} else {
		slot = static_cast<std::uint32_t>(entries.size());
		entries.push_back(Entry {});
		entries.back().id = id;
		entries.back().kind = TTag::kind;
		entries.back().active = true;
		entries.back().state = ResidencyState::Known;
	}
	slotByAsset[id] = slot;
	return { id, entries[slot].generation, slot };
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
std::optional<Kld::HandleId> ResourceRegistry::kaldiHandle(ResourceHandle<TTag> handle) const
{
	const Entry* entry = entryFor(handle);
	return entry ? entry->kaldiHandle : std::nullopt;
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
