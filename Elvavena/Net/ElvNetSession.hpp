#ifndef ELVNETSESSION_HPP
#define ELVNETSESSION_HPP
/**
 * @file ElvNetSession.hpp
 * @brief Poll-driven reliable datagram session.
 *
 * `Session` owns the protocol state for a set of peers: handshake, per-channel
 * reliability, acknowledgement windows, fragmentation, heartbeat and timeout.
 * It deliberately owns neither a thread nor a tick loop; the caller decides
 * when to call `poll()` and `flush()`.
 *
 * The transport is supplied by the caller and must outlive the session. A
 * session is single-threaded: all methods must be called from one thread.
 */
#include <Elvavena/Net/ElvDatagramTransport.hpp>
#include <Elvavena/Net/ElvNetCongestion.hpp>
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <Elvavena/Util/ElvUtilGlobals.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>

namespace Elv {
namespace Net {

/**
 * @class SessionEvents
 * @brief Optional notifications emitted by a @ref Session.
 *
 * Every callback has a no-op default so games only override what they need.
 * Pointers and spans passed to the callbacks are valid only for the duration of
 * the call.
 */
class SessionEvents {
public:
	/** @brief Virtual destructor. */
	virtual ~SessionEvents() = default;

	/**
	 * @brief Called when a peer completes the handshake.
	 * @param peer Connected peer.
	 * @param endpoint Peer address.
	 */
	virtual void onPeerConnected(PeerId peer, const Endpoint& endpoint) {
		(void)peer;
		(void)endpoint;
	}

	/**
	 * @brief Called when a peer disconnects or times out.
	 * @param peer Disconnected peer.
	 * @param reason Reason code.
	 */
	virtual void onPeerDisconnected(PeerId peer, NetError reason) {
		(void)peer;
		(void)reason;
	}

	/**
	 * @brief Called for each complete application payload.
	 * @param peer Sender.
	 * @param channel Channel that carried the payload.
	 * @param payload Complete payload; valid only during the call.
	 */
	virtual void onPacketReceived(PeerId peer, ChannelId channel,
		std::span<const std::byte> payload) {
		(void)peer;
		(void)channel;
		(void)payload;
	}

	/**
	 * @brief Called when a reliable payload is acknowledged.
	 * @param peer Peer that acknowledged.
	 * @param userTag Tag supplied to @ref Session::send.
	 */
	virtual void onPacketAcknowledged(PeerId peer, std::uint64_t userTag) {
		(void)peer;
		(void)userTag;
	}

	/**
	 * @brief Called when a reliable payload exhausts its retransmissions.
	 * @param peer Peer that did not acknowledge.
	 * @param userTag Tag supplied to @ref Session::send.
	 */
	virtual void onPacketLost(PeerId peer, std::uint64_t userTag) {
		(void)peer;
		(void)userTag;
	}
};

/**
 * @struct SessionConfig
 * @brief Session limits, timers and channel policies.
 */
struct SessionConfig {
	std::uint16_t protocolVersion = NetProtocolVersion; ///< Handshake version.
	std::uint64_t schemaHash = 0;                       ///< Optional schema fingerprint.
	std::size_t mtu = 1200;                             ///< Maximum datagram size.
	std::size_t maxPayload = 64 * 1024;                 ///< Maximum application payload.
	std::uint8_t channelCount = 4;                      ///< Active channels.
	std::array<ChannelReliability, MaxNetChannels> channels {
		ChannelReliability::ReliableOrdered,
		ChannelReliability::Unreliable,
		ChannelReliability::UnreliableSequenced,
		ChannelReliability::ReliableUnordered
	}; ///< Per-channel policy.
	NetDuration heartbeatInterval = std::chrono::milliseconds(500); ///< Idle ping interval.
	NetDuration peerTimeout = std::chrono::milliseconds(5000);      ///< Silence before disconnect.
	NetDuration fragmentTimeout = std::chrono::milliseconds(2000);  ///< Reassembly deadline.
	NetDuration retransmissionTimeout = std::chrono::milliseconds(200); ///< Default RTO.
	std::uint32_t maxPeers = 64;                        ///< Server peer limit.
	std::uint8_t maxRetransmits = 10;                   ///< Reliable retry limit.
	bool allowNewPeers = true;                          ///< Accept handshake requests.
	std::uint64_t handshakeSeed = 0;                    ///< RNG seed; zero uses a fixed default.
	CongestionController* congestion = nullptr;         ///< Optional pacing policy.
	SessionEvents* events = nullptr;                    ///< Optional event sink.
};

/**
 * @struct PeerInfo
 * @brief Read-only snapshot of one peer.
 */
struct PeerInfo {
	PeerId id = InvalidPeer;          ///< Assigned peer identity.
	Endpoint endpoint;                ///< Peer address.
	SessionState state = SessionState::Disconnected; ///< Peer lifecycle state.
};

/**
 * @class Session
 * @brief Reliable UDP-style session over a @ref DatagramTransport.
 *
 * The class is non-copyable and non-movable. It owns no transport and no
 * thread; the transport must outlive it, and the caller is responsible for
 * calling @ref poll and @ref flush from one thread.
 */
class MH_UTIL_API Session {
public:
	/**
	 * @brief Creates a session over a transport.
	 * @param transport Non-owning transport; must outlive the session.
	 * @param config Session limits and policies.
	 */
	explicit Session(DatagramTransport& transport, SessionConfig config = {});

