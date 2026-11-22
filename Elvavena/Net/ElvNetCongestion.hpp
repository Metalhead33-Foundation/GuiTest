#ifndef ELVNETCONGESTION_HPP
#define ELVNETCONGESTION_HPP
/**
 * @file ElvNetCongestion.hpp
 * @brief Pluggable send-rate and retransmission-timer policy.
 *
 * Reliable UDP needs two related decisions: when a reliable packet should be
 * retransmitted, and how many bytes may be in flight. Both are policy, not
 * protocol structure, so the session delegates them to this interface.
 *
 * The default `NullCongestion` permits every send and uses a fixed RTO, which
 * is appropriate for tests and low-rate control traffic. `FixedRateCongestion`
 * adds a simple per-peer token bucket and is the first production default.
 */
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace Elv {
namespace Net {

/**
 * @class CongestionController
 * @brief Interface for send pacing and retransmission timeouts.
 *
 * Implementations are called from the session thread only.
 */
class CongestionController {
public:
	/** @brief Virtual destructor. */
	virtual ~CongestionController() = default;

	/**
	 * @brief Returns whether a datagram of @p bytes may be sent now.
	 * @param peer Destination peer.
	 * @param bytes Datagram size including headers.
	 * @param now Current session time.
	 * @return True when the send is allowed.
	 */
	[[nodiscard]] virtual bool canSend(PeerId peer, std::size_t bytes, TimePoint now) = 0;

	/**
	 * @brief Records a datagram handed to the transport.
	 * @param peer Destination peer.
	 * @param bytes Datagram size.
	 * @param now Current session time.
	 */
	virtual void onSent(PeerId peer, std::size_t bytes, TimePoint now) = 0;

	/**
	 * @brief Records an acknowledgement.
	 * @param peer Peer that acknowledged.
	 * @param bytes Acknowledged payload size.
	 * @param rtt Measured round-trip time.
	 */
	virtual void onAcked(PeerId peer, std::size_t bytes, NetDuration rtt) = 0;

	/**
	 * @brief Records a drop or retransmission timeout.
	 * @param peer Affected peer.
	 * @param bytes Payload size.
	 */
	virtual void onLost(PeerId peer, std::size_t bytes) = 0;

	/**
	 * @brief Returns the retransmission timeout for a peer.
	 * @param peer Peer to query.
	 * @return Time to wait before resending a reliable datagram.
	 */
	[[nodiscard]] virtual NetDuration retransmissionTimeout(PeerId peer) const = 0;

	/** @brief Resets all per-peer state. */
	virtual void reset() = 0;
};

/**
 * @class NullCongestion
 * @brief Unrestricted send policy with a fixed retransmission timeout.
 */
class NullCongestion final : public CongestionController {
public:
	/**
	 * @brief Creates an unrestricted policy.
	 * @param retransmissionTimeout Fixed RTO; must be positive.
	 */
	explicit NullCongestion(NetDuration retransmissionTimeout = std::chrono::milliseconds(200))
		: timeout(retransmissionTimeout > NetDuration::zero()
			? retransmissionTimeout : std::chrono::milliseconds(200)) {}

	/** @brief Always allows a send. */
	[[nodiscard]] bool canSend(PeerId, std::size_t, TimePoint) override { return true; }

	/** @brief Ignores send bookkeeping. */
	void onSent(PeerId, std::size_t, TimePoint) override {}

	/** @brief Ignores acknowledgements. */
	void onAcked(PeerId, std::size_t, NetDuration) override {}

	/** @brief Ignores losses. */
	void onLost(PeerId, std::size_t) override {}

	/** @brief Returns the configured fixed RTO. */
	[[nodiscard]] NetDuration retransmissionTimeout(PeerId) const override { return timeout; }

	/** @brief No per-peer state to reset. */
	void reset() override {}

private:
	NetDuration timeout; ///< Fixed retransmission timeout.
};

/**
 * @class FixedRateCongestion
 * @brief Per-peer token-bucket pacing with a fixed retransmission timeout.
 *
 * The bucket refills at @p bytesPerSecond and holds a quarter-second burst.
 * This is deliberately simple: enough to stop a session from flooding a link,
 * with the interface left open for a real RTT-driven controller later.
 */
class FixedRateCongestion final : public CongestionController {
public:
	/**
	 * @brief Creates a paced policy.
	 * @param bytesPerSecond Sustained send rate per peer; must be positive.
	 * @param retransmissionTimeout Fixed RTO; must be positive.
	 */
	explicit FixedRateCongestion(std::size_t bytesPerSecond = 64 * 1024,
		NetDuration retransmissionTimeout = std::chrono::milliseconds(200))
		: rate(std::max<std::size_t>(bytesPerSecond, 1)),
		  timeout(retransmissionTimeout > NetDuration::zero()
			? retransmissionTimeout : std::chrono::milliseconds(200)),
		  burst(std::max<std::size_t>(rate / 4, 1)) {}

	/** @brief Checks the peer's bucket without consuming it. */
	[[nodiscard]] bool canSend(PeerId peer, std::size_t bytes, TimePoint now) override {
		Bucket& bucket = bucketFor(peer, now);
		refill(bucket, now);
		return bucket.tokens >= static_cast<double>(bytes);
	}

	/** @brief Consumes tokens for a sent datagram. */
	void onSent(PeerId peer, std::size_t bytes, TimePoint now) override {
		Bucket& bucket = bucketFor(peer, now);
		refill(bucket, now);
		bucket.tokens = std::max(0.0, bucket.tokens - static_cast<double>(bytes));
	}

	/** @brief Acknowledgements do not currently change the rate. */
	void onAcked(PeerId, std::size_t, NetDuration) override {}

	/** @brief Losses do not currently change the rate. */
	void onLost(PeerId, std::size_t) override {}

	/** @brief Returns the configured fixed RTO. */
	[[nodiscard]] NetDuration retransmissionTimeout(PeerId) const override { return timeout; }

	/** @brief Drops all peer buckets. */
	void reset() override { buckets.clear(); }

private:
	/** @brief Per-peer token bucket. */
	struct Bucket {
		double tokens = 0.0;  ///< Available bytes.
		TimePoint updated {}; ///< Last refill time.
	};

	/**
	 * @brief Finds or creates a bucket initialized to a full burst.
	 * @param peer Peer key.
	 * @param now Current time.
	 * @return Reference to the bucket.
	 */
	Bucket& bucketFor(PeerId peer, TimePoint now) {
		auto [iterator, inserted] = buckets.try_emplace(peer);
		if (inserted) {
			iterator->second.tokens = static_cast<double>(burst);
			iterator->second.updated = now;
		}
		return iterator->second;
	}

	/**
	 * @brief Refills a bucket based on elapsed time.
	 * @param bucket Bucket to refill.
	 * @param now Current time.
	 */
	void refill(Bucket& bucket, TimePoint now) {
		if (now <= bucket.updated) return;
		const double seconds = std::chrono::duration<double>(now - bucket.updated).count();
		bucket.tokens = std::min(static_cast<double>(burst),
			bucket.tokens + seconds * static_cast<double>(rate));
		bucket.updated = now;
	}

	std::size_t rate;                            ///< Bytes per second.
	NetDuration timeout;                         ///< Fixed RTO.
	std::size_t burst;                           ///< Bucket capacity in bytes.
	std::unordered_map<PeerId, Bucket> buckets;  ///< Per-peer state.
};

} // namespace Net
} // namespace Elv

#endif // ELVNETCONGESTION_HPP

