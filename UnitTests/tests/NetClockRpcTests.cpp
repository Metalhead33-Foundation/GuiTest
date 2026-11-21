/**
 * @file NetClockRpcTests.cpp
 * @brief Tests for clock estimation, tick conversion and typed RPC dispatch.
 *
 * The clock cases use synthetic probes with known RTT and offset, including a
 * lower-RTT sample that should win over an earlier noisy one. The RPC cases
 * cover typed argument round-trips, unknown ids, truncation, policies and
 * handler exceptions.
 */
#include <catch2/catch_test_macros.hpp>
#include <Elvavena/Net/ElvNetClockSync.hpp>
#include <Elvavena/Net/ElvNetRpc.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace std::chrono_literals;
using Elv::Net::ClockEstimator;
using Elv::Net::RpcContext;
using Elv::Net::RpcDispatcher;
using Elv::Net::RpcEncoder;
using Elv::Net::RpcPolicy;
using Elv::Net::RpcRegistry;
using Elv::Net::RpcStatus;
using Elv::Net::Tick;
using Elv::Net::TickClock;
using Elv::Net::TimePoint;

/** @brief Captures one RPC invocation. */
struct CapturedCall {
	int value = 0;
	std::string text;
	bool flag = false;
	RpcContext context;
};

/** @brief Policy that rejects every call. */
struct DenyAllPolicy final : RpcPolicy {
	bool allow(const RpcContext&, Elv::Net::RpcId) const override { return false; }
};

enum class TestColour : std::uint8_t {
	Red = 1,
	Green = 2,
};

} // namespace

TEST_CASE("Clock estimator derives RTT and offset from a probe", "[net][clock]")
{
	const TimePoint epoch {};
	ClockEstimator estimator(8, 1.0);
	CHECK_FALSE(estimator.hasSample());

	estimator.addSample(epoch, epoch + 100ms, epoch + 1000ms);
	REQUIRE(estimator.hasSample());
	CHECK(estimator.rtt() == 100ms);
	CHECK(estimator.offset() == 950ms);
	CHECK(estimator.serverToLocal(epoch + 1000ms) == epoch + 50ms);
	CHECK(estimator.localToServer(epoch) == epoch + 950ms);
	CHECK(estimator.samples().size() == 1);
}

TEST_CASE("Clock estimator prefers the lowest-RTT sample and smooths it", "[net][clock]")
{
	const TimePoint epoch {};
	ClockEstimator estimator(8, 0.5);
	estimator.addSample(epoch, epoch + 100ms, epoch + 1000ms); // RTT 100, offset 950
	estimator.addSample(epoch + 200ms, epoch + 220ms, epoch + 1200ms); // RTT 20, offset 990
	CHECK(estimator.rtt() == 60ms);    // 0.5 * 100 + 0.5 * 20
	CHECK(estimator.offset() == 970ms); // 0.5 * 950 + 0.5 * 990

	estimator.addSample(epoch, epoch + 50ms, epoch + 500ms); // ignore high offset, lower RTT
	CHECK(estimator.samples().size() == 3);
	estimator.addSample(epoch + 500ms, epoch + 400ms, epoch); // invalid, ignored
	CHECK(estimator.samples().size() == 3);
}

TEST_CASE("Clock estimator keeps a bounded history", "[net][clock]")
{
	const TimePoint epoch {};
	ClockEstimator estimator(3, 1.0);
	for (int index = 0; index < 6; ++index) {
		const auto send = epoch + std::chrono::milliseconds(index * 10);
		estimator.addSample(send, send + 20ms, send + 1000ms);
	}
	CHECK(estimator.samples().size() == 3);
	CHECK(estimator.rtt() == 20ms);
}

TEST_CASE("Tick clock converts in both directions", "[net][clock]")
{
	const TimePoint epoch {};
	TickClock clock(60.0, 0, epoch);
	CHECK(clock.tickAt(epoch) == 0);
	CHECK(clock.tickAt(epoch + 1000ms) == 60);
	CHECK(clock.tickAt(epoch + 1500ms) == 90);
	CHECK(clock.timeOf(60) == epoch + 1000ms);
	CHECK(clock.timeOf(90) == epoch + 1500ms);
	CHECK(clock.tickAt(epoch - 1000ms) == 0);
	CHECK(clock.timeOf(0) == epoch);
	CHECK_THROWS(TickClock(0.0));
}

