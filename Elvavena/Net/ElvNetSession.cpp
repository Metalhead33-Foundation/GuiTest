#include "ElvNetSession.hpp"
#include <Elvavena/Net/ElvNetFragment.hpp>
#include <Elvavena/Net/ElvNetProtocol.hpp>
#include <Elvavena/Net/ElvNetSequence.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <span>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Elv {
namespace Net {
namespace {

using namespace std::chrono_literals;

/// @brief Smallest MTU that can carry a non-empty fragment after both headers.
constexpr std::size_t MinSessionMtu = PacketHeaderBytes + FragmentHeaderBytes + 16;

/// @brief Largest datagram the session accepts, independent of the transport.
constexpr std::size_t MaxSessionMtu = 1500;

/// @brief Upper bound on a single application payload.
constexpr std::size_t MaxSessionPayload = 1024 * 1024;

/// @brief Handshake rejection reasons.
enum class RejectReason : std::uint8_t {
	VersionMismatch = 1,
	SchemaMismatch = 2,
	ServerFull = 3,
	NotAccepting = 4,
};

/**
 * @brief Returns whether a channel policy requires acknowledgements.
 * @param reliability Channel policy.
 * @return True for reliable unordered and ordered channels.
 */
constexpr bool isReliable(ChannelReliability reliability) noexcept {
	return reliability == ChannelReliability::ReliableUnordered
		|| reliability == ChannelReliability::ReliableOrdered;
}

/** @brief A reliable datagram retained until acknowledged. */
struct PendingPacket {
	NetSequence sequence = 0;             ///< Per-channel sequence.
	std::vector<std::byte> packet;        ///< Fully encoded datagram.
	std::size_t payloadSize = 0;          ///< Application bytes, for pacing.
	std::uint64_t userTag = 0;            ///< Tag reported on ack/loss.
	TimePoint lastSent {};                ///< Last transmission time.
	unsigned retransmits = 0;             ///< Retransmission count.
};

/** @brief Per-peer, per-channel protocol state. */
struct ChannelState {
	ChannelReliability reliability = ChannelReliability::Unreliable; ///< Delivery policy.
	NetSequence nextOutgoing = 0;         ///< Next sequence assigned to a send.
	AckBitfield incoming;                 ///< Window of received sequences.
	NetSequence nextExpected = 0;         ///< Next ordered sequence to deliver.
	bool hasSequenced = false;            ///< Whether an unreliable-sequenced packet arrived.
	NetSequence latestSequenced = 0;      ///< Newest unreliable-sequenced packet.
	std::uint16_t nextFragmentId = 0;     ///< Next message identifier.
	std::vector<PendingPacket> pending;   ///< Unacknowledged reliable datagrams.
	std::map<NetSequence, std::vector<std::byte>> ordered; ///< Out-of-order reliable payloads.
};

/** @brief One in-progress fragmented message. */
struct FragmentState {
	FragmentReassembler reassembler; ///< Slice storage.
	TimePoint deadline {};           ///< Reassembly timeout.
};

/** @brief Complete state for one peer. */
struct PeerState {
	PeerId id = InvalidPeer;             ///< Assigned identity.
	Endpoint endpoint;                   ///< Peer address.
	std::uint32_t token = 0;             ///< Connection token.
	SessionState state = SessionState::Disconnected; ///< Lifecycle state.
	TimePoint lastReceived {};           ///< Last valid datagram.
	TimePoint lastHeartbeat {};          ///< Last datagram sent (data or ping).
	std::uint64_t clientNonce = 0;       ///< Handshake nonce, for repeat requests.
	std::array<ChannelState, MaxNetChannels> channels; ///< Per-channel state.
};

} // namespace

struct Session::Impl {
	DatagramTransport& transport;          ///< Non-owning transport.
	SessionConfig config;                  ///< Sanitized configuration.
	SessionEvents* events = nullptr;       ///< Optional event sink.
	NullCongestion defaultCongestion;      ///< Fallback pacing policy.
	CongestionController* congestion = nullptr; ///< Active pacing policy.
	std::unordered_map<PeerId, PeerState> peers; ///< Peer state by identity.
	std::unordered_map<Endpoint, PeerId, EndpointHash> endpointToPeer; ///< Address lookup.
	std::unordered_map<std::uint32_t, PeerId> tokenToPeer; ///< Token lookup.
	std::map<std::tuple<PeerId, ChannelId, std::uint16_t>, FragmentState> fragments; ///< Active reassemblies.
	std::vector<std::byte> receiveBuffer;  ///< Reused polling buffer.
	PeerId localId = 1;                    ///< This endpoint's peer id.
	PeerId nextPeerId = 2;                 ///< Next server-assigned id.
	std::uint64_t randomState = 0x9E3779B97F4A7C15ull; ///< Handshake RNG.
	std::uint64_t nonceCounter = 1;        ///< Nonce source.
	NetError lastError = NetError::None;   ///< Most recent error.
	bool clientMode = false;               ///< Whether @ref connect was called.

