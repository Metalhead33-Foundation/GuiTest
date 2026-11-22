#ifndef ELVNETSNAPSHOT_HPP
#define ELVNETSNAPSHOT_HPP
/**
 * @file ElvNetSnapshot.hpp
 * @brief Snapshot framing, per-component delta encoding and ack-tag baselines.
 *
 * A snapshot is a sequence of spawn/update/despawn records. Component payloads
 * are encoded with the registered `ComponentOps` table and framed as a byte
 * length plus bytes, so the reader can return zero-copy views without knowing a
 * component's internal bit layout.
 *
 * The writer keeps two baseline stores:
 * - the acknowledged baseline used to compute the next delta;
 * - pending baselines keyed by the `SendOptions::userTag` of an in-flight
 *   snapshot. `acknowledge(tag)` promotes one; `reject(tag)` drops it.
 *
 * Wire layout (all integers MSB-first, payloads byte-aligned):
 * - header: magic `ELVS`, version u8, flags u8, tick u32, baseline tick u32,
 *   record count u32;
 * - spawn: type u8, NetId u32, kind u16, owner u32, present mask u64, payloads;
 * - update: type u8, NetId u32, changed mask u64, removed mask u64, payloads;
 * - despawn: type u8, NetId u32.
 *
 * Each payload is mode u8 (full or field delta), length u32, then bytes.
 */
