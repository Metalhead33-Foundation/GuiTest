#ifndef LTEGPUASSETSTREAMER_HPP
#define LTEGPUASSETSTREAMER_HPP

#include <Lotte/LotteLib.hpp>
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

enum class GpuResourceKind : std::uint8_t {
	Buffer,
	Texture,
	Image
};

enum class GpuResidencyState : std::uint8_t {
	Missing,
	QueuedUpload,
	RecordedUpload,
	Resident,
	Evictable,
	QueuedDestroy,
	RecordedDestroy,
	Failed
};

enum class GpuFailureReason : std::uint8_t {
	None,
	CpuResourceMissing,
	CpuResourceNotResident,
	PayloadMissing,
	UnsupportedPayloadType,
	PayloadTooLarge,
	BudgetExceeded,
	BackendRejected,
	Unknown
};

struct BufferUploadOptions {
	std::uint32_t policy = 0;
};

struct TextureUploadOptions {
	std::uint8_t mipLevels = 1;
	bool generateMipmaps = false;
};

struct GpuResourceRecord {
	Euph::Asset::AssetId assetId = 0;
	GpuResourceKind kind = GpuResourceKind::Buffer;
	GpuResidencyState state = GpuResidencyState::Missing;
	GpuFailureReason failureReason = GpuFailureReason::None;
	std::string failureMessage;
	Kld::HandleId handle = 0;
	std::uint64_t gpuBytes = 0;
	Euph::Asset::StreamPriority priority {};
	std::uint64_t lastTouchedFrame = 0;
	std::uint32_t strongRefs = 0;
	bool pinned = false;
	BufferUploadOptions bufferOptions {};
	TextureUploadOptions textureOptions {};
};

struct RecordedGpuResource {
	Euph::Asset::AssetId assetId = 0;
	GpuResourceKind kind = GpuResourceKind::Buffer;
	Kld::HandleId handle = 0;
	std::uint64_t gpuBytes = 0;
};

class MH_LOTTE_API GpuCommandBatch {
public:
	Kld::CommandBuffer commands;
	std::vector<std::shared_ptr<void> > retainedPayloads;
	std::vector<RecordedGpuResource> uploads;
	std::vector<RecordedGpuResource> destroys;

	explicit GpuCommandBatch(Kld::HandleAllocator& allocator, std::pmr::memory_resource* memory = std::pmr::get_default_resource());
	GpuCommandBatch(const GpuCommandBatch&) = delete;
	GpuCommandBatch& operator=(const GpuCommandBatch&) = delete;
	GpuCommandBatch(GpuCommandBatch&&) noexcept = default;
	GpuCommandBatch& operator=(GpuCommandBatch&&) noexcept = default;

	bool empty() const noexcept;
	std::uint64_t uploadBytes() const noexcept;
	std::size_t commandCount() const noexcept;
};

MH_LOTTE_API Euph::Asset::ResidentPayload makeDecodeTargetPayload(std::shared_ptr<Euph::Media::Image::DecodeTarget> target, std::uint64_t bytes = 0);

class MH_LOTTE_API GpuAssetStreamer {
private:
	Euph::Asset::ResourceRegistry& cpuRegistry;
	Kld::HandleAllocator handleAllocator;
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
	explicit GpuAssetStreamer(Euph::Asset::ResourceRegistry& resourceRegistry, Kld::HandleId firstHandleId = 1);

	bool requestBuffer(Euph::Asset::AssetId id, BufferUploadOptions options = {}, Euph::Asset::StreamPriority priority = {});
	bool requestTexture(Euph::Asset::AssetId id, TextureUploadOptions options = {}, Euph::Asset::StreamPriority priority = {});
	bool cancelUpload(Euph::Asset::AssetId id);

	bool makeEvictable(Euph::Asset::AssetId id);
	bool requestEvict(Euph::Asset::AssetId id);
	void pin(Euph::Asset::AssetId id);
	void unpin(Euph::Asset::AssetId id);
	bool retain(Euph::Asset::AssetId id);
	bool release(Euph::Asset::AssetId id);
	std::uint32_t strongRefs(Euph::Asset::AssetId id) const;
	bool touch(Euph::Asset::AssetId id, std::uint64_t frameIndex);
	void setPriority(Euph::Asset::AssetId id, Euph::Asset::StreamPriority priority);

	void setBudget(std::uint64_t bytes) noexcept;
	std::uint64_t budget() const noexcept;
	std::uint64_t currentUsage() const noexcept;

	GpuCommandBatch recordUploads(std::uint64_t maxUploadBytes = std::numeric_limits<std::uint64_t>::max(), std::pmr::memory_resource* memory = std::pmr::get_default_resource());
	void commitUploads(const GpuCommandBatch& batch);
	void failUploads(const GpuCommandBatch& batch, GpuFailureReason reason = GpuFailureReason::BackendRejected, std::string message = {});

	GpuCommandBatch recordEvictions(std::size_t maxDestroyCount = std::numeric_limits<std::size_t>::max(), std::pmr::memory_resource* memory = std::pmr::get_default_resource());
	void commitEvictions(const GpuCommandBatch& batch);
	void failEvictions(const GpuCommandBatch& batch, std::string message = {});

	const GpuResourceRecord* find(Euph::Asset::AssetId id) const;
	GpuResidencyState state(Euph::Asset::AssetId id) const;
	GpuFailureReason failureReason(Euph::Asset::AssetId id) const;
	const std::string& failureMessage(Euph::Asset::AssetId id) const;
	std::optional<Kld::HandleId> handle(Euph::Asset::AssetId id) const;
	std::size_t pendingUploads() const noexcept;
	std::size_t pendingDestroys() const noexcept;
};

} // namespace Asset
} // namespace Lotte

#endif // LTEGPUASSETSTREAMER_HPP
