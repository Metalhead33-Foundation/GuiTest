#include "LteGpuAssetStreamer.hpp"
#include <algorithm>
#include <utility>

namespace Lotte {
namespace Asset {
namespace {

bool isResidentCpuState(Euph::Asset::ResidencyState state)
{
	return state == Euph::Asset::ResidencyState::Resident || state == Euph::Asset::ResidencyState::Evictable;
}

bool isBytePayloadType(const std::string& type)
{
	return type.empty() || type == "bytes" || type == "blob" || type == "buffer";
}

bool isDecodeTargetPayloadType(const std::string& type)
{
	return type == "decode-target" || type == "image/decode-target" || type == "texture/decode-target";
}

std::uint64_t decodeTargetBytes(const Euph::Media::Image::DecodeTarget& target)
{
	std::uint64_t bytes = 0;
	for (const auto& frame : target.getFrames())
		bytes += static_cast<std::uint64_t>(frame.data.size());
	if (target.getPalette())
		bytes += static_cast<std::uint64_t>(target.getPalette()->data.size());
	return bytes;
}

std::uint64_t estimateUploadBytes(const GpuResourceRecord& record, const Euph::Asset::ResidentPayload& payload)
{
	if (record.kind == GpuResourceKind::Buffer) {
		if (!isBytePayloadType(payload.type) || !payload.data)
			return 0;
		if (payload.bytes != 0)
			return payload.bytes;
		const auto bytes = std::static_pointer_cast<std::vector<std::byte> >(payload.data);
		return bytes ? static_cast<std::uint64_t>(bytes->size()) : 0;
	}

	if (record.kind == GpuResourceKind::Texture) {
		if (!isDecodeTargetPayloadType(payload.type) || !payload.data)
			return 0;
		if (payload.bytes != 0)
			return payload.bytes;
		const auto target = std::static_pointer_cast<Euph::Media::Image::DecodeTarget>(payload.data);
		return target ? decodeTargetBytes(*target) : 0;
	}

	return payload.bytes;
}

} // namespace

GpuCommandBatch::GpuCommandBatch(Kld::HandleAllocator& allocator, std::pmr::memory_resource* memory)
	: commands(allocator, memory)
{
}

bool GpuCommandBatch::empty() const noexcept
{
	return commands.size() == 0 && uploads.empty() && destroys.empty();
}

std::uint64_t GpuCommandBatch::uploadBytes() const noexcept
{
	std::uint64_t bytes = 0;
	for (const auto& upload : uploads)
		bytes += upload.gpuBytes;
	return bytes;
}

std::size_t GpuCommandBatch::commandCount() const noexcept
{
	return commands.size();
}

Euph::Asset::ResidentPayload makeDecodeTargetPayload(std::shared_ptr<Euph::Media::Image::DecodeTarget> target, std::uint64_t bytes)
{
	Euph::Asset::ResidentPayload payload;
	if (target && bytes == 0)
		bytes = decodeTargetBytes(*target);
	payload.data = std::move(target);
	payload.bytes = bytes;
	payload.type = "decode-target";
	return payload;
}

GpuAssetStreamer::GpuAssetStreamer(Euph::Asset::ResourceRegistry& resourceRegistry, Kld::HandleId firstHandleId)
	: cpuRegistry(resourceRegistry), handleAllocator(firstHandleId)
{
}

GpuResourceRecord& GpuAssetStreamer::ensureRecord(Euph::Asset::AssetId id, GpuResourceKind kind)
{
	auto [it, inserted] = records.emplace(id, GpuResourceRecord {});
	if (inserted)
		it->second.assetId = id;
	if (it->second.handle == 0)
		it->second.kind = kind;
	return it->second;
}

const GpuResourceRecord* GpuAssetStreamer::lookup(Euph::Asset::AssetId id) const
{
	const auto it = records.find(id);
	return it == records.end() ? nullptr : &it->second;
}

GpuResourceRecord* GpuAssetStreamer::lookup(Euph::Asset::AssetId id)
{
	const auto it = records.find(id);
	return it == records.end() ? nullptr : &it->second;
}

bool GpuAssetStreamer::queueUpload(Euph::Asset::AssetId id)
{
	if (std::find(uploadQueue.begin(), uploadQueue.end(), id) == uploadQueue.end())
		uploadQueue.push_back(id);
	return true;
}

bool GpuAssetStreamer::queueDestroy(Euph::Asset::AssetId id)
{
	if (std::find(destroyQueue.begin(), destroyQueue.end(), id) == destroyQueue.end())
		destroyQueue.push_back(id);
	return true;
}

void GpuAssetStreamer::removeQueuedUpload(Euph::Asset::AssetId id)
{
	uploadQueue.erase(std::remove(uploadQueue.begin(), uploadQueue.end(), id), uploadQueue.end());
}

void GpuAssetStreamer::removeQueuedDestroy(Euph::Asset::AssetId id)
{
	destroyQueue.erase(std::remove(destroyQueue.begin(), destroyQueue.end(), id), destroyQueue.end());
}

void GpuAssetStreamer::failRecord(GpuResourceRecord& record, GpuFailureReason reason, std::string message)
{
	record.state = GpuResidencyState::Failed;
	record.failureReason = reason;
	record.failureMessage = std::move(message);
	record.handle = 0;
	record.gpuBytes = 0;
	removeQueuedUpload(record.assetId);
	removeQueuedDestroy(record.assetId);
}

bool GpuAssetStreamer::canUseCpuPayload(Euph::Asset::AssetId id, const Euph::Asset::ResidentPayload*& payload, GpuResourceRecord& record)
{
	if (!cpuRegistry.isAlive(id)) {
		failRecord(record, GpuFailureReason::CpuResourceMissing, "CPU resource is not known to Euphemy");
		return false;
	}
	if (!isResidentCpuState(cpuRegistry.state(id))) {
		failRecord(record, GpuFailureReason::CpuResourceNotResident, "CPU resource is not resident");
		return false;
	}
	payload = cpuRegistry.payload(id);
	if (!payload || !*payload) {
		failRecord(record, GpuFailureReason::PayloadMissing, "CPU resident resource has no retained payload");
		return false;
	}
	return true;
}

bool GpuAssetStreamer::recordBufferUpload(GpuCommandBatch& batch, GpuResourceRecord& record, const Euph::Asset::ResidentPayload& payload)
{
	if (!isBytePayloadType(payload.type)) {
		failRecord(record, GpuFailureReason::UnsupportedPayloadType, "Buffer uploads require a bytes/blob/buffer payload");
		return false;
	}
	const auto bytes = std::static_pointer_cast<std::vector<std::byte> >(payload.data);
	if (!bytes) {
		failRecord(record, GpuFailureReason::PayloadMissing, "Buffer upload payload is empty");
		return false;
	}
	if (bytes->size() > std::numeric_limits<std::uint32_t>::max()) {
		failRecord(record, GpuFailureReason::PayloadTooLarge, "Buffer upload exceeds Kaldi's 32-bit buffer size");
		return false;
	}

	auto buffer = batch.commands.createBuffer(static_cast<std::uint32_t>(bytes->size()), record.bufferOptions.policy, bytes->empty() ? nullptr : bytes->data());
	record.handle = buffer.release();
	record.gpuBytes = payload.bytes != 0 ? payload.bytes : static_cast<std::uint64_t>(bytes->size());
	record.state = GpuResidencyState::RecordedUpload;
	record.failureReason = GpuFailureReason::None;
	record.failureMessage.clear();
	batch.retainedPayloads.push_back(payload.data);
	batch.uploads.push_back({ record.assetId, record.kind, record.handle, record.gpuBytes });
	return true;
}

bool GpuAssetStreamer::recordTextureUpload(GpuCommandBatch& batch, GpuResourceRecord& record, const Euph::Asset::ResidentPayload& payload)
{
	if (!isDecodeTargetPayloadType(payload.type)) {
		failRecord(record, GpuFailureReason::UnsupportedPayloadType, "Texture uploads require a DecodeTarget payload");
		return false;
	}
	const auto target = std::static_pointer_cast<Euph::Media::Image::DecodeTarget>(payload.data);
	if (!target) {
		failRecord(record, GpuFailureReason::PayloadMissing, "Texture upload payload is empty");
		return false;
	}

	auto texture = batch.commands.createTexture2DFromDecodeTarget(target.get(), std::max<std::uint8_t>(1, record.textureOptions.mipLevels), record.textureOptions.generateMipmaps);
	record.handle = texture.release();
	record.gpuBytes = payload.bytes != 0 ? payload.bytes : decodeTargetBytes(*target);
	record.state = GpuResidencyState::RecordedUpload;
	record.failureReason = GpuFailureReason::None;
	record.failureMessage.clear();
	batch.retainedPayloads.push_back(payload.data);
	batch.uploads.push_back({ record.assetId, record.kind, record.handle, record.gpuBytes });
	return true;
}

bool GpuAssetStreamer::recordDestroy(GpuCommandBatch& batch, GpuResourceRecord& record)
{
	if (record.handle == 0)
		return false;

	switch (record.kind) {
		case GpuResourceKind::Buffer: batch.commands.destroyBuffer(record.handle); break;
		case GpuResourceKind::Texture: batch.commands.destroyTexture(record.handle); break;
		case GpuResourceKind::Image: batch.commands.destroyImage(record.handle); break;
	}
	record.state = GpuResidencyState::RecordedDestroy;
	record.failureReason = GpuFailureReason::None;
	record.failureMessage.clear();
	batch.destroys.push_back({ record.assetId, record.kind, record.handle, record.gpuBytes });
	return true;
}

std::vector<GpuResourceRecord*> GpuAssetStreamer::uploadCandidates()
{
	std::vector<GpuResourceRecord*> result;
	result.reserve(uploadQueue.size());
	for (Euph::Asset::AssetId id : uploadQueue) {
		GpuResourceRecord* record = lookup(id);
		if (record && record->state == GpuResidencyState::QueuedUpload)
			result.push_back(record);
	}
	std::sort(result.begin(), result.end(), [](const GpuResourceRecord* left, const GpuResourceRecord* right) {
		if (left->priority.value != right->priority.value)
			return left->priority.value > right->priority.value;
		if (left->lastTouchedFrame != right->lastTouchedFrame)
			return left->lastTouchedFrame < right->lastTouchedFrame;
		return left->assetId < right->assetId;
	});
	return result;
}

std::vector<GpuResourceRecord*> GpuAssetStreamer::evictionCandidates()
{
	std::vector<GpuResourceRecord*> result;
	result.reserve(records.size());
	for (auto& [id, record] : records) {
		(void)id;
		if (record.state == GpuResidencyState::Evictable && record.handle != 0 && !record.pinned && record.strongRefs == 0)
			result.push_back(&record);
	}
	std::sort(result.begin(), result.end(), [](const GpuResourceRecord* left, const GpuResourceRecord* right) {
		if (left->priority.value != right->priority.value)
			return left->priority.value < right->priority.value;
		if (left->lastTouchedFrame != right->lastTouchedFrame)
			return left->lastTouchedFrame < right->lastTouchedFrame;
		if (left->gpuBytes != right->gpuBytes)
			return left->gpuBytes > right->gpuBytes;
		return left->assetId < right->assetId;
	});
	return result;
}

bool GpuAssetStreamer::requestBuffer(Euph::Asset::AssetId id, BufferUploadOptions options, Euph::Asset::StreamPriority priority)
{
	if (id == 0)
		return false;
	GpuResourceRecord& record = ensureRecord(id, GpuResourceKind::Buffer);
	if (record.handle != 0 && record.kind != GpuResourceKind::Buffer)
		return false;
	if (record.state == GpuResidencyState::RecordedUpload || record.state == GpuResidencyState::QueuedDestroy || record.state == GpuResidencyState::RecordedDestroy)
		return false;
	record.kind = GpuResourceKind::Buffer;
	record.bufferOptions = options;
	record.priority = priority;
	if (record.state == GpuResidencyState::Resident || record.state == GpuResidencyState::Evictable)
		return true;
	record.state = GpuResidencyState::QueuedUpload;
	record.failureReason = GpuFailureReason::None;
	record.failureMessage.clear();
	return queueUpload(id);
}

bool GpuAssetStreamer::requestTexture(Euph::Asset::AssetId id, TextureUploadOptions options, Euph::Asset::StreamPriority priority)
{
	if (id == 0)
		return false;
	GpuResourceRecord& record = ensureRecord(id, GpuResourceKind::Texture);
	if (record.handle != 0 && record.kind != GpuResourceKind::Texture)
		return false;
	if (record.state == GpuResidencyState::RecordedUpload || record.state == GpuResidencyState::QueuedDestroy || record.state == GpuResidencyState::RecordedDestroy)
		return false;
	record.kind = GpuResourceKind::Texture;
	record.textureOptions = options;
	record.priority = priority;
	if (record.state == GpuResidencyState::Resident || record.state == GpuResidencyState::Evictable)
		return true;
	record.state = GpuResidencyState::QueuedUpload;
	record.failureReason = GpuFailureReason::None;
	record.failureMessage.clear();
	return queueUpload(id);
}

bool GpuAssetStreamer::cancelUpload(Euph::Asset::AssetId id)
{
	GpuResourceRecord* record = lookup(id);
	if (!record || record->state != GpuResidencyState::QueuedUpload)
		return false;
	record->state = GpuResidencyState::Missing;
	removeQueuedUpload(id);
	return true;
}

bool GpuAssetStreamer::makeEvictable(Euph::Asset::AssetId id)
{
	GpuResourceRecord* record = lookup(id);
	if (!record)
		return false;
	if (record->state == GpuResidencyState::Evictable)
		return true;
	if (record->state != GpuResidencyState::Resident)
		return false;
	record->state = GpuResidencyState::Evictable;
	return true;
}

bool GpuAssetStreamer::requestEvict(Euph::Asset::AssetId id)
{
	GpuResourceRecord* record = lookup(id);
	if (!record)
		return false;
	if (record->state == GpuResidencyState::QueuedUpload)
		return cancelUpload(id);
	if ((record->state != GpuResidencyState::Resident && record->state != GpuResidencyState::Evictable) || record->handle == 0)
		return false;
	if (record->pinned || record->strongRefs != 0)
		return false;
	record->state = GpuResidencyState::QueuedDestroy;
	return queueDestroy(id);
}

void GpuAssetStreamer::pin(Euph::Asset::AssetId id)
{
	if (GpuResourceRecord* record = lookup(id))
		record->pinned = true;
}

void GpuAssetStreamer::unpin(Euph::Asset::AssetId id)
{
	if (GpuResourceRecord* record = lookup(id))
		record->pinned = false;
}

bool GpuAssetStreamer::retain(Euph::Asset::AssetId id)
{
	GpuResourceRecord* record = lookup(id);
	if (!record || record->state == GpuResidencyState::Missing || record->state == GpuResidencyState::Failed)
		return false;
	++record->strongRefs;
	return true;
}

bool GpuAssetStreamer::release(Euph::Asset::AssetId id)
{
	GpuResourceRecord* record = lookup(id);
	if (!record || record->strongRefs == 0)
		return false;
	--record->strongRefs;
	return true;
}

std::uint32_t GpuAssetStreamer::strongRefs(Euph::Asset::AssetId id) const
{
	const GpuResourceRecord* record = lookup(id);
	return record ? record->strongRefs : 0;
}

bool GpuAssetStreamer::touch(Euph::Asset::AssetId id, std::uint64_t frameIndex)
{
	GpuResourceRecord* record = lookup(id);
	if (!record)
		return false;
	record->lastTouchedFrame = frameIndex;
	return true;
}

void GpuAssetStreamer::setPriority(Euph::Asset::AssetId id, Euph::Asset::StreamPriority priority)
{
	if (GpuResourceRecord* record = lookup(id))
		record->priority = priority;
}

void GpuAssetStreamer::setBudget(std::uint64_t bytes) noexcept
{
	gpuBudgetBytes = bytes;
}

std::uint64_t GpuAssetStreamer::budget() const noexcept
{
	return gpuBudgetBytes;
}

std::uint64_t GpuAssetStreamer::currentUsage() const noexcept
{
	std::uint64_t bytes = 0;
	for (const auto& [id, record] : records) {
		(void)id;
		if (record.handle != 0)
			bytes += record.gpuBytes;
	}
	return bytes;
}

GpuCommandBatch GpuAssetStreamer::recordUploads(std::uint64_t maxUploadBytes, std::pmr::memory_resource* memory)
{
	GpuCommandBatch batch(handleAllocator, memory);
	if (maxUploadBytes == 0)
		return batch;

	std::uint64_t recordedBytes = 0;
	std::uint64_t projectedUsage = currentUsage();
	for (GpuResourceRecord* record : uploadCandidates()) {
		const Euph::Asset::ResidentPayload* payload = nullptr;
		if (!canUseCpuPayload(record->assetId, payload, *record))
			continue;

		const std::uint64_t estimatedBytes = estimateUploadBytes(*record, *payload);
		if (estimatedBytes > gpuBudgetBytes) {
			failRecord(*record, GpuFailureReason::BudgetExceeded, "GPU resource is larger than the configured budget");
			continue;
		}
		if (recordedBytes + estimatedBytes > maxUploadBytes || projectedUsage + estimatedBytes > gpuBudgetBytes)
			continue;

		bool recorded = false;
		switch (record->kind) {
			case GpuResourceKind::Buffer: recorded = recordBufferUpload(batch, *record, *payload); break;
			case GpuResourceKind::Texture: recorded = recordTextureUpload(batch, *record, *payload); break;
			case GpuResourceKind::Image: break;
		}

		if (recorded) {
			recordedBytes += record->gpuBytes;
			projectedUsage += record->gpuBytes;
			removeQueuedUpload(record->assetId);
		}
	}
	return batch;
}

void GpuAssetStreamer::commitUploads(const GpuCommandBatch& batch)
{
	for (const auto& upload : batch.uploads) {
		GpuResourceRecord* record = lookup(upload.assetId);
		if (!record || record->state != GpuResidencyState::RecordedUpload || record->handle != upload.handle)
			continue;
		record->state = GpuResidencyState::Resident;
		record->gpuBytes = upload.gpuBytes;
		record->failureReason = GpuFailureReason::None;
		record->failureMessage.clear();
	}
}

void GpuAssetStreamer::failUploads(const GpuCommandBatch& batch, GpuFailureReason reason, std::string message)
{
	for (const auto& upload : batch.uploads) {
		GpuResourceRecord* record = lookup(upload.assetId);
		if (!record || record->state != GpuResidencyState::RecordedUpload || record->handle != upload.handle)
			continue;
		failRecord(*record, reason, message);
	}
}

GpuCommandBatch GpuAssetStreamer::recordEvictions(std::size_t maxDestroyCount, std::pmr::memory_resource* memory)
{
	GpuCommandBatch batch(handleAllocator, memory);
	if (maxDestroyCount == 0)
		return batch;

	std::size_t recordedCount = 0;
	std::uint64_t projectedUsage = currentUsage();
	const auto queued = destroyQueue;
	for (Euph::Asset::AssetId id : queued) {
		if (recordedCount >= maxDestroyCount)
			return batch;
		GpuResourceRecord* record = lookup(id);
		if (!record || record->state != GpuResidencyState::QueuedDestroy)
			continue;
		const std::uint64_t bytes = record->gpuBytes;
		if (recordDestroy(batch, *record)) {
			projectedUsage = projectedUsage > bytes ? projectedUsage - bytes : 0;
			removeQueuedDestroy(id);
			++recordedCount;
		}
	}

	if (projectedUsage <= gpuBudgetBytes)
		return batch;

	for (GpuResourceRecord* record : evictionCandidates()) {
		if (recordedCount >= maxDestroyCount || projectedUsage <= gpuBudgetBytes)
			break;
		const std::uint64_t bytes = record->gpuBytes;
		if (recordDestroy(batch, *record)) {
			projectedUsage = projectedUsage > bytes ? projectedUsage - bytes : 0;
			++recordedCount;
		}
	}
	return batch;
}

void GpuAssetStreamer::commitEvictions(const GpuCommandBatch& batch)
{
	for (const auto& destroy : batch.destroys) {
		GpuResourceRecord* record = lookup(destroy.assetId);
		if (!record || record->state != GpuResidencyState::RecordedDestroy || record->handle != destroy.handle)
			continue;
		record->state = GpuResidencyState::Missing;
		record->handle = 0;
		record->gpuBytes = 0;
		record->strongRefs = 0;
		record->failureReason = GpuFailureReason::None;
		record->failureMessage.clear();
	}
}

void GpuAssetStreamer::failEvictions(const GpuCommandBatch& batch, std::string message)
{
	for (const auto& destroy : batch.destroys) {
		GpuResourceRecord* record = lookup(destroy.assetId);
		if (!record || record->state != GpuResidencyState::RecordedDestroy || record->handle != destroy.handle)
			continue;
		record->state = GpuResidencyState::Evictable;
		record->failureReason = GpuFailureReason::BackendRejected;
		record->failureMessage = message;
	}
}

const GpuResourceRecord* GpuAssetStreamer::find(Euph::Asset::AssetId id) const
{
	return lookup(id);
}

GpuResidencyState GpuAssetStreamer::state(Euph::Asset::AssetId id) const
{
	const GpuResourceRecord* record = lookup(id);
	return record ? record->state : GpuResidencyState::Missing;
}

GpuFailureReason GpuAssetStreamer::failureReason(Euph::Asset::AssetId id) const
{
	const GpuResourceRecord* record = lookup(id);
	return record ? record->failureReason : GpuFailureReason::None;
}

const std::string& GpuAssetStreamer::failureMessage(Euph::Asset::AssetId id) const
{
	static const std::string empty;
	const GpuResourceRecord* record = lookup(id);
	return record ? record->failureMessage : empty;
}

std::optional<Kld::HandleId> GpuAssetStreamer::handle(Euph::Asset::AssetId id) const
{
	const GpuResourceRecord* record = lookup(id);
	if (!record || record->handle == 0)
		return std::nullopt;
	if (record->state == GpuResidencyState::Resident || record->state == GpuResidencyState::Evictable)
		return record->handle;
	return std::nullopt;
}

std::size_t GpuAssetStreamer::pendingUploads() const noexcept
{
	return uploadQueue.size();
}

std::size_t GpuAssetStreamer::pendingDestroys() const noexcept
{
	return destroyQueue.size();
}

} // namespace Asset
} // namespace Lotte
