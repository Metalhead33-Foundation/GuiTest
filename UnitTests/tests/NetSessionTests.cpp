/**
 * @file NetSessionTests.cpp
 * @brief End-to-end tests for the poll-driven reliable session.
 *
 * Every case runs two sessions over the deterministic loopback wire. The tests
 * cover handshake recovery, reliable ordered delivery under packet loss,
 * fragmentation, acknowledgement tags, heartbeats/timeouts and graceful
 * disconnect without touching an OS socket.
 */
#include <catch2/catch_test_macros.hpp>
#include <Elvavena/Net/ElvLoopbackTransport.hpp>
#include <Elvavena/Net/ElvNetSession.hpp>
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include "support/TestSupport.hpp"

namespace {

using namespace std::chrono_literals;
using Elv::Net::ChannelId;
using Elv::Net::Endpoint;
using Elv::Net::InvalidPeer;
using Elv::Net::LoopbackNetwork;
using Elv::Net::LoopbackTransport;
using Elv::Net::NetError;
using Elv::Net::PeerId;
using Elv::Net::SendOptions;
using Elv::Net::Session;
using Elv::Net::SessionConfig;
using Elv::Net::SessionEvents;
using Elv::Net::SessionState;
using Elv::Net::TimePoint;

/** @brief Event sink that records everything the tests need to inspect. */
struct RecordingEvents final : SessionEvents {
	std::vector<std::vector<std::byte>> received; ///< Complete payloads.
	std::vector<ChannelId> channels;             ///< Channel of each payload.
	std::vector<std::uint64_t> acked;            ///< Acknowledgement tags.
	std::vector<std::uint64_t> lost;             ///< Lost tags.
	std::vector<PeerId> connected;               ///< Connected peer ids.
	std::vector<std::pair<PeerId, NetError>> disconnected; ///< Disconnect events.

	void onPeerConnected(PeerId peer, const Endpoint&) override {
		connected.push_back(peer);
	}

	void onPeerDisconnected(PeerId peer, NetError reason) override {
		disconnected.emplace_back(peer, reason);
	}

	void onPacketReceived(PeerId, ChannelId channel,
		std::span<const std::byte> payload) override {
		received.emplace_back(payload.begin(), payload.end());
		channels.push_back(channel);
	}

	void onPacketAcknowledged(PeerId, std::uint64_t userTag) override {
		acked.push_back(userTag);
	}

	void onPacketLost(PeerId, std::uint64_t userTag) override {
		lost.push_back(userTag);
	}
};

/** @brief Builds a session configuration with short test timers. */
SessionConfig makeSessionConfig() {
	SessionConfig config;
	config.retransmissionTimeout = 20ms;
	config.heartbeatInterval = 50ms;
	config.peerTimeout = 2s;
	config.fragmentTimeout = 300ms;
	config.handshakeSeed = 123456789;
	return config;
}

/** @brief Advances the wire and pumps both sessions for a number of steps. */
void pump(LoopbackNetwork& network, Session& first, Session& second,
	TimePoint& now, std::chrono::milliseconds step, int iterations) {
	for (int index = 0; index < iterations; ++index) {
		network.advanceTo(now);
		first.poll(now);
		second.poll(now);
		first.flush(now);
		second.flush(now);
		now += step;
	}
}

/** @brief Creates a loopback configuration with a short one-way delay. */
LoopbackNetwork::Config makeWireConfig(std::chrono::milliseconds latency,
	double loss = 0.0) {
	LoopbackNetwork::Config config;
	config.latency = latency;
	config.loss = loss;
	return config;
}

} // namespace

TEST_CASE("Session handshake connects both sides", "[net][session]")
{
	LoopbackNetwork network(makeWireConfig(5ms));
	LoopbackTransport serverTransport(network);
	LoopbackTransport clientTransport(network);
	Session server(serverTransport, makeSessionConfig());
	Session client(clientTransport, makeSessionConfig());
	RecordingEvents serverEvents;
	RecordingEvents clientEvents;
	server.setEvents(&serverEvents);
	client.setEvents(&clientEvents);

	TimePoint now {};
	REQUIRE(client.connect(*serverTransport.localEndpoint(), now) == NetError::None);
	CHECK(client.state() == SessionState::Connecting);
	pump(network, server, client, now, 5ms, 40);

	CHECK(client.state() == SessionState::Connected);
	CHECK(server.state() == SessionState::Connected);
	CHECK(client.localPeer() != InvalidPeer);
	CHECK(server.localPeer() == 1);
	CHECK(server.peerCount() == 1);
	CHECK(client.peerCount() == 1);
	REQUIRE(client.peer(1).has_value());
	CHECK(client.peer(1)->state == SessionState::Connected);
	CHECK(serverEvents.connected.size() == 1);
	CHECK(clientEvents.connected.size() == 1);
}

