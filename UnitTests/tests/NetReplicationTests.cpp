/**
 * @file NetReplicationTests.cpp
 * @brief Tests for NetId, component schemas, the entity registry and interest hooks.
 *
 * These cases cover the object-lifecycle foundation: stale handles must not
 * resolve, component definitions must be validated before use, the existing
 * byte schema must survive a bit-stream round-trip, and interest policies must
 * see only live records.
 */
#include <catch2/catch_test_macros.hpp>
#include <Elvavena/Io/ElvBitStream.hpp>
#include <Elvavena/Net/ElvNetId.hpp>
#include <Elvavena/Net/ElvNetInterest.hpp>
#include <Elvavena/Net/ElvNetReplicationRegistry.hpp>
#include <Elvavena/Net/ElvNetReplicationSchema.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace {

using Elv::Io::BitReader;
using Elv::Io::BitWriter;
using Elv::Net::ComponentDefinition;
using Elv::Net::ComponentId;
using Elv::Net::ComponentRegistry;
using Elv::Net::EntityKindDefinition;
using Elv::Net::InvalidPeer;
using Elv::Net::MaxTrackedComponents;
using Elv::Net::NetId;
using Elv::Net::NetIdAllocator;
using Elv::Net::NetIdHash;
using Elv::Net::objectComponentOps;
using Elv::Net::OwnerInterest;
using Elv::Net::ReplicateAllInterest;
using Elv::Net::ReplicationFlags;
using Elv::Net::ReplicationRegistry;

/** @brief Small schema-described component used by the codec test. */
struct Health {
	std::int32_t current = 0;
	float maximum = 0.0f;
};

const Elv::Io::TypeDefinition Int32Type {
	Elv::Io::DataType::INT32, 0, 0, nullptr, nullptr
};

const Elv::Io::ObjectField HealthFields[] {
	{ static_cast<std::uint16_t>(offsetof(Health, current)), &Int32Type },
	{ static_cast<std::uint16_t>(offsetof(Health, maximum)), &Int32Type }
};

const Elv::Io::ObjectDefinition HealthSchema {
	HealthFields, 2, sizeof(Health)
};

constexpr ComponentId HealthId = 3;
constexpr Elv::Net::EntityKindId PlayerKind = 7;

/** @brief Registers the health component and player kind. */
bool registerHealth(ComponentRegistry& registry) {
	ComponentDefinition definition;
	definition.id = HealthId;
	definition.name = "Health";
	definition.size = sizeof(Health);
	definition.schema = &HealthSchema;
	definition.ops = objectComponentOps();
	definition.opsContext = &HealthSchema;
	definition.flags = ReplicationFlags::Reliable;
	if (!registry.registerComponent(definition)) return false;
	const ComponentId components[] { HealthId };
	const EntityKindDefinition kind { PlayerKind, "Player", components, 1 };
	return registry.registerEntityKind(kind);
}

} // namespace

TEST_CASE("NetId allocator advances generations on reuse", "[net][id]")
{
	NetIdAllocator allocator;
	const NetId first = allocator.acquire();
	const NetId second = allocator.acquire();
	REQUIRE(first.isValid());
	REQUIRE(second.isValid());
	CHECK(first != second);
	CHECK(allocator.isAlive(first));
	CHECK(NetIdHash {}(first) == NetIdHash {}(first));

	CHECK(allocator.release(first));
	CHECK_FALSE(allocator.isAlive(first));
	CHECK_FALSE(allocator.release(first));

	const NetId reused = allocator.acquire();
	REQUIRE(reused.isValid());
	CHECK(reused.index() == first.index());
	CHECK(reused.generation() != first.generation());
	CHECK(allocator.isAlive(reused));
	CHECK_FALSE(NetId::fromParts(0, 1).isValid());
}

