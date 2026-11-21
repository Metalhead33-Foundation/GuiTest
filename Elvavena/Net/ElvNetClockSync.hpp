#ifndef ELVNETCLOCKSYNC_HPP
#define ELVNETCLOCKSYNC_HPP
/**
 * @file ElvNetClockSync.hpp
 * @brief RTT/offset estimation and tick/time conversion helpers.
 *
 * Network clocks are estimated from round-trip samples: the client records when
 * it sent a probe and when the reply arrived, and the server supplies its clock
 * reading. The sample with the lowest RTT is the least distorted by queueing,
 * so its offset is the best estimate; an exponential moving average keeps the
 * value stable between probes.
 *
 * `TickClock` is the companion conversion between wall time and simulation
 * ticks. Neither type owns a thread or a timer.
 */
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <Elvavena/Util/ElvRingBuffer.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>

namespace Elv {
namespace Net {

/**
 * @struct ClockSample
 * @brief One round-trip probe between a local and a remote clock.
 */
struct ClockSample {
	TimePoint clientSend {};    ///< Local time when the request was sent.
	TimePoint clientReceive {}; ///< Local time when the reply arrived.
	TimePoint serverTime {};    ///< Remote clock reading carried by the reply.

	/** @brief Returns the measured round-trip time. */
	[[nodiscard]] NetDuration roundTrip() const noexcept {
		return clientReceive > clientSend ? clientReceive - clientSend : NetDuration::zero();
	}

	/**
	 * @brief Returns the remote-minus-local offset at the probe midpoint.
	 * @return Offset to add to a local time to obtain remote time.
	 */
	[[nodiscard]] NetDuration offset() const noexcept {
		const NetDuration rtt = roundTrip();
		const TimePoint midpoint = clientSend + rtt / 2;
		return serverTime - midpoint;
	}
};

/**
 * @class ClockEstimator
 * @brief Tracks a bounded history of probes and estimates RTT and offset.
 *
 * The estimator keeps the most recent @p capacity samples, uses the lowest-RTT
 * sample as the offset source, and smooths both RTT and offset with an
 * exponential moving average.
 */
class ClockEstimator {
public:
	/**
	 * @brief Creates an estimator.
	 * @param capacity Number of recent samples to retain; at least one.
	 * @param smoothing EWMA weight for new values in (0, 1].
	 */
	explicit ClockEstimator(std::size_t capacity = 16, double smoothing = 0.1)
		: history(std::max<std::size_t>(capacity, 1)),
		  alpha(std::clamp(smoothing, 1e-6, 1.0)) {}

	/**
	 * @brief Adds a probe sample.
	 * @param clientSend Local send time.
	 * @param clientReceive Local receive time.
	 * @param serverTime Remote clock reading.
	 * @note Samples where the receive time precedes the send time are ignored.
	 */
	void addSample(TimePoint clientSend, TimePoint clientReceive, TimePoint serverTime) {
		if (clientReceive < clientSend) return;
		history.pushOverwrite(ClockSample { clientSend, clientReceive, serverTime });
		recompute();
	}

	/** @brief Drops all samples and estimates. */
	void reset() noexcept {
		history.clear();
		best.reset();
		smoothedRtt = std::chrono::duration<double, std::nano>::zero();
		smoothedOffset = std::chrono::duration<double, std::nano>::zero();
		initialized = false;
	}

	/** @brief Returns whether at least one valid sample exists. */
	[[nodiscard]] bool hasSample() const noexcept { return best.has_value(); }

	/** @brief Returns the smoothed round-trip time. */
	[[nodiscard]] NetDuration rtt() const noexcept {
		return std::chrono::duration_cast<NetDuration>(smoothedRtt);
	}

	/** @brief Returns the smoothed remote-minus-local offset. */
	[[nodiscard]] NetDuration offset() const noexcept {
		return std::chrono::duration_cast<NetDuration>(smoothedOffset);
	}

	/**
	 * @brief Converts a remote clock reading to the local clock.
	 * @param serverTime Remote time.
	 * @return Local time.
	 */
	[[nodiscard]] TimePoint serverToLocal(TimePoint serverTime) const noexcept {
		return serverTime - offset();
	}

	/**
	 * @brief Converts a local clock reading to the remote clock.
	 * @param localTime Local time.
	 * @return Remote time.
	 */
	[[nodiscard]] TimePoint localToServer(TimePoint localTime) const noexcept {
		return localTime + offset();
	}

