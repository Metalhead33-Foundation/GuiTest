#ifndef ELVLOOPBACKTRANSPORT_HPP
#define ELVLOOPBACKTRANSPORT_HPP
/**
 * @file ElvLoopbackTransport.hpp
 * @brief Deterministic in-process datagram transport for tests and local play.
 *
 * A `LoopbackNetwork` is a virtual wire shared by any number of endpoints. It
 * schedules each datagram for delivery using a virtual clock and can inject
 * latency, jitter, loss and duplication. The pseudo-random generator is seeded,
 * so a failing test can be replayed exactly.
 *
 * The network must outlive every transport attached to it. Neither class is
 * thread-safe; use one network per simulation thread.
 */
#include <Elvavena/Net/ElvDatagramTransport.hpp>
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <map>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace Elv {
namespace Net {

/**
 * @class LoopbackNetwork
 * @brief Virtual datagram wire with configurable impairments.
 */
class LoopbackNetwork {
public:
	/// @brief Transport family tag stored in every loopback endpoint.
	static constexpr std::uint16_t AddressFamily = 0x4C42; // "LB"

	/**
	 * @struct Config
	 * @brief Impairment and sizing knobs.
	 */
	struct Config {
		NetDuration latency = std::chrono::milliseconds(40); ///< One-way base latency.
		NetDuration jitter = NetDuration::zero();            ///< Uniform +/- jitter around latency.
		double loss = 0.0;                                   ///< Probability in [0, 1] of dropping.
		double duplicate = 0.0;                              ///< Probability in [0, 1] of duplicating.
		std::size_t maxDatagramSize = 1200;                  ///< Maximum accepted payload.
	};

	/** @brief Creates a virtual wire with default settings. */
	LoopbackNetwork() noexcept = default;

	/**
	 * @brief Creates a virtual wire.
	 * @param config Impairment configuration.
	 * @param seed Deterministic RNG seed; zero is replaced by a fixed default.
	 */
	explicit LoopbackNetwork(Config config,
		std::uint64_t seed = 0x9E3779B97F4A7C15ull) noexcept
		: config_(sanitize(std::move(config))), randomState(seed == 0 ? 0x9E3779B97F4A7C15ull : seed) {}

	/**
	 * @brief Creates a new endpoint on this wire.
	 * @return Opaque endpoint usable with a @ref LoopbackTransport.
	 */
	Endpoint createEndpoint() {
		const std::uint64_t id = nextEndpointId++;
		Endpoint endpoint;
		endpoint.family = AddressFamily;
		endpoint.size = static_cast<std::uint16_t>(sizeof(id));
		std::memcpy(endpoint.storage.data(), &id, sizeof(id));
		inboxes.try_emplace(id);
		return endpoint;
	}

	/**
	 * @brief Removes an endpoint and discards its queued datagrams.
	 * @param endpoint Endpoint previously returned by @ref createEndpoint.
	 */
	void destroyEndpoint(const Endpoint& endpoint) {
		if (!isEndpointValid(endpoint)) return;
		inboxes.erase(endpointId(endpoint));
	}

	/**
	 * @brief Returns whether an endpoint belongs to this wire and still exists.
	 * @param endpoint Endpoint to validate.
	 * @return True when the endpoint is live.
	 */
	[[nodiscard]] bool isEndpointValid(const Endpoint& endpoint) const {
		if (endpoint.size != sizeof(std::uint64_t) || endpoint.family != AddressFamily) return false;
		return inboxes.find(endpointId(endpoint)) != inboxes.end();
	}

	/** @brief Returns the current virtual time. */
	[[nodiscard]] TimePoint now() const noexcept { return currentTime; }

	/**
	 * @brief Advances virtual time and delivers every datagram whose due time
	 *        is at or before @p target.
	 * @param target New virtual time; must not move backwards.
	 */
	void advanceTo(TimePoint target) {
		if (target < currentTime) return;
		while (!pending.empty() && pending.begin()->first <= target) {
			auto iterator = pending.begin();
			Pending delivery = std::move(iterator->second);
			pending.erase(iterator);
			auto inbox = inboxes.find(endpointId(delivery.destination));
			if (inbox != inboxes.end()) {
				inbox->second.push_back(std::move(delivery.delivery));
			}
		}
		currentTime = target;
	}

	/**
	 * @brief Advances virtual time by a relative duration.
	 * @param delta Non-negative duration.
	 */
	void advance(NetDuration delta) { advanceTo(currentTime + delta); }

	/** @brief Returns the active configuration. */
	[[nodiscard]] const Config& config() const noexcept { return config_; }

	/** @brief Returns the number of live endpoints. */
	[[nodiscard]] std::size_t endpointCount() const noexcept { return inboxes.size(); }

private:
	friend class LoopbackTransport;

	/** @brief A datagram waiting in an endpoint inbox. */
	struct Delivery {
		Endpoint source;                ///< Original sender.
		std::vector<std::byte> payload; ///< Owned payload copy.
	};

	/** @brief A datagram scheduled for future delivery. */
	struct Pending {
		Endpoint destination; ///< Recipient.
		Delivery delivery;    ///< Sender and payload.
	};

	/**
	 * @brief Normalizes invalid configuration values.
	 * @param config Caller configuration.
	 * @return Sanitized copy.
	 */
	static Config sanitize(Config config) noexcept {
		if (config.latency < NetDuration::zero()) config.latency = NetDuration::zero();
		if (config.jitter < NetDuration::zero()) config.jitter = NetDuration::zero();
		config.loss = std::clamp(config.loss, 0.0, 1.0);
		config.duplicate = std::clamp(config.duplicate, 0.0, 1.0);
		if (config.maxDatagramSize == 0) config.maxDatagramSize = 1200;
		return config;
	}

	/**
	 * @brief Extracts the integer endpoint identifier.
	 * @param endpoint Valid endpoint.
	 * @return Identifier stored in the endpoint.
	 */
	[[nodiscard]] static std::uint64_t endpointId(const Endpoint& endpoint) noexcept {
		std::uint64_t id = 0;
		std::memcpy(&id, endpoint.storage.data(), sizeof(id));
		return id;
	}

	/**
	 * @brief Queues one datagram, applying loss and duplication.
	 * @param source Sender endpoint.
	 * @param destination Recipient endpoint.
	 * @param payload Bytes to copy.
	 * @return @ref NetError::None when accepted (including intentional loss).
	 */
	NetError enqueue(const Endpoint& source, const Endpoint& destination,
		std::span<const std::byte> payload) {
		if (!isEndpointValid(source) || !isEndpointValid(destination)) {
			return NetError::InvalidEndpoint;
		}
		if (payload.size() > config_.maxDatagramSize) return NetError::MessageTooLarge;
		if (config_.loss >= 1.0 || (config_.loss > 0.0 && nextUnit() < config_.loss)) {
			return NetError::None;
		}

		const auto schedule = [&]() {
			NetDuration jitterOffset = NetDuration::zero();
			if (config_.jitter > NetDuration::zero()) {
				const double alpha = nextUnit() * 2.0 - 1.0;
				jitterOffset = NetDuration(static_cast<NetDuration::rep>(
					std::llround(alpha * static_cast<double>(config_.jitter.count()))));
			}
			TimePoint due = currentTime + config_.latency + jitterOffset;
			if (due < currentTime) due = currentTime;
			pending.emplace(due, Pending {
				destination,
				Delivery { source, std::vector<std::byte>(payload.begin(), payload.end()) }
			});
		};

		schedule();
		if (config_.duplicate >= 1.0 || (config_.duplicate > 0.0 && nextUnit() < config_.duplicate)) {
			schedule();
		}
		return NetError::None;
	}

	/**
	 * @brief Returns the inbox for a live endpoint.
	 * @param endpoint Endpoint to look up.
	 * @return Reference to the endpoint's inbox.
	 * @pre The endpoint must be valid.
	 */
	std::deque<Delivery>& inboxFor(const Endpoint& endpoint) {
		return inboxes.at(endpointId(endpoint));
	}

	/**
	 * @brief Advances the xorshift64 generator.
	 * @return Next pseudo-random value.
	 */
	std::uint64_t nextRandom() noexcept {
		randomState ^= randomState << 13;
		randomState ^= randomState >> 7;
		randomState ^= randomState << 17;
		return randomState;
	}

	/**
	 * @brief Returns a uniform value in [0, 1).
	 * @return Next pseudo-random unit value.
	 */
	double nextUnit() noexcept {
		return static_cast<double>(nextRandom() >> 11) * (1.0 / 9007199254740992.0);
	}

	Config config_;                                        ///< Sanitized configuration.
	std::uint64_t randomState = 0x9E3779B97F4A7C15ull;      ///< xorshift64 state.
	std::uint64_t nextEndpointId = 1;                      ///< Monotonic endpoint handle.
	TimePoint currentTime {};                              ///< Virtual clock.
	std::map<std::uint64_t, std::deque<Delivery>> inboxes; ///< Per-endpoint queues.
	std::multimap<TimePoint, Pending> pending;             ///< Scheduled deliveries.
};


/**
 * @class LoopbackTransport
 * @brief A `DatagramTransport` endpoint attached to a @ref LoopbackNetwork.
 *
 * Construction creates an endpoint on the network; destruction removes it.
 * The network must outlive the transport. The transport is non-copyable and
 * non-movable so its endpoint identity cannot be duplicated accidentally.
 */
class LoopbackTransport final : public DatagramTransport {
public:
	/**
	 * @brief Creates an endpoint on a virtual wire.
	 * @param network Wire that carries the datagrams.
	 */
	explicit LoopbackTransport(LoopbackNetwork& network)
		: network(&network), local(network.createEndpoint()), open(true) {}

	/** @brief Removes the endpoint from the wire. */
	~LoopbackTransport() override { close(); }

	LoopbackTransport(const LoopbackTransport&) = delete;
	LoopbackTransport& operator=(const LoopbackTransport&) = delete;
	LoopbackTransport(LoopbackTransport&&) = delete;
	LoopbackTransport& operator=(LoopbackTransport&&) = delete;

	/** @brief Detaches the endpoint from the wire; idempotent. */
	void close() noexcept {
		if (open && network != nullptr) {
			network->destroyEndpoint(local);
		}
		open = false;
		local = Endpoint {};
	}

	/** @brief Returns whether the endpoint is still attached. */
	[[nodiscard]] bool isOpen() const override { return open; }

	/** @brief Loopback endpoints have no name-resolution concept. */
	[[nodiscard]] std::optional<Endpoint> resolve(
		std::string_view, std::string_view) const override {
		return std::nullopt;
	}

	/** @brief Returns the endpoint handle while attached. */
	[[nodiscard]] std::optional<Endpoint> localEndpoint() const override {
		if (!open) return std::nullopt;
		return local;
	}

	/**
	 * @brief Queues a datagram on the virtual wire.
	 * @param destination Target endpoint.
	 * @param datagram Bytes to send.
	 * @return Status from the network.
	 */
	NetError sendTo(const Endpoint& destination,
		std::span<const std::byte> datagram) override {
		if (!open || network == nullptr) return NetError::Closed;
		return network->enqueue(local, destination, datagram);
	}

	/**
	 * @brief Pops one queued datagram.
	 * @param buffer Destination buffer.
	 * @param status Optional status out-parameter.
	 * @return Datagram on success, empty optional when the queue is empty or an
	 *         error occurred.
	 */
	std::optional<ReceivedDatagram> receiveFrom(
		std::span<std::byte> buffer, NetError* status = nullptr) override {
		if (status != nullptr) *status = NetError::None;
		if (!open || network == nullptr) {
			if (status != nullptr) *status = NetError::Closed;
			return std::nullopt;
		}

		auto& inbox = network->inboxFor(local);
		if (inbox.empty()) {
			if (status != nullptr) *status = NetError::WouldBlock;
			return std::nullopt;
		}

		LoopbackNetwork::Delivery delivery = std::move(inbox.front());
		inbox.pop_front();
		if (delivery.payload.size() > buffer.size()) {
			if (status != nullptr) *status = NetError::MessageTooLarge;
			return std::nullopt;
		}

		if (!delivery.payload.empty()) {
			std::memcpy(buffer.data(), delivery.payload.data(), delivery.payload.size());
		}
		return ReceivedDatagram { delivery.source, delivery.payload.size() };
	}

	/** @brief Returns the configured maximum datagram size. */
	[[nodiscard]] std::size_t maxDatagramSize() const override {
		return network != nullptr ? network->config().maxDatagramSize : 0;
	}

	/** @brief Returns the wire this transport is attached to. */
	[[nodiscard]] LoopbackNetwork* wire() const noexcept { return network; }

private:
	LoopbackNetwork* network = nullptr; ///< Non-owning wire.
	Endpoint local;                     ///< This endpoint's handle.
	bool open = false;                  ///< Attachment state.
};

} // namespace Net
} // namespace Elv

#endif // ELVLOOPBACKTRANSPORT_HPP

