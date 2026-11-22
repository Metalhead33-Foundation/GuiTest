#ifndef ELVNETLOCKSTEP_HPP
#define ELVNETLOCKSTEP_HPP
/**
 * @file ElvNetLockstep.hpp
 * @brief Deterministic input collection and checksum exchange for lockstep.
 *
 * `LockstepInputCollector<T>` keeps one `InputQueue<T>` per peer and only emits a
 * frame when every configured peer supplied that tick. Presentation/simulation
 * remains game-owned; this class is the gate that decides when a tick is ready.
 *
 * `LockstepChecksumExchange` records local and remote per-tick checksums and can
 * report the first divergent tick for a peer.
 */
#include <Elvavena/Net/ElvNetPrediction.hpp>
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Elv {
namespace Net {

/**
 * @class LockstepInputCollector
 * @brief Per-peer input queues with all-peers-ready frame collection.
 * @tparam T Input value type.
 */
template <typename T>
class LockstepInputCollector {
public:
	/** @struct PeerInput
	 *  @brief Input from one peer.
	 */
	struct PeerInput {
		PeerId peer = InvalidPeer;
		T input {};
	};

	/** @struct Frame
	 *  @brief Complete inputs for one tick.
	 */
	struct Frame {
		Tick tick = 0;
		std::vector<PeerInput> inputs;
	};

	/**
	 * @brief Creates a collector for a fixed peer set.
	 * @param peers Peer ids, including the local peer.
	 * @param capacity Per-peer input history capacity.
	 */
	explicit LockstepInputCollector(std::span<const PeerId> peers, std::size_t capacity = 128)
		: capacity_(capacity) {
		for (PeerId peer : peers) {
			if (findPeer(peer) != nullptr) continue;
			peers_.push_back(Peer { peer, InputQueue<T>(capacity) });
		}
	}

	/**
	 * @brief Submits one peer's input for a tick.
	 * @param peer Peer id.
	 * @param tick Tick.
	 * @param input Input value.
	 * @return False for an unknown peer or zero capacity.
	 */
	bool submit(PeerId peer, Tick tick, const T& input) {
		Peer* target = findPeer(peer);
		return target != nullptr && target->inputs.push(tick, input);
	}

	/** @brief Returns whether every peer supplied @p tick. */
	[[nodiscard]] bool ready(Tick tick) const {
		if (peers_.empty()) return false;
		for (const Peer& peer : peers_) {
			if (peer.inputs.find(tick) == nullptr) return false;
		}
		return true;
	}

	/**
	 * @brief Collects a frame for an explicit tick.
	 * @param tick Tick to collect.
	 * @param frame Receives peer inputs in configured order.
	 * @return False when the tick is not ready.
	 */
	[[nodiscard]] bool collect(Tick tick, Frame& frame) const {
		if (!ready(tick)) return false;
		frame.tick = tick;
		frame.inputs.clear();
		frame.inputs.reserve(peers_.size());
		for (const Peer& peer : peers_) {
			const T* input = peer.inputs.find(tick);
			if (input == nullptr) return false;
			frame.inputs.push_back(PeerInput { peer.id, *input });
		}
		return true;
	}

	/**
	 * @brief Collects the next sequential tick when ready and advances it.
	 * @param frame Receives the frame.
	 * @return True when the next tick was complete.
	 */
	bool tryCollectNext(Frame& frame) {
		if (!collect(nextTick_, frame)) return false;
		++nextTick_;
		return true;
	}

	/** @brief Returns the next tick `tryCollectNext` expects. */
	[[nodiscard]] Tick nextTick() const noexcept { return nextTick_; }
	/** @brief Sets the next expected tick. */
	void setNextTick(Tick tick) noexcept { nextTick_ = tick; }
	/** @brief Returns the number of configured peers. */
	[[nodiscard]] std::size_t peerCount() const noexcept { return peers_.size(); }
	/** @brief Returns the per-peer input capacity. */
	[[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

	/** @brief Removes inputs older than @p tick from every peer. */
	void pruneBefore(Tick tick) {
		for (Peer& peer : peers_) peer.inputs.pruneBefore(tick);
		if (nextTick_ < tick) nextTick_ = tick;
	}

	/** @brief Clears all peer inputs and resets the next tick. */
	void clear() noexcept {
		for (Peer& peer : peers_) peer.inputs.clear();
		nextTick_ = 0;
	}

private:
	/** @brief One peer and its input history. */
	struct Peer {
		PeerId id = InvalidPeer;
		InputQueue<T> inputs;
	};

	/** @brief Finds a peer by id. */
	[[nodiscard]] Peer* findPeer(PeerId peer) noexcept {
		for (Peer& candidate : peers_) {
			if (candidate.id == peer) return &candidate;
		}
		return nullptr;
	}

	/** @brief Finds a peer by id. */
	[[nodiscard]] const Peer* findPeer(PeerId peer) const noexcept {
		for (const Peer& candidate : peers_) {
			if (candidate.id == peer) return &candidate;
		}
		return nullptr;
	}

	std::vector<Peer> peers_;
	std::size_t capacity_ = 128;
	Tick nextTick_ = 0;
};

/**
 * @class LockstepChecksumExchange
 * @brief Local and per-peer tick checksum history with divergence lookup.
 */
class LockstepChecksumExchange {
public:
	/**
	 * @brief Creates the exchange for a fixed peer set.
	 * @param peers Peer ids, including the local peer.
	 * @param capacity Per-peer/local checksum capacity.
	 */
	explicit LockstepChecksumExchange(std::span<const PeerId> peers, std::size_t capacity = 128)
		: local_(capacity), capacity_(capacity) {
		for (PeerId peer : peers) {
			if (findPeer(peer) != nullptr) continue;
			peers_.push_back(Peer { peer, ChecksumHistory(capacity) });
		}
	}

	/** @brief Records the local checksum for a tick. */
	bool recordLocal(Tick tick, std::uint32_t checksum) { return local_.record(tick, checksum); }

	/** @brief Records a remote peer's checksum for a tick. */
	bool recordRemote(PeerId peer, Tick tick, std::uint32_t checksum) {
		Peer* target = findPeer(peer);
		return target != nullptr && target->checksums.record(tick, checksum);
	}

	/**
	 * @brief Compares local/remote checksums at one tick.
	 * @param peer Remote peer.
	 * @param tick Tick.
	 * @param differs Receives comparison result.
	 * @return False when either side is missing.
	 */
	[[nodiscard]] bool compare(PeerId peer, Tick tick, bool& differs) const {
		const Peer* target = findPeer(peer);
		std::uint32_t localValue = 0;
		std::uint32_t remoteValue = 0;
		if (target == nullptr || !local_.find(tick, localValue)
			|| !target->checksums.find(tick, remoteValue)) {
			return false;
		}
		differs = localValue != remoteValue;
		return true;
	}

	/**
	 * @brief Finds the first divergent tick at or after @p from.
	 * @param peer Remote peer.
	 * @param from First tick to inspect.
	 * @param mismatch Receives the divergent tick.
	 * @return True when a divergence was found.
	 */
	[[nodiscard]] bool firstMismatch(PeerId peer, Tick from, Tick& mismatch) const {
		const Peer* target = findPeer(peer);
		if (target == nullptr) return false;
		for (const ChecksumHistory::Entry& entry : local_.entries()) {
			if (entry.tick < from) continue;
			std::uint32_t remoteValue = 0;
			if (!target->checksums.find(entry.tick, remoteValue)) continue;
			if (remoteValue != entry.checksum) {
				mismatch = entry.tick;
				return true;
			}
		}
		return false;
	}

	/** @brief Removes checksums older than @p tick. */
	void pruneBefore(Tick tick) {
		local_.pruneBefore(tick);
		for (Peer& peer : peers_) peer.checksums.pruneBefore(tick);
	}

	/** @brief Returns the local checksum history. */
	[[nodiscard]] const ChecksumHistory& local() const noexcept { return local_; }
	/** @brief Returns the number of configured peers. */
	[[nodiscard]] std::size_t peerCount() const noexcept { return peers_.size(); }
	/** @brief Returns the capacity. */
	[[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
	/** @brief Clears all histories. */
	void clear() noexcept {
		local_.clear();
		for (Peer& peer : peers_) peer.checksums.clear();
	}

private:
	/** @brief One peer and its checksum history. */
	struct Peer {
		PeerId id = InvalidPeer;
		ChecksumHistory checksums;
	};

	/** @brief Finds a peer by id. */
	[[nodiscard]] Peer* findPeer(PeerId peer) noexcept {
		for (Peer& candidate : peers_) {
			if (candidate.id == peer) return &candidate;
		}
		return nullptr;
	}

	/** @brief Finds a peer by id. */
	[[nodiscard]] const Peer* findPeer(PeerId peer) const noexcept {
		for (const Peer& candidate : peers_) {
			if (candidate.id == peer) return &candidate;
		}
		return nullptr;
	}

	ChecksumHistory local_;
	std::vector<Peer> peers_;
	std::size_t capacity_ = 128;
};

} // namespace Net
} // namespace Elv
#endif // ELVNETLOCKSTEP_HPP