TEST_CASE("RPC registry round-trips typed arguments", "[net][rpc]")
{
	RpcRegistry registry;
	CapturedCall captured;
	REQUIRE(registry.bind<7, int, std::string, bool>(
		[&](const RpcContext& context, int value, const std::string& text, bool flag) {
			captured.value = value;
			captured.text = text;
			captured.flag = flag;
			captured.context = context;
		}));
	CHECK(registry.size() == 1);
	CHECK(registry.contains(7));
	CHECK_FALSE(registry.bind<7, int>([](const RpcContext&, int) {}));

	const std::vector<std::byte> payload = RpcEncoder::encode<7>(256, 42,
		std::string("hello"), true);
	REQUIRE_FALSE(payload.empty());

	RpcDispatcher dispatcher(registry);
	RpcContext context;
	context.sender = 3;
	context.tick = 99;
	context.server = true;
	context.user = &captured;
	CHECK(dispatcher.dispatch(payload, context) == RpcStatus::Ok);
	CHECK(captured.value == 42);
	CHECK(captured.text == "hello");
	CHECK(captured.flag);
	CHECK(captured.context.sender == 3);
	CHECK(captured.context.tick == 99);
	CHECK(captured.context.server);
	CHECK(captured.context.user == &captured);
}

TEST_CASE("RPC codecs handle enums and binary blobs", "[net][rpc]")
{
	RpcRegistry registry;
	TestColour colour = TestColour::Red;
	std::vector<std::byte> blob;
	REQUIRE(registry.bind<8, TestColour, std::vector<std::byte>>(
		[&](const RpcContext&, TestColour value, const std::vector<std::byte>& bytes) {
			colour = value;
			blob = bytes;
		}));

	const std::vector<std::byte> bytes {
		std::byte { 0xAA }, std::byte { 0xBB }, std::byte { 0xCC }
	};
	const std::vector<std::byte> payload = RpcEncoder::encode<8>(128,
		TestColour::Green, bytes);
	REQUIRE_FALSE(payload.empty());
	RpcDispatcher dispatcher(registry);
	CHECK(dispatcher.dispatch(payload, RpcContext {}) == RpcStatus::Ok);
	CHECK(colour == TestColour::Green);
	CHECK(blob == bytes);
}

TEST_CASE("RPC dispatcher reports unknown, malformed and denied calls", "[net][rpc]")
{
	RpcRegistry registry;
	REQUIRE(registry.bind<1, int>([](const RpcContext&, int) {}));
	RpcDispatcher dispatcher(registry);

	const std::vector<std::byte> unknown = RpcEncoder::encode<2>(64, 5);
	CHECK(dispatcher.dispatch(unknown, RpcContext {}) == RpcStatus::UnknownProcedure);

	std::vector<std::byte> truncated = RpcEncoder::encode<1>(64, 123456);
	REQUIRE_FALSE(truncated.empty());
	truncated.resize(1); // id only, no integer varint
	CHECK(dispatcher.dispatch(truncated, RpcContext {}) == RpcStatus::Malformed);

	const std::vector<std::byte> valid = RpcEncoder::encode<1>(64, 5);
	DenyAllPolicy deny;
	RpcDispatcher denied(registry, &deny);
	CHECK(denied.dispatch(valid, RpcContext {}) == RpcStatus::Unauthorized);

	CHECK(RpcEncoder::encode<1>(1, 5).empty());
}

TEST_CASE("RPC handler exceptions are contained", "[net][rpc]")
{
	RpcRegistry registry;
	REQUIRE(registry.bind<3, int>([](const RpcContext&, int) {
		throw std::runtime_error("boom");
	}));
	const std::vector<std::byte> payload = RpcEncoder::encode<3>(64, 1);
	REQUIRE_FALSE(payload.empty());
	RpcDispatcher dispatcher(registry);
	CHECK(dispatcher.dispatch(payload, RpcContext {}) == RpcStatus::HandlerError);
}

