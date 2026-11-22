#ifndef ELVNETPROTOCOL_HPP
#define ELVNETPROTOCOL_HPP
/**
 * @file ElvNetProtocol.hpp
 * @brief Wire-level packet header, handshake and fragment layouts.
 *
 * Every datagram starts with the same header, encoded with the MSB-first bit
 * writer. The header carries a magic value, protocol version, packet type, the
 * connection token, a per-channel sequence and a 64-entry acknowledgement
 * window. Type-specific fields follow the header.
 *
 * These helpers only encode and decode fields. They do not validate peer state
 * or allocate buffers; that belongs to the session.
 */
#include <Elvavena/Io/ElvBitStream.hpp>
#include <Elvavena/Net/ElvNetSequence.hpp>
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <cstddef>
#include <cstdint>

namespace Elv {
namespace Net {

/// @brief Magic value at the start of every packet ("MH").
inline constexpr std::uint16_t PacketMagic = 0x4D48;

/// @brief Encoded size of @ref PacketHeader in bytes.
inline constexpr std::size_t PacketHeaderBytes = 21;

/// @brief Encoded size of @ref FragmentHeader in bytes.
inline constexpr std::size_t FragmentHeaderBytes = 10;

/**
 * @enum PacketType
 * @brief Datagram kinds understood by the session.
 */
enum class PacketType : std::uint8_t {
	ConnectRequest = 1,  ///< Client asks to join.
	ConnectAccept = 2,   ///< Server accepts and assigns an identity.
	ConnectReject = 3,   ///< Server refuses the connection.
	Data = 4,            ///< Whole application payload.
	Fragment = 5,        ///< Slice of a larger application payload.
	Ack = 6,             ///< Acknowledgement window update.
	Ping = 7,            ///< Heartbeat request.
	Pong = 8,            ///< Heartbeat response.
	Disconnect = 9,      ///< Graceful teardown notice.
};

/**
 * @struct PacketHeader
 * @brief Common header present on every datagram.
 */
struct PacketHeader {
	PacketType type = PacketType::Data; ///< Packet kind.
	std::uint8_t channel = 0;           ///< Reliability channel.
	NetSequence sequence = 0;           ///< Sender's per-channel sequence.
	NetSequence ack = 0;                ///< Newest sequence received from the peer.
	std::uint64_t ackBits = 0;          ///< Previous 64 received sequences.
	std::uint32_t token = 0;            ///< Connection token; zero during handshake.
};

/**
 * @struct HandshakeRequest
 * @brief Payload of @ref PacketType::ConnectRequest.
 */
struct HandshakeRequest {
	std::uint16_t version = NetProtocolVersion; ///< Sender protocol version.
	std::uint64_t schemaHash = 0;               ///< Replication schema fingerprint.
	std::uint64_t clientNonce = 0;              ///< Replay-detection nonce.
};

/**
 * @struct HandshakeAccept
 * @brief Payload of @ref PacketType::ConnectAccept.
 */
struct HandshakeAccept {
	std::uint32_t assignedPeerId = InvalidPeer; ///< Identity assigned to the client.
	std::uint64_t clientNonce = 0;              ///< Echo of the request nonce.
	std::uint64_t serverNonce = 0;              ///< Server-chosen nonce.
};

/**
 * @struct FragmentHeader
 * @brief Extra fields of @ref PacketType::Fragment.
 */
struct FragmentHeader {
	std::uint16_t fragmentId = 0;  ///< Message identifier within the channel.
	std::uint16_t index = 0;       ///< Zero-based fragment index.
	std::uint16_t count = 0;       ///< Total number of fragments.
	std::uint32_t totalSize = 0;   ///< Reassembled payload size.
};

/**
 * @brief Encodes the common packet header.
 * @param writer Destination bit writer.
 * @param header Header values.
 * @return True on success.
 */
inline bool encodePacketHeader(Io::BitWriter& writer, const PacketHeader& header) {
	return writer.writeBits(PacketMagic, 16)
		&& writer.writeBits(NetProtocolVersion, 8)
		&& writer.writeBits(static_cast<std::uint8_t>(header.type), 8)
		&& writer.writeBits(header.token, 32)
		&& writer.writeBits(header.channel, 8)
		&& writer.writeBits(header.sequence, 16)
		&& writer.writeBits(header.ack, 16)
		&& writer.writeBits(header.ackBits, 64);
}

/**
 * @brief Decodes the common packet header.
 * @param reader Source bit reader.
 * @param header Receives the decoded header.
 * @return True on success and when magic/version match.
 */
inline bool decodePacketHeader(Io::BitReader& reader, PacketHeader& header) {
	std::uint64_t magic = 0;
	std::uint64_t version = 0;
	std::uint64_t type = 0;
	if (!reader.readBits(magic, 16)) return false;
	if (!reader.readBits(version, 8)) return false;
	if (magic != PacketMagic || version != NetProtocolVersion) return false;
	if (!reader.readBits(type, 8)) return false;
	std::uint64_t token = 0;
	std::uint64_t channel = 0;
	std::uint64_t sequence = 0;
	std::uint64_t ack = 0;
	if (!reader.readBits(token, 32)) return false;
	if (!reader.readBits(channel, 8)) return false;
	if (!reader.readBits(sequence, 16)) return false;
	if (!reader.readBits(ack, 16)) return false;
	if (!reader.readBits(header.ackBits, 64)) return false;
	header.type = static_cast<PacketType>(type);
	header.token = static_cast<std::uint32_t>(token);
	header.channel = static_cast<std::uint8_t>(channel);
	header.sequence = static_cast<NetSequence>(sequence);
	header.ack = static_cast<NetSequence>(ack);
	return true;
}

/**
 * @brief Encodes a handshake request payload.
 * @param writer Destination bit writer.
 * @param request Request fields.
 * @return True on success.
 */
inline bool encodeHandshakeRequest(Io::BitWriter& writer, const HandshakeRequest& request) {
	return writer.writeBits(request.version, 16)
		&& writer.writeBits(request.schemaHash, 64)
		&& writer.writeBits(request.clientNonce, 64);
}

/**
 * @brief Decodes a handshake request payload.
 * @param reader Source bit reader.
 * @param request Receives the decoded fields.
 * @return True on success.
 */
inline bool decodeHandshakeRequest(Io::BitReader& reader, HandshakeRequest& request) {
	std::uint64_t version = 0;
	if (!reader.readBits(version, 16)) return false;
	if (!reader.readBits(request.schemaHash, 64)) return false;
	if (!reader.readBits(request.clientNonce, 64)) return false;
	request.version = static_cast<std::uint16_t>(version);
	return true;
}

/**
 * @brief Encodes a handshake accept payload.
 * @param writer Destination bit writer.
 * @param accept Accept fields.
 * @return True on success.
 */
inline bool encodeHandshakeAccept(Io::BitWriter& writer, const HandshakeAccept& accept) {
	return writer.writeBits(accept.assignedPeerId, 32)
		&& writer.writeBits(accept.clientNonce, 64)
		&& writer.writeBits(accept.serverNonce, 64);
}

/**
 * @brief Decodes a handshake accept payload.
 * @param reader Source bit reader.
 * @param accept Receives the decoded fields.
 * @return True on success.
 */
inline bool decodeHandshakeAccept(Io::BitReader& reader, HandshakeAccept& accept) {
	std::uint64_t peerId = 0;
	if (!reader.readBits(peerId, 32)) return false;
	if (!reader.readBits(accept.clientNonce, 64)) return false;
	if (!reader.readBits(accept.serverNonce, 64)) return false;
	accept.assignedPeerId = static_cast<std::uint32_t>(peerId);
	return true;
}

/**
 * @brief Encodes fragment-specific fields.
 * @param writer Destination bit writer.
 * @param fragment Fragment fields.
 * @return True on success.
 */
inline bool encodeFragmentHeader(Io::BitWriter& writer, const FragmentHeader& fragment) {
	return writer.writeBits(fragment.fragmentId, 16)
		&& writer.writeBits(fragment.index, 16)
		&& writer.writeBits(fragment.count, 16)
		&& writer.writeBits(fragment.totalSize, 32);
}

/**
 * @brief Decodes fragment-specific fields.
 * @param reader Source bit reader.
 * @param fragment Receives the decoded fields.
 * @return True on success.
 */
inline bool decodeFragmentHeader(Io::BitReader& reader, FragmentHeader& fragment) {
	std::uint64_t fragmentId = 0;
	std::uint64_t index = 0;
	std::uint64_t count = 0;
	std::uint64_t totalSize = 0;
	if (!reader.readBits(fragmentId, 16)) return false;
	if (!reader.readBits(index, 16)) return false;
	if (!reader.readBits(count, 16)) return false;
	if (!reader.readBits(totalSize, 32)) return false;
	fragment.fragmentId = static_cast<std::uint16_t>(fragmentId);
	fragment.index = static_cast<std::uint16_t>(index);
	fragment.count = static_cast<std::uint16_t>(count);
	fragment.totalSize = static_cast<std::uint32_t>(totalSize);
	return true;
}

} // namespace Net
} // namespace Elv

#endif // ELVNETPROTOCOL_HPP

