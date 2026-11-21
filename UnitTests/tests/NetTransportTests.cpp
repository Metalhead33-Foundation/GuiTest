/**
 * @file NetTransportTests.cpp
 * @brief Tests for sequence windows, checksums, ring buffers and loopback transport.
 *
 * These cases cover the transport-independent M0 layer: wrap-safe acking, the
 * stable checksum algorithms, the preallocated history container, endpoint
 * value semantics and the deterministic virtual wire. No OS socket is touched.
 */
#include <catch2/catch_test_macros.hpp>
#include <Elvavena/Net/ElvLoopbackTransport.hpp>
#include <Elvavena/Net/ElvNetSequence.hpp>
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <Elvavena/Util/ElvChecksum.hpp>
#include <Elvavena/Util/ElvRingBuffer.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace {

using Elv::Net::AckBitfield;
using Elv::Net::Endpoint;
using Elv::Net::EndpointHash;
using Elv::Net::LoopbackNetwork;
using Elv::Net::LoopbackTransport;
using Elv::Net::NetDuration;
using Elv::Net::NetError;
using Elv::Net::NetSequence;
using Elv::Net::sequenceDistance;
using Elv::Net::sequenceGreaterThan;
using namespace std::chrono_literals;

/** @brief Views a string literal as bytes for checksum calls. */
std::span<const std::byte> asBytes(const std::string& text) {
	return std::span<const std::byte>(
		reinterpret_cast<const std::byte*>(text.data()), text.size());
}

} // namespace

TEST_CASE("Sequence comparisons wrap at the 16-bit boundary", "[net][sequence]")
{
	CHECK(sequenceGreaterThan(1, 65535));
	CHECK_FALSE(sequenceGreaterThan(65535, 1));
	CHECK_FALSE(sequenceGreaterThan(7, 7));
	CHECK_FALSE(sequenceGreaterThan(static_cast<NetSequence>(0x8000), 0));
	CHECK(sequenceDistance(65535, 1) == 2);
	CHECK(sequenceDistance(0, 0) == 0);
}

TEST_CASE("Ack bitfield tracks the latest 64 sequences", "[net][sequence]")
{
	AckBitfield ack;
	CHECK(ack.empty());
	CHECK(ack.markReceived(100));
	CHECK(ack.latest() == 100);
	CHECK(ack.bits() == 0b1ull);

	CHECK(ack.markReceived(102));
	CHECK(ack.latest() == 102);
	CHECK(ack.isReceived(102));
	CHECK_FALSE(ack.isReceived(101));
	CHECK(ack.isReceived(100));

	CHECK(ack.markReceived(101));
	CHECK(ack.bits() == 0b111ull);
	CHECK_FALSE(ack.markReceived(101)); // duplicate
	CHECK_FALSE(ack.markReceived(30));  // older than the window
	CHECK_FALSE(ack.isReceived(30));
}

TEST_CASE("Ack bitfield handles wraparound", "[net][sequence]")
{
	AckBitfield ack;
	ack.reset(65534);
	CHECK(ack.markReceived(1));
	CHECK(ack.latest() == 1);
	CHECK(ack.isReceived(65534));
	CHECK_FALSE(ack.isReceived(65535));
	CHECK(ack.markReceived(0));
	CHECK(ack.isReceived(65535) == false); // not received yet
	CHECK(ack.isReceived(65534));
	CHECK(ack.isReceived(0));
	CHECK(ack.markReceived(65535));
	CHECK(ack.isReceived(65535));
}

TEST_CASE("Checksums match their published vectors", "[net][checksum]")
{
	const std::string digits = "123456789";
	CHECK(Elv::Util::crc32c(asBytes(digits)) == 0xE3069283u);

	const std::string a = "a";
	const std::string foobar = "foobar";
	CHECK(Elv::Util::fnv1a64(asBytes(a)) == 0xAF63DC4C8601EC8Cull);
	CHECK(Elv::Util::fnv1a64(asBytes(foobar)) == 0x85944171F73967E8ull);
}

TEST_CASE("Incremental CRC matches the one-shot value", "[net][checksum]")
{
	const std::string first = "1234";
	const std::string second = "56789";
	Elv::Util::Crc32C crc;
	crc.update(asBytes(first));
	crc.update(asBytes(second));
	CHECK(crc.finalize() == Elv::Util::crc32c(asBytes(first + second)));
	crc.reset();
	crc.update(asBytes(first + second));
	CHECK(crc.finalize() == 0xE3069283u);
}

