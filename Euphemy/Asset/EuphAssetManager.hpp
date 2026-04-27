#ifndef EUPHASSETMANAGER_HPP
#define EUPHASSETMANAGER_HPP

#include <Euphemy/Asset/EuphAssetRegistry.hpp>
#include <Euphemy/Asset/EuphAsyncIoScheduler.hpp>
#include <Euphemy/Config/EuphLib.hpp>
#include <memory_resource>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <typeindex>
#include <unordered_map>

namespace Euph {
namespace Asset {

using Blob = std::pmr::vector<std::byte>;

class MH_EUPH_API IAssetDecoder {
public:
	virtual ~IAssetDecoder() = default;
	virtual std::string_view typeName() const = 0;
	virtual std::type_index outputType() const = 0;
	virtual std::shared_ptr<const void> decode(std::span<const std::byte> bytes,
											   const AssetRecord& record,
											   std::pmr::memory_resource* memRes) = 0;
};

class MH_EUPH_API RawBlobDecoder : public IAssetDecoder {
public:
	std::string_view typeName() const override;
	std::type_index outputType() const override;
	std::shared_ptr<const void> decode(std::span<const std::byte> bytes,
									   const AssetRecord& record,
									   std::pmr::memory_resource* memRes) override;
};

struct AssetState {
	mutable std::mutex mutex;
	AssetId id = 0;
	ResidencyState residency = ResidencyState::Unloaded;
	std::string error;
	std::type_index type = typeid(void);
	std::shared_ptr<const void> asset;
	std::size_t bytes = 0;
	std::uint64_t lastAccess = 0;
	bool inFlight = false;
};

template <typename T>
class AssetHandle {
private:
	std::weak_ptr<AssetState> weakState;
	AssetId assetId = 0;

public:
	AssetHandle() = default;
	AssetHandle(AssetId id, const std::shared_ptr<AssetState>& state)
		: weakState(state), assetId(id)
	{
	}

	AssetId id() const {
		return assetId;
	}

	bool valid() const {
		return !weakState.expired();
	}

	ResidencyState stateValue() const {
		auto locked = weakState.lock();
		if(!locked) {
			return ResidencyState::Unloaded;
		}
		std::lock_guard<std::mutex> lock(locked->mutex);
		return locked->residency;
	}

	ResidencyState state_enum() const {
		return stateValue();
	}

	ResidencyState state() const {
		return stateValue();
	}

	bool ready() const {
		return static_cast<bool>(lock());
	}

	std::string error() const {
		auto locked = weakState.lock();
		if(!locked) {
			return {};
		}
		std::lock_guard<std::mutex> lock(locked->mutex);
		return locked->error;
	}

	std::shared_ptr<const T> lock() const {
		auto locked = weakState.lock();
		if(!locked) {
			return {};
		}
		std::lock_guard<std::mutex> guard(locked->mutex);
		if(locked->residency != ResidencyState::Resident || !locked->asset || locked->type != std::type_index(typeid(T))) {
			return {};
		}
		return std::static_pointer_cast<const T>(locked->asset);
	}
};

class MH_EUPH_API AssetManager {
private:
	AssetRegistry& registry;
	AsyncIoScheduler& scheduler;
	std::pmr::memory_resource* memRes;
	std::unordered_map<AssetId, std::shared_ptr<AssetState>> states;
	std::unordered_map<std::string, std::unique_ptr<IAssetDecoder>> decoders;
	std::vector<std::thread> loaders;
	mutable std::mutex mutex;
	std::size_t cpuBudgetBytes;
	std::size_t residentBytes;
	std::atomic_uint64_t accessCounter;

	std::shared_ptr<AssetState> getOrCreateState(AssetId id);
	void startLoad(const std::shared_ptr<AssetState>& state, const AssetRecord& record, StreamPriority priority);
	void loaderThread(std::shared_ptr<AssetState> state, AssetRecord record, AsyncRead read);
	std::vector<std::byte> decompressIfNeeded(const AssetRecord& record, std::vector<std::byte>&& bytes);
	void setFailed(const std::shared_ptr<AssetState>& state, const std::string& message);
	void noteResidentBytes(std::size_t oldBytes, std::size_t newBytes);

public:
	AssetManager(AssetRegistry& registry,
				 AsyncIoScheduler& scheduler,
				 std::size_t cpuBudgetBytes = 64 * 1024 * 1024,
				 std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
	~AssetManager();

	template <typename T>
	AssetHandle<T> request(AssetId id, StreamPriority priority = {}) {
		auto state = getOrCreateState(id);
		{
			std::lock_guard<std::mutex> stateLock(state->mutex);
			state->lastAccess = ++accessCounter;
			if(state->residency == ResidencyState::Resident || state->inFlight) {
				return AssetHandle<T>(id, state);
			}
		}

		const AssetRecord* record = registry.find(id);
		if(!record) {
			setFailed(state, "Asset id is not registered");
			return AssetHandle<T>(id, state);
		}
		startLoad(state, *record, priority);
		return AssetHandle<T>(id, state);
	}

	void registerDecoder(std::unique_ptr<IAssetDecoder> decoder);
	void collectGarbage();
	void setCpuBudgetBytes(std::size_t bytes);
	std::size_t getCpuBudgetBytes() const;
	std::size_t getResidentBytes() const;
	std::shared_ptr<AssetState> findState(AssetId id) const;
};

} // namespace Asset
} // namespace Euph

#endif // EUPHASSETMANAGER_HPP
