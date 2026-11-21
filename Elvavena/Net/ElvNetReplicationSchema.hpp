#ifndef ELVNETREPLICATIONSCHEMA_HPP
#define ELVNETREPLICATIONSCHEMA_HPP
/**
 * @file ElvNetReplicationSchema.hpp
 * @brief Component and entity-kind descriptors for replication.
 *
 * The registry is deliberately type-erased: a game describes each networked
 * component once with a stable `ComponentId`, its in-memory size, an optional
 * `Elv::Io::ObjectDefinition` schema, and an encode/decode/equal ops table. The
 * replication registry can then iterate components without an inheritance
 * hierarchy or RTTI.
 *
 * `objectComponentOps()` adapts the existing byte-oriented serialization schema
 * to the bit-stream contract; `makeTrivialComponentOps<T>()` is the zero-cost
 * option for plain-old-data components.
 */
#include <Elvavena/Io/ElvBitStream.hpp>
#include <Elvavena/Io/ElvDataSerializationSchema.hpp>
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <Elvavena/Util/ElvChecksum.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <type_traits>
#include <vector>

namespace Elv {
namespace Net {

/// @brief Stable identifier of a replicated component.
using ComponentId = std::uint16_t;

/// @brief Stable identifier of an entity kind.
using EntityKindId = std::uint16_t;

/// @brief Number of component slots tracked in one entity record.
inline constexpr std::size_t MaxTrackedComponents = 64;

/**
 * @enum ReplicationFlags
 * @brief Bit flags describing how a component is replicated.
 */
enum class ReplicationFlags : std::uint8_t {
	None = 0,               ///< No special behaviour.
	Reliable = 1,           ///< Prefer a reliable channel.
	OwnerAuthoritative = 2, ///< Only the owning peer may mutate it.
	Interpolated = 4,       ///< Clients may interpolate it.
	Persistent = 8,         ///< Keep it across respawns.
};

/** @brief Bitwise OR for replication flags. */
constexpr ReplicationFlags operator|(ReplicationFlags lhs, ReplicationFlags rhs) noexcept {
	return static_cast<ReplicationFlags>(static_cast<std::uint8_t>(lhs)
		| static_cast<std::uint8_t>(rhs));
}

/** @brief Bitwise AND test for replication flags. */
constexpr bool hasFlag(ReplicationFlags value, ReplicationFlags flag) noexcept {
	return (static_cast<std::uint8_t>(value) & static_cast<std::uint8_t>(flag)) != 0;
}

/**
 * @struct FieldQuantization
 * @brief Optional bit-packing override for one schema field.
 */
struct FieldQuantization {
	std::uint16_t fieldIndex = 0; ///< Index into the object's field array.
	float minimum = 0.0f;         ///< Lower bound of the quantized range.
	float maximum = 0.0f;         ///< Upper bound of the quantized range.
	std::uint8_t bits = 0;        ///< Bits per value, 1 through 32.
};

/**
 * @struct ComponentOps
 * @brief Type-erased encode/decode/equality table for one component.
 *
 * `context` is supplied by @ref ComponentDefinition::opsContext; the object
 * schema codec uses it for the `ObjectDefinition` pointer.
 */
struct ComponentOps {
	/**
	 * @brief Encodes a component into a bit stream.
	 * @param writer Destination writer.
	 * @param data Component instance.
	 * @param context Ops-specific context.
	 * @return True on success.
	 */
	bool (*encode)(Io::BitWriter& writer, const void* data, const void* context);

	/**
	 * @brief Decodes a component from a bit stream.
	 * @param reader Source reader.
	 * @param data Component instance to populate.
	 * @param context Ops-specific context.
	 * @return True on success.
	 */
	bool (*decode)(Io::BitReader& reader, void* data, const void* context);