	Impl(DatagramTransport& transport, SessionConfig config)
		: transport(transport),
		  config(sanitize(config)),
		  events(this->config.events),
		  defaultCongestion(this->config.retransmissionTimeout),
		  congestion(this->config.congestion != nullptr
			? this->config.congestion : &defaultCongestion),
		  randomState(this->config.handshakeSeed != 0
			? this->config.handshakeSeed : 0x9E3779B97F4A7C15ull) {
		receiveBuffer.resize(this->config.mtu);
	}

	/**
	 * @brief Clamps configuration values into a usable range.
	 * @param value Caller configuration.
	 * @return Sanitized copy.
	 */
	static SessionConfig sanitize(SessionConfig value) {
		value.mtu = std::clamp<std::size_t>(value.mtu, MinSessionMtu, MaxSessionMtu);
		value.maxPayload = std::clamp<std::size_t>(value.maxPayload, 1, MaxSessionPayload);
		value.channelCount = static_cast<std::uint8_t>(
			std::clamp<std::size_t>(value.channelCount, 1, MaxNetChannels));
		if (value.heartbeatInterval <= NetDuration::zero()) value.heartbeatInterval = 500ms;
		if (value.peerTimeout <= value.heartbeatInterval) value.peerTimeout = value.heartbeatInterval * 5;
		if (value.fragmentTimeout <= NetDuration::zero()) value.fragmentTimeout = 2000ms;
		if (value.retransmissionTimeout <= NetDuration::zero()) value.retransmissionTimeout = 200ms;
		if (value.maxPeers == 0) value.maxPeers = 1;
		return value;
	}

	/** @brief Advances the xorshift64 handshake generator. */
	std::uint64_t nextRandom() {
		randomState ^= randomState << 13;
		randomState ^= randomState >> 7;
		randomState ^= randomState << 17;
		return randomState;
	}

	/** @brief Creates a token that is not currently in use. */
	std::uint32_t makeToken() {
		for (;;) {
			const std::uint32_t token = static_cast<std::uint32_t>(nextRandom());
			if (token != 0 && tokenToPeer.find(token) == tokenToPeer.end()) return token;
		}
	}

	/** @brief Creates a nonce for a handshake message. */
	std::uint64_t makeNonce() { return nonceCounter++ ^ nextRandom(); }

	/**
	 * @brief Builds a peer state with the configured channel policies.
	 * @param id Peer identity.
	 * @param endpoint Peer address.
	 * @param token Connection token.
	 * @param state Initial lifecycle state.
	 * @param now Current time.
	 * @return Fully initialized peer state.
	 */
	PeerState makePeer(PeerId id, const Endpoint& endpoint, std::uint32_t token,
		SessionState state, TimePoint now) const {
		PeerState peer;
		peer.id = id;
		peer.endpoint = endpoint;
		peer.token = token;
		peer.state = state;
		peer.lastReceived = now;
		peer.lastHeartbeat = now;
		for (std::size_t index = 0; index < config.channelCount; ++index) {
			peer.channels[index].reliability = config.channels[index];
		}
		return peer;
	}


	/**
	 * @brief Builds a data-bearing header from the current channel window.
	 * @param peer Destination peer.
	 * @param channel Channel state.
	 * @param channelId Channel index.
	 * @param type Packet type.
	 * @param sequence Sender sequence.
	 * @return Filled header.
	 */
	static PacketHeader makeHeader(const PeerState& peer, const ChannelState& channel,
		ChannelId channelId, PacketType type, NetSequence sequence) {
		PacketHeader header;
		header.type = type;
		header.token = peer.token;
		header.channel = channelId;
		header.sequence = sequence;
		header.ack = channel.incoming.latest();
		header.ackBits = channel.incoming.bits();
		return header;
	}

	/**
	 * @brief Encodes a header, optional fragment metadata and payload.
	 * @param header Common header.
	 * @param fragment Optional fragment fields.
	 * @param payload Payload bytes.
	 * @return Encoded datagram, or an empty vector on failure.
	 */
	std::vector<std::byte> encodePacket(const PacketHeader& header,
		const FragmentHeader* fragment, std::span<const std::byte> payload) const {
		std::array<std::byte, MaxSessionMtu> buffer {};
		Io::BitWriter writer(buffer);
		if (!encodePacketHeader(writer, header)) return {};
		if (fragment != nullptr && !encodeFragmentHeader(writer, *fragment)) return {};
		if (!writer.writeBytes(payload)) return {};
		return std::vector<std::byte>(buffer.begin(), buffer.begin() + writer.byteSize());
	}

