#ifndef ELVDATAGRAMTRANSPORT_HPP
#define ELVDATAGRAMTRANSPORT_HPP
/**
 * @file ElvDatagramTransport.hpp
 * @brief Abstract datagram transport contract.
 *
 * This is the networking analogue of Elv::Io::Device: the foundational library
 * defines the seam, while Euphemy supplies the OS socket implementation. A
 * session, a reliability layer or a test harness can therefore be written once
 * and run over UDP, a deterministic loopback or a console-specific transport.
 *
 * The interface is intentionally non-blocking. `receiveFrom` returns an empty
 * optional when no datagram is queued; a blocking wrapper belongs to the
 * platform layer, not to the protocol.
 */
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace Elv {
namespace Net {

/**
 * @class DatagramTransport
 * @brief Sends and receives whole datagrams through opaque endpoints.
 *
 * Implementations are not required to be thread-safe. Sessions call one
 * transport from a single thread.
 */
class DatagramTransport {
public:
	/**
	 * @struct ReceivedDatagram
	 * @brief Result of a successful non-blocking receive.
	 */
	struct ReceivedDatagram {
		Endpoint source;   ///< Sender address.
		std::size_t size;  ///< Number of bytes copied into the caller's buffer.
	};

	/** @brief Virtual destructor for polymorphic transports. */
	virtual ~DatagramTransport() = default;

	/** @brief Returns whether the transport is open and usable. */
	[[nodiscard]] virtual bool isOpen() const = 0;

	/**
	 * @brief Resolves a host and service name into a transport endpoint.
	 * @param host Hostname or textual address.
	 * @param service Port number or service name.
	 * @return Endpoint on success, empty optional on failure or when the
	 *         transport has no name-resolution concept.
	 */
	[[nodiscard]] virtual std::optional<Endpoint> resolve(
		std::string_view host, std::string_view service) const = 0;

	/**
	 * @brief Returns the local endpoint the transport is bound to.
	 * @return Endpoint on success, empty optional when unbound or unsupported.
	 */
	[[nodiscard]] virtual std::optional<Endpoint> localEndpoint() const = 0;

	/**
	 * @brief Sends one datagram.
	 * @param destination Target endpoint.
	 * @param datagram Bytes to send.
	 * @return @ref NetError::None on success, or a descriptive error.
	 * @note The implementation copies or consumes the payload before returning;
	 *       the caller may reuse the buffer immediately.
	 */
	virtual NetError sendTo(const Endpoint& destination,
		std::span<const std::byte> datagram) = 0;

	/**
	 * @brief Receives one datagram if available.
	 * @param buffer Destination buffer.
	 * @param status Optional out-parameter receiving a status code.
	 * @return Received datagram on success, empty optional for
	 *         @ref NetError::WouldBlock or an error.
	 * @note Explicitly passing an empty optional is not an error; callers
	 *       should use @p status to distinguish "empty" from "closed".
	 */
	virtual std::optional<ReceivedDatagram> receiveFrom(
		std::span<std::byte> buffer, NetError* status = nullptr) = 0;

	/** @brief Returns the largest datagram the transport accepts. */
	[[nodiscard]] virtual std::size_t maxDatagramSize() const = 0;
};

} // namespace Net
} // namespace Elv

#endif // ELVDATAGRAMTRANSPORT_HPP
