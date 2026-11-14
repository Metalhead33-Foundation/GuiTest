#ifndef EUPHASSETTYPES_H
#define EUPHASSETTYPES_H
/**
 * @file EuphAssetTypes.hpp
 * @brief Defines the small value types used by Euphemy's asset streaming runtime.
 *
 * @section euph_asset_types_role Role in the module
 *
 * This header contains only *passive data*: the asset identifier, the residency and
 * failure enumerations, the policy flag set, the priority value and the @ref
 * Euph::Asset::AssetRecord catalog descriptor. It depends on nothing but the standard
 * library, so build steps, editors, cookers and other libraries can describe assets
 * without including the runtime in `EuphAssetRuntime.hpp`.
 *
 * The split between this header and the runtime is deliberate:
 *
 * | Concern | Declared here | Implemented in `EuphAssetRuntime.hpp` |
 * |---|---|---|
 * | What an asset *is* (`AssetRecord`) | yes | catalog storage and lookup |
 * | Which states exist (`ResidencyState`) | yes | the transition table and the table of entries |
 * | Why something failed (`FailureReason`) | yes | the code that records the reason |
 * | How important something is (`StreamPriority`) | yes | queue ordering and eviction ordering |
 *
 * @section euph_asset_types_units Units, identifiers and ownership
 *
 * - **Identifiers.** @ref Euph::Asset::AssetId is a 64-bit unsigned integer. The value
 *   `0` is reserved as "no asset" and is rejected by every component that accepts an
 *   id, so `AssetId` itself is only an identifier, not a validity check. Nothing in
 *   Euphemy assigns ids; they come from whatever produced the catalog (a cooker, a
 *   package index, or a hand-written table).
 * - **Byte counts.** Every `...Bytes`, `storedSize`, `decodedSize` and budget value is a
 *   count of `std::byte`, never a count of elements, frames or samples.
 * - **Pixels/samples.** This header says nothing about payload layout. The meaning of a
 *   payload is carried by the free-form `AssetRecord::type` string and by the type tag
 *   of a `ResidentPayload`; those strings are an agreement between the producer of the
 *   catalog and the `Decoder` callback, not something Euphemy interprets (with the one
 *   exception documented on @ref Euph::Asset::AssetRecord::type).
 * - **Ownership.** All of these types are values. Copying a @ref
 *   Euph::Asset::AssetRecord copies its strings and dependency list; copy cost is
 *   therefore proportional to the metadata size, which is why the runtime stores
 *   records in an `std::unordered_map` and hands out pointers/spans.
 *
 * @section euph_asset_types_example Putting the values together
 *
 * @code
 * using namespace Euph::Asset;
 *
 * AssetRecord atlas;
 * atlas.id = 42;
 * atlas.path = "gfx/ui.atlas";
 * atlas.offset = 4096;          // byte offset inside the device the loader opens
 * atlas.storedSize = 1024;      // 0 would mean "read until the end of the device"
 * atlas.compression = Compression::Zstd;
 * atlas.type = "image";         // maps to ResourceKind::Image
 *
 * AssetRecord page;
 * page.id = 43;
 * page.path = "gfx/ui.atlas";
 * page.type = "blob";
 * page.dependencies.push_back(42);  // must be resident before this one is decoded
 * @endcode
 *
 * @see Euph::Asset::AssetRecord, Euph::Asset::ResidencyState, Euph::Asset::ResidencyFlag
 * @see Euph::Asset::AssetCatalog, Euph::Asset::ResourceRegistry, Euph::Asset::StreamingScheduler
 */