	/**
	 * @brief Sends an encoded datagram and retains it when reliable.
	 * @param peer Destination peer.
	 * @param channel Channel that owns the sequence space.
	 * @param packet Encoded datagram; moved into the pending queue when retained.
	 * @param payloadSize Application bytes carried by the datagram.
	 * @param reliable Whether the datagram must be acknowledged.
	 * @param sequence Sender sequence.
	 * @param userTag Tag reported on ack/loss.
	 * @param now Current time.
	 * @return True when the transport accepted the datagram.
	 */
	bool sendPacket(PeerState& peer, ChannelState& channel, std::vector<std::byte> packet,
		std::size_t payloadSize, bool reliable, NetSequence sequence,
		std::uint64_t userTag, TimePoint now) {
		if (packet.empty()) {
			lastError = NetError::ProtocolError;
			return false;
		}
		if (!congestion->canSend(peer.id, packet.size(), now)) {
			lastError = NetError::ResourceExhausted;
			return false;
		}
		const NetError error = transport.sendTo(peer.endpoint, packet);
		if (error != NetError::None) {
			lastError = error;
			return false;
		}
		congestion->onSent(peer.id, packet.size(), now);
		peer.lastHeartbeat = now;
		if (reliable) {
			channel.pending.push_back(PendingPacket {
				sequence, std::move(packet), payloadSize, userTag, now, 0
			});
		}
		lastError = NetError::None;
		return true;
	}


	/**
	 * @brief Sends a handshake rejection.
	 * @param endpoint Refused endpoint.
	 * @param reason Rejection reason.
	 * @param now Current time (unused; kept for symmetry).
	 */
	void sendReject(const Endpoint& endpoint, RejectReason reason, TimePoint now) {
		(void)now;
		PacketHeader header;
		header.type = PacketType::ConnectReject;
		header.token = 0;
		const std::array<std::byte, 1> payload { std::byte { static_cast<std::uint8_t>(reason) } };
		const std::vector<std::byte> packet = encodePacket(header, nullptr, payload);
		if (packet.empty()) return;
		const NetError error = transport.sendTo(endpoint, packet);
		if (error != NetError::None) lastError = error;
	}

	/**
	 * @brief Sends a connect accept to a freshly added peer.
	 * @param peer Peer state.
	 * @param now Current time.
	 */
	void sendConnectAccept(PeerState& peer, TimePoint now) {
		HandshakeAccept accept;
		accept.assignedPeerId = peer.id;
		accept.clientNonce = peer.clientNonce;
		accept.serverNonce = makeNonce();
		std::array<std::byte, 32> payload {};
		Io::BitWriter writer(payload);
		if (!encodeHandshakeAccept(writer, accept)) return;
		PacketHeader header;
		header.type = PacketType::ConnectAccept;
		header.token = peer.token;
		std::vector<std::byte> packet = encodePacket(header, nullptr,
			std::span<const std::byte>(payload.data(), writer.byteSize()));
		if (packet.empty()) return;
		sendPacket(peer, peer.channels[0], std::move(packet), 0, false, 0, 0, now);
	}

	/**
	 * @brief Sends a connect request for a placeholder peer.
	 * @param peer Placeholder peer.
	 * @param now Current time.
	 */
	void sendConnectRequest(PeerState& peer, TimePoint now) {
		HandshakeRequest request;
		request.version = config.protocolVersion;
		request.schemaHash = config.schemaHash;
		request.clientNonce = peer.clientNonce;
		std::array<std::byte, 32> payload {};
		Io::BitWriter writer(payload);
		if (!encodeHandshakeRequest(writer, request)) return;
		PacketHeader header;
		header.type = PacketType::ConnectRequest;
		header.token = 0;
		std::vector<std::byte> packet = encodePacket(header, nullptr,
			std::span<const std::byte>(payload.data(), writer.byteSize()));
		if (packet.empty()) return;
		sendPacket(peer, peer.channels[0], std::move(packet), 0, false, 0, 0, now);
	}

	/**
	 * @brief Sends the current acknowledgement window for a channel.
	 * @param peer Peer.
	 * @param channel Channel state.
	 * @param channelId Channel index.
	 * @param now Current time.
	 */
	void sendAck(PeerState& peer, ChannelState& channel, ChannelId channelId, TimePoint now) {
		PacketHeader header;
		header.type = PacketType::Ack;
		header.token = peer.token;
		header.channel = channelId;
		header.ack = channel.incoming.latest();
		header.ackBits = channel.incoming.bits();
		std::vector<std::byte> packet = encodePacket(header, nullptr, {});
		if (packet.empty()) return;
		sendPacket(peer, channel, std::move(packet), 0, false, 0, 0, now);
	}

