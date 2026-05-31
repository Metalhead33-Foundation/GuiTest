#ifndef LTEGPUASSETSTREAMER_HPP
#define LTEGPUASSETSTREAMER_HPP
/**
 * @file LteGpuAssetStreamer.hpp
 * @brief Bridges Euphemy CPU-resident assets into Kaldi command-buffer uploads.
 *
 * Lotte owns this integration layer so Euphemy can remain independent of Kaldi.
 * The streamer records create/destroy commands into caller-owned batches, keeps
 * upload payload memory alive for the recorded command span, and publishes Kaldi
 * handles only after the caller commits accepted work.
 */

#include <Lotte/LotteLib.hpp>
#include <Lotte/LteKld.hpp>
#include <Euphemy/Asset/EuphAssetRuntime.hpp>
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Kaldi/KldCommandBuffer.hpp>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <memory_resource>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Lotte {
namespace Asset {

/** @brief GPU resource category recorded by the Lotte/Kaldi bridge. */
enum class GpuResourceKind : std::uint8_t {
	Buffer,  /**< Kaldi buffer object created from a byte payload. */
	Texture, /**< Kaldi sampled texture created from a DecodeTarget payload. */
	Image    /**< Kaldi storage/render image; reserved for image command paths. */
};

/**
 * @brief Lifetime state for a GPU-side resource tracked by GpuAssetStreamer.
 *
 * Recorded states mean commands have been emitted but the caller has not yet
 * confirmed whether the backend accepted or rejected the submitted batch.
 */
enum class GpuResidencyState : std::uint8_t {
	Missing,         /**< No committed GPU resource is tracked. */
	QueuedUpload,    /**< An upload request is waiting to be recorded. */
	RecordedUpload,  /**< Create commands were recorded and await commit/fail. */
	Resident,        /**< A committed Kaldi handle is available to callers. */
	Evictable,       /**< The resource is resident and eligible for policy eviction. */
	QueuedDestroy,   /**< An explicit destroy request is waiting to be recorded. */
	RecordedDestroy, /**< Destroy commands were recorded and await commit/fail. */
	Failed           /**< Upload or destroy scheduling failed. */
};

/** @brief Diagnostic reason for a GPU-side streaming failure. */
enum class GpuFailureReason : std::uint8_t {
	None,                   /**< No failure is recorded. */
	CpuResourceMissing,     /**< Euphemy does not know the source CPU resource. */
	CpuResourceNotResident, /**< The source CPU resource is not resident yet. */
	PayloadMissing,         /**< The source resource has no retained payload. */
	UnsupportedPayloadType, /**< The payload type is not usable for this GPU kind. */
	PayloadTooLarge,        /**< The payload exceeds the Kaldi command's size limits. */
	BudgetExceeded,         /**< The upload cannot fit within the GPU budget. */
	BackendRejected,        /**< The caller reported that submitted commands failed. */
	Unknown                 /**< An uncategorized failure occurred. */
};

/** @brief Options used when recording a Kaldi buffer create command. */
struct BufferUploadOptions {
	/** @brief Kaldi buffer policy/usage bits passed through unchanged. */
	std::uint32_t policy = 0;
};

/** @brief Options used when recording a Kaldi texture create command. */
struct TextureUploadOptions {
	/** @brief Number of mip levels requested for the texture. */
	std::uint8_t mipLevels = 1;
	/** @brief Whether the backend should generate mipmaps when supported. */
	bool generateMipmaps = false;
};

/** @brief Diagnostic and policy record for one GPU-side resource. */
struct GpuResourceRecord {
	/** @brief Euphemy asset id this GPU resource mirrors. */
	Euph::Asset::AssetId assetId = 0;
	/** @brief GPU resource kind. */
	GpuResourceKind kind = GpuResourceKind::Buffer;
	/** @brief Current GPU-side residency state. */
	GpuResidencyState state = GpuResidencyState::Missing;
	/** @brief Last GPU-side failure reason. */
	GpuFailureReason failureReason = GpuFailureReason::None;
	/** @brief Optional human-readable failure details. */
	std::string failureMessage;
	/** @brief Committed or recorded Kaldi handle id. */
	Kld::HandleId handle = 0;
	/** @brief Estimated bytes charged to the GPU budget. */
	std::uint64_t gpuBytes = 0;
	/** @brief Priority used for upload ordering and eviction decisions. */
	Euph::Asset::StreamPriority priority {};
	/** @brief Last frame index that touched this GPU resource. */
	std::uint64_t lastTouchedFrame = 0;
	/** @brief Strong references that block eviction/destruction. */
	std::uint32_t strongRefs = 0;
	/** @brief True when policy eviction must skip this resource. */
	bool pinned = false;
	/** @brief Last requested buffer upload options. */
	BufferUploadOptions bufferOptions {};
	/** @brief Last requested texture upload options. */
	TextureUploadOptions textureOptions {};
};

/** @brief One GPU resource affected by a recorded command batch. */
struct RecordedGpuResource {
	/** @brief Asset id associated with the recorded command. */
	Euph::Asset::AssetId assetId = 0;
	/** @brief GPU resource kind affected by the command. */
	GpuResourceKind kind = GpuResourceKind::Buffer;
	/** @brief Kaldi handle id created or destroyed by the command. */
	Kld::HandleId handle = 0;
	/** @brief Byte cost associated with the recorded resource. */
	std::uint64_t gpuBytes = 0;
};

/**
 * @brief Move-only bundle of Kaldi commands and retained upload payloads.
 *
 * The batch owns the command buffer used for recording and holds shared payload
 * references so pointer arguments embedded in Kaldi commands remain valid until
 * the caller submits or discards the batch.
 */
class MH_LOTTE_API GpuCommandBatch {
public:
	/** @brief Non-owning target for recorded Kaldi commands. */
	CommandBuffer* commands = nullptr;
	/** @brief Write-buffer offset where this batch's recording begins. */
	std::size_t commandBegin = 0;
	/** @brief Payloads kept alive for the command buffer's pointer arguments. */
	std::vector<std::shared_ptr<void> > retainedPayloads;
	/** @brief Resources created by this batch. */
	std::vector<RecordedGpuResource> uploads;
	/** @brief Resources destroyed by this batch. */
	std::vector<RecordedGpuResource> destroys;

	/** @brief Creates an empty batch that records into target. */
	explicit GpuCommandBatch(CommandBuffer& target, std::pmr::memory_resource* memory = std::pmr::get_default_resource());
	GpuCommandBatch(const GpuCommandBatch&) = delete;
	GpuCommandBatch& operator=(const GpuCommandBatch&) = delete;
	/** @brief Moves a batch without copying retained payloads or commands. */
	GpuCommandBatch(GpuCommandBatch&& other) noexcept;
	/** @brief Replaces this batch with another moved batch. */
	GpuCommandBatch& operator=(GpuCommandBatch&& other) noexcept;

	/** @brief Returns true when no commands or resource effects were recorded. */
	bool empty() const noexcept;
	/** @brief Returns the total GPU byte cost of recorded uploads. */
	std::uint64_t uploadBytes() const noexcept;
	/** @brief Returns the number of Kaldi commands in the batch. */
	std::size_t commandCount() const noexcept;
};

/**
 * @brief Wraps a DecodeTarget in a Euphemy ResidentPayload for texture upload.
 *
 * @param target Decoded image payload to retain.
 * @param bytes Optional byte cost. When zero, frame and palette bytes are summed.
 * @return A ResidentPayload tagged as "decode-target".
 */
MH_LOTTE_API Euph::Asset::ResidentPayload makeDecodeTargetPayload(std::shared_ptr<Euph::Media::Image::DecodeTarget> target, std::uint64_t bytes = 0);

/**
 * @brief Schedules GPU upload and removal commands for Euphemy resident assets.
 *
 * GpuAssetStreamer does not submit to a backend or wait on fences. Callers
 * record uploads/evictions into batches, submit those batches through Kaldi,
 * then call commit or fail once submission ownership is resolved.
 */
class MH_LOTTE_API GpuAssetStreamer {
private:
	Euph::Asset::ResourceRegistry& cpuRegistry;
	HandleAllocator handleMemory;
	OpAllocator opMemory;
	CommandBuffer commandHub;
	std::unordered_map<Euph::Asset::AssetId, GpuResourceRecord> records;
	std::vector<Euph::Asset::AssetId> uploadQueue;
	std::vector<Euph::Asset::AssetId> destroyQueue;
	std::uint64_t gpuBudgetBytes = std::numeric_limits<std::uint64_t>::max();

	GpuResourceRecord& ensureRecord(Euph::Asset::AssetId id, GpuResourceKind kind);
	const GpuResourceRecord* lookup(Euph::Asset::AssetId id) const;
	GpuResourceRecord* lookup(Euph::Asset::AssetId id);
	bool queueUpload(Euph::Asset::AssetId id);
	bool queueDestroy(Euph::Asset::AssetId id);
	void removeQueuedUpload(Euph::Asset::AssetId id);
	void removeQueuedDestroy(Euph::Asset::AssetId id);
	void failRecord(GpuResourceRecord& record, GpuFailureReason reason, std::string message);
	bool canUseCpuPayload(Euph::Asset::AssetId id, const Euph::Asset::ResidentPayload*& payload, GpuResourceRecord& record);
	bool recordBufferUpload(GpuCommandBatch& batch, GpuResourceRecord& record, const Euph::Asset::ResidentPayload& payload);
	bool recordTextureUpload(GpuCommandBatch& batch, GpuResourceRecord& record, const Euph::Asset::ResidentPayload& payload);
	bool recordDestroy(GpuCommandBatch& batch, GpuResourceRecord& record);
	std::vector<GpuResourceRecord*> uploadCandidates();
	std::vector<GpuResourceRecord*> evictionCandidates();
public:
	/** @brief Creates a streamer over a Euphemy CPU registry. */
	explicit GpuAssetStreamer(Euph::Asset::ResourceRegistry& resourceRegistry, Kld::HandleId firstHandleId = 1);

	/** @brief Queues a buffer upload from a Euphemy byte/blob payload. */
	bool requestBuffer(Euph::Asset::AssetId id, BufferUploadOptions options = {}, Euph::Asset::StreamPriority priority = {});
	/** @brief Queues a texture upload from a Euphemy DecodeTarget payload. */
	bool requestTexture(Euph::Asset::AssetId id, TextureUploadOptions options = {}, Euph::Asset::StreamPriority priority = {});
	/** @brief Cancels a queued upload that has not yet been recorded. */
	bool cancelUpload(Euph::Asset::AssetId id);

	/** @brief Marks a committed GPU resource as eligible for budget eviction. */
	bool makeEvictable(Euph::Asset::AssetId id);
	/** @brief Queues explicit destruction for an unpinned, unreferenced resource. */
	bool requestEvict(Euph::Asset::AssetId id);
	/** @brief Pins a resource so eviction will skip it. */
	void pin(Euph::Asset::AssetId id);
	/** @brief Clears a resource pin. */
	void unpin(Euph::Asset::AssetId id);
	/** @brief Adds a strong reference that blocks eviction and explicit destroy. */
	bool retain(Euph::Asset::AssetId id);
	/** @brief Releases one strong reference. */
	bool release(Euph::Asset::AssetId id);
	/** @brief Returns the strong reference count for a GPU resource. */
	std::uint32_t strongRefs(Euph::Asset::AssetId id) const;
	/** @brief Updates the last-touched frame used by eviction ordering. */
	bool touch(Euph::Asset::AssetId id, std::uint64_t frameIndex);
	/** @brief Updates the priority used by upload and eviction ordering. */
	void setPriority(Euph::Asset::AssetId id, Euph::Asset::StreamPriority priority);

	/** @brief Sets the maximum GPU bytes tracked by this streamer. */
	void setBudget(std::uint64_t bytes) noexcept;
	/** @brief Returns the configured GPU byte budget. */
	std::uint64_t budget() const noexcept;
	/** @brief Returns bytes held by committed or recorded GPU handles. */
	std::uint64_t currentUsage() const noexcept;

	/** @brief Records queued uploads into a move-only command batch. */
	GpuCommandBatch recordUploads(std::uint64_t maxUploadBytes = std::numeric_limits<std::uint64_t>::max(), std::pmr::memory_resource* memory = std::pmr::get_default_resource());
	/** @brief Publishes handles from a successfully accepted upload batch. */
	void commitUploads(const GpuCommandBatch& batch);
	/** @brief Marks recorded uploads as failed after backend rejection. */
	void failUploads(const GpuCommandBatch& batch, GpuFailureReason reason = GpuFailureReason::BackendRejected, std::string message = {});

	/** @brief Records explicit and budget-driven destruction commands. */
	GpuCommandBatch recordEvictions(std::size_t maxDestroyCount = std::numeric_limits<std::size_t>::max(), std::pmr::memory_resource* memory = std::pmr::get_default_resource());
	/** @brief Commits recorded destructions and clears handles from records. */
	void commitEvictions(const GpuCommandBatch& batch);
	/** @brief Restores recorded destructions to evictable state after rejection. */
	void failEvictions(const GpuCommandBatch& batch, std::string message = {});

	/** @brief Returns the diagnostic record for id, or nullptr when absent. */
	const GpuResourceRecord* find(Euph::Asset::AssetId id) const;
	/** @brief Returns the GPU residency state for id. */
	GpuResidencyState state(Euph::Asset::AssetId id) const;
	/** @brief Returns the last GPU-side failure reason for id. */
	GpuFailureReason failureReason(Euph::Asset::AssetId id) const;
	/** @brief Returns the last GPU-side failure message for id. */
	const std::string& failureMessage(Euph::Asset::AssetId id) const;
	/** @brief Returns a committed Kaldi handle when the resource is visible. */
	std::optional<Kld::HandleId> handle(Euph::Asset::AssetId id) const;
	/** @brief Returns the number of upload requests waiting to be recorded. */
	std::size_t pendingUploads() const noexcept;
	/** @brief Returns the number of explicit destroy requests waiting to be recorded. */
	std::size_t pendingDestroys() const noexcept;
};

} // namespace Asset
} // namespace Lotte

#endif // LTEGPUASSETSTREAMER_HPP