	/**
	 * @brief Compares two component instances.
	 * @param lhs First instance.
	 * @param rhs Second instance.
	 * @param context Ops-specific context.
	 * @return True when the values are equal; may be null when unknown.
	 */
	bool (*equal)(const void* lhs, const void* rhs, const void* context);
};

/**
 * @struct ComponentDefinition
 * @brief Static description of one replicated component type.
 */
struct ComponentDefinition {
	ComponentId id = 0;                         ///< Stable id; must be < MaxTrackedComponents.
	const char* name = "";                      ///< Debug name.
	std::size_t size = 0;                       ///< In-memory size in bytes.
	const Io::ObjectDefinition* schema = nullptr; ///< Optional byte schema.
	const ComponentOps* ops = nullptr;          ///< Required encode/decode table.
	const void* opsContext = nullptr;           ///< Context forwarded to ops.
	ReplicationFlags flags = ReplicationFlags::None; ///< Replication policy.
	const FieldQuantization* quantizations = nullptr; ///< Optional per-field packing.
	std::size_t quantizationCount = 0;          ///< Number of quantization entries.
};

/**
 * @struct EntityKindDefinition
 * @brief Static list of components carried by one entity kind.
 */
struct EntityKindDefinition {
	EntityKindId id = 0;              ///< Stable kind id.
	const char* name = "";            ///< Debug name.
	const ComponentId* components = nullptr; ///< Component ids.
	std::size_t componentCount = 0;   ///< Number of component ids.
};

/**
 * @brief Creates an ops table for a trivially copyable component.
 * @tparam T Pod-like component type.
 * @return Static ops table; the bytes are copied verbatim.
 */
template <typename T>
	requires std::is_trivially_copyable_v<T>
const ComponentOps* makeTrivialComponentOps() noexcept {
	static const ComponentOps ops {
		[](Io::BitWriter& writer, const void* data, const void*) {
			return writer.writeBytes(data, sizeof(T));
		},
		[](Io::BitReader& reader, void* data, const void*) {
			return reader.readBytes(data, sizeof(T));
		},
		[](const void* lhs, const void* rhs, const void*) {
			return std::memcmp(lhs, rhs, sizeof(T)) == 0;
		}
	};
	return &ops;
}

/**
 * @brief Returns the ops table adapting `Io::ObjectDefinition` to bit streams.
 *
 * The context passed to these ops must be the matching `ObjectDefinition*`.
 * Payloads are length-prefixed serialized bytes, so the existing byte schema is
 * reused without exposing a buffer device in the public API.
 */
[[nodiscard]] MH_UTIL_API const ComponentOps* objectComponentOps() noexcept;

/**
 * @class ComponentRegistry
 * @brief Validates and stores component and entity-kind definitions.
 *
 * Registration is a startup operation. Lookups are allocation-free; component
 * ids are bounded so an entity can store a 64-bit presence mask.
 */
class ComponentRegistry {
public:
	/**
	 * @brief Registers a component definition.
	 * @param definition Validated definition.
	 * @return False when the id is duplicated or the definition is invalid.
	 */
	bool registerComponent(const ComponentDefinition& definition) {
		if (definition.id >= MaxTrackedComponents || definition.size == 0) return false;
		if (definition.ops == nullptr || definition.ops->encode == nullptr
			|| definition.ops->decode == nullptr) {
			return false;
		}
		if (definition.schema != nullptr && definition.schema->stride != definition.size) {
			return false;
		}
		if (definition.quantizations != nullptr) {
			if (definition.schema == nullptr) return false;
			for (std::size_t index = 0; index < definition.quantizationCount; ++index) {
				const FieldQuantization& quantization = definition.quantizations[index];
				if (quantization.bits == 0 || quantization.bits > 32) return false;
				if (!(quantization.maximum > quantization.minimum)) return false;
				if (definition.schema != nullptr
					&& quantization.fieldIndex >= definition.schema->fieldCount) {
					return false;
				}
			}
		} else if (definition.quantizationCount != 0) {
			return false;
		}
		if (components[definition.id].has_value()) return false;
		components[definition.id] = definition;
		++registeredComponents;
		return true;
	}

	/**
	 * @brief Looks up a component definition.
	 * @param id Component id.
	 * @return Definition pointer, or null when unregistered.
	 */
	[[nodiscard]] const ComponentDefinition* findComponent(ComponentId id) const {
		if (id >= MaxTrackedComponents || !components[id].has_value()) return nullptr;
		return &*components[id];
	}