	/**
	 * @brief Sends an empty heartbeat packet.
	 * @param peer Peer.
	 * @param type Ping or Pong.
	 * @param now Current time.
	 */
	void sendHeartbeat(PeerState& peer, PacketType type, TimePoint now) {
		PacketHeader header;
		header.type = type;
		header.token = peer.token;
		std::vector<std::byte> packet = encodePacket(header, nullptr, {});
		if (packet.empty()) return;
		sendPacket(peer, peer.channels[0], std::move(packet), 0, false, 0, 0, now);
	}

	/**
	 * @brief Delivers a complete payload to the event sink.
	 * @param peer Sender.
	 * @param channel Channel index.
	 * @param payload Complete payload.
	 */
	void deliver(PeerId peer, ChannelId channel, std::span<const std::byte> payload) {
		if (events != nullptr) events->onPacketReceived(peer, channel, payload);
	}


	/**
	 * @brief Applies an acknowledgement window to the sender's pending queue.
	 * @param peer Acknowledging peer.
	 * @param channel Channel state.
	 * @param header Received ack fields.
	 * @param now Current time.
	 */
	void handleAck(PeerState& peer, ChannelState& channel, const PacketHeader& header, TimePoint now) {
		AckBitfield remote;
		remote.assign(header.ack, header.ackBits);
		if (remote.empty()) return;

		auto& pending = channel.pending;
		for (auto iterator = pending.begin(); iterator != pending.end();) {
			if (!remote.isReceived(iterator->sequence)) {
				++iterator;
				continue;
			}
			const NetDuration rtt = now > iterator->lastSent
				? now - iterator->lastSent : NetDuration::zero();
			if (events != nullptr) {
				events->onPacketAcknowledged(peer.id, iterator->userTag);
			}
			congestion->onAcked(peer.id, iterator->payloadSize, rtt);
			iterator = pending.erase(iterator);
		}
	}

	/**
	 * @brief Delivers consecutive reliable-ordered payloads already buffered.
	 * @param peer Sender.
	 * @param channel Channel state.
	 * @param channelId Channel index.
	 */
	void drainOrdered(PeerState& peer, ChannelState& channel, ChannelId channelId) {
		for (;;) {
			auto iterator = channel.ordered.find(channel.nextExpected);
			if (iterator == channel.ordered.end()) break;
			deliver(peer.id, channelId, iterator->second);
			channel.ordered.erase(iterator);
			++channel.nextExpected;
		}
	}

	/**
	 * @brief Applies a whole application payload according to channel policy.
	 * @param peer Sender.
	 * @param channel Channel state.
	 * @param channelId Channel index.
	 * @param header Packet header.
	 * @param payload Application bytes.
	 * @param now Current time.
	 */
	void handleData(PeerState& peer, ChannelState& channel, ChannelId channelId,
		const PacketHeader& header, std::span<const std::byte> payload, TimePoint now) {
		const bool reliable = isReliable(channel.reliability);
		bool fresh = true;
		if (reliable) {
			fresh = channel.incoming.markReceived(header.sequence);
			sendAck(peer, channel, channelId, now);
		}
		if (!fresh) return;

		switch (channel.reliability) {
		case ChannelReliability::Unreliable:
			deliver(peer.id, channelId, payload);
			break;
		case ChannelReliability::UnreliableSequenced:
			if (!channel.hasSequenced
				|| sequenceGreaterThan(header.sequence, channel.latestSequenced)) {
				channel.hasSequenced = true;
				channel.latestSequenced = header.sequence;
				deliver(peer.id, channelId, payload);
			}
			break;
		case ChannelReliability::ReliableUnordered:
			deliver(peer.id, channelId, payload);
			break;
		case ChannelReliability::ReliableOrdered:
			if (header.sequence == channel.nextExpected) {
				deliver(peer.id, channelId, payload);
				++channel.nextExpected;
				drainOrdered(peer, channel, channelId);
			} else if (sequenceGreaterThan(header.sequence, channel.nextExpected)) {
				channel.ordered.emplace(header.sequence,
					std::vector<std::byte>(payload.begin(), payload.end()));
			}
			break;
		}
	}