TEST_CASE("Reliable ordered payloads arrive and are acknowledged", "[net][session]")
{
	LoopbackNetwork network(makeWireConfig(5ms));
	LoopbackTransport serverTransport(network);
	LoopbackTransport clientTransport(network);
	Session server(serverTransport, makeSessionConfig());
	Session client(clientTransport, makeSessionConfig());
	RecordingEvents serverEvents;
	RecordingEvents clientEvents;
	server.setEvents(&serverEvents);
	client.setEvents(&clientEvents);

	TimePoint now {};
	REQUIRE(client.connect(*serverTransport.localEndpoint(), now) == NetError::None);
	pump(network, server, client, now, 5ms, 40);
	REQUIRE(client.state() == SessionState::Connected);

	const std::vector<std::byte> payload {
		std::byte { 0x10 }, std::byte { 0x20 }, std::byte { 0x30 }, std::byte { 0x40 }
	};
	SendOptions options;
	options.channel = 0;
	options.userTag = 42;
	REQUIRE(client.send(1, payload, options, now) == NetError::None);
	pump(network, server, client, now, 5ms, 20);

	REQUIRE(serverEvents.received.size() == 1);
	CHECK(serverEvents.received[0] == payload);
	CHECK(serverEvents.channels[0] == 0);
	CHECK(std::find(clientEvents.acked.begin(), clientEvents.acked.end(), 42)
		!= clientEvents.acked.end());
}

TEST_CASE("Reliable delivery survives a lossy link", "[net][session]")
{
	LoopbackNetwork network(makeWireConfig(5ms, 0.35), 987654321ull);
	LoopbackTransport serverTransport(network);
	LoopbackTransport clientTransport(network);
	Session server(serverTransport, makeSessionConfig());
	Session client(clientTransport, makeSessionConfig());
	RecordingEvents serverEvents;
	RecordingEvents clientEvents;
	server.setEvents(&serverEvents);
	client.setEvents(&clientEvents);

	TimePoint now {};
	REQUIRE(client.connect(*serverTransport.localEndpoint(), now) == NetError::None);
	pump(network, server, client, now, 5ms, 400);
	REQUIRE(client.state() == SessionState::Connected);
	REQUIRE(server.state() == SessionState::Connected);

	const std::vector<std::byte> payload {
		std::byte { 0xAA }, std::byte { 0xBB }, std::byte { 0xCC }, std::byte { 0xDD }
	};
	SendOptions options;
	options.userTag = 7;
	REQUIRE(client.send(1, payload, options, now) == NetError::None);
	pump(network, server, client, now, 5ms, 500);

	REQUIRE(serverEvents.received.size() == 1);
	CHECK(serverEvents.received[0] == payload);
	CHECK(std::find(clientEvents.acked.begin(), clientEvents.acked.end(), 7)
		!= clientEvents.acked.end());
}

TEST_CASE("Payloads larger than the MTU are fragmented and reassembled", "[net][session]")
{
	LoopbackNetwork network(makeWireConfig(5ms));
	LoopbackTransport serverTransport(network);
	LoopbackTransport clientTransport(network);
	SessionConfig config = makeSessionConfig();
	config.mtu = 512;
	config.maxPayload = 16 * 1024;
	Session server(serverTransport, config);
	Session client(clientTransport, config);
	RecordingEvents serverEvents;
	RecordingEvents clientEvents;
	server.setEvents(&serverEvents);
	client.setEvents(&clientEvents);

	TimePoint now {};
	REQUIRE(client.connect(*serverTransport.localEndpoint(), now) == NetError::None);
	pump(network, server, client, now, 5ms, 40);
	REQUIRE(client.state() == SessionState::Connected);

	const std::vector<std::byte> payload = UnitTests::makePattern(3000, 0x12345678u);
	SendOptions options;
	options.userTag = 99;
	REQUIRE(client.send(1, payload, options, now) == NetError::None);
	pump(network, server, client, now, 5ms, 100);

	REQUIRE(serverEvents.received.size() == 1);
	CHECK(serverEvents.received[0] == payload);
	CHECK(std::count(clientEvents.acked.begin(), clientEvents.acked.end(), 99) >= 1);
}

