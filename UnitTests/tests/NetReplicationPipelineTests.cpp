/**
 * @file NetReplicationPipelineTests.cpp
 * @brief M5 reference server/client pipeline tests.
 */
#include <catch2/catch_test_macros.hpp>
#include <Elvavena/Net/ElvNetReplicationPipeline.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace {

using namespace Elv;
using namespace Elv::Net;

struct Position {
	float x = 0.0f;
	float y = 0.0f;
};

constexpr ComponentId PositionId = 1;
constexpr EntityKindId ActorKind = 9;
const ComponentId ActorComponents[] { PositionId };

ComponentDefinition positionDefinition() {
	ComponentDefinition definition;
	definition.id = PositionId;
	definition.name = "Position";
	definition.size = sizeof(Position);
	definition.ops = makeTrivialComponentOps<Position>();
	return definition;
}

void registerSchema(ComponentRegistry& registry) {
	REQUIRE(registry.registerComponent(positionDefinition()));
	EntityKindDefinition kind;
	kind.id = ActorKind;
	kind.name = "Actor";
	kind.components = ActorComponents;
	kind.componentCount = 1;
	REQUIRE(registry.registerEntityKind(kind));
}

} // namespace

TEST_CASE("Replication server builds per-peer interest snapshots", "[net][pipeline]")
{
	ComponentRegistry registry;
	registerSchema(registry);
	ReplicationRegistry entities;
	const NetId ownedByFive = entities.spawn(ActorKind, 5);
	const NetId ownedBySix = entities.spawn(ActorKind, 6);
	Position first { 1.0f, 2.0f };
	Position second { 3.0f, 4.0f };
	REQUIRE(entities.attach(ownedByFive, PositionId, &first));
	REQUIRE(entities.attach(ownedBySix, PositionId, &second));
	OwnerInterest interest;
	ReplicationServer server(registry, entities, interest);

	const SnapshotWriteResult five = server.buildSnapshot(5, 1);
	REQUIRE(five.ok);
	SnapshotReader reader;
	REQUIRE(reader.parse(five.payload, registry));
	REQUIRE(reader.records().size() == 1);
	CHECK(reader.records()[0].id == ownedByFive);
	REQUIRE(server.acknowledge(5, five.tag));

	const SnapshotWriteResult six = server.buildSnapshot(6, 1);
	REQUIRE(six.ok);
	REQUIRE(reader.parse(six.payload, registry));
	REQUIRE(reader.records().size() == 1);
	CHECK(reader.records()[0].id == ownedBySix);

	first.x = 9.0f;
	const SnapshotWriteResult fiveDelta = server.buildSnapshot(5, 2);
	REQUIRE(fiveDelta.ok);
	REQUIRE(reader.parse(fiveDelta.payload, registry));
	REQUIRE(reader.records().size() == 1);
	CHECK(reader.records()[0].type == SnapshotRecordType::Update);
	CHECK(reader.records()[0].componentMask == (1ull << PositionId));

	REQUIRE(server.removePeer(5));
	CHECK(server.writerFor(5) == nullptr);
}

TEST_CASE("Replication client stores parsed snapshots in its jitter buffer", "[net][pipeline]")
{
	ComponentRegistry registry;
	registerSchema(registry);
	ReplicationRegistry entities;
	const NetId id = entities.spawn(ActorKind);
	Position position { 1.0f, 2.0f };
	REQUIRE(entities.attach(id, PositionId, &position));
	ReplicateAllInterest interest;
	ReplicationServer server(registry, entities, interest);
	const SnapshotWriteResult result = server.buildSnapshot(5, 10);
	REQUIRE(result.ok);

	ReplicationClient client(4);
	REQUIRE(client.receive(10, result.payload, registry));
	CHECK(client.reader().tick() == 10);
	REQUIRE(client.reader().records().size() == 1);
	CHECK(client.frames().latest() != nullptr);
	CHECK(client.frames().latest()->tick == 10);
	const std::array<std::byte, 1> malformed { std::byte { 0 } };
	CHECK_FALSE(client.receive(11, malformed, registry));
}