	/**
	 * @brief Registers an entity kind.
	 * @param definition Kind definition.
	 * @return False when the id is duplicated, empty, or references an unknown
	 *         or duplicated component.
	 */
	bool registerEntityKind(const EntityKindDefinition& definition) {
		if (definition.componentCount == 0 || definition.components == nullptr) return false;
		for (const EntityKindDefinition& existing : entityKinds) {
			if (existing.id == definition.id) return false;
		}
		std::uint64_t mask = 0;
		for (std::size_t index = 0; index < definition.componentCount; ++index) {
			const ComponentId id = definition.components[index];
			if (findComponent(id) == nullptr) return false;
			const std::uint64_t bit = 1ull << id;
			if ((mask & bit) != 0) return false;
			mask |= bit;
		}
		entityKinds.push_back(definition);
		return true;
	}

	/**
	 * @brief Looks up an entity kind.
	 * @param id Entity kind id.
	 * @return Definition pointer, or null when unregistered.
	 */
	[[nodiscard]] const EntityKindDefinition* findEntityKind(EntityKindId id) const {
		for (const EntityKindDefinition& definition : entityKinds) {
			if (definition.id == id) return &definition;
		}
		return nullptr;
	}

	/** @brief Returns the number of registered components. */
	[[nodiscard]] std::size_t componentCount() const noexcept { return registeredComponents; }

	/** @brief Returns the number of registered entity kinds. */
	[[nodiscard]] std::size_t entityKindCount() const noexcept { return entityKinds.size(); }

	/**
	 * @brief Computes a stable fingerprint of the registered schema.
	 * @return FNV-1a hash over component ids, sizes, field layouts and kinds.
	 */
	[[nodiscard]] std::uint64_t schemaHash() const noexcept {
		std::uint64_t hash = 14695981039346656037ull;
		const auto mix = [&hash](std::uint64_t value) {
			for (unsigned index = 0; index < 8; ++index) {
				hash ^= (value >> (index * 8)) & 0xFFull;
				hash *= 1099511628211ull;
			}
		};
		for (std::size_t id = 0; id < MaxTrackedComponents; ++id) {
			if (!components[id].has_value()) continue;
			const ComponentDefinition& definition = *components[id];
			mix(definition.id);
			mix(definition.size);
			mix(static_cast<std::uint8_t>(definition.flags));
			mix(definition.schema != nullptr ? definition.schema->stride : 0);
			mix(definition.schema != nullptr ? definition.schema->fieldCount : 0);
			if (definition.schema != nullptr) {
				for (std::size_t field = 0; field < definition.schema->fieldCount; ++field) {
					const Io::ObjectField& descriptor = definition.schema->fields[field];
					mix(descriptor.offset);
					mix(descriptor.typeDef != nullptr
						? static_cast<std::uint8_t>(descriptor.typeDef->dataType) : 0);
					mix(descriptor.typeDef != nullptr ? descriptor.typeDef->length : 0);
					mix(descriptor.typeDef != nullptr ? descriptor.typeDef->elementStride : 0);
				}
			}
		}
		for (const EntityKindDefinition& definition : entityKinds) {
			mix(definition.id);
			mix(definition.componentCount);
			for (std::size_t index = 0; index < definition.componentCount; ++index) {
				mix(definition.components[index]);
			}
		}
		return hash;
	}

	/** @brief Removes every definition. */
	void clear() noexcept {
		for (auto& component : components) component.reset();
		entityKinds.clear();
		registeredComponents = 0;
	}

private:
	std::array<std::optional<ComponentDefinition>, MaxTrackedComponents> components; ///< By id.
	std::vector<EntityKindDefinition> entityKinds; ///< Registered kinds.
	std::size_t registeredComponents = 0;          ///< Number of component slots used.
};

} // namespace Net
} // namespace Elv

#endif // ELVNETREPLICATIONSCHEMA_HPP