	/**
	 * @brief Processes one fragment and delivers the message when complete.
	 * @param peer Sender.
	 * @param channel Channel state.
	 * @param channelId Channel index.
	 * @param header Common header.
	 * @param body Bytes after the common header.
	 * @param now Current time.
	 */
	void handleFragment(PeerState& peer, ChannelState& channel, ChannelId channelId,
		const PacketHeader& header, std::span<const std::byte> body, TimePoint now) {
		if (body.size() < FragmentHeaderBytes) {
			lastError = NetError::ProtocolError;
			return;
		}
		Io::BitReader reader(body);
		FragmentHeader fragment;
		if (!decodeFragmentHeader(reader, fragment)) {
			lastError = NetError::ProtocolError;
			return;
		}
		const std::span<const std::byte> payload = body.subspan(reader.byteSize());

		const bool reliable = isReliable(channel.reliability);
		bool fresh = true;
		if (reliable) {
			fresh = channel.incoming.markReceived(header.sequence);
			sendAck(peer, channel, channelId, now);
		}
		if (!fresh) return;

		const auto key = std::make_tuple(peer.id, channelId, fragment.fragmentId);
		auto iterator = fragments.find(key);
		if (iterator == fragments.end()) {
			FragmentState state;
			if (!state.reassembler.begin(fragment.count, fragment.totalSize)) {
				lastError = NetError::ProtocolError;
				return;
			}
			state.deadline = now + config.fragmentTimeout;
			iterator = fragments.emplace(key, std::move(state)).first;
		} else if (iterator->second.reassembler.count() != fragment.count
			|| iterator->second.reassembler.totalSize() != fragment.totalSize) {
			fragments.erase(iterator);
			lastError = NetError::ProtocolError;
			return;
		}

		const FragmentStatus status = iterator->second.reassembler.add(fragment.index, payload);
		if (status == FragmentStatus::Invalid) {
			fragments.erase(iterator);
			lastError = NetError::ProtocolError;
			return;
		}
		if (status != FragmentStatus::Complete) return;

		std::vector<std::byte> assembled;
		const bool valid = iterator->second.reassembler.assemble(assembled);
		fragments.erase(iterator);
		if (!valid) {
			lastError = NetError::ProtocolError;
			return;
		}
		deliver(peer.id, channelId, assembled);
	}


	/**
	 * @brief Handles a server-side connect request.
	 * @param reader Reader positioned after the common header.
	 * @param source Requesting endpoint.
	 * @param now Current time.
	 */
	void handleConnectRequest(Io::BitReader& reader, const Endpoint& source, TimePoint now) {
		if (clientMode) return;
		HandshakeRequest request;
		if (!decodeHandshakeRequest(reader, request)) {
			lastError = NetError::ProtocolError;
			return;
		}
		if (request.version != config.protocolVersion) {
			sendReject(source, RejectReason::VersionMismatch, now);
			return;
		}
		if (config.schemaHash != 0 && request.schemaHash != 0
			&& request.schemaHash != config.schemaHash) {
			sendReject(source, RejectReason::SchemaMismatch, now);
			return;
		}
		if (!config.allowNewPeers) {
			sendReject(source, RejectReason::NotAccepting, now);
			return;
		}

		auto existing = endpointToPeer.find(source);
		if (existing != endpointToPeer.end()) {
			auto peer = peers.find(existing->second);
			if (peer != peers.end()) sendConnectAccept(peer->second, now);
			return;
		}
		if (peers.size() >= config.maxPeers) {
			sendReject(source, RejectReason::ServerFull, now);
			return;
		}

		const PeerId id = nextPeerId++;
		const std::uint32_t token = makeToken();
		PeerState peer = makePeer(id, source, token, SessionState::Connected, now);
		peer.clientNonce = request.clientNonce;
		peers.emplace(id, std::move(peer));
		endpointToPeer.emplace(source, id);
		tokenToPeer.emplace(token, id);
		sendConnectAccept(peers.at(id), now);
		if (events != nullptr) events->onPeerConnected(id, source);
	}

	/**
	 * @brief Handles a client-side connect accept.
	 * @param reader Reader positioned after the common header.
	 * @param header Common header carrying the token.
	 * @param source Server endpoint.
	 * @param now Current time.
	 */
	void handleConnectAccept(Io::BitReader& reader, const PacketHeader& header,
		const Endpoint& source, TimePoint now) {
		auto mapping = endpointToPeer.find(source);
		if (mapping == endpointToPeer.end()) return;
		auto iterator = peers.find(mapping->second);
		if (iterator == peers.end() || iterator->second.state != SessionState::Connecting) return;

		HandshakeAccept accept;
		if (!decodeHandshakeAccept(reader, accept)) {
			lastError = NetError::ProtocolError;
			return;
		}
		if (accept.clientNonce != iterator->second.clientNonce
			|| accept.assignedPeerId == InvalidPeer || header.token == 0) {
			lastError = NetError::ProtocolError;
			return;
		}

		PeerState& peer = iterator->second;
		peer.token = header.token;
		peer.state = SessionState::Connected;
		peer.lastReceived = now;
		localId = accept.assignedPeerId;
		tokenToPeer.emplace(peer.token, peer.id);
		if (events != nullptr) events->onPeerConnected(peer.id, source);
	}

