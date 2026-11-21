/**
 * @file UdpTransportTests.cpp
 * @brief Localhost smoke tests for the Euphemy UDP transport.
 *
 * The deterministic session tests use the loopback wire. This case exercises
 * the real platform path instead: create two non-blocking UDP sockets, bind one
 * to an ephemeral port, exchange datagrams and verify payload/source handling.
 */
#include <catch2/catch_test_macros.hpp>
#include <Euphemy/Net/EuphUdpTransport.hpp>
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>

namespace {

using namespace std::chrono_literals;
using Euph::Net::UdpTransport;

/**
 * @brief Polls a transport until a datagram arrives or the deadline expires.
 * @param transport Transport to poll.
 * @param buffer Destination buffer.
 * @param from Receives the source endpoint.
 * @return Received size, or zero on timeout.
 */
std::size_t receiveEventually(UdpTransport& transport, std::span<std::byte> buffer,
	Elv::Net::Endpoint& from) {
	for (int attempt = 0; attempt < 500; ++attempt) {
		Elv::Net::NetError status = Elv::Net::NetError::None;
		const auto received = transport.receiveFrom(buffer, &status);
		if (received.has_value()) {
			from = received->source;
			return received->size;
		}
		if (status != Elv::Net::NetError::WouldBlock) return 0;
		std::this_thread::sleep_for(1ms);
	}
	return 0;
}

} // namespace

TEST_CASE("UDP transport exchanges datagrams on localhost", "[net][socket]")
{
	UdpTransport server;
	REQUIRE(server.bind("127.0.0.1", "0") == Elv::Net::NetError::None);
	CHECK(server.nativeHandle() != Elv::Io::InvalidNativeHandle);
	const auto local = server.localEndpoint();
	REQUIRE(local.has_value());
	REQUIRE(local->family == AF_INET);

	const auto* address = reinterpret_cast<const sockaddr_in*>(local->storage.data());
	REQUIRE(address->sin_family == AF_INET);
	const std::uint16_t port = ntohs(address->sin_port);
	REQUIRE(port != 0);

	UdpTransport client;
	const auto resolved = client.resolve("127.0.0.1", std::to_string(port));
	REQUIRE(resolved.has_value());

	const std::array<std::byte, 4> request {
		std::byte { 0x01 }, std::byte { 0x02 }, std::byte { 0x03 }, std::byte { 0x04 }
	};
	REQUIRE(client.sendTo(*resolved, request) == Elv::Net::NetError::None);

	std::array<std::byte, 64> serverBuffer {};
	Elv::Net::Endpoint clientEndpoint;
	REQUIRE(receiveEventually(server, serverBuffer, clientEndpoint) == request.size());
	CHECK(serverBuffer[0] == request[0]);
	CHECK(serverBuffer[3] == request[3]);

	const std::array<std::byte, 3> reply {
		std::byte { 0xAA }, std::byte { 0xBB }, std::byte { 0xCC }
	};
	REQUIRE(server.sendTo(clientEndpoint, reply) == Elv::Net::NetError::None);

	std::array<std::byte, 64> clientBuffer {};
	Elv::Net::Endpoint serverEndpoint;
	REQUIRE(receiveEventually(client, clientBuffer, serverEndpoint) == reply.size());
	CHECK(clientBuffer[0] == reply[0]);
	CHECK(clientBuffer[2] == reply[2]);
}