#include <Elvavena/Io/ElvBitStream.hpp>
#include <Elvavena/Net/ElvNetId.hpp>
#include <Elvavena/Net/ElvNetReplicationRegistry.hpp>
#include <Elvavena/Net/ElvNetReplicationSchema.hpp>
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <limits>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Elv {
namespace Net {

/** @brief Stable snapshot wire version. Version 2 added the component payload mode. */
inline constexpr std::uint8_t SnapshotWireVersion = 2;

/** @brief Snapshot magic (`ELVS`). */
inline constexpr std::uint32_t SnapshotMagic = 0x454C5653u;

/**
 * @enum SnapshotError
 * @brief Writer/reader failure categories.
 */
enum class SnapshotError : std::uint8_t {
	None,                 ///< Success.
	InvalidArgument,      ///< Null/invalid input or range.
	MissingComponent,     ///< Entity references an unregistered component.
	EncodeFailed,         ///< Component ops failed to encode.
	BufferTooSmall,       ///< Writer could not size the output.
	TooManyRecords,       ///< Record count exceeds u32.
	BadMagic,             ///< Wrong snapshot magic.
	UnsupportedVersion,   ///< Wire version mismatch.
	Truncated,            ///< Source ended before a field/payload.
	InvalidRecord,        ///< Unknown record type or malformed masks.
	UnknownComponent,     ///< Record references an unregistered component.
	UnknownEntityKind,    ///< Spawn references an unregistered kind.
	DuplicateEntity,      ///< Two records for one NetId.
	TrailingData,         ///< Bytes remain after the declared records.
};

/**
 * @enum SnapshotRecordType
 * @brief Kind of one snapshot record.
 */
enum class SnapshotRecordType : std::uint8_t {
	Spawn = 1,  ///< Creates/replaces an entity and sends all components.
	Update = 2, ///< Sends changed components and removed-component bits.
	Despawn = 3 ///< Destroys an entity.
};

/**
 * @enum SnapshotComponentMode
 * @brief Whether a component payload is full state or a baseline delta.
 */
enum class SnapshotComponentMode : std::uint8_t {
	Full = 0,       ///< Regular `ComponentOps` payload.
	FieldDelta = 1  ///< `FieldDeltaOps` payload relative to the reader's baseline.
};

/**
 * @struct SnapshotComponentView
 * @brief One component payload view into the reader's owned storage.
 */
struct SnapshotComponentView {
	ComponentId id = 0;                    ///< Component id.
	SnapshotComponentMode mode = SnapshotComponentMode::Full; ///< Payload kind.
	std::span<const std::byte> payload;    ///< Encoded component bytes or field delta.
};

/**
 * @struct SnapshotRecord
 * @brief Parsed snapshot record.
 */
struct SnapshotRecord {
	SnapshotRecordType type = SnapshotRecordType::Update; ///< Record type.
	NetId id;                                             ///< Entity handle.
	EntityKindId kind = 0;                                ///< Spawn kind; zero for update/despawn.
	PeerId owner = InvalidPeer;                           ///< Spawn owner.
	std::uint64_t componentMask = 0;                      ///< Present (spawn) or changed (update) components.
	std::uint64_t removedMask = 0;                        ///< Components removed by this update.
	std::vector<SnapshotComponentView> components;        ///< Payloads for set `componentMask` bits.

	/** @brief Returns whether a component is present/changed. */
	[[nodiscard]] bool hasComponent(ComponentId component) const noexcept {
		return component < MaxTrackedComponents && (componentMask & (1ull << component)) != 0;
	}

	/** @brief Returns whether a component was removed. */
	[[nodiscard]] bool hasRemoved(ComponentId component) const noexcept {
		return component < MaxTrackedComponents && (removedMask & (1ull << component)) != 0;
	}
};

/**
 * @class SnapshotReader
 * @brief Parses and validates a snapshot payload.
 *
 * Component payload views point into storage owned by this reader. A subsequent
 * `parse()` invalidates them.
 */
class SnapshotReader {
public:
	/**
	 * @brief Parses a snapshot.
	 * @param payload Wire bytes.
	 * @param registry Schema used to validate component/kind ids.
	 * @return True on success; inspect @ref error on failure.
	 */
	bool parse(std::span<const std::byte> payload, const ComponentRegistry& registry);

	/** @brief Returns the snapshot tick. */
	[[nodiscard]] std::uint32_t tick() const noexcept { return tick_; }
	/** @brief Returns the baseline tick used by the writer. */
	[[nodiscard]] std::uint32_t baselineTick() const noexcept { return baselineTick_; }
	/** @brief Returns parsed records. */
	[[nodiscard]] std::span<const SnapshotRecord> records() const noexcept { return records_; }
	/** @brief Returns the last parse error. */
	[[nodiscard]] SnapshotError error() const noexcept { return error_; }

private:
	/**
	 * @brief Latching failure helper.
	 * @param error Error to store.
	 * @return Always false.
	 */
	bool fail(SnapshotError error) noexcept {
		if (error_ == SnapshotError::None) error_ = error;
		return false;
	}

	/**
	 * @brief Reads and validates the component payloads for a record.
	 * @param reader Source reader.
	 * @param registry Component schema.
	 * @param record Record receiving payload views.
	 * @return True on success.
	 */
	bool readComponents(Io::BitReader& reader, const ComponentRegistry& registry,
		SnapshotRecord& record) {
		for (ComponentId id = 0; id < MaxTrackedComponents; ++id) {
			if ((record.componentMask & (1ull << id)) == 0) continue;
			if (registry.findComponent(id) == nullptr) return fail(SnapshotError::UnknownComponent);
			std::uint64_t modeValue = 0;
			std::uint64_t length = 0;
			if (!reader.readBits(modeValue, 8)) return fail(SnapshotError::Truncated);
			if (modeValue > static_cast<std::uint64_t>(SnapshotComponentMode::FieldDelta)) {
				return fail(SnapshotError::InvalidRecord);
			}
			if (!reader.readBits(length, 32)) return fail(SnapshotError::Truncated);
			if (!reader.alignToByte()) return fail(SnapshotError::Truncated);
			if (length > reader.remainingBits() / 8) return fail(SnapshotError::Truncated);
			payloads_.emplace_back(static_cast<std::size_t>(length));
			if (length != 0 && !reader.readBytes(payloads_.back())) {
				return fail(SnapshotError::Truncated);
			}
			record.components.push_back(SnapshotComponentView {
				id, static_cast<SnapshotComponentMode>(modeValue), payloads_.back() });
		}
		return true;
	}

	std::uint32_t tick_ = 0;                 ///< Snapshot tick.
	std::uint32_t baselineTick_ = 0;         ///< Baseline tick from the writer.
	std::vector<SnapshotRecord> records_;    ///< Parsed records.
	std::vector<std::vector<std::byte>> payloads_; ///< Owned payload storage.
	SnapshotError error_ = SnapshotError::None; ///< Last error.
};


inline bool SnapshotReader::parse(std::span<const std::byte> payload,
	const ComponentRegistry& registry) {
	records_.clear();
	payloads_.clear();
	error_ = SnapshotError::None;
	Io::BitReader reader(payload);

	std::uint64_t magic = 0;
	std::uint64_t version = 0;
	std::uint64_t flags = 0;
	std::uint64_t tick = 0;
	std::uint64_t baselineTick = 0;
	std::uint64_t recordCount = 0;
	if (!reader.readBits(magic, 32)) return fail(SnapshotError::Truncated);
	if (magic != SnapshotMagic) return fail(SnapshotError::BadMagic);
	if (!reader.readBits(version, 8)) return fail(SnapshotError::Truncated);
	if (version != SnapshotWireVersion) return fail(SnapshotError::UnsupportedVersion);
	if (!reader.readBits(flags, 8)) return fail(SnapshotError::Truncated);
	if (!reader.readBits(tick, 32) || !reader.readBits(baselineTick, 32)
		|| !reader.readBits(recordCount, 32)) {
		return fail(SnapshotError::Truncated);
	}
	// Every record carries at least a type byte and a NetId. Reject counts that
	// cannot possibly fit before reserving attacker-controlled storage.
	const std::uint64_t minimumRecordBits = 40;
	if (recordCount > reader.remainingBits() / minimumRecordBits) {
		return fail(SnapshotError::Truncated);
	}
	tick_ = static_cast<std::uint32_t>(tick);
	baselineTick_ = static_cast<std::uint32_t>(baselineTick);
	records_.reserve(static_cast<std::size_t>(recordCount));
	std::unordered_set<std::uint32_t> seen;
	seen.reserve(static_cast<std::size_t>(recordCount));

	for (std::uint64_t index = 0; index < recordCount; ++index) {
		std::uint64_t typeValue = 0;
		std::uint64_t idValue = 0;
		if (!reader.readBits(typeValue, 8) || !reader.readBits(idValue, 32)) {
			return fail(SnapshotError::Truncated);
		}
		const auto type = static_cast<SnapshotRecordType>(typeValue);
		if (type != SnapshotRecordType::Spawn && type != SnapshotRecordType::Update
			&& type != SnapshotRecordType::Despawn) {
			return fail(SnapshotError::InvalidRecord);
		}
		SnapshotRecord record;
		record.type = type;
		record.id = NetId::fromParts(idValue & NetId::MaxIndex,
			(idValue >> NetId::IndexBits) & NetId::GenerationMask);
		if (!record.id.isValid() || !seen.insert(idValue).second) {
			return fail(SnapshotError::DuplicateEntity);
		}
		if (type == SnapshotRecordType::Spawn) {
			std::uint64_t kind = 0;
			std::uint64_t owner = 0;
			if (!reader.readBits(kind, 16) || !reader.readBits(owner, 32)
				|| !reader.readBits(record.componentMask, 64)) {
				return fail(SnapshotError::Truncated);
			}
			record.kind = static_cast<EntityKindId>(kind);
			record.owner = static_cast<PeerId>(owner);
			if (registry.findEntityKind(record.kind) == nullptr) {
				return fail(SnapshotError::UnknownEntityKind);
			}
		} else if (type == SnapshotRecordType::Update) {
			if (!reader.readBits(record.componentMask, 64)
				|| !reader.readBits(record.removedMask, 64)) {
				return fail(SnapshotError::Truncated);
			}
			if ((record.componentMask & record.removedMask) != 0) {
				return fail(SnapshotError::InvalidRecord);
			}
		}
		if (type != SnapshotRecordType::Despawn && !readComponents(reader, registry, record)) {
			return false;
		}
		records_.push_back(std::move(record));
	}

	if (!reader.alignToByte() || reader.remainingBits() != 0) {
		return fail(SnapshotError::TrailingData);
	}
	return true;
}

/**
 * @struct SnapshotWriteResult
 * @brief Encoded snapshot plus the ack tag to pass to `SendOptions::userTag`.
 */
struct SnapshotWriteResult {
	bool ok = false;                    ///< Encoding succeeded.
	SnapshotError error = SnapshotError::None; ///< Failure category.
	std::uint64_t tag = 0;              ///< Ack tag for this snapshot.
	std::uint32_t tick = 0;             ///< Snapshot tick.
	std::uint32_t baselineTick = 0;     ///< Baseline tick used for the delta.
	std::vector<std::byte> payload;     ///< Wire bytes.
};

/**
 * @class SnapshotWriter
 * @brief Builds spawn/update/despawn snapshots against acknowledged baselines.
 *
 * The writer never owns game components. It encodes current component values,
 * compares them with the last acknowledged payload, and keeps the candidate
 * baseline pending until the caller reports the matching `userTag` as acked.
 */
class SnapshotWriter {
public:
	/**
	 * @brief Creates a writer over a component schema.
	 * @param registry Component/entity-kind registry.
	 * @param maxPendingBaselines Maximum in-flight baselines; zero is unlimited.
	 */
	explicit SnapshotWriter(const ComponentRegistry& registry,
		std::size_t maxPendingBaselines = 16)
		: registry_(registry), maxPendingBaselines_(maxPendingBaselines) {}

	/**
	 * @brief Encodes the current registry state as a snapshot.
	 * @param tick Current snapshot tick.
	 * @param entities Live replicated entities.
	 * @return Encoded payload, tag and error status.
	 */
	SnapshotWriteResult write(std::uint32_t tick, const ReplicationRegistry& entities) {
		return writeImpl(tick, entities, nullptr);
	}

	/**
	 * @brief Encodes only entities visible in an interest-filtered snapshot.
	 * @param tick Current snapshot tick.
	 * @param entities Live replicated entities.
	 * @param visible Relevant entity ids from an `InterestManager`.
	 * @return Encoded payload, tag and error status.
	 */
	SnapshotWriteResult writeFiltered(std::uint32_t tick, const ReplicationRegistry& entities,
		std::span<const NetId> visible) {
		std::unordered_set<std::uint32_t> visibleSet;
		visibleSet.reserve(visible.size());
		for (NetId id : visible) {
			if (id.isValid()) visibleSet.insert(id.value());
		}
		return writeImpl(tick, entities, &visibleSet);
	}

	/**
	 * @brief Promotes the pending baseline associated with an ack tag.
	 * @param tag `SendOptions::userTag` from the sent snapshot.
	 * @return True when the tag was pending.
	 */
	bool acknowledge(std::uint64_t tag);

	/**
	 * @brief Drops a pending baseline for a lost snapshot.
	 * @param tag `SendOptions::userTag` from the sent snapshot.
	 * @return True when the tag was pending.
	 */
	bool reject(std::uint64_t tag);

	/** @brief Clears all baselines and pending tags. */
	void reset();

	/** @brief Returns the acknowledged baseline tick. */
	[[nodiscard]] std::uint32_t baselineTick() const noexcept { return baselineTick_; }

	/** @brief Returns the number of entities in the acknowledged baseline. */
	[[nodiscard]] std::size_t baselineEntityCount() const noexcept { return baseline_.size(); }

	/** @brief Returns the number of in-flight baselines. */
	[[nodiscard]] std::size_t pendingCount() const noexcept { return pending_.size(); }

private:
	/** @brief Encoded component values for one baseline entity. */
	struct BaselineEntity {
		EntityKindId kind = 0;
		PeerId owner = InvalidPeer;
		std::uint64_t mask = 0;
		std::array<std::vector<std::byte>, MaxTrackedComponents> payloads;
		std::array<std::vector<std::byte>, MaxTrackedComponents> values;
	};

	using BaselineMap = std::unordered_map<NetId, BaselineEntity, NetIdHash>;

	/** @brief One encoded component in a pending record. */
	struct PendingComponent {
		ComponentId id = 0;
		SnapshotComponentMode mode = SnapshotComponentMode::Full;
		std::vector<std::byte> payload;
	};

	/** @brief One record being encoded. */
	struct PendingRecord {
		SnapshotRecordType type = SnapshotRecordType::Update;
		NetId id;
		EntityKindId kind = 0;
		PeerId owner = InvalidPeer;
		std::uint64_t mask = 0;
		std::uint64_t removedMask = 0;
		std::vector<PendingComponent> components;
	};

	/** @brief Candidate baseline held until an ack arrives. */
	struct PendingSnapshot {
		std::uint32_t tick = 0;
		BaselineMap baseline;
	};

	const ComponentRegistry& registry_;
	std::size_t maxPendingBaselines_ = 16;
	BaselineMap baseline_;
	std::uint32_t baselineTick_ = 0;
	std::unordered_map<std::uint64_t, PendingSnapshot> pending_;
	std::deque<std::uint64_t> pendingOrder_;
	std::uint64_t nextTag_ = 1;

	/**
	 * @brief Encodes one component into a byte payload.
	 * @param definition Component definition.
	 * @param data Component instance.
	 * @param payload Receives encoded bytes.
	 * @return True on success.
	 */
	static bool encodeComponent(const ComponentDefinition& definition, const void* data,
		std::vector<std::byte>& payload);

	/**
	 * @brief Encodes a field-delta payload against a baseline.
	 * @param definition Component definition.
	 * @param current Current component instance.
	 * @param baseline Previous component bytes.
	 * @param payload Receives encoded delta bytes.
	 * @return True on success.
	 */
	static bool encodeFieldDelta(const ComponentDefinition& definition, const void* current,
		const void* baseline, std::vector<std::byte>& payload);

	/**
	 * @brief Shared snapshot implementation with an optional visible-id filter.
	 * @param tick Snapshot tick.
	 * @param entities Live entities.
	 * @param visible Null for all entities, otherwise the visible NetId set.
	 * @return Encoded snapshot.
	 */
	SnapshotWriteResult writeImpl(std::uint32_t tick, const ReplicationRegistry& entities,
		const std::unordered_set<std::uint32_t>* visible);

	/**
	 * @brief Removes a tag from both pending stores.
	 * @param tag Tag to remove.
	 * @return True when found.
	 */
	bool removePending(std::uint64_t tag);
};


inline bool SnapshotWriter::encodeComponent(const ComponentDefinition& definition,
	const void* data, std::vector<std::byte>& payload) {
	if (definition.ops == nullptr || definition.ops->encode == nullptr || data == nullptr) {
		return false;
	}
	std::size_t capacity = std::max<std::size_t>(64, definition.size + 64);
	for (int attempt = 0; attempt < 6; ++attempt) {
		payload.resize(capacity);
		Io::BitWriter writer(payload);
		if (definition.ops->encode(writer, data, definition.opsContext)) {
			if (!writer.alignToByte()) return false;
			payload.resize(writer.byteSize());
			return true;
		}
		if (writer.status() != Io::BitStreamStatus::BufferFull) return false;
		if (capacity > std::numeric_limits<std::size_t>::max() / 2) return false;
		capacity *= 2;
	}
	return false;
}

inline bool SnapshotWriter::encodeFieldDelta(const ComponentDefinition& definition,
	const void* current, const void* baseline, std::vector<std::byte>& payload) {
	if (definition.fieldDelta == nullptr || definition.fieldDelta->encode == nullptr
		|| current == nullptr || baseline == nullptr) {
		return false;
	}
	std::size_t capacity = std::max<std::size_t>(64, definition.size + 64);
	for (int attempt = 0; attempt < 6; ++attempt) {
		payload.resize(capacity);
		Io::BitWriter writer(payload);
		if (definition.fieldDelta->encode(writer, current, baseline,
			definition.fieldDeltaContext)) {
			if (!writer.alignToByte()) return false;
			payload.resize(writer.byteSize());
			return true;
		}
		if (writer.status() != Io::BitStreamStatus::BufferFull) return false;
		if (capacity > std::numeric_limits<std::size_t>::max() / 2) return false;
		capacity *= 2;
	}
	return false;
}

inline SnapshotWriteResult SnapshotWriter::writeImpl(std::uint32_t tick,
	const ReplicationRegistry& entities, const std::unordered_set<std::uint32_t>* visible) {
	SnapshotWriteResult result;
	result.tick = tick;
	result.baselineTick = baselineTick_;
	std::vector<PendingRecord> records;
	BaselineMap candidate;
	std::unordered_set<std::uint32_t> seen;
	bool failed = false;
	SnapshotError failure = SnapshotError::None;

	entities.forEach([&](const EntityRecord& entity) {
		if (failed) return;
		if (visible != nullptr && visible->find(entity.id.value()) == visible->end()) return;
		if (registry_.findEntityKind(entity.kind) == nullptr) {
			failed = true;
			failure = SnapshotError::UnknownEntityKind;
			return;
		}
		const auto old = baseline_.find(entity.id);
		const bool spawn = old == baseline_.end() || old->second.kind != entity.kind
			|| old->second.owner != entity.owner;
		BaselineEntity candidateEntity;
		candidateEntity.kind = entity.kind;
		candidateEntity.owner = entity.owner;
		candidateEntity.mask = entity.componentMask;
		PendingRecord record;
		record.type = spawn ? SnapshotRecordType::Spawn : SnapshotRecordType::Update;
		record.id = entity.id;
		record.kind = entity.kind;
		record.owner = entity.owner;

		for (ComponentId id = 0; id < MaxTrackedComponents; ++id) {
			const std::uint64_t bit = 1ull << id;
			const bool attached = (entity.componentMask & bit) != 0;
			if (!attached) {
				if (!spawn && (old->second.mask & bit) != 0) record.removedMask |= bit;
				continue;
			}
			const ComponentDefinition* definition = registry_.findComponent(id);
			if (definition == nullptr || entity.components[id] == nullptr) {
				failed = true;
				failure = SnapshotError::MissingComponent;
				return;
			}
			const bool hadComponent = !spawn && (old->second.mask & bit) != 0;
			const bool hasFieldBaseline = definition->fieldDelta != nullptr
				&& definition->fieldDelta->changed != nullptr && hadComponent
				&& old->second.values[id].size() == definition->size;
			std::vector<std::byte> payload;
			SnapshotComponentMode mode = SnapshotComponentMode::Full;
			bool changed = false;
			if (hasFieldBaseline) {
				changed = definition->fieldDelta->changed(entity.components[id],
					old->second.values[id].data(), definition->fieldDeltaContext);
				if (changed && !encodeFieldDelta(*definition, entity.components[id],
					old->second.values[id].data(), payload)) {
					failed = true;
					failure = SnapshotError::EncodeFailed;
					return;
				}
				if (changed) mode = SnapshotComponentMode::FieldDelta;
			} else {
				if (!encodeComponent(*definition, entity.components[id], payload)) {
					failed = true;
					failure = SnapshotError::EncodeFailed;
					return;
				}
				candidateEntity.payloads[id] = payload;
				changed = spawn || !hadComponent || old->second.payloads[id] != payload;
			}
			if (definition->fieldDelta != nullptr) {
				candidateEntity.values[id].resize(definition->size);
				std::memcpy(candidateEntity.values[id].data(), entity.components[id],
					definition->size);
			}
			if (changed) {
				record.mask |= bit;
				record.components.push_back(PendingComponent { id, mode, std::move(payload) });
			}
		}
		if (spawn) {
			record.mask = entity.componentMask;
			record.removedMask = 0;
		}
		seen.insert(entity.id.value());
		candidate.emplace(entity.id, std::move(candidateEntity));
		if (!spawn && record.mask == 0 && record.removedMask == 0) return;
		records.push_back(std::move(record));
	});

	if (failed) {
		result.error = failure;
		return result;
	}
	std::vector<NetId> despawns;
	for (const auto& entry : baseline_) {
		if (seen.find(entry.first.value()) == seen.end()) despawns.push_back(entry.first);
	}
	std::sort(despawns.begin(), despawns.end(), [](NetId lhs, NetId rhs) {
		return lhs.value() < rhs.value();
	});
	for (NetId id : despawns) {
		PendingRecord record;
		record.type = SnapshotRecordType::Despawn;
		record.id = id;
		records.push_back(std::move(record));
	}
	if (records.size() > std::numeric_limits<std::uint32_t>::max()) {
		result.error = SnapshotError::TooManyRecords;
		return result;
	}


	const std::uint64_t tag = nextTag_++;
	if (nextTag_ == 0) nextTag_ = 1;
	result.tag = tag;

	std::size_t estimate = 16 + records.size() * 32;
	for (const PendingRecord& record : records) {
		for (const auto& component : record.components) estimate += component.payload.size() + 5;
	}
	bool encoded = false;
	for (int attempt = 0; attempt < 6 && !encoded; ++attempt) {
		std::vector<std::byte> output(estimate);
		Io::BitWriter writer(output);
		const auto put = [&writer](std::uint64_t value, unsigned bits) {
			return writer.writeBits(value, bits);
		};
		bool ok = put(SnapshotMagic, 32) && put(SnapshotWireVersion, 8) && put(0, 8)
			&& put(tick, 32) && put(baselineTick_, 32)
			&& put(static_cast<std::uint64_t>(records.size()), 32);
		for (const PendingRecord& record : records) {
			if (!ok) break;
			ok = put(static_cast<std::uint64_t>(record.type), 8)
				&& put(record.id.value(), 32);
			if (!ok) break;
			if (record.type == SnapshotRecordType::Spawn) {
				ok = put(record.kind, 16) && put(record.owner, 32) && put(record.mask, 64);
			} else if (record.type == SnapshotRecordType::Update) {
				ok = put(record.mask, 64) && put(record.removedMask, 64);
			}
			for (const PendingComponent& component : record.components) {
				if (!ok) break;
				ok = writer.alignToByte()
					&& put(static_cast<std::uint64_t>(component.mode), 8)
					&& put(component.payload.size(), 32)
					&& writer.writeBytes(component.payload);
			}
		}
		if (ok) ok = writer.alignToByte();
		if (ok) {
			output.resize(writer.byteSize());
			result.payload = std::move(output);
			result.ok = true;
			encoded = true;
			break;
		}
		if (writer.status() != Io::BitStreamStatus::BufferFull) {
			result.error = SnapshotError::BufferTooSmall;
			return result;
		}
		if (estimate > std::numeric_limits<std::size_t>::max() / 2) {
			result.error = SnapshotError::BufferTooSmall;
			return result;
		}
		estimate *= 2;
	}
	if (!encoded) {
		result.error = SnapshotError::BufferTooSmall;
		return result;
	}

	if (maxPendingBaselines_ != 0 && pending_.size() >= maxPendingBaselines_) {
		while (!pendingOrder_.empty() && pending_.find(pendingOrder_.front()) == pending_.end()) {
			pendingOrder_.pop_front();
		}
		if (!pendingOrder_.empty()) {
			pending_.erase(pendingOrder_.front());
			pendingOrder_.pop_front();
		}
	}
	pending_.emplace(tag, PendingSnapshot { tick, std::move(candidate) });
	pendingOrder_.push_back(tag);
	return result;
}


inline bool SnapshotWriter::removePending(std::uint64_t tag) {
	const auto found = pending_.find(tag);
	if (found == pending_.end()) return false;
	pending_.erase(found);
	pendingOrder_.clear();
	for (const auto& entry : pending_) pendingOrder_.push_back(entry.first);
	return true;
}

inline bool SnapshotWriter::acknowledge(std::uint64_t tag) {
	const auto found = pending_.find(tag);
	if (found == pending_.end()) return false;
	PendingSnapshot snapshot = std::move(found->second);
	pending_.erase(found);
	if (snapshot.tick >= baselineTick_) {
		baseline_ = std::move(snapshot.baseline);
		baselineTick_ = snapshot.tick;
	}
	for (auto iterator = pending_.begin(); iterator != pending_.end();) {
		if (iterator->second.tick <= snapshot.tick) iterator = pending_.erase(iterator);
		else ++iterator;
	}
	pendingOrder_.clear();
	for (const auto& entry : pending_) pendingOrder_.push_back(entry.first);
	return true;
}

inline bool SnapshotWriter::reject(std::uint64_t tag) {
	return removePending(tag);
}

inline void SnapshotWriter::reset() {
	baseline_.clear();
	pending_.clear();
	pendingOrder_.clear();
	baselineTick_ = 0;
	nextTag_ = 1;
}

} // namespace Net
} // namespace Elv
#endif // ELVNETSNAPSHOT_HPP