	/**
	 * @brief Handles a server rejection.
	 * @param reader Reader positioned after the common header.
	 * @param source Server endpoint.
	 * @param now Current time.
	 */
	void handleConnectReject(Io::BitReader& reader, const Endpoint& source, TimePoint now) {
		(void)now;
		std::uint64_t reason = 0;
		NetError error = NetError::Unauthorized;
		if (reader.readBits(reason, 8)) {
			if (reason == static_cast<std::uint64_t>(RejectReason::ServerFull)) {
				error = NetError::ResourceExhausted;
			} else if (reason == static_cast<std::uint64_t>(RejectReason::VersionMismatch)
				|| reason == static_cast<std::uint64_t>(RejectReason::SchemaMismatch)) {
				error = NetError::ProtocolError;
			}
		}
		auto mapping = endpointToPeer.find(source);
		if (mapping == endpointToPeer.end()) return;
		removePeer(mapping->second, error);
	}

	/**
	 * @brief Forgets a peer, its tokens and its in-progress fragments.
	 * @param peer Peer identity.
	 * @param reason Reason reported to the event sink.
	 */
	void removePeer(PeerId peer, NetError reason) {
		auto iterator = peers.find(peer);
		if (iterator == peers.end()) return;
		const Endpoint endpoint = iterator->second.endpoint;
		const std::uint32_t token = iterator->second.token;
		endpointToPeer.erase(endpoint);
		if (token != 0) tokenToPeer.erase(token);
		for (auto fragment = fragments.begin(); fragment != fragments.end();) {
			if (std::get<0>(fragment->first) == peer) {
				fragment = fragments.erase(fragment);
			} else {
				++fragment;
			}
		}
		peers.erase(iterator);
		if (events != nullptr) events->onPeerDisconnected(peer, reason);
	}

	/**
	 * @brief Finds a peer by token and verifies the source address.
	 * @param token Connection token.
	 * @param source Datagram source.
	 * @return Peer state, or null when unknown/mismatched.
	 */
	PeerState* findByToken(std::uint32_t token, const Endpoint& source) {
		const auto mapping = tokenToPeer.find(token);
		if (mapping == tokenToPeer.end()) return nullptr;
		const auto iterator = peers.find(mapping->second);
		if (iterator == peers.end()) return nullptr;
		if (iterator->second.endpoint != source) return nullptr;
		return &iterator->second;
	}


	/**
	 * @brief Encodes and sends a payload, splitting it when it exceeds the MTU.
	 * @param peer Destination peer.
	 * @param channel Channel state.
	 * @param channelId Channel index.
	 * @param payload Application bytes.
	 * @param userTag Tag reported on ack/loss.
	 * @param now Current time.
	 * @return Status of the send.
	 */
	NetError sendPayload(PeerState& peer, ChannelState& channel, ChannelId channelId,
		std::span<const std::byte> payload, std::uint64_t userTag, TimePoint now) {
		const bool reliable = isReliable(channel.reliability);
		const std::size_t maxSingle = config.mtu - PacketHeaderBytes;

		if (payload.size() <= maxSingle) {
			const NetSequence sequence = channel.nextOutgoing++;
			PacketHeader header = makeHeader(peer, channel, channelId, PacketType::Data, sequence);
			std::vector<std::byte> packet = encodePacket(header, nullptr, payload);
			if (!sendPacket(peer, channel, std::move(packet), payload.size(),
				reliable, sequence, userTag, now)) {
				return lastError;
			}
			return NetError::None;
		}

		const std::size_t maxFragment = config.mtu - PacketHeaderBytes - FragmentHeaderBytes;
		if (maxFragment == 0) return NetError::MessageTooLarge;
		const std::size_t count = (payload.size() + maxFragment - 1) / maxFragment;
		if (count == 0 || count > FragmentReassembler::DefaultMaxFragments || count > 0xFFFFu) {
			return NetError::MessageTooLarge;
		}

		const std::uint16_t fragmentId = channel.nextFragmentId++;
		for (std::size_t index = 0; index < count; ++index) {
			const std::size_t offset = index * maxFragment;
			const std::size_t length = std::min(maxFragment, payload.size() - offset);
			FragmentHeader fragment;
			fragment.fragmentId = fragmentId;
			fragment.index = static_cast<std::uint16_t>(index);
			fragment.count = static_cast<std::uint16_t>(count);
			fragment.totalSize = static_cast<std::uint32_t>(payload.size());
			const NetSequence sequence = channel.nextOutgoing++;
			PacketHeader header = makeHeader(peer, channel, channelId, PacketType::Fragment, sequence);
			std::vector<std::byte> packet = encodePacket(header, &fragment,
				payload.subspan(offset, length));
			if (!sendPacket(peer, channel, std::move(packet), length,
				reliable, sequence, userTag, now)) {
				return lastError;
			}
		}
		return NetError::None;
	}