	/** @brief Destroys the session and detaches from the event sink. */
	~Session();

	Session(const Session&) = delete;
	Session& operator=(const Session&) = delete;
	Session(Session&&) = delete;
	Session& operator=(Session&&) = delete;

	/**
	 * @brief Starts a client handshake with a server endpoint.
	 * @param server Server address.
	 * @param now Current session time.
	 * @return @ref NetError::None when the request was sent.
	 */
	NetError connect(const Endpoint& server, TimePoint now);

	/**
	 * @brief Sends a disconnect notice and forgets a peer.
	 * @param peer Peer to drop.
	 * @param now Current session time.
	 * @return @ref NetError::None when the peer existed.
	 */
	NetError disconnect(PeerId peer, TimePoint now);

	/**
	 * @brief Sends one application payload.
	 * @param peer Destination peer.
	 * @param payload Bytes to send.
	 * @param options Channel, reliability and acknowledgement tag.
	 * @param now Current session time.
	 * @return Status of the send.
	 */
	NetError send(PeerId peer, std::span<const std::byte> payload,
		const SendOptions& options, TimePoint now);

	/**
	 * @brief Sends the same payload to every connected peer.
	 * @param payload Bytes to send.
	 * @param options Channel, reliability and acknowledgement tag.
	 * @param now Current session time.
	 * @return First error encountered, or @ref NetError::None.
	 */
	NetError broadcast(std::span<const std::byte> payload,
		const SendOptions& options, TimePoint now);

	/**
	 * @brief Drains incoming datagrams and advances protocol state.
	 * @param now Current session time.
	 */
	void poll(TimePoint now);

	/**
	 * @brief Retransmits expired packets, sends heartbeats and expires peers.
	 * @param now Current session time.
	 */
	void flush(TimePoint now);

	/**
	 * @brief Replaces the optional event sink.
	 * @param events Event sink; may be null.
	 * @note The sink must outlive the session or be cleared first.
	 */
	void setEvents(SessionEvents* events);

	/** @brief Returns the aggregate session state. */
	[[nodiscard]] SessionState state() const;

	/** @brief Returns this endpoint's peer identity, or @ref InvalidPeer. */
	[[nodiscard]] PeerId localPeer() const;

	/**
	 * @brief Looks up a peer by identity.
	 * @param peer Peer identity.
	 * @return Snapshot when the peer exists.
	 */
	[[nodiscard]] std::optional<PeerInfo> peer(PeerId peer) const;

	/** @brief Returns the number of known peers. */
	[[nodiscard]] std::size_t peerCount() const;

	/** @brief Returns the most recent non-blocking error. */
	[[nodiscard]] NetError lastError() const;

	/** @brief Returns the active configuration. */
	[[nodiscard]] const SessionConfig& config() const;

	/** @brief Drops all peers, fragments and queued packets. */
	void reset();

private:
	struct Impl;                 ///< Hidden protocol state.
	std::unique_ptr<Impl> impl;  ///< Pimpl to keep the header compact.
};

} // namespace Net
} // namespace Elv

#endif // ELVNETSESSION_HPP