#include <cstdint>
#include <string>
#include <vector>
namespace Euph {
namespace Asset {

/**
 * @brief Stable, catalog-level identifier for an asset record or resource entry.
 *
 * The value is chosen by the catalog producer and must be unique within one
 * @ref Euph::Asset::AssetCatalog; the catalog rejects records with id `0`. The registry
 * uses the id as its primary key and the runtime never rewrites it.
 *
 * @note Ids are also the tie-breaker for deterministic scheduling decisions: when two
 *       requests have equal priority, the lower id is started first, and when two
 *       eviction candidates have equal priority, age and cost, the lower id is evicted
 *       first. Choosing dense, monotonically assigned ids therefore makes behaviour
 *       reproducible.
 *
 * @warning An id is *not* a generation. A recycled `AssetId` (the same number describing
 *          a different asset after a catalog rebuild) cannot be detected by the runtime;
 *          use @ref Euph::Asset::ResourceHandle, which carries a
 *          @ref Euph::Asset::ResourceRegistry::Entry::generation, for that.
 */
using AssetId = std::uint64_t;

/**
 * @brief High-level lifetime state for a CPU-side streamed asset.
 *
 * These states intentionally describe Euphemy's non-GPU resource lifecycle.
 * Renderer/backend upload state is represented outside Euphemy (Lotte's
 * `GpuResidencyState` is the matching GPU-side ladder).
 *
 * @section euph_asset_state_ladder The nominal ladder
 *
 * @code
 * Missing ──► Known ──► Requested ──► LoadingIO ──► Decoding ──► Resident
 *                                                                  │   ▲
 *                                                                  ▼   │
 *                                                              Evictable
 *                                                                  │
 *                                                                  ▼
 *                                                   Evicting ──► Known ──► (cycle)
 * @endcode
 *
 * Two detours exist. A request whose dependencies are not resident yet parks in
 * @ref ResidencyState::WaitingDependencies until they are (the scheduler wakes it automatically), and
 * integrations that need an explicit hand-off boundary (for example a renderer that must
 * batch its uploads into one frame) may park a decoded asset in @ref ResidencyState::WaitingCommit and
 * then move it through @ref ResidencyState::Committing. Euphemy's own
 * @ref Euph::Asset::StreamingScheduler never uses those two states; it commits straight
 * from @ref ResidencyState::Decoding to @ref ResidencyState::Resident.
 *
 * @section euph_asset_state_legal Legal transitions
 *
 * The registry enforces the table below in
 * @ref Euph::Asset::ResourceRegistry::setState. Any transition not listed is rejected and
 * records @ref FailureReason::InvalidTransition without changing the state. Two rules
 * apply everywhere: staying in the same state is a no-op that succeeds, and **every**
 * state may move to @ref ResidencyState::Failed. (The failure helper
 * @ref Euph::Asset::ResourceRegistry::fail bypasses the table entirely and always
 * succeeds, because a failure must be recordable from any state, including states with no
 * legal successors.)
 *
 * | From | Legal targets |
 * |---|---|
 * | @ref ResidencyState::Missing | @ref ResidencyState::Known |
 * | @ref ResidencyState::Known | `Requested`, `WaitingDependencies`, `Missing` |
 * | @ref ResidencyState::Requested | `WaitingDependencies`, `LoadingIO`, `Known` |
 * | @ref ResidencyState::WaitingDependencies | `Requested`, `LoadingIO`, `Known` |
 * | @ref ResidencyState::LoadingIO | `Decoding`, `Known` |
 * | @ref ResidencyState::Decoding | `WaitingCommit`, `Resident`, `Known` |
 * | @ref ResidencyState::WaitingCommit | `Committing`, `Resident`, `Known` |
 * | @ref ResidencyState::Committing | `Resident`, `Known` |
 * | @ref ResidencyState::Resident | `Evictable`, `Evicting` |
 * | @ref ResidencyState::Evictable | `Resident`, `Evicting`, `Requested` |
 * | @ref ResidencyState::Evicting | `Known`, `Missing` |
 * | @ref ResidencyState::Failed | `Requested`, `Known`, `Missing` |
 *
 * The `... -> Known` edges are the "abandon and start over" path: they keep the entry (and
 * therefore its id, slot and generation) alive while dropping the claim that anything is
 * resident. Euphemy uses them when a resource is evicted or a retried request is reset.
 *
 * @note `Resident -> Requested` is *not* legal; a resident asset must pass through `Evictable`
 *       (or be destroyed) before it can be requested again. In practice a request for a resident
 *       asset is simply satisfied without a transition, which is what
 *       @ref Euph::Asset::StreamingScheduler::request does.
 */
enum class ResidencyState : std::uint8_t {
	Missing,              /**< No active registry entry or no known loaded state. */
	Known,                /**< Metadata exists, but the asset has not been requested. */
	Requested,            /**< A load request has been accepted but work has not begun. */
	WaitingDependencies,  /**< The asset is blocked on required dependencies. */
	LoadingIO,            /**< Bytes are being read from an I/O device. */
	Decoding,             /**< Raw bytes are being transformed into a resident payload. */
	WaitingCommit,        /**< Decoded data is waiting for an external commit boundary. */
	Committing,           /**< The external commit boundary is currently being processed. */
	Resident,             /**< The payload is available to callers. */
	Evictable,            /**< The payload is resident and may be removed by policy. */
	Evicting,             /**< Eviction has begun but is not fully committed. */
	Failed,               /**< Loading, decoding, dependency resolution, or commit failed. */

