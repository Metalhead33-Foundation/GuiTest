#ifndef ELVNETTYPES_HPP
#define ELVNETTYPES_HPP
/**
 * @file ElvNetTypes.hpp
 * @brief Shared identifier, endpoint and error vocabulary for Elv::Net.
 *
 * These types deliberately depend only on the standard library and Elvavena's
 * endianness helper. No OS socket header is included here: concrete transports
 * live in Euphemy and expose their addressing details through the opaque
 * @ref Elv::Net::Endpoint storage.
 */
#include <Elvavena/Util/ElvEndianness.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>

namespace Elv {
namespace Net {

/// @brief Identifies a peer inside a session. Never derived from an address.
using PeerId = std::uint32_t;

/// @brief Value used when no peer is selected.
inline constexpr PeerId InvalidPeer = std::numeric_limits<PeerId>::max();

/// @brief Index of an independent reliability channel.
using ChannelId = std::uint8_t;

/// @brief Simulation tick used by snapshots, inputs and clock conversion.
using Tick = std::uint64_t;

/// @brief 16-bit per-channel packet sequence number.
using NetSequence = std::uint16_t;

/// @brief Clock used by sessions and transports.
using NetClock = std::chrono::steady_clock;

/// @brief Session time point. Tests inject arbitrary values for determinism.
using TimePoint = NetClock::time_point;

/// @brief Session duration type.
using NetDuration = NetClock::duration;

/// @brief Wire byte order used by the networking layer.
inline constexpr Util::Endian NetEndian = Util::Endian::Big;

/// @brief Current library-level protocol version.
inline constexpr std::uint16_t NetProtocolVersion = 1;

/// @brief Maximum number of simultaneously open channels per peer.
inline constexpr std::size_t MaxNetChannels = 32;

/**
 * @enum NetError
 * @brief Non-throwing result codes used by transports and sessions.
 */
enum class NetError : std::uint8_t {
	None,               ///< Operation succeeded.
	WouldBlock,         ///< No datagram is currently available.
	MessageTooLarge,    ///< Payload exceeds the transport or fragment limit.
	InvalidEndpoint,    ///< Endpoint is malformed, unknown or of the wrong family.
	NotConnected,       ///< The peer or transport is not connected.
	Closed,             ///< The transport or session has been closed.
	Timeout,            ///< The peer did not answer within the configured window.
	ProtocolError,      ///< A packet violated the wire format or state machine.
	Unauthorized,       ///< The operation is not allowed for this peer.
	ResourceExhausted,  ///< A queue, window or reassembly buffer is full.
	Unsupported,        ///< The transport does not implement the requested operation.
};

/**
 * @brief Returns whether an error code represents success.
 * @param error Error code to test.
 * @return True for @ref NetError::None.
 */
constexpr bool netErrorOk(NetError error) noexcept {
	return error == NetError::None;
}

/**
 * @enum SessionState
 * @brief Lifecycle state of a network session.
 */
enum class SessionState : std::uint8_t {
	Disconnected, ///< No transport activity.
	Connecting,   ///< Handshake in progress.
	Connected,    ///< Handshake completed.
	Disconnecting ///< Disconnect announced, waiting for teardown.
};

/**
 * @enum ChannelReliability
 * @brief Delivery guarantee selected per channel.
 */
enum class ChannelReliability : std::uint8_t {
	Unreliable,          ///< May be lost, duplicated or reordered.
	UnreliableSequenced, ///< May be lost, but older packets are discarded.
	ReliableUnordered,   ///< Retransmitted and deduplicated, order not preserved.
	ReliableOrdered,     ///< Retransmitted, deduplicated and delivered in order.
};

/**
 * @struct SendOptions
 * @brief Per-message routing and lifetime options.
 */
struct SendOptions {
	ChannelId channel = 0;         ///< Target channel; its configured reliability applies.
	std::uint64_t userTag = 0;     ///< Opaque tag reported on ack/loss.
	bool flushImmediately = false; ///< Ask the session to flush now.
};

/**
 * @struct Endpoint
 * @brief Opaque, transport-defined network address.
 *
 * Elvavena never interprets the bytes. A UDP transport stores a
 * `sockaddr_storage` here; a loopback transport stores an integer handle. Both
 * agree on @ref size (valid bytes) and @ref family (transport-specific tag).
 *
 * The storage is deliberately larger than IPv6 and aligned generously so
 * platform address structures fit without an allocation.
 */
struct Endpoint {
	/// @brief Size of the opaque storage in bytes.
	static constexpr std::size_t StorageSize = 128;

	alignas(std::max_align_t) std::array<std::byte, StorageSize> storage {}; ///< Transport-defined address bytes.
	std::uint16_t size = 0;                        ///< Number of valid bytes in @ref storage.
	std::uint16_t family = 0;                      ///< Transport-defined address family tag.

	/** @brief Returns whether the endpoint carries a non-empty address. */
	[[nodiscard]] bool isValid() const noexcept { return size != 0; }

	/**
	 * @brief Compares two endpoints byte-for-byte within their valid size.
	 * @param other Endpoint to compare with.
	 * @return True when family, size and all valid bytes match.
	 */
	[[nodiscard]] bool operator==(const Endpoint& other) const noexcept {
		if (family != other.family || size != other.size) return false;
		if (size > StorageSize) return false;
		for (std::size_t index = 0; index < size; ++index) {
			if (storage[index] != other.storage[index]) return false;
		}
		return true;
	}

	/**
	 * @brief Compares two endpoints for inequality.
	 * @param other Endpoint to compare with.
	 * @return True when they differ.
	 */
	[[nodiscard]] bool operator!=(const Endpoint& other) const noexcept { return !(*this == other); }
};

/**
 * @struct EndpointHash
 * @brief Hash functor for using an @ref Endpoint as a map key.
 */
struct EndpointHash {
	/**
	 * @brief Hashes the valid endpoint bytes and family tag.
	 * @param endpoint Endpoint to hash.
	 * @return Stable in-process hash value.
	 */
	[[nodiscard]] std::size_t operator()(const Endpoint& endpoint) const noexcept {
		std::size_t hash = 1469598103934665603ull;
		const std::size_t count = endpoint.size <= Endpoint::StorageSize
			? endpoint.size : Endpoint::StorageSize;
		for (std::size_t index = 0; index < count; ++index) {
			hash ^= static_cast<std::size_t>(std::to_integer<std::uint8_t>(endpoint.storage[index]));
			hash *= 1099511628211ull;
		}
		hash ^= static_cast<std::size_t>(endpoint.family) + 0x9E3779B9ull
			+ (hash << 6) + (hash >> 2);
		return hash;
	}
};

} // namespace Net
} // namespace Elv

#endif // ELVNETTYPES_HPP

