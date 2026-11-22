/**
 * @file NetSnapshotTests.cpp
 * @brief M3b snapshot framing, delta and ack-baseline tests.
 */
#include <catch2/catch_test_macros.hpp>
#include <Elvavena/Net/ElvNetSnapshot.hpp>
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

struct Velocity {
	float x = 0.0f;
	float y = 0.0f;
};

struct Health {
	std::uint32_t value = 0;
};

constexpr ComponentId PositionId = 1;
constexpr ComponentId VelocityId = 2;
constexpr ComponentId HealthId = 3;
constexpr EntityKindId PlayerKind = 7;
const ComponentId PlayerComponents[] { PositionId, VelocityId, HealthId };

ComponentDefinition componentDefinition(ComponentId id, const char* name, std::size_t size,
	const ComponentOps* ops) {
	ComponentDefinition definition;
	definition.id = id;
	definition.name = name;
	definition.size = size;
	definition.ops = ops;
	return definition;
}

void registerSchema(ComponentRegistry& registry) {
	REQUIRE(registry.registerComponent(componentDefinition(PositionId, "Position",
		sizeof(Position), makeTrivialComponentOps<Position>())));
	REQUIRE(registry.registerComponent(componentDefinition(VelocityId, "Velocity",
		sizeof(Velocity), makeTrivialComponentOps<Velocity>())));
	REQUIRE(registry.registerComponent(componentDefinition(HealthId, "Health",
		sizeof(Health), makeTrivialComponentOps<Health>())));
	EntityKindDefinition kind;
	kind.id = PlayerKind;
	kind.name = "Player";
	kind.components = PlayerComponents;
	kind.componentCount = std::size(PlayerComponents);
	REQUIRE(registry.registerEntityKind(kind));
}

const SnapshotComponentView* findView(const SnapshotRecord& record, ComponentId id) {
	for (const SnapshotComponentView& view : record.components) {
		if (view.id == id) return &view;
	}
	return nullptr;
}

template <typename T>
T decode(const SnapshotComponentView& view, const ComponentRegistry& registry) {
	T value {};
	const ComponentDefinition* definition = registry.findComponent(view.id);
	REQUIRE(definition != nullptr);
	Io::BitReader reader(view.payload);
	REQUIRE(definition->ops->decode(reader, &value, definition->opsContext));
	return value;
}

} // namespace

TEST_CASE("Snapshot writer emits a full spawn baseline", "[net][snapshot]")
{
	ComponentRegistry registry;
	registerSchema(registry);
	ReplicationRegistry entities;
	const NetId id = entities.spawn(PlayerKind, 5);
	REQUIRE(id.isValid());
	Position position { 1.0f, 2.0f };
	Velocity velocity { 3.0f, 4.0f };
	Health health { 100 };
	REQUIRE(entities.attach(id, PositionId, &position));
	REQUIRE(entities.attach(id, VelocityId, &velocity));
	REQUIRE(entities.attach(id, HealthId, &health));

	SnapshotWriter writer(registry);
	const SnapshotWriteResult result = writer.write(1, entities);
	REQUIRE(result.ok);
	CHECK(result.tag != 0);
	CHECK(result.baselineTick == 0);
	CHECK(writer.pendingCount() == 1);

	SnapshotReader reader;
	REQUIRE(reader.parse(result.payload, registry));
	CHECK(reader.tick() == 1);
	REQUIRE(reader.records().size() == 1);
	const SnapshotRecord& record = reader.records()[0];
	CHECK(record.type == SnapshotRecordType::Spawn);
	CHECK(record.id == id);
	CHECK(record.kind == PlayerKind);
	CHECK(record.owner == 5);
	CHECK(record.componentMask == ((1ull << PositionId) | (1ull << VelocityId) | (1ull << HealthId)));
	CHECK(record.removedMask == 0);
	REQUIRE(record.components.size() == 3);
	CHECK(decode<Position>(*findView(record, PositionId), registry).x == 1.0f);
	CHECK(decode<Velocity>(*findView(record, VelocityId), registry).y == 4.0f);
	CHECK(decode<Health>(*findView(record, HealthId), registry).value == 100);
}