TEST_CASE("Component registry validates definitions and entity kinds", "[net][replication]")
{
	ComponentRegistry registry;
	REQUIRE(registerHealth(registry));
	CHECK(registry.componentCount() == 1);
	CHECK(registry.entityKindCount() == 1);
	REQUIRE(registry.findComponent(HealthId) != nullptr);
	REQUIRE(registry.findEntityKind(PlayerKind) != nullptr);
	CHECK(registry.findComponent(HealthId)->size == sizeof(Health));

	const std::uint64_t hash = registry.schemaHash();
	CHECK(hash != 0);
	CHECK_FALSE(registry.registerComponent(*registry.findComponent(HealthId)));

	ComponentDefinition invalid;
	invalid.id = 4;
	invalid.size = sizeof(Health) + 1; // stride mismatch
	invalid.schema = &HealthSchema;
	invalid.ops = objectComponentOps();
	invalid.opsContext = &HealthSchema;
	CHECK_FALSE(registry.registerComponent(invalid));

	const ComponentId unknown[] { 63 };
	const EntityKindDefinition badKind { 8, "Bad", unknown, 1 };
	CHECK_FALSE(registry.registerEntityKind(badKind));
}

TEST_CASE("Object component codec round-trips through the schema", "[net][replication]")
{
	ComponentRegistry registry;
	REQUIRE(registerHealth(registry));
	const ComponentDefinition* definition = registry.findComponent(HealthId);
	REQUIRE(definition != nullptr);
	REQUIRE(definition->ops != nullptr);

	Health source { 100, 250.5f };
	std::array<std::byte, 256> buffer {};
	BitWriter writer(buffer);
	REQUIRE(definition->ops->encode(writer, &source, definition->opsContext));
	REQUIRE(writer.ok());

	BitReader reader(std::span<const std::byte>(buffer.data(), writer.byteSize()));
	Health decoded {};
	REQUIRE(definition->ops->decode(reader, &decoded, definition->opsContext));
	CHECK(decoded.current == source.current);
	CHECK(decoded.maximum == source.maximum);
	CHECK(definition->ops->equal(&source, &decoded, definition->opsContext));

	decoded.maximum = 1.0f;
	CHECK_FALSE(definition->ops->equal(&source, &decoded, definition->opsContext));
}

TEST_CASE("Replication registry composes components and rejects stale ids", "[net][replication]")
{
	ReplicationRegistry registry;
	int payload = 5;
	const NetId entity = registry.spawn(PlayerKind, 2, &payload);
	REQUIRE(entity.isValid());
	CHECK(registry.size() == 1);
	REQUIRE(registry.find(entity) != nullptr);
	CHECK(registry.find(entity)->owner == 2);
	CHECK(registry.find(entity)->user == &payload);

	std::array<int, 2> components { 10, 20 };
	REQUIRE(registry.attach(entity, HealthId, &components[0]));
	REQUIRE(registry.attach(entity, 5, &components[1]));
	CHECK(registry.find(entity)->componentMask == ((1ull << HealthId) | (1ull << 5)));
	CHECK(registry.find(entity)->component(HealthId) == &components[0]);
	CHECK(registry.find(entity)->hasComponent(5));
	CHECK(registry.detach(entity, 5));
	CHECK_FALSE(registry.find(entity)->hasComponent(5));
	CHECK_FALSE(registry.attach(entity, MaxTrackedComponents, &components[0]));

	CHECK(registry.destroy(entity));
	CHECK(registry.size() == 0);
	CHECK(registry.find(entity) == nullptr);
	CHECK_FALSE(registry.destroy(entity));

	const NetId reused = registry.spawn(PlayerKind);
	REQUIRE(reused.isValid());
	CHECK(reused.index() == entity.index());
	CHECK(reused.generation() != entity.generation());
	CHECK(registry.find(entity) == nullptr);
	CHECK(registry.find(reused) != nullptr);
}

TEST_CASE("Interest hooks filter live records", "[net][replication]")
{
	ReplicationRegistry registry;
	const NetId owned = registry.spawn(PlayerKind, 5);
	const NetId other = registry.spawn(PlayerKind, 6);
	const NetId shared = registry.spawn(PlayerKind, InvalidPeer);

	ReplicateAllInterest all;
	std::vector<NetId> visible;
	all.collectRelevant(5, registry, visible);
	CHECK(visible.size() == 3);

	OwnerInterest owner;
	visible.clear();
	owner.collectRelevant(5, registry, visible);
	CHECK(visible.size() == 2);
	CHECK(std::find(visible.begin(), visible.end(), owned) != visible.end());
	CHECK(std::find(visible.begin(), visible.end(), shared) != visible.end());
	CHECK(std::find(visible.begin(), visible.end(), other) == visible.end());

	registry.destroy(shared);
	visible.clear();
	all.collectRelevant(5, registry, visible);
	CHECK(visible.size() == 2);
}