TEST_CASE("Session times out a silent peer", "[net][session]")
{
	LoopbackNetwork network(makeWireConfig(5ms));
	LoopbackTransport serverTransport(network);
	LoopbackTransport clientTransport(network);
	SessionConfig config = makeSessionConfig();
	config.heartbeatInterval = 100ms;
	config.peerTimeout = 500ms;
	Session server(serverTransport, config);
	Session client(clientTransport, config);
	RecordingEvents serverEvents;
	server.setEvents(&serverEvents);

	TimePoint now {};
	REQUIRE(client.connect(*serverTransport.localEndpoint(), now) == NetError::None);
	pump(network, server, client, now, 5ms, 40);
	REQUIRE(server.state() == SessionState::Connected);

	for (int index = 0; index < 200; ++index) {
		network.advanceTo(now);
		server.poll(now);
		server.flush(now);
		now += 10ms;
	}

	CHECK(server.peerCount() == 0);
	REQUIRE(serverEvents.disconnected.size() == 1);
	CHECK(serverEvents.disconnected[0].second == NetError::Timeout);
}


TEST_CASE("Disconnect removes the peer on both sides", "[net][session]")
{
	LoopbackNetwork network(makeWireConfig(5ms));
	LoopbackTransport serverTransport(network);
	LoopbackTransport clientTransport(network);
	Session server(serverTransport, makeSessionConfig());
	Session client(clientTransport, makeSessionConfig());
	RecordingEvents serverEvents;
	server.setEvents(&serverEvents);

	TimePoint now {};
	REQUIRE(client.connect(*serverTransport.localEndpoint(), now) == NetError::None);
	pump(network, server, client, now, 5ms, 40);
	REQUIRE(server.state() == SessionState::Connected);

	REQUIRE(client.disconnect(1, now) == NetError::None);
	CHECK(client.peerCount() == 0);
	pump(network, server, client, now, 5ms, 20);

	CHECK(server.peerCount() == 0);
	REQUIRE(serverEvents.disconnected.size() == 1);
	CHECK(serverEvents.disconnected[0].second == NetError::None);
}

TEST_CASE("Unreliable channels deliver without acknowledgement", "[net][session]")
{
	LoopbackNetwork network(makeWireConfig(5ms));
	LoopbackTransport serverTransport(network);
	LoopbackTransport clientTransport(network);
	Session server(serverTransport, makeSessionConfig());
	Session client(clientTransport, makeSessionConfig());
	RecordingEvents serverEvents;
	RecordingEvents clientEvents;
	server.setEvents(&serverEvents);
	client.setEvents(&clientEvents);

	TimePoint now {};
	REQUIRE(client.connect(*serverTransport.localEndpoint(), now) == NetError::None);
	pump(network, server, client, now, 5ms, 40);
	REQUIRE(client.state() == SessionState::Connected);

	const std::vector<std::byte> payload { std::byte { 0x55 }, std::byte { 0x66 } };
	SendOptions options;
	options.channel = 1;
	options.userTag = 123;
	REQUIRE(client.send(1, payload, options, now) == NetError::None);
	pump(network, server, client, now, 5ms, 20);

	REQUIRE(serverEvents.received.size() == 1);
	CHECK(serverEvents.received[0] == payload);
	CHECK(serverEvents.channels[0] == 1);
	CHECK(clientEvents.acked.empty());
}

TEST_CASE("Send validates channel, payload size and connection state", "[net][session]")
{
	LoopbackNetwork network(makeWireConfig(5ms));
	LoopbackTransport serverTransport(network);
	LoopbackTransport clientTransport(network);
	SessionConfig config = makeSessionConfig();
	config.maxPayload = 16;
	Session server(serverTransport, config);
	Session client(clientTransport, config);

	TimePoint now {};
	SendOptions options;
	const std::vector<std::byte> payload(17, std::byte { 0x01 });
	CHECK(client.send(1, payload, options, now) == NetError::NotConnected);

	REQUIRE(client.connect(*serverTransport.localEndpoint(), now) == NetError::None);
	pump(network, server, client, now, 5ms, 40);
	REQUIRE(client.state() == SessionState::Connected);

	CHECK(client.send(1, payload, options, now) == NetError::MessageTooLarge);
	CHECK(client.send(999, payload, options, now) == NetError::NotConnected);
	options.channel = 31;
	CHECK(client.send(1, std::span<const std::byte>(payload).first(1), options, now)
		== NetError::Unsupported);
}

