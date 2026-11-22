#ifndef ELVNETINTEREST_HPP
#define ELVNETINTEREST_HPP
/**
 * @file ElvNetInterest.hpp
 * @brief Hook for filtering which entities are relevant to a peer.
 *
 * Interest management is deliberately a batch hook: the session/replication
 * layer asks once per peer per snapshot for the relevant entity ids. A spatial
 * grid, an octree or a gameplay visibility rule can implement this without the
 * foundation hardcoding distance, rooms or team logic.
 */
#include <Elvavena/Net/ElvNetReplicationRegistry.hpp>
#include <Elvavena/Net/ElvNetTypes.hpp>
#include <cstddef>
#include <vector>

namespace Elv {
namespace Net {

/**
 * @class InterestManager
 * @brief Selects the entities visible to one peer.
 */
class InterestManager {
public:
	/** @brief Virtual destructor. */
	virtual ~InterestManager() = default;

	/**
	 * @brief Appends the ids relevant to a peer.
	 * @param peer Viewer.
	 * @param registry All live entities.
	 * @param out Destination vector; the caller clears it before the call.
	 */
	virtual void collectRelevant(PeerId peer, const ReplicationRegistry& registry,
		std::vector<NetId>& out) = 0;

	/**
	 * @brief Optional per-tick update for spatial indices or caches.
	 * @param tick Current tick.
	 */
	virtual void update(Tick tick) {
		(void)tick;
	}
};

/**
 * @class ReplicateAllInterest
 * @brief Default policy: every peer sees every entity.
 */
class ReplicateAllInterest final : public InterestManager {
public:
	/** @brief Appends every live entity id. */
	void collectRelevant(PeerId, const ReplicationRegistry& registry,
		std::vector<NetId>& out) override {
		registry.forEach([&out](const EntityRecord& entity) { out.push_back(entity.id); });
	}
};

/**
 * @class OwnerInterest
 * @brief Replicates owned and unowned entities to every peer.
 *
 * This is a useful small policy for co-op or shared-world prototypes. It is a
 * convenience, not the foundation's opinion: games can replace it with a
 * spatial or gameplay-specific implementation.
 */
class OwnerInterest final : public InterestManager {
public:
	/** @brief Appends entities owned by @p peer or by nobody. */
	void collectRelevant(PeerId peer, const ReplicationRegistry& registry,
		std::vector<NetId>& out) override {
		registry.forEach([peer, &out](const EntityRecord& entity) {
			if (entity.owner == peer || entity.owner == InvalidPeer) out.push_back(entity.id);
		});
	}
};

} // namespace Net
} // namespace Elv

#endif // ELVNETINTEREST_HPP
