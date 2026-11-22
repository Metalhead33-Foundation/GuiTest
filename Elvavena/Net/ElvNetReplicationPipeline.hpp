#ifndef ELVNETREPLICATIONPIPELINE_HPP
#define ELVNETREPLICATIONPIPELINE_HPP
/**
 * @file ElvNetReplicationPipeline.hpp
 * @brief Reference server/client glue around snapshot writer/reader and interest.
 *
 * `ReplicationServer` owns one `SnapshotWriter` per peer, asks the configured
 * `InterestManager` for that peer's visible entities, and routes ack/loss tags
 * back to the matching writer. `ReplicationClient` parses incoming payloads and
 * keeps a jitter-buffer copy of recent frames; applying records to the game ECS
 * remains game-owned.
 */
#include <Elvavena/Net/ElvNetInterest.hpp>
#include <Elvavena/Net/ElvNetInterpolation.hpp>
#include <Elvavena/Net/ElvNetReplicationRegistry.hpp>
#include <Elvavena/Net/ElvNetReplicationSchema.hpp>
#include <Elvavena/Net/ElvNetSnapshot.hpp>
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

namespace Elv {
namespace Net {

/**
 * @class ReplicationServer
 * @brief Per-peer interest-filtered snapshot production and ack routing.
 */
class ReplicationServer {
public:
	/**
	 * @brief Creates a server pipeline.
	 * @param registry Component/kind schema.
	 * @param entities Live entity registry.
	 * @param interest Per-peer visibility policy.
	 */
	ReplicationServer(const ComponentRegistry& registry, ReplicationRegistry& entities,
		InterestManager& interest)
		: registry_(registry), entities_(entities), interest_(interest) {}

	/**
	 * @brief Builds a snapshot for one peer.
	 * @param peer Viewer peer.
	 * @param tick Snapshot tick.
	 * @return Encoded snapshot or an error result.
	 */
	SnapshotWriteResult buildSnapshot(PeerId peer, Tick tick) {
		interest_.update(tick);
		visible_.clear();
		interest_.collectRelevant(peer, entities_, visible_);
		auto writer = writers_.find(peer);
		if (writer == writers_.end()) {
			writer = writers_.try_emplace(peer, registry_).first;
		}
		return writer->second.writeFiltered(static_cast<std::uint32_t>(tick), entities_, visible_);
	}

	/**
	 * @brief Promotes the baseline associated with a peer's ack tag.
	 * @param peer Peer that acknowledged.
	 * @param tag `SendOptions::userTag`.
	 * @return True when the tag was pending.
	 */
	bool acknowledge(PeerId peer, std::uint64_t tag) {
		auto writer = writers_.find(peer);
		return writer != writers_.end() && writer->second.acknowledge(tag);
	}

	/**
	 * @brief Drops a lost snapshot's pending baseline.
	 * @param peer Peer whose snapshot was lost.
	 * @param tag `SendOptions::userTag`.
	 * @return True when the tag was pending.
	 */
	bool reject(PeerId peer, std::uint64_t tag) {
		auto writer = writers_.find(peer);
		return writer != writers_.end() && writer->second.reject(tag);
	}

	/** @brief Removes a peer's baselines. */
	bool removePeer(PeerId peer) { return writers_.erase(peer) != 0; }

	/** @brief Returns a peer's writer, or null. */
	[[nodiscard]] SnapshotWriter* writerFor(PeerId peer) {
		auto writer = writers_.find(peer);
		return writer == writers_.end() ? nullptr : &writer->second;
	}

	/** @brief Returns the component schema. */
	[[nodiscard]] const ComponentRegistry& componentRegistry() const noexcept { return registry_; }
	/** @brief Returns the entity registry. */
	[[nodiscard]] ReplicationRegistry& entities() noexcept { return entities_; }

private:
	const ComponentRegistry& registry_;
	ReplicationRegistry& entities_;
	InterestManager& interest_;
	std::unordered_map<PeerId, SnapshotWriter> writers_;
	std::vector<NetId> visible_;
};

/**
 * @class ReplicationClient
 * @brief Snapshot receiver with jitter-buffer storage.
 */
class ReplicationClient {
public:
	/** @brief Creates a client with a snapshot-frame capacity. */
	explicit ReplicationClient(std::size_t capacity = 32) : frames_(capacity) {}

	/**
	 * @brief Stores and parses one snapshot.
	 * @param tick Snapshot tick.
	 * @param payload Wire bytes.
	 * @param registry Component schema.
	 * @return True when the payload parsed successfully.
	 */
	bool receive(std::uint32_t tick, std::span<const std::byte> payload,
		const ComponentRegistry& registry) {
		if (!reader_.parse(payload, registry)) return false;
		frames_.push(tick, payload);
		return true;
	}

	/** @brief Returns the latest parsed snapshot. */
	[[nodiscard]] const SnapshotReader& reader() const noexcept { return reader_; }
	/** @brief Returns the retained frame jitter buffer. */
	[[nodiscard]] SnapshotJitterBuffer& frames() noexcept { return frames_; }
	/** @brief Returns the retained frame jitter buffer. */
	[[nodiscard]] const SnapshotJitterBuffer& frames() const noexcept { return frames_; }

private:
	SnapshotReader reader_;
	SnapshotJitterBuffer frames_;
};

} // namespace Net
} // namespace Elv
#endif // ELVNETREPLICATIONPIPELINE_HPP
