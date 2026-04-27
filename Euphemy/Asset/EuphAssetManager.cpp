#include "EuphAssetManager.hpp"
#include <Euphemy/Io/EuphBufferDevice.hpp>
#include <Euphemy/Io/EuphZstd.hpp>
#include <algorithm>
#include <exception>
#include <stdexcept>

namespace Euph {
namespace Asset {

std::string_view RawBlobDecoder::typeName() const
{
	return "blob";
}

std::type_index RawBlobDecoder::outputType() const
{
	return typeid(Blob);
}

std::shared_ptr<const void> RawBlobDecoder::decode(std::span<const std::byte> bytes,
												   const AssetRecord&,
												   std::pmr::memory_resource* memRes)
{
	auto blob = std::make_shared<Blob>(Blob::allocator_type(memRes));
	blob->assign(bytes.begin(), bytes.end());
	return blob;
}

AssetManager::AssetManager(AssetRegistry& registry,
						   AsyncIoScheduler& scheduler,
						   std::size_t cpuBudgetBytes,
						   std::pmr::memory_resource* memRes)
	: registry(registry),
	  scheduler(scheduler),
	  memRes(memRes),
	  cpuBudgetBytes(cpuBudgetBytes),
	  residentBytes(0),
	  accessCounter(0)
{
	registerDecoder(std::make_unique<RawBlobDecoder>());
	registerDecoder(std::make_unique<RawBlobDecoder>());
	decoders["raw"] = std::make_unique<RawBlobDecoder>();
}

AssetManager::~AssetManager()
{
	for(auto& loader : loaders) {
		if(loader.joinable()) {
			loader.join();
		}
	}
}

std::shared_ptr<AssetState> AssetManager::getOrCreateState(AssetId id)
{
	std::lock_guard<std::mutex> lock(mutex);
	auto it = states.find(id);
	if(it != states.end()) {
		return it->second;
	}
	auto state = std::make_shared<AssetState>();
	state->id = id;
	states.emplace(id, state);
	return state;
}

void AssetManager::startLoad(const std::shared_ptr<AssetState>& state, const AssetRecord& record, StreamPriority priority)
{
	{
		std::lock_guard<std::mutex> stateLock(state->mutex);
		if(state->inFlight || state->residency == ResidencyState::Resident) {
			return;
		}
		state->residency = ResidencyState::Queued;
		state->error.clear();
		state->inFlight = true;
	}

	AsyncRead read = scheduler.read(record.path, record.offset, record.storedSize, priority);
	std::lock_guard<std::mutex> lock(mutex);
	loaders.emplace_back(&AssetManager::loaderThread, this, state, record, std::move(read));
}

void AssetManager::loaderThread(std::shared_ptr<AssetState> state, AssetRecord record, AsyncRead read)
{
	try {
		{
			std::lock_guard<std::mutex> stateLock(state->mutex);
			state->residency = ResidencyState::LoadingIO;
		}
		AsyncReadResult readResult = read.future().get();
		if(readResult.cancelled) {
			setFailed(state, "Asset load was cancelled");
			return;
		}
		if(!readResult.error.empty()) {
			setFailed(state, readResult.error);
			return;
		}

		{
			std::lock_guard<std::mutex> stateLock(state->mutex);
			state->residency = ResidencyState::Decoding;
		}
		std::vector<std::byte> decodedBytes = decompressIfNeeded(record, std::move(readResult.data));

		std::unique_ptr<IAssetDecoder>* decoderPtr = nullptr;
		{
			std::lock_guard<std::mutex> lock(mutex);
			auto decoderIt = decoders.find(record.type);
			if(decoderIt == decoders.end()) {
				decoderIt = decoders.find("blob");
			}
			if(decoderIt != decoders.end()) {
				decoderPtr = &decoderIt->second;
			}
		}
		if(!decoderPtr || !*decoderPtr) {
			setFailed(state, "No decoder registered for asset type: " + record.type);
			return;
		}

		std::shared_ptr<const void> decoded = (*decoderPtr)->decode(decodedBytes, record, memRes);
		const std::size_t newBytes = decodedBytes.size();
		std::size_t oldBytes = 0;
		{
			std::lock_guard<std::mutex> stateLock(state->mutex);
			oldBytes = state->bytes;
			state->asset = std::move(decoded);
			state->type = (*decoderPtr)->outputType();
			state->bytes = newBytes;
			state->residency = ResidencyState::Resident;
			state->error.clear();
			state->inFlight = false;
			state->lastAccess = ++accessCounter;
		}
		noteResidentBytes(oldBytes, newBytes);
		collectGarbage();
	} catch(const std::exception& e) {
		setFailed(state, e.what());
	} catch(...) {
		setFailed(state, "Unknown asset load failure");
	}
}

std::vector<std::byte> AssetManager::decompressIfNeeded(const AssetRecord& record, std::vector<std::byte>&& bytes)
{
	if(record.compression == Compression::None) {
		return std::move(bytes);
	}
	if(record.compression != Compression::Zstd) {
		throw std::runtime_error("Unsupported asset compression");
	}
	if(record.decodedSize == 0) {
		throw std::runtime_error("Zstd asset records must provide decodedSize");
	}

	Euph::Io::ReadOnlyBufferWrapper source(bytes.data(), bytes.size());
	Euph::Io::ZstdDecompressor decompressor(&source);
	std::vector<std::byte> decoded(static_cast<std::size_t>(record.decodedSize));
	const size_t readCount = decompressor.read(decoded.data(), 1, decoded.size());
	decoded.resize(readCount);
	if(readCount != record.decodedSize) {
		throw std::runtime_error("Zstd asset decoded to an unexpected size");
	}
	return decoded;
}

void AssetManager::setFailed(const std::shared_ptr<AssetState>& state, const std::string& message)
{
	std::lock_guard<std::mutex> stateLock(state->mutex);
	state->residency = ResidencyState::Failed;
	state->error = message;
	state->inFlight = false;
}

void AssetManager::noteResidentBytes(std::size_t oldBytes, std::size_t newBytes)
{
	std::lock_guard<std::mutex> lock(mutex);
	residentBytes -= std::min(residentBytes, oldBytes);
	residentBytes += newBytes;
}

void AssetManager::registerDecoder(std::unique_ptr<IAssetDecoder> decoder)
{
	if(!decoder) {
		return;
	}
	std::lock_guard<std::mutex> lock(mutex);
	decoders[std::string(decoder->typeName())] = std::move(decoder);
}

void AssetManager::collectGarbage()
{
	std::vector<std::shared_ptr<AssetState>> candidates;
	{
		std::lock_guard<std::mutex> lock(mutex);
		if(residentBytes <= cpuBudgetBytes) {
			return;
		}
		for(auto& it : states) {
			candidates.push_back(it.second);
		}
	}

	std::sort(candidates.begin(), candidates.end(), [](const std::shared_ptr<AssetState>& left, const std::shared_ptr<AssetState>& right) {
		std::lock_guard<std::mutex> leftLock(left->mutex);
		std::lock_guard<std::mutex> rightLock(right->mutex);
		return left->lastAccess < right->lastAccess;
	});

	for(auto& state : candidates) {
		std::size_t freed = 0;
		{
			std::lock_guard<std::mutex> stateLock(state->mutex);
			if(state->residency != ResidencyState::Resident || !state->asset || state->asset.use_count() > 1) {
				continue;
			}
			state->residency = ResidencyState::Evicting;
			freed = state->bytes;
			state->asset.reset();
			state->bytes = 0;
			state->type = typeid(void);
			state->residency = ResidencyState::Unloaded;
		}
		{
			std::lock_guard<std::mutex> lock(mutex);
			residentBytes -= std::min(residentBytes, freed);
			if(residentBytes <= cpuBudgetBytes) {
				return;
			}
		}
	}
}

void AssetManager::setCpuBudgetBytes(std::size_t bytes)
{
	{
		std::lock_guard<std::mutex> lock(mutex);
		cpuBudgetBytes = bytes;
	}
	collectGarbage();
}

std::size_t AssetManager::getCpuBudgetBytes() const
{
	std::lock_guard<std::mutex> lock(mutex);
	return cpuBudgetBytes;
}

std::size_t AssetManager::getResidentBytes() const
{
	std::lock_guard<std::mutex> lock(mutex);
	return residentBytes;
}

std::shared_ptr<AssetState> AssetManager::findState(AssetId id) const
{
	std::lock_guard<std::mutex> lock(mutex);
	auto it = states.find(id);
	return it == states.end() ? nullptr : it->second;
}

} // namespace Asset
} // namespace Euph
