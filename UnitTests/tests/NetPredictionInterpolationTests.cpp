/**
 * @file NetPredictionInterpolationTests.cpp
 * @brief M4 tests for interpolation, jitter, prediction and lockstep helpers.
 */
#include <catch2/catch_test_macros.hpp>
#include <Elvavena/Net/ElvNetInterpolation.hpp>
#include <Elvavena/Net/ElvNetLockstep.hpp>
#include <Elvavena/Net/ElvNetPrediction.hpp>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace {

using namespace Elv;
using namespace Elv::Net;

struct Vec2 {
	float x = 0.0f;
	float y = 0.0f;
};

Vec2 operator*(Vec2 value, float scale) { return { value.x * scale, value.y * scale }; }
Vec2 operator*(float scale, Vec2 value) { return value * scale; }
Vec2 operator+(Vec2 lhs, Vec2 rhs) { return { lhs.x + rhs.x, lhs.y + rhs.y }; }
Vec2& operator+=(Vec2& lhs, Vec2 rhs) {
	lhs = lhs + rhs;
	return lhs;
}

struct Transform {
	float x = 0.0f;
	float y = 0.0f;
	std::int32_t health = 0;
};

} // namespace

TEST_CASE("Jitter buffer interpolates and clamps out-of-order samples", "[net][interpolation]")
{
	JitterBuffer<float> buffer(4);
	REQUIRE(buffer.push(200, 10.0f));
	REQUIRE(buffer.push(100, 0.0f));
	REQUIRE(buffer.size() == 2);
	float value = 0.0f;
	const InterpolationOps* ops = makeInterpolationOps<float>();
	REQUIRE(buffer.sample(150, value, ops));
	CHECK(value == 5.0f);
	REQUIRE(buffer.sample(50, value, ops));
	CHECK(value == 0.0f);
	REQUIRE(buffer.sample(250, value, ops));
	CHECK(value == 10.0f);

	JitterBuffer<Vec2> vectors(4);
	REQUIRE(vectors.push(0, Vec2 { 0.0f, 0.0f }));
	REQUIRE(vectors.push(10, Vec2 { 10.0f, 20.0f }));
	Vec2 vector {};
	REQUIRE(vectors.sample(5, vector, makeInterpolationOps<Vec2>()));
	CHECK(vector.x == 5.0f);
	CHECK(vector.y == 10.0f);
}

TEST_CASE("Schema interpolation follows explicit field rules", "[net][interpolation]")
{
	Io::TypeDefinition floatType { Io::DataType::INT32, 4, 0, nullptr, nullptr };
	Io::TypeDefinition intType { Io::DataType::INT32, 4, 0, nullptr, nullptr };
	const Io::ObjectField fields[] {
		{ static_cast<std::uint16_t>(offsetof(Transform, x)), &floatType },
		{ static_cast<std::uint16_t>(offsetof(Transform, y)), &floatType },
		{ static_cast<std::uint16_t>(offsetof(Transform, health)), &intType }
	};
	const Io::ObjectDefinition schema { fields, 3, sizeof(Transform) };
	const SchemaInterpolationRule rules[] {
		{ 0, SchemaInterpolationKind::Float },
		{ 1, SchemaInterpolationKind::Float },
		{ 2, SchemaInterpolationKind::Int }
	};
	const SchemaInterpolationDesc desc { &schema, rules, 3 };
	const InterpolationOps ops = schemaInterpolationOps(desc);
	const Transform first { 0.0f, 0.0f, 10 };
	const Transform second { 10.0f, 20.0f, 20 };
	Transform out {};
	REQUIRE(ops.interpolate(&out, &first, &second, 0.5f, ops.context));
	CHECK(out.x == 5.0f);
	CHECK(out.y == 10.0f);
	CHECK(out.health == 15);
}

TEST_CASE("Snapshot jitter buffer orders payloads and overwrites capacity", "[net][interpolation]")
{
	SnapshotJitterBuffer buffer(3);
	const std::array<std::byte, 1> one { std::byte { 1 } };
	const std::array<std::byte, 1> two { std::byte { 2 } };
	const std::array<std::byte, 1> three { std::byte { 3 } };
	REQUIRE(buffer.push(20, one));
	REQUIRE(buffer.push(10, two));
	REQUIRE(buffer.push(30, three));
	REQUIRE(buffer.latest() != nullptr);
	CHECK(buffer.latest()->tick == 30);
	REQUIRE(buffer.atOrBefore(25) != nullptr);
	CHECK(buffer.atOrBefore(25)->tick == 20);
	REQUIRE(buffer.atOrAfter(15) != nullptr);
	CHECK(buffer.atOrAfter(15)->tick == 20);
	REQUIRE(buffer.push(40, one));
	CHECK(buffer.size() == 3);
	CHECK(buffer.oldest()->tick == 20);
}