	/** @brief Returns the EWMA smoothing factor. */
	[[nodiscard]] double smoothing() const noexcept { return alpha; }

	/** @brief Returns the retained sample history, oldest first. */
	[[nodiscard]] const Util::RingBuffer<ClockSample>& samples() const noexcept {
		return history;
	}

private:
	/** @brief Re-selects the best sample and updates the smoothed estimates. */
	void recompute() {
		using DoubleNanos = std::chrono::duration<double, std::nano>;
		std::optional<ClockSample> candidate;
		for (std::size_t index = 0; index < history.size(); ++index) {
			const ClockSample& sample = history[index];
			if (!candidate.has_value() || sample.roundTrip() < candidate->roundTrip()) {
				candidate = sample;
			}
		}
		if (!candidate.has_value()) return;

		const DoubleNanos rtt = DoubleNanos(candidate->roundTrip());
		const DoubleNanos offset = DoubleNanos(candidate->offset());
		if (!initialized) {
			smoothedRtt = rtt;
			smoothedOffset = offset;
			initialized = true;
		} else {
			const double keep = 1.0 - alpha;
			smoothedRtt = DoubleNanos(smoothedRtt.count() * keep + rtt.count() * alpha);
			smoothedOffset = DoubleNanos(smoothedOffset.count() * keep + offset.count() * alpha);
		}
		best = candidate;
	}

	Util::RingBuffer<ClockSample> history; ///< Recent probes, oldest first.
	double alpha;                          ///< EWMA weight for new values.
	std::optional<ClockSample> best;       ///< Lowest-RTT sample in the window.
	std::chrono::duration<double, std::nano> smoothedRtt {};    ///< Smoothed RTT.
	std::chrono::duration<double, std::nano> smoothedOffset {}; ///< Smoothed offset.
	bool initialized = false;              ///< Whether the EWMA has a seed value.
};

/**
 * @class TickClock
 * @brief Converts between wall time and fixed-rate simulation ticks.
 */
class TickClock {
public:
	/**
	 * @brief Creates a tick clock.
	 * @param tickRate Ticks per second; must be positive.
	 * @param startTick Tick value at @p epoch.
	 * @param epoch Time corresponding to @p startTick.
	 * @throws std::invalid_argument When @p tickRate is not positive.
	 */
	explicit TickClock(double tickRate, Tick startTick = 0, TimePoint epoch = {})
		: rate(tickRate), start(startTick), origin(epoch) {
		if (!(tickRate > 0.0) || !std::isfinite(tickRate)) {
			throw std::invalid_argument("TickClock requires a positive finite tick rate");
		}
	}

	/**
	 * @brief Converts a time point to the nearest tick.
	 * @param time Local or server time on the same epoch as @ref epoch.
	 * @return Tick number; times before the epoch clamp to @ref startTick.
	 */
	[[nodiscard]] Tick tickAt(TimePoint time) const {
		if (time <= origin) return start;
		const double seconds = std::chrono::duration<double>(time - origin).count();
		const auto delta = static_cast<std::uint64_t>(std::llround(seconds * rate));
		return start + delta;
	}

	/**
	 * @brief Converts a tick to its time point.
	 * @param tick Tick number; values before @ref startTick clamp to the epoch.
	 * @return Time point.
	 */
	[[nodiscard]] TimePoint timeOf(Tick tick) const {
		if (tick <= start) return origin;
		const double seconds = static_cast<double>(tick - start) / rate;
		return origin + std::chrono::duration_cast<NetDuration>(
			std::chrono::duration<double>(seconds));
	}

	/** @brief Returns the tick rate. */
	[[nodiscard]] double tickRate() const noexcept { return rate; }

	/** @brief Returns the tick at the epoch. */
	[[nodiscard]] Tick startTick() const noexcept { return start; }

	/** @brief Returns the epoch time. */
	[[nodiscard]] TimePoint epoch() const noexcept { return origin; }

private:
	double rate;         ///< Ticks per second.
	Tick start = 0;      ///< Tick at @ref origin.
	TimePoint origin {}; ///< Time corresponding to @ref start.
};

} // namespace Net
} // namespace Elv

#endif // ELVNETCLOCKSYNC_HPP