	/**
	 * @brief Dispatches one decoded datagram.
	 * @param datagram Complete datagram bytes.
	 * @param source Sender address.
	 * @param now Current time.
	 */
	void handleDatagram(std::span<const std::byte> datagram, const Endpoint& source, TimePoint now) {
		Io::BitReader reader(datagram);
		PacketHeader header;
		if (!decodePacketHeader(reader, header)) {
			lastError = NetError::ProtocolError;
			return;
		}
		const std::span<const std::byte> body = datagram.subspan(reader.byteSize());

		switch (header.type) {
		case PacketType::ConnectRequest:
			handleConnectRequest(reader, source, now);
			return;
		case PacketType::ConnectAccept:
			handleConnectAccept(reader, header, source, now);
			return;
		case PacketType::ConnectReject:
			handleConnectReject(reader, source, now);
			return;
		default:
			break;
		}

		PeerState* peer = findByToken(header.token, source);
		if (peer == nullptr) return;
		peer->lastReceived = now;
		if (header.channel >= config.channelCount) {
			lastError = NetError::ProtocolError;
			return;
		}
		ChannelState& channel = peer->channels[header.channel];

		switch (header.type) {
		case PacketType::Data:
			handleData(*peer, channel, header.channel, header, body, now);
			break;
		case PacketType::Fragment:
			handleFragment(*peer, channel, header.channel, header, body, now);
			break;
		case PacketType::Ack:
			handleAck(*peer, channel, header, now);
			break;
		case PacketType::Ping:
			sendHeartbeat(*peer, PacketType::Pong, now);
			break;
		case PacketType::Pong:
			break;
		case PacketType::Disconnect:
			removePeer(peer->id, NetError::None);
			break;
		default:
			lastError = NetError::ProtocolError;
			break;
		}
	}


	/**
	 * @brief Retransmits expired packets and expires peers/fragments.
	 * @param now Current time.
	 */
	void flush(TimePoint now) {
		std::vector<PeerId> dead;
		for (auto& entry : peers) {
			PeerState& peer = entry.second;
			if (now - peer.lastReceived > config.peerTimeout) {
				dead.push_back(peer.id);
				continue;
			}

			for (std::size_t index = 0; index < config.channelCount; ++index) {
				ChannelState& channel = peer.channels[index];
				const NetDuration rto = congestion->retransmissionTimeout(peer.id);
				for (auto iterator = channel.pending.begin(); iterator != channel.pending.end();) {
					if (now - iterator->lastSent < rto) {
						++iterator;
						continue;
					}
					if (iterator->retransmits >= config.maxRetransmits) {
						if (events != nullptr) {
							events->onPacketLost(peer.id, iterator->userTag);
						}
						congestion->onLost(peer.id, iterator->payloadSize);
						iterator = channel.pending.erase(iterator);
						continue;
					}
					const NetError error = transport.sendTo(peer.endpoint, iterator->packet);
					if (error != NetError::None) {
						lastError = error;
						++iterator;
						continue;
					}
					congestion->onSent(peer.id, iterator->packet.size(), now);
					peer.lastHeartbeat = now;
					iterator->lastSent = now;
					++iterator->retransmits;
					++iterator;
				}
			}

			if (peer.state == SessionState::Connecting) {
				if (now - peer.lastHeartbeat >= config.retransmissionTimeout) {
					sendConnectRequest(peer, now);
				}
			} else if (now - peer.lastHeartbeat >= config.heartbeatInterval) {
				sendHeartbeat(peer, PacketType::Ping, now);
			}
		}

		for (const PeerId peer : dead) removePeer(peer, NetError::Timeout);

		for (auto iterator = fragments.begin(); iterator != fragments.end();) {
			if (now > iterator->second.deadline) {
				iterator = fragments.erase(iterator);
			} else {
				++iterator;
			}
		}
	}