TEST_CASE("Input queues, delay buffer and checksum history keep tick order", "[net][prediction]")
{
	InputQueue<int> queue(3);
	REQUIRE(queue.push(2, 20));
	REQUIRE(queue.push(1, 10));
	REQUIRE(queue.push(4, 40));
	REQUIRE(queue.entries().size() == 3);
	CHECK(queue.entries()[0].tick == 1);
	CHECK(queue.entries()[1].tick == 2);
	REQUIRE(queue.push(3, 30));
	CHECK(queue.entries()[0].tick == 2); // capacity evicted the oldest
	int value = 0;
	REQUIRE(queue.get(3, value));
	CHECK(value == 30);
	queue.pruneBefore(3);
	CHECK(queue.entries()[0].tick == 3);

	InputDelayBuffer<int> delayed(2, 8);
	REQUIRE(delayed.push(10, 100));
	REQUIRE(delayed.delayedFor(12, value));
	CHECK(value == 100);
	CHECK_FALSE(delayed.delayedFor(11, value));

	ChecksumHistory history(8);
	REQUIRE(history.record(1, 7));
	REQUIRE(history.record(2, 8));
	bool differs = false;
	REQUIRE(history.differs(1, 7, differs));
	CHECK_FALSE(differs);
	REQUIRE(history.differs(1, 9, differs));
	CHECK(differs);
	CHECK_FALSE(history.differs(3, 1, differs));
}

TEST_CASE("Prediction session detects checksum rollback and replays inputs", "[net][prediction]")
{
	PredictionSession<int> session(8);
	REQUIRE(session.recordLocal(5, 1));
	REQUIRE(session.recordConfirmed(5, 2));
	REQUIRE(session.recordChecksum(5, 42));
	std::uint32_t checksum = 0;
	REQUIRE(session.localChecksum(5, checksum));
	CHECK(checksum == 42);
	Tick from = 0;
	CHECK_FALSE(session.needsRollback(5, 42, from));
	REQUIRE(session.needsRollback(5, 43, from));
	CHECK(from == 5);
	CHECK_FALSE(session.needsRollback(6, 43, from));
	REQUIRE(session.confirmedInput(5) != nullptr);
	CHECK(*session.confirmedInput(5) == 2);

	InputQueue<int> replay(8);
	REQUIRE(replay.push(10, 1));
	REQUIRE(replay.push(11, 2));
	REQUIRE(replay.push(12, 3));
	int state = 0;
	const Tick next = replayInputs(replay, 10, 12, state,
		[](Tick, int input, int& value) { value += input; });
	CHECK(state == 6);
	CHECK(next == 13);

	session.pruneBefore(6);
	CHECK_FALSE(session.localChecksum(5, checksum));
}

TEST_CASE("Lockstep collector waits for every peer before a frame", "[net][lockstep]")
{
	const std::array<PeerId, 2> peers { 1, 2 };
	LockstepInputCollector<int> collector(peers, 8);
	REQUIRE(collector.peerCount() == 2);
	REQUIRE(collector.submit(1, 0, 10));
	CHECK_FALSE(collector.ready(0));
	REQUIRE(collector.submit(2, 0, 20));
	CHECK(collector.ready(0));
	LockstepInputCollector<int>::Frame frame;
	REQUIRE(collector.tryCollectNext(frame));
	CHECK(frame.tick == 0);
	REQUIRE(frame.inputs.size() == 2);
	CHECK(frame.inputs[0].input == 10);
	CHECK(frame.inputs[1].input == 20);
	CHECK(collector.nextTick() == 1);
	CHECK_FALSE(collector.tryCollectNext(frame));
	CHECK_FALSE(collector.submit(99, 1, 0));

	REQUIRE(collector.submit(2, 1, 21));
	REQUIRE(collector.submit(1, 1, 11));
	REQUIRE(collector.tryCollectNext(frame));
	CHECK(frame.tick == 1);
	collector.pruneBefore(2);
	CHECK(collector.nextTick() == 2);
}

TEST_CASE("Lockstep checksum exchange finds the first divergent tick", "[net][lockstep]")
{
	const std::array<PeerId, 1> peers { 2 };
	LockstepChecksumExchange exchange(peers, 8);
	REQUIRE(exchange.recordLocal(0, 100));
	REQUIRE(exchange.recordLocal(1, 101));
	REQUIRE(exchange.recordRemote(2, 0, 100));
	REQUIRE(exchange.recordRemote(2, 1, 999));
	bool differs = false;
	REQUIRE(exchange.compare(2, 0, differs));
	CHECK_FALSE(differs);
	REQUIRE(exchange.compare(2, 1, differs));
	CHECK(differs);
	Tick mismatch = 0;
	REQUIRE(exchange.firstMismatch(2, 0, mismatch));
	CHECK(mismatch == 1);
	CHECK_FALSE(exchange.compare(2, 3, differs));
	exchange.pruneBefore(1);
	CHECK(exchange.local().entries().size() == 1);
}