	/** @name Compatibility aliases */
	///@{
	Unloaded = Missing,            /**< Alias of @ref ResidencyState::Missing ("unloaded"). */
	Queued = Requested,            /**< Alias of @ref ResidencyState::Requested ("queued"). */
	WaitingUpload = WaitingCommit, /**< Alias of @ref ResidencyState::WaitingCommit. */
	Uploading = Committing         /**< Alias of @ref ResidencyState::Committing. */
	///@}
};

/**
 * @brief Machine-readable reason for a terminal or diagnostic resource failure.
 *
 * A reason is attached to a registry entry by
 * @ref Euph::Asset::ResourceRegistry::fail, together with a human-readable message, and is
 * cleared again as soon as the entry leaves @ref ResidencyState::Failed (see
 * @ref Euph::Asset::ResourceRegistry::setState).
 *
 * @note @ref Euph::Asset::ResourceRegistry::failureReason returns
 *       @ref FailureReason::AssetNotFound for an id the registry does not know, and
 *       @ref FailureReason::None for a live entry that never failed. Callers that only care
 *       whether something failed should compare the state instead.
 * @note The scheduler reports a missing callback and a callback that threw with the same reason
 *       (@ref FailureReason::IoError during load, @ref FailureReason::DecodeError during decode);
 *       read the message when the distinction matters.
 */
enum class FailureReason : std::uint8_t {
	None,               /**< No failure is recorded. */
	AssetNotFound,      /**< The requested asset id does not exist in the catalog. */
	MissingDependency,  /**< A declared dependency is absent from the catalog. */
	DependencyCycle,    /**< Dependency traversal found a cycle. */
	InvalidTransition,  /**< A caller attempted an illegal residency state transition. */
	IoError,            /**< Raw byte loading failed. */
	DecodeError,        /**< Decode/decompression failed. */
	CommitError,        /**< The external commit boundary rejected the resource. */
	Canceled,           /**< The request was canceled before completion. */
	BudgetExceeded,     /**< A residency or inflight budget prevented progress. */
	Unknown,            /**< An uncategorized failure occurred. */
	UploadError = CommitError /**< Alias of @ref FailureReason::CommitError for upload-boundary callers. */
};

/**
 * @brief Policy and diagnostic flags attached to a resource registry entry.
 *
 * The flags form a bitset: combine them with `|`, test them with
 * @ref Euph::Asset::hasFlag. `ResidencyFlag::None` is the empty set and is therefore never
 * "present".
 *
 * Ownership is split by intent:
 *
 * - **Caller-managed policy.** @ref ResidencyFlag::Pinned,
 *   @ref ResidencyFlag::PlaceholderAllowed and @ref ResidencyFlag::Reloadable are read by policy
 *   code (the eviction pass honours `Pinned`) but are never set by the runtime itself. They
 *   describe what the *application* is willing to allow.
 * - **Runtime-managed state.** @ref ResidencyFlag::CpuResident is maintained by
 *   @ref Euph::Asset::ResourceRegistry::setPayload and `clearPayload`, so it always agrees with
 *   whether a payload is stored. @ref ResidencyFlag::EvictionRequested is currently only ever
 *   *cleared* (by @ref Euph::Asset::ResidencyManager::evictUntilWithinBudgetDetailed); treat it
 *   as reserved and do not depend on it being set.
 * - **Integration-managed state.** @ref ResidencyFlag::ExternalResident is a convention for layers
 *   that own a copy elsewhere (a GPU upload, a decoded audio buffer). Euphemy neither sets nor
 *   clears it.
 *
 * @warning The eviction pass only consults @ref ResidencyFlag::Pinned and the strong reference
 *          count; the remaining flags are advisory and never affect budgets.
 */
enum class ResidencyFlag : std::uint32_t {
	None = 0,                    /**< No flags are set. */
	Pinned = 1u << 0,            /**< The resource must not be evicted by policy. */
	PlaceholderAllowed = 1u << 1,/**< A typed placeholder may be substituted. */
	Reloadable = 1u << 2,        /**< The resource can be loaded again after eviction. */
	CpuResident = 1u << 3,       /**< The CPU-side payload is resident. */
	ExternalResident = 1u << 4,  /**< Some external system has a resident copy. */
	EvictionRequested = 1u << 5  /**< Eviction has been requested but not completed. */
};

/**
 * @brief Compression format declared by an asset record's stored bytes.
 *
 * This is a *declaration*, not an instruction: neither
 * @ref Euph::Asset::StreamingScheduler nor any other part of Euphemy decompresses stored
 * bytes. The Decoder callback receives the raw bytes exactly as the loader produced them
 * and is responsible for interpreting this field (or ignoring it, if the loader already
 * returned decompressed data - `Io/EuphZstd.hpp` provides a decompressing read proxy for
 * that case).
 */
enum class Compression {
	None, /**< Stored bytes are uncompressed. */
	Zstd  /**< Stored bytes use Zstandard compression. */
};

/**
 * @brief Integer priority used to order streaming and eviction decisions.
 *
 * The wrapper exists so that the ordering direction stays explicit at every call site:
 * **larger values mean more important**. That single convention is then applied
 * consistently:
 *
 * - the scheduler starts the highest-priority queued request first
 *   (@ref Euph::Asset::StreamingScheduler::request path);
 * - the eviction pass evicts the *lowest*-priority candidate first
 *   (@ref Euph::Asset::ResidencyManager::evictUntilWithinBudgetDetailed).
 *
 * Ties are broken deterministically by the runtime (see @ref Euph::Asset::AssetId), so a
 * priority value never needs to be unique.
 *
 * @note The default value `0` is also what the registry reports for entries that never had
 *       a priority set, so `0` doubles as "unspecified". Give background work a negative
 *       value instead of relying on `0` meaning "lowest", because an explicit `0` cannot
 *       be told apart from the default.
 */
struct StreamPriority {
	/** @brief Larger values are treated as more important by schedulers. */
	int value = 0;

