#ifndef ELVNETREPLICATIONREGISTRY_HPP
#define ELVNETREPLICATIONREGISTRY_HPP
/**
 * @file ElvNetReplicationRegistry.hpp
 * @brief Non-owning entity/component registry for replication.
 *
 * The registry owns only bookkeeping: a generation-checked `NetId`, the entity
 * kind, the owning peer, an optional game pointer, and a fixed array of
 * component pointers. Component storage remains the game's ECS (or any other
 * data-oriented container); the registry never allocates or destroys it.
 *
 * This keeps the foundation game-agnostic while still giving the replication
 * layer a stable, iterable view of what exists and which components are present.
 */
#include <Elvavena/Net/ElvNetId.hpp>
#include <Elvavena/Net/ElvNetReplicationSchema.hpp>
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace Elv {
namespace Net {

/**
 * @struct EntityRecord
 * @brief One live replicated entity.
 */
struct EntityRecord {
	NetId id;                              ///< Generation-checked handle.
	EntityKindId kind = 0;                 ///< Entity kind.
	PeerId owner = InvalidPeer;            ///< Authoritative/owning peer, if any.
	void* user = nullptr;                  ///< Game-owned payload (ECS row, object, ...).
	std::uint64_t componentMask = 0;       ///< Bit N is set when component N is attached.
	std::array<void*, MaxTrackedComponents> components {}; ///< Non-owning component pointers.

	/**
	 * @brief Returns whether a component is attached.
	 * @param id Component id.
	 */
	[[nodiscard]] bool hasComponent(ComponentId id) const noexcept {
		return id < MaxTrackedComponents && (componentMask & (1ull << id)) != 0;
	}

	/**
	 * @brief Returns the attached component pointer, or null.
	 * @param id Component id.
	 */
	[[nodiscard]] void* component(ComponentId id) const noexcept {
		return hasComponent(id) ? components[id] : nullptr;
	}
};

/**
 * @class ReplicationRegistry
 * @brief Slot table of live entities and their component pointers.
 *
 * @note Not thread-safe. All mutation and iteration must happen on one thread.
 */
class ReplicationRegistry {
public:
	/**
	 * @brief Creates a replicated entity.
	 * @param kind Entity kind.
	 * @param owner Owning peer, or @ref InvalidPeer.
	 * @param user Game-owned payload pointer.
	 * @return New handle, or an invalid handle when the index space is full.
	 */
	[[nodiscard]] NetId spawn(EntityKindId kind, PeerId owner = InvalidPeer, void* user = nullptr) {
		const NetId id = allocator.acquire();
		if (!id.isValid()) return NetId {};
		const std::size_t index = id.index();
		if (slots.size() <= index) {
			slots.resize(index + 1);
			alive.resize(index + 1, 0);
		}
		slots[index] = EntityRecord {};
		slots[index].id = id;
		slots[index].kind = kind;
		slots[index].owner = owner;
		slots[index].user = user;
		alive[index] = 1;
		++liveCount;
		return id;
	}

	/**
	 * @brief Destroys an entity and invalidates its handle.
	 * @param id Entity handle.
	 * @return True when the handle was live.
	 */
	bool destroy(NetId id) {
		EntityRecord* record = find(id);
		if (record == nullptr) return false;
		const std::size_t index = id.index();
		slots[index] = EntityRecord {};
		alive[index] = 0;
		--liveCount;
		return allocator.release(id);
	}

	/**
	 * @brief Attaches a component pointer.
	 * @param id Entity handle.
	 * @param componentId Component id; must be below @ref MaxTrackedComponents.
	 * @param component Non-owning component pointer; must not be null.
	 * @return True on success.
	 */
	bool attach(NetId id, ComponentId componentId, void* component) {
		EntityRecord* record = find(id);
		if (record == nullptr || componentId >= MaxTrackedComponents || component == nullptr) {
			return false;
		}
		record->components[componentId] = component;
		record->componentMask |= (1ull << componentId);
		return true;
	}

	/**
	 * @brief Detaches a component pointer.
	 * @param id Entity handle.
	 * @param componentId Component id.
	 * @return True when the component was attached.
	 */
	bool detach(NetId id, ComponentId componentId) {
		EntityRecord* record = find(id);
		if (record == nullptr || componentId >= MaxTrackedComponents
			|| !record->hasComponent(componentId)) {
			return false;
		}
		record->components[componentId] = nullptr;
		record->componentMask &= ~(1ull << componentId);
		return true;
	}

	/**
	 * @brief Returns the live record for a handle.
	 * @param id Entity handle.
	 * @return Record pointer, or null for a stale/unknown handle.
	 */
	[[nodiscard]] EntityRecord* find(NetId id) {
		if (!id.isValid() || id.index() >= slots.size() || !alive[id.index()]) return nullptr;
		EntityRecord& record = slots[id.index()];
		return record.id == id ? &record : nullptr;
	}

	/**
	 * @brief Returns the live record for a handle.
	 * @param id Entity handle.
	 * @return Record pointer, or null for a stale/unknown handle.
	 */
	[[nodiscard]] const EntityRecord* find(NetId id) const {
		if (!id.isValid() || id.index() >= slots.size() || !alive[id.index()]) return nullptr;
		const EntityRecord& record = slots[id.index()];
		return record.id == id ? &record : nullptr;
	}

	/** @brief Returns the number of live entities. */
	[[nodiscard]] std::size_t size() const noexcept { return liveCount; }

	/** @brief Returns whether a handle is currently live. */
	[[nodiscard]] bool isAlive(NetId id) const { return find(id) != nullptr; }

	/**
	 * @brief Visits every live record in slot order.
	 * @tparam Fn Callable accepting `const EntityRecord&`.
	 * @param function Visitor.
	 */
	template <typename Fn>
	void forEach(Fn&& function) const {
		for (std::size_t index = 0; index < slots.size(); ++index) {
			if (alive[index]) function(static_cast<const EntityRecord&>(slots[index]));
		}
	}

	/**
	 * @brief Visits every live record in slot order.
	 * @tparam Fn Callable accepting `EntityRecord&`.
	 * @param function Visitor.
	 */
	template <typename Fn>
	void forEachMutable(Fn&& function) {
		for (std::size_t index = 0; index < slots.size(); ++index) {
			if (alive[index]) function(slots[index]);
		}
	}

	/** @brief Destroys all entities and resets id generations. */
	void clear() {
		slots.clear();
		alive.clear();
		allocator.reset();
		liveCount = 0;
	}

private:
	NetIdAllocator allocator;         ///< Slot index/generation source.
	std::vector<EntityRecord> slots;  ///< Entity records by slot index.
	std::vector<std::uint8_t> alive;  ///< Liveness flag per slot.
	std::size_t liveCount = 0;        ///< Number of live entities.
};

} // namespace Net
} // namespace Elv

#endif // ELVNETREPLICATIONREGISTRY_HPP