TEST_CASE("Ring buffer preserves logical order across wrap", "[net][ringbuffer]")
{
	Elv::Util::RingBuffer<int> buffer(4);
	CHECK(buffer.empty());
	CHECK(buffer.capacity() == 4);
	CHECK(buffer.push(1));
	CHECK(buffer.push(2));
	CHECK(buffer.push(3));
	CHECK(buffer.push(4));
	CHECK(buffer.full());
	CHECK_FALSE(buffer.push(5));
	CHECK(buffer[0] == 1);
	CHECK(buffer[3] == 4);

	CHECK(buffer.pushOverwrite(5));
	CHECK(buffer.size() == 4);
	CHECK(buffer[0] == 2);
	CHECK(buffer[3] == 5);

	int value = 0;
	CHECK(buffer.pop(value));
	CHECK(value == 2);
	CHECK(buffer.popBack(value));
	CHECK(value == 5);
	CHECK(buffer.size() == 2);
	CHECK(buffer[0] == 3);
	CHECK(buffer[1] == 4);

	buffer.clear();
	CHECK(buffer.empty());
}

TEST_CASE("Ring buffer supports move-only elements and zero capacity", "[net][ringbuffer]")
{
	Elv::Util::RingBuffer<std::unique_ptr<int>> buffer(2);
	CHECK(buffer.push(std::make_unique<int>(7)));
	CHECK(buffer.push(std::make_unique<int>(8)));
	CHECK_FALSE(buffer.push(std::make_unique<int>(9)));

	std::unique_ptr<int> value;
	CHECK(buffer.pop(value));
	REQUIRE(value != nullptr);
	CHECK(*value == 7);
	CHECK(buffer.pushOverwrite(std::make_unique<int>(10)));
	CHECK(buffer.popBack(value));
	REQUIRE(value != nullptr);
	CHECK(*value == 10);

	Elv::Util::RingBuffer<int> empty(0);
	CHECK_FALSE(empty.push(1));
	CHECK_FALSE(empty.pushOverwrite(1));
}

TEST_CASE("Endpoints have value semantics and a stable hash", "[net][endpoint]")
{
	Endpoint a;
	CHECK_FALSE(a.isValid());
	a.family = 7;
	a.size = 4;
	a.storage[0] = std::byte { 1 };
	a.storage[1] = std::byte { 2 };
	a.storage[2] = std::byte { 3 };
	a.storage[3] = std::byte { 4 };

	Endpoint b = a;
	CHECK(a == b);
	CHECK_FALSE(a != b);
	CHECK(EndpointHash {}(a) == EndpointHash {}(b));

	b.storage[1] = std::byte { 9 };
	CHECK(a != b);
}

TEST_CASE("Loopback transport delivers after the configured latency", "[net][loopback]")
{
	LoopbackNetwork network;
	LoopbackTransport sender(network);
	LoopbackTransport receiver(network);
	REQUIRE(network.endpointCount() == 2);
	REQUIRE(sender.localEndpoint().has_value());
	REQUIRE(receiver.localEndpoint().has_value());

	const std::array<std::byte, 3> payload {
		std::byte { 0x10 }, std::byte { 0x20 }, std::byte { 0x30 }
	};
	REQUIRE(sender.sendTo(*receiver.localEndpoint(), payload) == NetError::None);

	std::array<std::byte, 8> receiveBuffer {};
	NetError status = NetError::None;
	CHECK_FALSE(receiver.receiveFrom(receiveBuffer, &status).has_value());
	CHECK(status == NetError::WouldBlock);

	network.advance(39ms);
	CHECK_FALSE(receiver.receiveFrom(receiveBuffer, &status).has_value());
	CHECK(status == NetError::WouldBlock);

	network.advance(1ms);
	const auto received = receiver.receiveFrom(receiveBuffer, &status);
	REQUIRE(received.has_value());
	CHECK(status == NetError::None);
	CHECK(received->source == *sender.localEndpoint());
	CHECK(received->size == payload.size());
	CHECK(receiveBuffer[0] == payload[0]);
	CHECK(receiveBuffer[1] == payload[1]);
	CHECK(receiveBuffer[2] == payload[2]);
}

