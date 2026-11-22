#ifndef ELVNETSEQUENCE_HPP
#define ELVNETSEQUENCE_HPP
/**
 * @file ElvNetSequence.hpp
 * @brief Wrap-safe sequence comparisons, ack bitfields and ordering helpers.
 *
 * UDP-based protocols number packets with a small counter that wraps. Plain
 * integer comparisons are wrong across the wrap point, so this header provides
 * the standard half-range comparison plus a 64-bit acknowledgement window.
 * Both are allocation-free and independent of any transport.
 */
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <cstdint>

namespace Elv {
namespace Net {

/**
 * @brief Returns the forward distance from @p from to @p to in sequence space.
 * @param from Earlier sequence candidate.
 * @param to Later sequence candidate.
 * @return Number of sequence steps from @p from to @p to, modulo 2^16.
 */
constexpr NetSequence sequenceDistance(NetSequence from, NetSequence to) noexcept {
	return static_cast<NetSequence>(to - from);
}

/**
 * @brief Wrap-safe "is later than" comparison for 16-bit sequences.
 * @param lhs Sequence to test.
 * @param rhs Reference sequence.
 * @return True when @p lhs lies in the half-range after @p rhs.
 * @note Exactly half the sequence space away is ambiguous and returns false.
 */
constexpr bool sequenceGreaterThan(NetSequence lhs, NetSequence rhs) noexcept {
	const NetSequence distance = sequenceDistance(rhs, lhs);
	return distance != 0 && distance < 0x8000u;
}

/**
 * @class AckBitfield
 * @brief Tracks the most recent sequence and the previous 63 received values.
 *
 * Bit 0 represents @ref latest; bit N represents `latest - N`. The bitfield is
 * what a receiver puts in an ack packet and what a sender consults before
 * retransmitting.
 */
class AckBitfield {
public:
	/// @brief Number of sequence values representable by the window.
	static constexpr unsigned WindowSize = 64;

	/** @brief Creates an empty bitfield. */
	AckBitfield() noexcept = default;

	/** @brief Resets the bitfield to empty. */
	void reset() noexcept {
		latest_ = 0;
		bits_ = 0;
		hasLatest_ = false;
	}

	/**
	 * @brief Resets the bitfield and marks @p sequence as received.
	 * @param sequence First sequence to mark.
	 */
	void reset(NetSequence sequence) noexcept {
		reset();
		markReceived(sequence);
	}

	/**
	 * @brief Marks a sequence as received and advances the window if needed.
	 * @param sequence Sequence that arrived.
	 * @return True when the sequence was new; false for duplicates or values
	 *         older than the 64-entry window.
	 */
	bool markReceived(NetSequence sequence) noexcept {
		if (!hasLatest_) {
			latest_ = sequence;
			bits_ = 1ull;
			hasLatest_ = true;
			return true;
		}

		if (sequence == latest_) {
			if ((bits_ & 1ull) != 0) return false;
			bits_ |= 1ull;
			return true;
		}

		if (sequenceGreaterThan(sequence, latest_)) {
			const NetSequence shift = sequenceDistance(latest_, sequence);
			if (shift >= WindowSize) {
				bits_ = 1ull;
			} else {
				bits_ = (bits_ << shift) | 1ull;
			}
			latest_ = sequence;
			return true;
		}

		const NetSequence age = sequenceDistance(sequence, latest_);
		if (age >= WindowSize) return false;
		const std::uint64_t mask = 1ull << age;
		if ((bits_ & mask) != 0) return false;
		bits_ |= mask;
		return true;
	}

	/**
	 * @brief Queries whether a sequence is inside the window and marked.
	 * @param sequence Sequence to query.
	 * @return True when received and still tracked.
	 */
	[[nodiscard]] bool isReceived(NetSequence sequence) const noexcept {
		if (!hasLatest_) return false;
		if (sequence == latest_) return (bits_ & 1ull) != 0;
		if (sequenceGreaterThan(sequence, latest_)) return false;
		const NetSequence age = sequenceDistance(sequence, latest_);
		if (age >= WindowSize) return false;
		return (bits_ & (1ull << age)) != 0;
	}

	/**
	 * @brief Loads an acknowledgement bitfield received from a peer.
	 * @param latest Newest sequence acknowledged by the peer.
	 * @param bits Raw acknowledgement bitfield; bit 0 is @p latest.
	 * @note A zero bitfield clears the window because no datagram has been
	 *       acknowledged yet.
	 */
	void assign(NetSequence latest, std::uint64_t bits) noexcept {
		if (bits == 0) {
			reset();
			return;
		}
		latest_ = latest;
		bits_ = bits | 1ull;
		hasLatest_ = true;
	}

	/** @brief Returns the newest received sequence. */
	[[nodiscard]] NetSequence latest() const noexcept { return latest_; }

	/** @brief Returns the raw acknowledgement bitfield. */
	[[nodiscard]] std::uint64_t bits() const noexcept { return bits_; }

	/** @brief Returns whether any sequence has been marked. */
	[[nodiscard]] bool empty() const noexcept { return !hasLatest_; }

private:
	NetSequence latest_ = 0;   ///< Highest sequence observed.
	std::uint64_t bits_ = 0;   ///< Bit 0 is latest, bit N is latest - N.
	bool hasLatest_ = false;   ///< Distinguishes "empty" from "latest is zero".
};

} // namespace Net
} // namespace Elv

#endif // ELVNETSEQUENCE_HPP
