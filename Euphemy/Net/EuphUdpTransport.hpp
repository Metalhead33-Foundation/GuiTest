#ifndef EUPHUDPTRANSPORT_HPP
#define EUPHUDPTRANSPORT_HPP
/**
 * @file EuphUdpTransport.hpp
 * @brief POSIX/WinSock UDP implementation of Elv::Net::DatagramTransport.
 *
 * This adapter is deliberately thin: `Euph::Io::Socket` already owns the
 * platform handle, WinSock initialization and endpoint conversion. The
 * transport only adds the non-blocking datagram contract used by Elvavena's
 * session layer.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Euphemy/Io/EuphSocket.hpp>
#include <Elvavena/Net/ElvDatagramTransport.hpp>
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace Euph {
namespace Net {

/**
 * @class UdpTransport
 * @brief Non-blocking UDP transport over `Euph::Io::Socket`.
 *
 * Construction creates and configures a UDP socket but does not bind it. Call
 * @ref bind for a server socket, or use the socket as an unbound client and
 * pass the destination to @ref sendTo. The class is not thread-safe.
 */
class UdpTransport final : public Elv::Net::DatagramTransport {
public:
	/**
	 * @brief Creates an unbound UDP socket.
	 * @param family Address family.
	 * @throws std::runtime_error When the platform socket cannot be created.
	 */
	explicit UdpTransport(Io::AddressFamily family = Io::AddressFamily::IPV4)
		: family(family), socket(family, Io::SocketType::UDP, true) {
		socket.setBlocking(false);
	}

	/**
	 * @brief Binds the socket to a local address.
	 * @param host Local address; empty or null-equivalent means wildcard.
	 * @param service Port number or service name.
	 * @return @ref Elv::Net::NetError::None on success.
	 */
	Elv::Net::NetError bind(std::string_view host, std::string_view service) {
		const std::string hostCopy(host);
		const std::string serviceCopy(service);
		const auto endpoint = Io::Socket::resolveEndpoint(family, Io::SocketType::UDP,
			hostCopy.c_str(), serviceCopy.c_str(), true);
		if (!endpoint.has_value()) return Elv::Net::NetError::InvalidEndpoint;
		if (!socket.bindTo(*endpoint)) return Elv::Net::NetError::ProtocolError;
		return Elv::Net::NetError::None;
	}

	/**
	 * @brief Connects the socket to a remote address.
	 * @param host Remote hostname or textual address.
	 * @param service Port number or service name.
	 * @return @ref Elv::Net::NetError::None on success.
	 */
	Elv::Net::NetError connect(std::string_view host, std::string_view service) {
		const std::string hostCopy(host);
		const std::string serviceCopy(service);
		const auto endpoint = Io::Socket::resolveEndpoint(family, Io::SocketType::UDP,
			hostCopy.c_str(), serviceCopy.c_str(), false);
		if (!endpoint.has_value()) return Elv::Net::NetError::InvalidEndpoint;
		if (!socket.connectTo(*endpoint)) return Elv::Net::NetError::ProtocolError;
		return Elv::Net::NetError::None;
	}

	/** @brief Returns whether the underlying socket is valid. */
	[[nodiscard]] bool isOpen() const override { return socket.isValid(); }

	/** @brief Resolves a host/service pair into an endpoint. */
	[[nodiscard]] std::optional<Elv::Net::Endpoint> resolve(
		std::string_view host, std::string_view service) const override {
		const std::string hostCopy(host);
		const std::string serviceCopy(service);
		return Io::Socket::resolveEndpoint(family, Io::SocketType::UDP,
			hostCopy.c_str(), serviceCopy.c_str(), false);
	}

	/** @brief Returns the local endpoint, including the ephemeral port. */
	[[nodiscard]] std::optional<Elv::Net::Endpoint> localEndpoint() const override {
		return socket.localEndpoint();
	}

	/** @brief Sends one datagram to an explicit endpoint. */
	Elv::Net::NetError sendTo(const Elv::Net::Endpoint& destination,
		std::span<const std::byte> datagram) override {
		return socket.sendDatagram(destination, datagram);
	}

	/** @brief Receives one datagram if the socket has one queued. */
	std::optional<ReceivedDatagram> receiveFrom(
		std::span<std::byte> buffer, Elv::Net::NetError* status = nullptr) override {
		Elv::Net::Endpoint from;
		Elv::Net::NetError error = Elv::Net::NetError::None;
		const auto received = socket.receiveDatagram(buffer, from, &error);
		if (status != nullptr) *status = error;
		if (!received.has_value()) return std::nullopt;
		return ReceivedDatagram { from, *received };
	}

	/** @brief Returns the maximum UDP payload size. */
	[[nodiscard]] std::size_t maxDatagramSize() const override { return 65507; }

	/** @brief Returns the underlying socket for platform-specific options. */
	[[nodiscard]] Io::Socket& nativeSocket() { return socket; }

private:
	Io::AddressFamily family; ///< Address family used for name resolution.
	Io::Socket socket;        ///< Platform socket handle and options.
};

} // namespace Net
} // namespace Euph

#endif // EUPHUDPTRANSPORT_HPP