TEST_CASE("Loopback transport can drop every datagram", "[net][loopback]")
{
	LoopbackNetwork::Config config;
	config.latency = NetDuration::zero();
	config.loss = 1.0;
	LoopbackNetwork network(config);
	LoopbackTransport sender(network);
	LoopbackTransport receiver(network);

	const std::array<std::byte, 1> payload { std::byte { 0x5A } };
	REQUIRE(sender.sendTo(*receiver.localEndpoint(), payload) == NetError::None);
	network.advance(1ms);

	std::array<std::byte, 4> receiveBuffer {};
	NetError status = NetError::None;
	CHECK_FALSE(receiver.receiveFrom(receiveBuffer, &status).has_value());
	CHECK(status == NetError::WouldBlock);
}

TEST_CASE("Loopback transport can duplicate every datagram", "[net][loopback]")
{
	LoopbackNetwork::Config config;
	config.latency = 10ms;
	config.duplicate = 1.0;
	LoopbackNetwork network(config);
	LoopbackTransport sender(network);
	LoopbackTransport receiver(network);

	const std::array<std::byte, 2> payload { std::byte { 0xAA }, std::byte { 0xBB } };
	REQUIRE(sender.sendTo(*receiver.localEndpoint(), payload) == NetError::None);
	network.advance(10ms);

	std::array<std::byte, 8> receiveBuffer {};
	const auto first = receiver.receiveFrom(receiveBuffer);
	REQUIRE(first.has_value());
	CHECK(first->size == payload.size());
	const auto second = receiver.receiveFrom(receiveBuffer);
	REQUIRE(second.has_value());
	CHECK(second->size == payload.size());
	CHECK(second->source == first->source);

	NetError status = NetError::None;
	CHECK_FALSE(receiver.receiveFrom(receiveBuffer, &status).has_value());
	CHECK(status == NetError::WouldBlock);
}

TEST_CASE("Loopback transport rejects oversized payloads in both directions", "[net][loopback]")
{
	LoopbackNetwork::Config config;
	config.latency = 10ms;
	config.maxDatagramSize = 4;
	LoopbackNetwork network(config);
	LoopbackTransport sender(network);
	LoopbackTransport receiver(network);

	const std::array<std::byte, 5> tooLarge {
		std::byte { 1 }, std::byte { 2 }, std::byte { 3 },
		std::byte { 4 }, std::byte { 5 }
	};
	CHECK(sender.sendTo(*receiver.localEndpoint(), tooLarge) == NetError::MessageTooLarge);

	const std::array<std::byte, 4> accepted {
		std::byte { 1 }, std::byte { 2 }, std::byte { 3 }, std::byte { 4 }
	};
	REQUIRE(sender.sendTo(*receiver.localEndpoint(), accepted) == NetError::None);
	network.advance(10ms);

	std::array<std::byte, 2> smallBuffer {};
	NetError status = NetError::None;
	CHECK_FALSE(receiver.receiveFrom(smallBuffer, &status).has_value());
	CHECK(status == NetError::MessageTooLarge);
}

TEST_CASE("Closing a loopback transport detaches its endpoint", "[net][loopback]")
{
	LoopbackNetwork network;
	LoopbackTransport sender(network);
	LoopbackTransport receiver(network);
	REQUIRE(network.endpointCount() == 2);

	sender.close();
	CHECK_FALSE(sender.isOpen());
	CHECK_FALSE(sender.localEndpoint().has_value());
	CHECK(network.endpointCount() == 1);
	CHECK(sender.sendTo(*receiver.localEndpoint(), {}) == NetError::Closed);

	sender.close(); // idempotent
	CHECK(network.endpointCount() == 1);
}

TEST_CASE("Loopback transport rejects unknown destinations", "[net][loopback]")
{
	LoopbackNetwork network;
	LoopbackTransport sender(network);
	LoopbackTransport temporary(network);
	const Endpoint dead = *temporary.localEndpoint();
	temporary.close();

	CHECK(sender.sendTo(dead, {}) == NetError::InvalidEndpoint);
	CHECK_FALSE(network.isEndpointValid(dead));
}