	/**
	 * @brief Drains all available datagrams.
	 * @param now Current time.
	 */
	void poll(TimePoint now) {
		if (!transport.isOpen()) {
			lastError = NetError::Closed;
			return;
		}
		for (;;) {
			NetError status = NetError::None;
			const auto received = transport.receiveFrom(receiveBuffer, &status);
			if (!received.has_value()) {
				if (status != NetError::WouldBlock && status != NetError::None) {
					lastError = status;
				}
				break;
			}
			handleDatagram(std::span<const std::byte>(receiveBuffer.data(), received->size),
				received->source, now);
		}
	}
};


Session::Session(DatagramTransport& transport, SessionConfig config)
	: impl(std::make_unique<Impl>(transport, std::move(config))) {}

Session::~Session() = default;

NetError Session::connect(const Endpoint& server, TimePoint now) {
	if (!server.isValid()) return NetError::InvalidEndpoint;
	if (!impl->transport.isOpen()) return NetError::Closed;
	if (impl->clientMode) return NetError::Unsupported;
	if (!impl->peers.empty()) return NetError::Unsupported;

	impl->clientMode = true;
	impl->localId = InvalidPeer;
	PeerState peer = impl->makePeer(1, server, 0, SessionState::Connecting, now);
	peer.clientNonce = impl->makeNonce();
	impl->peers.emplace(1, std::move(peer));
	impl->endpointToPeer.emplace(server, 1);
	impl->sendConnectRequest(impl->peers.at(1), now);
	return impl->lastError;
}

NetError Session::disconnect(PeerId peer, TimePoint now) {
	const auto iterator = impl->peers.find(peer);
	if (iterator == impl->peers.end()) return NetError::NotConnected;

	PeerState& state = iterator->second;
	PacketHeader header;
	header.type = PacketType::Disconnect;
	header.token = state.token;
	std::vector<std::byte> packet = impl->encodePacket(header, nullptr, {});
	NetError error = NetError::None;
	if (!packet.empty()) {
		if (!impl->sendPacket(state, state.channels[0], std::move(packet),
			0, false, 0, 0, now)) {
			error = impl->lastError;
		}
	}
	impl->removePeer(peer, NetError::None);
	return error;
}

NetError Session::send(PeerId peer, std::span<const std::byte> payload,
	const SendOptions& options, TimePoint now) {
	const auto iterator = impl->peers.find(peer);
	if (iterator == impl->peers.end()
		|| iterator->second.state != SessionState::Connected) {
		return NetError::NotConnected;
	}
	if (options.channel >= impl->config.channelCount) return NetError::Unsupported;
	if (payload.size() > impl->config.maxPayload) return NetError::MessageTooLarge;

	PeerState& state = iterator->second;
	ChannelState& channel = state.channels[options.channel];
	const NetError error = impl->sendPayload(state, channel, options.channel,
		payload, options.userTag, now);
	if (options.flushImmediately) impl->flush(now);
	return error;
}

NetError Session::broadcast(std::span<const std::byte> payload,
	const SendOptions& options, TimePoint now) {
	NetError firstError = NetError::None;
	for (const auto& entry : impl->peers) {
		if (entry.second.state != SessionState::Connected) continue;
		const NetError error = send(entry.first, payload, options, now);
		if (firstError == NetError::None && error != NetError::None) firstError = error;
	}
	return firstError;
}

void Session::poll(TimePoint now) { impl->poll(now); }

void Session::flush(TimePoint now) { impl->flush(now); }

void Session::setEvents(SessionEvents* events) { impl->events = events; }

SessionState Session::state() const {
	bool connecting = false;
	for (const auto& entry : impl->peers) {
		if (entry.second.state == SessionState::Connected) return SessionState::Connected;
		if (entry.second.state == SessionState::Connecting) connecting = true;
	}
	return connecting ? SessionState::Connecting : SessionState::Disconnected;
}

PeerId Session::localPeer() const { return impl->localId; }

std::optional<PeerInfo> Session::peer(PeerId peer) const {
	const auto iterator = impl->peers.find(peer);
	if (iterator == impl->peers.end()) return std::nullopt;
	return PeerInfo { iterator->second.id, iterator->second.endpoint, iterator->second.state };
}

std::size_t Session::peerCount() const { return impl->peers.size(); }

NetError Session::lastError() const { return impl->lastError; }

const SessionConfig& Session::config() const { return impl->config; }

void Session::reset() {
	impl->peers.clear();
	impl->endpointToPeer.clear();
	impl->tokenToPeer.clear();
	impl->fragments.clear();
	impl->localId = 1;
	impl->nextPeerId = 2;
	impl->clientMode = false;
	impl->lastError = NetError::None;
	impl->congestion->reset();
}

} // namespace Net
} // namespace Elv