TEST_CASE("Snapshot delta waits for an ack and sends only changed components", "[net][snapshot]")
{
	ComponentRegistry registry;
	registerSchema(registry);
	ReplicationRegistry entities;
	const NetId id = entities.spawn(PlayerKind, 5);
	Position position { 1.0f, 2.0f };
	Velocity velocity { 3.0f, 4.0f };
	Health health { 100 };
	REQUIRE(entities.attach(id, PositionId, &position));
	REQUIRE(entities.attach(id, VelocityId, &velocity));
	REQUIRE(entities.attach(id, HealthId, &health));
	SnapshotWriter writer(registry);

	const SnapshotWriteResult first = writer.write(1, entities);
	REQUIRE(first.ok);
	position.x = 9.0f;
	const SnapshotWriteResult second = writer.write(2, entities);
	REQUIRE(second.ok);
	SnapshotReader unacked;
	REQUIRE(unacked.parse(second.payload, registry));
	REQUIRE(unacked.records().size() == 1);
	CHECK(unacked.records()[0].type == SnapshotRecordType::Spawn); // baseline is still empty

	REQUIRE(writer.acknowledge(first.tag));
	CHECK(writer.baselineTick() == 1);
	const SnapshotWriteResult delta = writer.write(3, entities);
	REQUIRE(delta.ok);
	SnapshotReader reader;
	REQUIRE(reader.parse(delta.payload, registry));
	REQUIRE(reader.records().size() == 1);
	const SnapshotRecord& record = reader.records()[0];
	CHECK(record.type == SnapshotRecordType::Update);
	CHECK(record.componentMask == (1ull << PositionId));
	CHECK(record.removedMask == 0);
	REQUIRE(record.components.size() == 1);
	CHECK(decode<Position>(record.components[0], registry).x == 9.0f);

	REQUIRE(writer.acknowledge(delta.tag));
	const SnapshotWriteResult unchanged = writer.write(4, entities);
	REQUIRE(unchanged.ok);
	SnapshotReader empty;
	REQUIRE(empty.parse(unchanged.payload, registry));
	CHECK(empty.records().empty());
}

TEST_CASE("Snapshot delta reports removed components and despawns", "[net][snapshot]")
{
	ComponentRegistry registry;
	registerSchema(registry);
	ReplicationRegistry entities;
	const NetId id = entities.spawn(PlayerKind, 5);
	Position position { 1.0f, 2.0f };
	Velocity velocity { 3.0f, 4.0f };
	Health health { 100 };
	REQUIRE(entities.attach(id, PositionId, &position));
	REQUIRE(entities.attach(id, VelocityId, &velocity));
	REQUIRE(entities.attach(id, HealthId, &health));
	SnapshotWriter writer(registry);
	const SnapshotWriteResult spawn = writer.write(1, entities);
	REQUIRE(spawn.ok);
	REQUIRE(writer.acknowledge(spawn.tag));

	REQUIRE(entities.detach(id, VelocityId));
	const SnapshotWriteResult removal = writer.write(2, entities);
	REQUIRE(removal.ok);
	SnapshotReader reader;
	REQUIRE(reader.parse(removal.payload, registry));
	REQUIRE(reader.records().size() == 1);
	CHECK(reader.records()[0].type == SnapshotRecordType::Update);
	CHECK(reader.records()[0].componentMask == 0);
	CHECK(reader.records()[0].removedMask == (1ull << VelocityId));
	REQUIRE(writer.acknowledge(removal.tag));

	REQUIRE(entities.destroy(id));
	const SnapshotWriteResult despawn = writer.write(3, entities);
	REQUIRE(despawn.ok);
	SnapshotReader despawnReader;
	REQUIRE(despawnReader.parse(despawn.payload, registry));
	REQUIRE(despawnReader.records().size() == 1);
	CHECK(despawnReader.records()[0].type == SnapshotRecordType::Despawn);
	CHECK(despawnReader.records()[0].id == id);
	REQUIRE(writer.acknowledge(despawn.tag));
	CHECK(writer.baselineEntityCount() == 0);
}


TEST_CASE("Snapshot pending tags can be rejected without promoting a baseline", "[net][snapshot]")
{
	ComponentRegistry registry;
	registerSchema(registry);
	ReplicationRegistry entities;
	const NetId id = entities.spawn(PlayerKind);
	Position position { 1.0f, 2.0f };
	REQUIRE(entities.attach(id, PositionId, &position));
	SnapshotWriter writer(registry);
	const SnapshotWriteResult result = writer.write(1, entities);
	REQUIRE(result.ok);
	REQUIRE(writer.reject(result.tag));
	CHECK_FALSE(writer.acknowledge(result.tag));
	CHECK(writer.baselineTick() == 0);
	CHECK(writer.baselineEntityCount() == 0);
	CHECK(writer.pendingCount() == 0);
}

TEST_CASE("Snapshot reader rejects malformed payloads", "[net][snapshot]")
{
	ComponentRegistry registry;
	registerSchema(registry);
	SnapshotReader reader;
	const std::array<std::byte, 4> junk {};
	CHECK_FALSE(reader.parse(junk, registry));
	CHECK(reader.error() == SnapshotError::BadMagic);

	ReplicationRegistry entities;
	const NetId id = entities.spawn(PlayerKind);
	Position position { 1.0f, 2.0f };
	REQUIRE(entities.attach(id, PositionId, &position));
	SnapshotWriter writer(registry);
	const SnapshotWriteResult result = writer.write(1, entities);
	REQUIRE(result.ok);

	std::vector<std::byte> wrongVersion = result.payload;
	wrongVersion[4] = std::byte { 99 };
	CHECK_FALSE(reader.parse(wrongVersion, registry));
	CHECK(reader.error() == SnapshotError::UnsupportedVersion);

	std::vector<std::byte> truncated = result.payload;
	truncated.pop_back();
	CHECK_FALSE(reader.parse(truncated, registry));
	CHECK(reader.error() == SnapshotError::Truncated);
}