	/**
	 * @brief Orders priorities by numeric value.
	 * @param right Priority to compare against.
	 * @return `true` when this priority is *less* important than @p right.
	 */
	constexpr bool operator<(const StreamPriority& right) const {
		return value < right.value;
	}

	/**
	 * @brief Compares priority values.
	 * @param right Priority to compare against.
	 * @return `true` when both priorities carry the same value.
	 */
	constexpr bool operator==(const StreamPriority& right) const = default;
};

/**
 * @brief Metadata for locating, decoding, and dependency-ordering one asset.
 *
 * An AssetRecord is descriptive only; actual lifetime, resident payloads, and budgets live
 * in @ref Euph::Asset::ResourceRegistry and @ref Euph::Asset::ResidencyManager.
 *
 * @section euph_asset_record_flow What consumes which field
 *
 * | Field | Consumer | Notes |
 * |---|---|---|
 * | @ref id | catalog, registry, scheduler | primary key; non-zero and unique |
 * | @ref path | the application's Loader callback | never interpreted by Euphemy |
 * | `offset`, `storedSize` | the Loader callback / the read job | the read window |
 * | `compression` | the Decoder callback | declared only |
 * | `decodedSize` | the application (budgeting) | informational; the runtime ignores it |
 * | @ref type | the scheduler, the Decoder callback | decides the initial `ResourceKind` |
 * | `dependencies` | @ref Euph::Asset::DependencyGraph, the scheduler | ordering only |
 *
 * `type` has exactly one meaning inside Euphemy: the scheduler maps a few well-known
 * strings to a @ref Euph::Asset::ResourceKind so that typed handles resolve correctly.
 *
 * | `type` | @ref Euph::Asset::ResourceKind |
 * |---|---|
 * | `image`, `texture`, `png`, `jpg`, `jpeg`, `tga`, `dds` | `ResourceKind::Image` |
 * | `audio`, `sound`, `music`, `wav`, `ogg`, `module` | `ResourceKind::Audio` |
 * | anything else | `ResourceKind::Blob` |
 *
 * A decoder may still report a different kind in
 * @ref Euph::Asset::DecodedAsset::kind; the scheduler then re-types the entry, so `type`
 * only has to be right at request time.
 *
 * @section euph_asset_record_readwindow How the bytes are read
 *
 * The scheduler hands the record to the Loader callback, which returns an open device, and
 * then reads @ref AssetRecord::storedSize bytes starting at @ref AssetRecord::offset
 *
 * - an `offset` beyond the device size is an error (@ref FailureReason::IoError);
 * - `storedSize == 0` means "read to the end of the device", i.e. `device.size() - offset`;
 * - a `storedSize` larger than what remains is an error;
 * - a read that ends early is an error: the read job loops until the requested count is
 *   filled and does not accept short reads.
 *
 * Several records may therefore point at the same `path` and differ only in `offset` and
 * `storedSize`, which is the intended way to describe packed archives.
 *
 * @section euph_asset_record_immutability Mutability
 *
 * @ref Euph::Asset::AssetCatalog::addRecord takes the record **by value** and stores it,
 * and the scheduler copies it into every job. Mutating a record after handing it to the
 * catalog has no effect on anything.
 */
struct AssetRecord {
	/** @brief Stable catalog identifier. */
	AssetId id = 0;
	/** @brief Optional device/path hint used by loaders. */
	std::string path;
	/** @brief Byte offset inside the loader-provided device. */
	std::uint64_t offset = 0;
	/** @brief Number of stored bytes to read from offset; `0` means "to the end of the device". */
	std::uint64_t storedSize = 0;
	/** @brief Expected decoded byte cost, if known. Informational: the runtime never reads it. */
	std::uint64_t decodedSize = 0;
	/** @brief Compression declared for the stored bytes; the Decoder callback acts on it. */
	Compression compression = Compression::None;
	/** @brief Free-form type string used by decoders and integration layers. */
	std::string type;
	/** @brief Direct dependencies that must be resident before this asset. */
	std::vector<AssetId> dependencies;

