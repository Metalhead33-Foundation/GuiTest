#ifndef ELVNETPREDICTION_HPP
#define ELVNETPREDICTION_HPP
/**
 * @file ElvNetPrediction.hpp
 * @brief Input queues, delay buffering, checksum history and rollback helpers.
 *
 * The simulation itself stays game-owned. These helpers provide the bookkeeping
 * a rollback netcode layer needs:
 * - ordered local/confirmed input queues;
 * - a delayed local-input view for server-authoritative input delivery;
 * - per-tick checksum history and mismatch detection;
 * - `PredictionSession`, which records both input streams and tells the caller
 *   from which tick a mismatch requires replay;
 * - `replayInputs`, a small loop that calls the game's simulation step.
 */
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace Elv {
namespace Net {

/**
 * @class InputQueue
 * @brief Capacity-bounded, tick-ordered input storage.
 * @tparam T Input value type.
 */
template <typename T>
class InputQueue {
public:
	/** @struct Entry
	 *  @brief Tick/input pair.
	 */
	struct Entry {
		Tick tick = 0;
		T input {};
	};

	/** @brief Creates a queue with a sample capacity. */
	explicit InputQueue(std::size_t capacity = 64) : capacity_(capacity) {}

	/**
	 * @brief Inserts or replaces an input.
	 * @param tick Input tick.
	 * @param input Input value.
	 * @return True on success; false when capacity is zero.
	 */
	bool push(Tick tick, const T& input) {
		if (capacity_ == 0) return false;
		const auto position = std::lower_bound(entries_.begin(), entries_.end(), tick,
			[](const Entry& entry, Tick candidate) { return entry.tick < candidate; });
		if (position != entries_.end() && position->tick == tick) {
			position->input = input;
			return true;
		}
		entries_.insert(position, Entry { tick, input });
		if (entries_.size() > capacity_) entries_.erase(entries_.begin());
		return true;
	}

	/** @brief Finds an input by tick. */
	[[nodiscard]] const T* find(Tick tick) const noexcept {
		const auto position = std::lower_bound(entries_.begin(), entries_.end(), tick,
			[](const Entry& entry, Tick candidate) { return entry.tick < candidate; });
		return position != entries_.end() && position->tick == tick ? &position->input : nullptr;
	}

	/** @brief Copies an input by tick. */
	[[nodiscard]] bool get(Tick tick, T& out) const {
		const T* value = find(tick);
		if (value == nullptr) return false;
		out = *value;
		return true;
	}

	/** @brief Returns the newest input. */
	[[nodiscard]] bool latest(Tick& tick, T& out) const {
		if (entries_.empty()) return false;
		tick = entries_.back().tick;
		out = entries_.back().input;
		return true;
	}

	/** @brief Returns the oldest input. */
	[[nodiscard]] bool oldest(Tick& tick, T& out) const {
		if (entries_.empty()) return false;
		tick = entries_.front().tick;
		out = entries_.front().input;
		return true;
	}

	/** @brief Removes samples older than @p tick. */
	void pruneBefore(Tick tick) {
		const auto position = std::lower_bound(entries_.begin(), entries_.end(), tick,
			[](const Entry& entry, Tick candidate) { return entry.tick < candidate; });
		entries_.erase(entries_.begin(), position);
	}

	/** @brief Returns entries in tick order. */
	[[nodiscard]] std::span<const Entry> entries() const noexcept { return entries_; }
	/** @brief Returns the sample count. */
	[[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
	/** @brief Returns the capacity. */
	[[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
	/** @brief Returns whether no inputs are stored. */
	[[nodiscard]] bool empty() const noexcept { return entries_.empty(); }
	/** @brief Removes every input. */
	void clear() noexcept { entries_.clear(); }

private:
	std::vector<Entry> entries_;
	std::size_t capacity_ = 64;
};

/**
 * @class InputDelayBuffer
 * @brief Stores local inputs and exposes them delayed by a fixed tick count.
 * @tparam T Input value type.
 */
template <typename T>
class InputDelayBuffer {
public:
	/** @brief Creates a delay buffer. */
	explicit InputDelayBuffer(Tick delay = 0, std::size_t capacity = 128)
		: queue_(capacity), delay_(delay) {}

	/** @brief Sets the delay in ticks. */
	void setDelay(Tick delay) noexcept { delay_ = delay; }
	/** @brief Returns the delay in ticks. */
	[[nodiscard]] Tick delay() const noexcept { return delay_; }
	/** @brief Inserts a local input. */
	bool push(Tick tick, const T& input) { return queue_.push(tick, input); }

	/** @brief Returns the input generated for @p tick - delay. */
	[[nodiscard]] bool delayedFor(Tick tick, T& out) const {
		return tick >= delay_ && queue_.get(tick - delay_, out);
	}

	/** @brief Finds an input at the delayed tick. */
	[[nodiscard]] const T* findDelayed(Tick tick) const noexcept {
		return tick >= delay_ ? queue_.find(tick - delay_) : nullptr;
	}

	/** @brief Removes inputs older than @p tick - delay. */
	void pruneBefore(Tick tick) { queue_.pruneBefore(tick > delay_ ? tick - delay_ : 0); }
	/** @brief Returns the number of stored inputs. */
	[[nodiscard]] std::size_t size() const noexcept { return queue_.size(); }
	/** @brief Returns the capacity. */
	[[nodiscard]] std::size_t capacity() const noexcept { return queue_.capacity(); }
	/** @brief Removes every input. */
	void clear() noexcept { queue_.clear(); }

private:
	InputQueue<T> queue_;
	Tick delay_ = 0;
};

/**
 * @class ChecksumHistory
 * @brief Capacity-bounded per-tick checksum history.
 */
class ChecksumHistory {
public:
	/** @struct Entry
	 *  @brief Tick/checksum pair.
	 */
	struct Entry {
		Tick tick = 0;
		std::uint32_t checksum = 0;
	};

	/** @brief Creates a history with a sample capacity. */
	explicit ChecksumHistory(std::size_t capacity = 128) : capacity_(capacity) {}

	/** @brief Records or replaces a tick checksum. */
	bool record(Tick tick, std::uint32_t checksum) {
		if (capacity_ == 0) return false;
		const auto position = std::lower_bound(entries_.begin(), entries_.end(), tick,
			[](const Entry& entry, Tick candidate) { return entry.tick < candidate; });
		if (position != entries_.end() && position->tick == tick) {
			position->checksum = checksum;
			return true;
		}
		entries_.insert(position, Entry { tick, checksum });
		if (entries_.size() > capacity_) entries_.erase(entries_.begin());
		return true;
	}

	/** @brief Finds the local checksum for a tick. */
	[[nodiscard]] bool find(Tick tick, std::uint32_t& checksum) const {
		const auto position = std::lower_bound(entries_.begin(), entries_.end(), tick,
			[](const Entry& entry, Tick candidate) { return entry.tick < candidate; });
		if (position == entries_.end() || position->tick != tick) return false;
		checksum = position->checksum;
		return true;
	}

	/** @brief Compares a remote checksum with the local value. */
	[[nodiscard]] bool differs(Tick tick, std::uint32_t remote, bool& differs) const {
		std::uint32_t local = 0;
		if (!find(tick, local)) return false;
		differs = local != remote;
		return true;
	}

	/** @brief Removes entries older than @p tick. */
	void pruneBefore(Tick tick) {
		const auto position = std::lower_bound(entries_.begin(), entries_.end(), tick,
			[](const Entry& entry, Tick candidate) { return entry.tick < candidate; });
		entries_.erase(entries_.begin(), position);
	}

	/** @brief Returns entries in tick order. */
	[[nodiscard]] std::span<const Entry> entries() const noexcept { return entries_; }
	/** @brief Returns the entry count. */
	[[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
	/** @brief Returns the capacity. */
	[[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
	/** @brief Removes every entry. */
	void clear() noexcept { entries_.clear(); }

private:
	std::vector<Entry> entries_;
	std::size_t capacity_ = 128;
};


/**
 * @class PredictionSession
 * @brief Local/confirmed input queues plus checksum rollback detection.
 * @tparam T Input value type.
 */
template <typename T>
class PredictionSession {
public:
	/** @brief Creates a session with matching queue/history capacities. */
	explicit PredictionSession(std::size_t capacity = 128)
		: local_(capacity), confirmed_(capacity), checksums_(capacity) {}

	/** @brief Records a locally predicted input. */
	bool recordLocal(Tick tick, const T& input) { return local_.push(tick, input); }
	/** @brief Records an authoritative/confirmed input. */
	bool recordConfirmed(Tick tick, const T& input) { return confirmed_.push(tick, input); }
	/** @brief Records the local checksum for a tick. */
	bool recordChecksum(Tick tick, std::uint32_t checksum) { return checksums_.record(tick, checksum); }

	/** @brief Returns a local input. */
	[[nodiscard]] const T* localInput(Tick tick) const noexcept { return local_.find(tick); }
	/** @brief Returns a confirmed input. */
	[[nodiscard]] const T* confirmedInput(Tick tick) const noexcept { return confirmed_.find(tick); }
	/** @brief Returns a local checksum. */
	[[nodiscard]] bool localChecksum(Tick tick, std::uint32_t& checksum) const {
		return checksums_.find(tick, checksum);
	}

	/**
	 * @brief Checks whether a remote checksum requires replay.
	 * @param tick Tick to compare.
	 * @param remote Remote checksum.
	 * @param from Receives the rollback start tick on mismatch.
	 * @return True when a mismatch was found; false for match or missing history.
	 */
	[[nodiscard]] bool needsRollback(Tick tick, std::uint32_t remote, Tick& from) const {
		bool differs = false;
		if (!checksums_.differs(tick, remote, differs) || !differs) return false;
		from = tick;
		return true;
	}

	/** @brief Removes local/confirmed/checksum data older than @p tick. */
	void pruneBefore(Tick tick) {
		local_.pruneBefore(tick);
		confirmed_.pruneBefore(tick);
		checksums_.pruneBefore(tick);
	}

	/** @brief Returns the local queue. */
	[[nodiscard]] const InputQueue<T>& local() const noexcept { return local_; }
	/** @brief Returns the confirmed queue. */
	[[nodiscard]] const InputQueue<T>& confirmed() const noexcept { return confirmed_; }
	/** @brief Returns the checksum history. */
	[[nodiscard]] const ChecksumHistory& checksums() const noexcept { return checksums_; }

private:
	InputQueue<T> local_;
	InputQueue<T> confirmed_;
	ChecksumHistory checksums_;
};

/**
 * @brief Replays confirmed inputs through a game-owned simulation step.
 * @tparam State Simulation state type.
 * @tparam Input Input value type.
 * @tparam Step Callable `(Tick, const Input&, State&)`.
 * @param inputs Confirmed input queue.
 * @param from First tick.
 * @param to Last tick, inclusive.
 * @param state Simulation state mutated by @p step.
 * @param step Game simulation callback.
 * @return First tick that was not replayed.
 */
template <typename State, typename Input, typename Step>
Tick replayInputs(const InputQueue<Input>& inputs, Tick from, Tick to, State& state, Step&& step) {
	Tick tick = from;
	for (; tick <= to; ++tick) {
		const Input* input = inputs.find(tick);
		if (input == nullptr) break;
		step(tick, *input, state);
		if (tick == std::numeric_limits<Tick>::max()) {
			++tick;
			break;
		}
	}
	return tick;
}

} // namespace Net
} // namespace Elv
#endif // ELVNETPREDICTION_HPP