	/**
	 * @brief Compares all catalog metadata fields.
	 * @param right Record to compare against.
	 * @return `true` when every field (including the ordered dependency list) is equal.
	 */
	bool operator==(const AssetRecord& right) const = default;
};

/**
 * @defgroup euph_asset_flag_ops ResidencyFlag bit operations
 * @brief Set algebra over @ref Euph::Asset::ResidencyFlag.
 *
 * The operators exist so that policy code can be written without casts:
 *
 * @code
 * using namespace Euph::Asset;
 * ResidencyFlag flags = ResidencyFlag::Pinned | ResidencyFlag::Reloadable;
 * flags |= ResidencyFlag::Reloadable;                 // union
 * const bool pinned = hasFlag(flags, ResidencyFlag::Pinned);
 * const bool both = hasFlag(flags, ResidencyFlag::Pinned | ResidencyFlag::Reloadable);
 * flags = flags & ~ResidencyFlag::Reloadable;         // no operator~, clear via the registry
 * @endcode
 *
 * There is deliberately no `operator~`: clearing a flag goes through
 * @ref Euph::Asset::ResourceRegistry::setFlag, which is the only place that owns entry
 * storage.
 * @{
 */

/**
 * @brief Combines two ResidencyFlag bitmasks.
 * @param left Bitmask to add to.
 * @param right Bitmask whose bits are added.
 * @return Union of @p left and @p right.
 */
constexpr ResidencyFlag operator|(ResidencyFlag left, ResidencyFlag right)
{
	return static_cast<ResidencyFlag>(static_cast<std::uint32_t>(left) | static_cast<std::uint32_t>(right));
}

/**
 * @brief Intersects two ResidencyFlag bitmasks.
 * @param left Bitmask to intersect.
 * @param right Bitmask to intersect with.
 * @return Bits present in both @p left and @p right.
 */
constexpr ResidencyFlag operator&(ResidencyFlag left, ResidencyFlag right)
{
	return static_cast<ResidencyFlag>(static_cast<std::uint32_t>(left) & static_cast<std::uint32_t>(right));
}

/**
 * @brief Adds flags to an existing ResidencyFlag bitmask.
 * @param left Bitmask that is modified in place.
 * @param right Bitmask whose bits are added.
 * @return Reference to @p left, for chaining.
 */
inline ResidencyFlag& operator|=(ResidencyFlag& left, ResidencyFlag right)
{
	left = left | right;
	return left;
}

/**
 * @brief Returns true when a flag is present in a bitmask.
 * @param flags Bitmask to test.
 * @param flag Flag (or union of flags) to look for.
 * @return `true` when every bit of @p flag is set in @p flags. Because
 *         `ResidencyFlag::None` has no bits, testing for it always returns `false`.
 */
constexpr bool hasFlag(ResidencyFlag flags, ResidencyFlag flag)
{
	return (flags & flag) != ResidencyFlag::None;
}

/** @} */

}
}
#endif // EUPHASSETTYPES_H
