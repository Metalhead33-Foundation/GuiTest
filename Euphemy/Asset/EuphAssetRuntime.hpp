#ifndef EUPHASSETRUNTIME_HPP
#define EUPHASSETRUNTIME_HPP
/**
 * @file EuphAssetRuntime.hpp
 * @brief CPU-side asset catalog, dependency, residency, and streaming runtime.
 *
 * This module deliberately owns genre-independent, non-GPU asset lifetime: catalog
 * metadata, typed handles, dependency ordering, budget accounting, retained CPU payloads,
 * and background I/O/decode scheduling. Renderer upload and GPU residency are integration
 * concerns outside Euphemy.
 *
 * @section euph_asset_runtime_overview What this module is (and is not)
 *
 * The runtime answers four questions about a set of assets described by
 * @ref Euph::Asset::AssetRecord values:
 *
 * 1. **What exists?** @ref Euph::Asset::AssetCatalog stores the records and is
 *    authoritative for ids, paths and dependency edges.
 * 2. **In what order may it load?** @ref Euph::Asset::DependencyGraph walks the catalog,
 *    detects missing dependencies and cycles, and returns a load-before-parent order.
 * 3. **What is resident right now?** @ref Euph::Asset::ResourceRegistry is the mutable
 *    table of entries: residency state, generation-checked handles, flags, reference
 *    counts, byte costs and the optional CPU payload.
 * 4. **What may be thrown away?** @ref Euph::Asset::ResidencyManager compares recorded
 *    costs against per-category budgets and evicts the cheapest-to-lose candidates.
 *
 * @ref Euph::Asset::StreamingScheduler is the only piece that *does* something over time:
 * it turns a request into a dependency-ordered background read, then a decode, and finally
 * a resident payload in the registry.
 *
 * Deliberately **not** in this module:
 *
 * - **No concrete I/O.** Devices come from the application through the
 *   `StreamingScheduler::Loader` callback (typically `Euph::Io::File`,
 *   `Euph::Io::PhysFS` or a memory device).
 * - **No decoders.** Image/audio decoding is supplied through the
 *   `StreamingScheduler::Decoder` callback, which may use the `Euphemy/Media` codecs.
 * - **No GPU knowledge.** There is no texture, buffer, Kaldi handle or upload queue here.
 *   Lotte's `Lotte::Asset::GpuAssetStreamer` sits *on top* of a @ref
 *   Euph::Asset::ResourceRegistry and consumes its payloads.
 * - **No threading of its own.** The scheduler posts work to an `Elv::Util::ThreadPool`
 *   that the caller owns.
 * - **No serialization.** Nothing here reads or writes a file format for its own state.
 *
 * @section euph_asset_runtime_pieces How the pieces fit together
 *
 * @code
 *                     +---------------------+
 *                     |     AssetCatalog    |  records + path index (read-only view)
 *                     +---------------------+
 *                        ^              |
 *       addRecord/       |              |  dependenciesOf()
 *       removeRecord     |              v
 *                     +---------------------+        +-----------------------+
 *                     |   DependencyGraph   |        |   StreamingScheduler  |
 *                     +---------------------+  <---  |  queued / waiting /   |
 *                                |                   |  running jobs         |
 *                                |                   +-----------------------+
 *                                v                        |          ^
 *                     +---------------------+             |          | pollCompletions()
 *                     |  ResourceRegistry   |  <----------+          |
 *                     |  entries: state,    |                        |
 *                     |  refs, costs,       |             +-----------------------+
 *                     |  payload            |  <-------   |   ResidencyManager    |
 *                     +---------------------+             |  budgets + eviction   |
 *                                |                        +-----------------------+
 *                                v
 *                     application / GPU layer (Lotte::Asset::GpuAssetStreamer, ...)
 * @endcode
 *
 * The intended construction order is catalog first (immutable afterwards), then registry,
 * then the residency manager, then the scheduler. All four refer to each other by reference
 * or pointer and must outlive their users - see @ref euph_asset_runtime_lifetime.
 *
 * @section euph_asset_runtime_lifecycle Who moves which state
 *
 * The states and their legal transitions are documented on
 * @ref Euph::Asset::ResidencyState. This table answers the complementary question: which
 * component is responsible for each move.
 *
 * | Transition | Performed by |
 * |---|---|
 * | (none) -> Known | `ResourceRegistry::create` / `createHandle` |
 * | Known -> Requested | `StreamingScheduler::request` (queueing) |
 * | Requested -> WaitingDependencies | `StreamingScheduler` (dependency gate) |
 * | Requested/WaitingDependencies -> LoadingIO | `StreamingScheduler` when an I/O slot frees up |
 * | LoadingIO -> Decoding | `StreamingScheduler` when the read finished |
 * | Decoding -> Resident | `StreamingScheduler` when the decode finished |
 * | Resident -> Evictable | `ResidencyManager::makeEvictable` (an application decision) |
 * | Evictable -> Evicting -> Known | `ResidencyManager::evictUntilWithinBudgetDetailed` |
 * | anything -> Failed | `StreamingScheduler` on error/cancel, or the application |
 * | WaitingCommit / Committing | *not used by Euphemy*: reserved for commit-boundary integrations |
 *
 * @section euph_asset_runtime_threading Threading contract
 *
 * **Nothing in this header is thread-safe.** The catalog, the registry, the residency
 * manager and the scheduler are plain objects with no locks, and they all assume a single
 * thread mutates them.
 *
 * Parallelism happens strictly inside the scheduler, and it is deliberately narrow:
 *
 * - `Elv::Util::ThreadPool` runs exactly two kinds of task: the byte read for one asset
 *   (device + record + stop token captured **by value**) and the decode callback for one
 *   asset (record + bytes + decoder captured **by value**). Neither task touches the
 *   catalog, the registry or the scheduler, so no lock is needed.
 * - The **Loader** callback runs on the calling thread, inside
 *   `StreamingScheduler::pollCompletions` -> `startQueuedRequests`. It may therefore open
 *   files on the thread that owns the device subsystem; it must not block for long,
 *   because it delays every other completion in that poll.
 * - The **Decoder** callback runs on a pool thread. It must not touch the registry; it only
 *   receives the record and the bytes and returns a
 *   @ref Euph::Asset::DecodedAsset.
 * - State only advances when the owning thread calls `StreamingScheduler::pollCompletions`.
 *   Nothing is delivered by callbacks, futures or threads: a job that finished on a worker
 *   stays invisible until the next poll.
 *
 * Two practical consequences: read residency states and payloads only from the thread that
 * polls, and if another thread must observe progress, hand it results through your own
 * queue or synchronize the registry externally.
 *
 * @section euph_asset_runtime_lifetime Object lifetime
 *
 * - The scheduler stores references to the catalog, the registry and the thread pool, plus
 *   copies of the two callbacks. All of them must outlive the scheduler.
 * - Destroying the scheduler does **not** wait for its background jobs: the futures are
 *   abandoned, so their results (and their registry updates) never happen. Destroy the
 *   scheduler before the pool and before the registry; destroying the pool itself does wait
 *   for running tasks to finish.
 * - Records handed to the catalog are copied, but the *pointers* returned by
 *   `AssetCatalog::find` and `ResourceRegistry::inspect` (and the spans returned by
 *   `AssetCatalog::dependenciesOf`) are views into container storage. They are invalidated
 *   by any operation that inserts or erases entries: `addRecord`, `removeRecord`, `clear`,
 *   `create`, `createHandle`, `destroy`. Re-fetch instead of caching them across such a
 *   call.
 * - `ResourceLease` is the RAII wrapper around a strong reference; use it for any CPU-side
 *   access that spans more than a single call - see @ref euph_asset_runtime_ownership.
 *
 * @section euph_asset_runtime_ownership Payload ownership
 *
 * A @ref Euph::Asset::ResidentPayload owns its object through `std::shared_ptr<void>`, so a
 * *copy* of the payload keeps the data alive independently of the registry entry. That is
 * what makes the GPU hand-off safe: an uploader can take a copy, let the entry be evicted,
 * and still use the bytes until the copy is dropped (Lotte's
 * `GpuCommandBatch::retainedPayloads` does exactly that).
 *
 * Strong references (`retain` / `release`, or a `ResourceLease`) are a different mechanism:
 * they do not own the payload, they only tell the eviction policy that somebody is using
 * the resource. Both are usually needed together:
 *
 * @code
 * // Keep the entry alive while we hold the payload, then keep the payload alive alone.
 * Euph::Asset::ResourceLease<Euph::Asset::ImageResource> lease(registry, handle);
 * const Euph::Asset::ResidentPayload* payload = registry.payload(handle.id);
 * if (payload) {
 *     Euph::Asset::ResidentPayload held = *payload;  // survives eviction of the entry
 *     // ... use held.data ...
 * }  // lease released here; `held` still owns the bytes until it goes out of scope
 * @endcode
 *
 * @section euph_asset_runtime_limits Known behaviour worth knowing
 *
 * - **The scheduler never enforces budgets.** It records `IoInflightBytes` and
 *   `DecodeInflightBytes` costs while work is in flight and the decoded costs on commit,
 *   but it never asks @ref Euph::Asset::ResidencyManager whether it may proceed. Budgets
 *   only affect eviction, which is a separate call
 *   (`ResidencyManager::evictUntilWithinBudget`).
 * - **Eviction only considers `Evictable` entries.** A `Resident` entry is never evicted,
 *   however far over budget the registry is; the application must mark it with
 *   `ResidencyManager::makeEvictable` first. Eviction is therefore predictable but manual.
 * - **A failed dependency strands its dependents.** A waiting request is only woken when
 *   all of its dependencies reach `Resident` or `Evictable`; a dependency that ends in
 *   `Failed` leaves the dependent in `WaitingDependencies` indefinitely. Check the
 *   dependency's state yourself, or cancel the dependent, when a dependency fails.
 * - **Cancellation is cooperative.** `cancel` removes a queued/waiting request immediately
 *   and requests a stop on a running read, but the job stays in the scheduler until the
 *   next `pollCompletions`. A read that observes the stop throws, so the entry usually ends
 *   up `Failed` with @ref Euph::Asset::FailureReason::IoError and the message
 *   `"Streaming read canceled"` rather than @ref Euph::Asset::FailureReason::Canceled. A
 *   decode that already started is not interruptible; its result is discarded.
 * - **Whole-record reads only.** There is no ranged or progressive loading: one request
 *   reads exactly `storedSize` bytes into one contiguous buffer, then decodes them in one
 *   call. Streaming a large file incrementally is an application concern.
 * - **Priorities are sampled, not updated in flight.** A queued request is started
 *   according to the priority stored in the registry at start time; changing it later only
 *   affects requests that have not started.
 *
 * @section euph_asset_runtime_integration Integration with a GPU layer
 *
 * The CPU runtime stops at "payload resident in the registry". An uploader on top is
 * expected to:
 *
 * 1. watch entries for `Resident` state (poll, or drive it from the loop that calls
 *    `pollCompletions`);
 * 2. read `ResourceRegistry::payload`, copy the payload, and record upload commands;
 * 3. commit the upload at a well-defined boundary and mark its own external state
 *    (@ref Euph::Asset::ResidencyFlag::ExternalResident is the conventional flag for "a
 *    copy exists elsewhere");
 * 4. call `ResidencyManager::makeEvictable` once the CPU copy is no longer the only one -
 *    or release it explicitly with `ResourceRegistry::clearPayload`.
 *
 * `Lotte::Asset::GpuAssetStreamer` is the reference implementation of that list, including
 * its own `GpuResidencyState` ladder, its own budget and its own failure reasons.
 *
 * @section euph_asset_runtime_usage Worked example
 *
 * @code
 * using namespace Euph;
 * using namespace Euph::Asset;
 *
 * // 1. Catalog: what exists. Built once, treated as read-only afterwards.
 * AssetCatalog catalog;
 * AssetRecord atlas;
 * atlas.id = 1;
 * atlas.path = "data.pak";
 * atlas.offset = 0;
 * atlas.storedSize = 512;
 * atlas.type = "image";
 * catalog.addRecord(atlas);
 *
 * AssetRecord page;
 * page.id = 2;
 * page.path = "data.pak";
 * page.offset = 512;
 * page.storedSize = 128;
 * page.type = "blob";
 * page.dependencies.push_back(1);
 * catalog.addRecord(page);
 *
 * const CatalogValidation validation = catalog.validateDependencies();
 * if (!validation.ok) {
 *     return;   // invalidRecords / missingDependencies / selfDependencies /
 *               // duplicateDependencies tell you what to fix
 * }
 *
 * // 2. Runtime state: what is resident, and how much we are willing to keep.
 * ResourceRegistry registry;
 * ResidencyManager residency(registry, &catalog);
 * residency.setBudget(BudgetKind::CpuBytes, 64ull * 1024ull * 1024ull);
 *
 * // 3. Scheduler: how to read and how to decode.
 * Elv::Util::ThreadPool pool(2);
 * StreamingScheduler scheduler(
 *     catalog, registry, pool,
 *     [](const AssetRecord& record) { return openDeviceFor(record.path); },  // Loader
 *     [](const AssetRecord& record, std::span<const std::byte> bytes) {      // Decoder
 *         DecodedAsset decoded;
 *         decoded.payload = makeBytePayload({ bytes.begin(), bytes.end() }, record.type);
 *         decoded.cpuBytes = bytes.size();
 *         decoded.kind = ResourceKind::Blob;
 *         return decoded;
 *     },
 *     StreamingSchedulerOptions { 2, true, false });
 *
 * // 4. Request, then pump once per frame (or in a dedicated loading loop).
 * const ResourceHandle<GenericBlobResource> pageHandle =
 *     registry.createHandle<GenericBlobResource>(2);
 * scheduler.request(2, StreamPriority { 10 });
 *
 * while (scheduler.pendingJobs() != 0 || scheduler.queuedRequests() != 0
 *        || scheduler.waitingRequests() != 0) {
 *     scheduler.pollCompletions();
 *     if (registry.state(2) == ResidencyState::Failed)
 *         break;   // registry.failureReason(2) / registry.failureMessage(2)
 * }
 *
 * if (registry.state(pageHandle) == ResidencyState::Resident) {
 *     const ResidentPayload* payload = registry.payload(2);
 *     // payload->data holds the bytes, payload->bytes is the accounted CPU cost
 * }
 *
 * // 5. When the asset is no longer needed, hand it to the eviction policy.
 * residency.makeEvictable(2);
 * residency.evictUntilWithinBudget();
 * @endcode
 *
 * @see Euph::Asset::AssetRecord, Euph::Asset::ResidencyState - the data model
 * @see Euph::Asset::AssetCatalog, Euph::Asset::DependencyGraph - metadata and ordering
 * @see Euph::Asset::ResourceRegistry, Euph::Asset::ResourceLease - residency bookkeeping
 * @see Euph::Asset::ResidencyManager - budgets and eviction
 * @see Euph::Asset::StreamingScheduler - the load pipeline
 * @see Lotte::Asset::GpuAssetStreamer - GPU-side integration built on this runtime
 */

#include <Euphemy/Asset/EuphAssetTypes.hpp>
#include <Euphemy/Config/EuphLib.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <Elvavena/Util/ElvThreadPool.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace Euph {
namespace Asset {

/**
 * @brief Broad CPU-side resource category used for typed handles and placeholders.
 *
 * The kind is a *tag*, not a payload type: it exists so that a
 * @ref Euph::Asset::ResourceHandle can be validated at compile time (through the tag
 * types) and so that the registry can keep one placeholder per category.
 *
 * The registry stores the kind in its entry. Two things can change it:
 *
 * - @ref Euph::Asset::ResourceRegistry::create sets it when the entry is created, and
 *   @ref Euph::Asset::ResourceRegistry::createHandle resets it when called with a
 *   different tag. Either way, handles of the previous kind stop validating.
 * - @ref Euph::Asset::StreamingScheduler::completeDecode re-types the entry when the
 *   decoder reported a non-`Blob` kind that differs from the stored one.
 *
 * `ResourceKind::Count` is a sentinel for fixed-size arrays; it is never a valid kind, and
 * the registry sizes its per-kind arrays with it.
 */
enum class ResourceKind : std::uint8_t {
	Image, /**< Decoded image-like payload. */
	Audio, /**< Decoded audio-like payload. */
	Blob,  /**< Generic byte payload. */
	Count  /**< Sentinel used for fixed-size arrays. */
};

/** @brief Handle tag for image resources. */
struct ImageResource { static constexpr ResourceKind kind = ResourceKind::Image; };
/** @brief Handle tag for audio resources. */
struct AudioResource { static constexpr ResourceKind kind = ResourceKind::Audio; };
/** @brief Handle tag for generic byte/blob resources. */
struct GenericBlobResource { static constexpr ResourceKind kind = ResourceKind::Blob; };
/** @brief Compatibility alias for image resources consumed as textures later. */
using TextureResource = ImageResource;
/**
 * @brief Handle tag for buffer-like byte resources.
 *
 * Validates against @ref ResourceKind::Blob as well, but is a distinct C++ type: a GPU
 * buffer uploader can therefore use its own tag (and its own placeholder slot semantics)
 * without inventing a new `ResourceKind` or colliding with
 * @ref Euph::Asset::GenericBlobResource handles.
 */
struct BufferResource { static constexpr ResourceKind kind = ResourceKind::Blob; };

/**
 * @brief Typed, generation-checked reference to a registry entry.
 *
 * Handles are cheap values. They become invalid when the registry entry is destroyed and later
 * reused with a new generation.
 *
 * @tparam TTag Tag type selecting the resource category. It must provide
 *         `static constexpr ResourceKind kind`. Use @ref Euph::Asset::ImageResource,
 *         @ref Euph::Asset::AudioResource, @ref Euph::Asset::GenericBlobResource or
 *         @ref Euph::Asset::BufferResource, or define your own type with the same shape.
 *
 * @section euph_asset_handle_identity Identity, not ownership
 *
 * A handle is three integers: the @ref Euph::Asset::AssetId, the registry @ref slot the entry
 * lived in, and the generation that slot had when the handle was made. It holds no reference and
 * keeps nothing alive (that is @ref Euph::Asset::ResourceLease's job), and every read goes back to
 * the registry:
 *
 * @code
 * using ImageHandle = Euph::Asset::ResourceHandle<Euph::Asset::ImageResource>;
 *
 * const ImageHandle handle = registry.createHandle<Euph::Asset::ImageResource>(42);
 * if (!registry.isAlive(handle)) {                      // destroyed, or slot recycled
 *     return;
 * }
 * if (registry.state(handle) == Euph::Asset::ResidencyState::Resident) {
 *     const Euph::Asset::ResidentPayload* payload = registry.payload(handle.id);
 * }
 * @endcode
 *
 * Because the slot and the generation are recorded, a stale handle is *detected* instead of
 * silently resolving to whatever occupies the slot now: recycling a slot bumps its generation, so
 * the registry rejects handles that captured the old one. Handles created by `createHandle<TTag>`
 * additionally carry their tag, so a `ResourceHandle<AudioResource>` never validates against an
 * entry of kind `Image`.
 *
 * @note `operator bool()` reports "this handle has any identity at all", not "the entry is alive":
 *       a default-constructed handle is falsy, a stale one is still truthy. Use
 *       @ref Euph::Asset::ResourceRegistry::isAlive for liveness.
 * @note The default @ref slot and generation are `0`, which can never match a live entry, because
 *       live entries start at generation `1`.
 */
template<typename TTag> struct ResourceHandle {
	/** @brief Asset id referenced by the handle. */
	AssetId id = 0;
	/** @brief Generation captured when the handle was created. */
	std::uint32_t generation = 0;
	/** @brief Registry slot captured when the handle was created. */
	std::uint32_t slot = 0;

	/**
	 * @brief Returns true when the handle has any non-null identity.
	 * @return `true` when the id or the generation is non-zero.
	 * @note This is *not* a liveness test; use
	 *       @ref Euph::Asset::ResourceRegistry::isAlive for that.
	 */
	constexpr explicit operator bool() const noexcept { return id != 0 || generation != 0; }

	/**
	 * @brief Compares raw handle identity.
	 * @return `true` when id, generation and slot all match.
	 */
	constexpr bool operator==(const ResourceHandle&) const = default;
};

/**
 * @brief Result of validating catalog dependency references.
 *
 * Produced by @ref Euph::Asset::AssetCatalog::validateDependencies, the "did I build the
 * catalog correctly" check to run after loading or cooking metadata. It is a *report*, not a
 * repair: it never changes the catalog.
 *
 * Every list is sorted and de-duplicated, so two reports are directly comparable and a list
 * entry means "this id has this problem" rather than "this problem occurred N times".
 */
struct CatalogValidation {
	/** @brief True when no invalid records or dependency errors were found. */
	bool ok = true;
	/** @brief Records with unusable metadata, such as id zero. */
	std::vector<AssetId> invalidRecords;
	/** @brief Dependency ids that do not exist in the catalog. */
	std::vector<AssetId> missingDependencies;
	/** @brief Records that directly depend on themselves. */
	std::vector<AssetId> selfDependencies;
	/** @brief Records that list the same direct dependency more than once. */
	std::vector<AssetId> duplicateDependencies;
};

/**
 * @brief Stores AssetRecord metadata and lightweight lookup indexes.
 *
 * AssetCatalog is authoritative for ids and dependencies. Path/type data are hints for
 * loaders and decoders, not resource lifetime state.
 *
 * @section euph_asset_catalog_role What the catalog does not do
 *
 * The catalog is the *static* half of the runtime. It never loads, decodes, evicts or tracks
 * residency - it only answers questions:
 *
 * - "does this id exist?" (@ref contains, @ref find);
 * - "which id has this path?" (@ref findByPath);
 * - "what does this record depend on?" (@ref dependenciesOf);
 * - "is the metadata self-consistent?" (@ref validateDependencies).
 *
 * Ordering and cycle detection live in @ref Euph::Asset::DependencyGraph (a view over a
 * catalog), and everything mutable lives in @ref Euph::Asset::ResourceRegistry.
 *
 * @section euph_asset_catalog_insertion Insertion rules
 *
 * @ref addRecord enforces three things and rejects the record otherwise (returning `false`
 * and leaving the catalog untouched):
 *
 * 1. `id` must be non-zero;
 * 2. `type` must be non-empty;
 * 3. if `path` is non-empty, no *other* id may already claim it.
 *
 * Adding a record whose id already exists **replaces** it, and the path index is repaired in
 * the process (the old path entry is dropped before the new one is inserted).
 *
 * `dependencies` is *not* validated on insertion: an id may name a record that has not been
 * added yet (forward references are normal for cooked data), which is exactly what
 * @ref validateDependencies is for.
 *
 * @section euph_asset_catalog_threading Threading and pointer lifetime
 *
 * Not thread-safe. @ref find, @ref findByPath and @ref dependenciesOf return views into the
 * internal containers, so any @ref addRecord, @ref removeRecord or @ref clear invalidates
 * them - including a rehash caused by adding an unrelated record. Copy what you need, or
 * re-fetch.
 */
class MH_EUPH_API AssetCatalog {
private:
	/** @brief Stored records, keyed by asset id. */
	std::unordered_map<AssetId, AssetRecord> records;
	/** @brief Optional path-hint to asset-id index used by @ref findByPath. */
	std::unordered_map<std::string, AssetId> pathIndex;
	/** @brief Returns a shared empty dependency list for unknown ids. */
	static const std::vector<AssetId>& emptyDependencies();
	/**
	 * @brief Removes any path index entry that points at @p id.
	 * @param id Asset id whose path entries should be dropped.
	 * @note Called before a record is inserted or after it is erased, so that the path index never
	 *       points at a stale id.
	 */
	void removePathIndexFor(AssetId id);
public:
	/**
	 * @brief Adds or replaces one record, updating the optional path index.
	 * @param record Record to store; taken by value and moved into the catalog.
	 * @return `true` when stored, `false` when rejected because the id is zero, the type is
	 *         empty, or another id already owns the record's non-empty path.
	 * @note Replacing a record keeps the id but discards the old metadata and dependencies
	 *       wholesale - there is no merge.
	 * @see euph_asset_catalog_insertion
	 */
	bool addRecord(AssetRecord record);
	/**
	 * @brief Removes a record and its path lookup entry.
	 * @param id Asset id to remove.
	 * @return `true` when a record was removed, `false` when the id was not in the catalog.
	 * @warning Removing a record does not touch the registry: an entry for @p id may stay
	 *          resident, and other records may still name @p id as a dependency (the next
	 *          @ref validateDependencies reports it as missing).
	 */
	bool removeRecord(AssetId id);
	/**
	 * @brief Removes all catalog records and indexes.
	 * @note Any pointer or span previously returned by @ref find, @ref findByPath or
	 *       @ref dependenciesOf is invalidated.
	 */
	void clear();
	/**
	 * @brief Returns true when id exists in the catalog.
	 * @param id Asset id to test.
	 * @return `true` when a record with that id is stored.
	 */
	bool contains(AssetId id) const;
	/**
	 * @brief Finds a record by id.
	 * @param id Asset id to look up.
	 * @return Pointer to the stored record, or `nullptr` when the id is unknown.
	 * @warning The pointer is invalidated by @ref addRecord, @ref removeRecord and @ref clear.
	 */
	const AssetRecord* find(AssetId id) const;
	/**
	 * @brief Finds a record by path hint.
	 * @param path Path string exactly as stored in @ref AssetRecord::path.
	 * @return Pointer to the stored record, or `nullptr` when no record claims that path.
	 * @note Because a path belongs to at most one id, this is a single hash lookup, not a
	 *       search over records.
	 */
	const AssetRecord* findByPath(const std::string& path) const;
	/**
	 * @brief Returns the direct dependency ids of a record.
	 * @param id Asset id to query.
	 * @return Span over the stored dependency list; empty but valid when the id is unknown or
	 *         the record has no dependencies.
	 * @note The span points into the catalog and is invalidated by any structural change.
	 */
	std::span<const AssetId> dependenciesOf(AssetId id) const;
	/**
	 * @brief Returns all asset ids currently stored in the catalog.
	 * @return Sorted vector of ids, so iteration order is deterministic across runs.
	 */
	std::vector<AssetId> assetIds() const;
	/**
	 * @brief Checks direct dependency references for missing or malformed data.
	 * @return Report listing invalid records, missing dependencies, self-dependencies and
	 *         duplicated dependencies; @ref CatalogValidation::ok is `true` only when all
	 *         four lists are empty.
	 * @note Only *direct* references are checked here. Cycles are detected by
	 *       @ref Euph::Asset::DependencyGraph::resolveTransitive, because a cycle is a
	 *       property of a traversal rather than of a single record.
	 */
	CatalogValidation validateDependencies() const;
	/**
	 * @brief Returns the number of records in the catalog.
	 * @return Record count, including records whose dependencies are broken.
	 */
	std::size_t size() const noexcept;
	/**
	 * @brief Returns true when the catalog has no records.
	 * @return `true` when @ref size is zero.
	 */
	bool empty() const noexcept;
};

/**
 * @brief Result of resolving dependency ordering for one asset.
 *
 * Returned by @ref Euph::Asset::DependencyGraph::resolveTransitive. Both the success case and
 * the failure cases are informative, so callers usually branch on @ref ok and then read the
 * matching list:
 *
 * | @ref ok | Meaning | Interesting fields |
 * |---|---|---|
 * | `true` | traversal completed | @ref orderedDependencies (load order, root excluded) |
 * | `false`, `MissingDependency` | an id along the way is not in the catalog | @ref missingDependencies |
 * | `false`, `DependencyCycle` | a dependency loop was found | @ref cycle |
 *
 * @warning Even when @ref ok is `false`, @ref orderedDependencies may still contain the assets
 *          that were successfully visited before the failure. Treat it as partial and do not
 *          load from it.
 */
struct DependencyResult {
	/** @brief True when dependency traversal completed without missing refs or cycles. */
	bool ok = true;
	/** @brief Failure category for the traversal. */
	FailureReason failure = FailureReason::None;
	/**
	 * @brief Transitive dependencies in load-before-parent order.
	 *
	 * The root asset is *not* included. Every dependency appears after all of its own
	 * dependencies, so the vector can be consumed front to back.
	 */
	std::vector<AssetId> orderedDependencies;
	/** @brief Missing dependency ids found during traversal. */
	std::vector<AssetId> missingDependencies;
	/**
	 * @brief Cycle path when traversal detects a dependency loop.
	 *
	 * Filled in as the traversal unwinds, so the first entry is the id where the loop was
	 * closed and the last entry repeats it (for `A -> B -> C -> A` the vector is
	 * `[A, C, B, A]`). The DFS root is therefore *not* necessarily the first element; read it
	 * as "the ids that form the loop", not as an ordered path.
	 */
	std::vector<AssetId> cycle;
};

class ResourceRegistry;

/**
 * @brief Resolves dependency relationships from an AssetCatalog.
 *
 * The graph is a view over the catalog; it does not copy or own records.
 *
 * @section euph_asset_depgraph_algorithm How traversal works
 *
 * @ref resolveTransitive performs a depth-first search with three colour marks (unvisited,
 * in progress, done) and emits each node **after** its dependencies, which yields a
 * load-before-parent order directly. That post-order emission is also what detects cycles:
 * meeting an in-progress node means the recursion found its own ancestor.
 *
 * @section euph_asset_depgraph_cost Cost
 *
 * Nothing is cached; every call re-walks the graph. `dependentsOf` and `hasDependency` are the
 * expensive ones: `dependentsOf` calls `hasDependency` for every record in the catalog, and
 * each of those resolves a full transitive closure, so it is roughly O(records x edges). Use
 * them for tooling and diagnostics, not per frame.
 *
 * @section euph_asset_depgraph_threading Threading
 *
 * The graph holds nothing but a catalog reference, so it is cheap to create on the stack. It
 * is const-only, but it is bound to the catalog's const-ness: the catalog must not be
 * structurally modified while a traversal is in progress.
 */
class MH_EUPH_API DependencyGraph {
private:
	/** @brief Catalog this graph is a view over. */
	const AssetCatalog& catalog;
	/**
	 * @brief Depth-first visitor used by @ref resolveTransitive.
	 * @param id Asset being visited.
	 * @param marks Per-traversal colour map (0 = unvisited, 1 = in progress, 2 = done).
	 * @param result Result object that receives ordering and diagnostics.
	 * @return `true` when the subtree resolved, `false` at the first missing id or cycle.
	 */
	bool visit(AssetId id, std::unordered_map<AssetId, std::uint8_t>& marks, DependencyResult& result) const;
public:
	/**
	 * @brief Creates a dependency graph view over an existing catalog.
	 * @param assetCatalog Catalog the graph reads from; must outlive the graph.
	 * @note Nothing is copied or precomputed, so construction is trivially cheap.
	 */
	explicit DependencyGraph(const AssetCatalog& assetCatalog);
	/**
	 * @brief Resolves all transitive dependencies for id in load order.
	 * @param id Root asset whose dependencies are needed.
	 * @return Ordering result; see @ref DependencyResult for the success and failure shapes.
	 * @note The root itself is excluded from
	 *       @ref DependencyResult::orderedDependencies, and a node reachable through several
	 *       paths is visited once.
	 */
	DependencyResult resolveTransitive(AssetId id) const;
	/**
	 * @brief Returns direct dependencies for id.
	 * @param id Asset whose direct dependencies are wanted.
	 * @return Copy of the record's dependency list; empty for unknown ids.
	 */
	std::vector<AssetId> directDependencies(AssetId id) const;
	/**
	 * @brief Returns catalog records that directly depend on id.
	 * @param id Dependency the caller is interested in.
	 * @return Ids of records whose transitive closure contains @p id.
	 * @warning This is a whole-catalog query (see @ref euph_asset_depgraph_cost); it is not a
	 *          cheap reverse index.
	 */
	std::vector<AssetId> dependentsOf(AssetId id) const;
	/**
	 * @brief Returns true when asset transitively depends on dependency.
	 * @param asset Asset whose closure is walked.
	 * @param dependency Id searched for in that closure.
	 * @return `true` when @p dependency appears in @p asset's transitive dependencies.
	 * @note Returns `false` when the traversal fails (missing id or cycle), so a broken
	 *       catalog reports "no dependency" rather than surfacing the error. Resolve
	 *       explicitly when the distinction matters.
	 */
	bool hasDependency(AssetId asset, AssetId dependency) const;
	/**
	 * @brief Checks whether every direct dependency for id is resident.
	 * @param id Asset whose dependencies are checked.
	 * @param registry Registry queried for residency states.
	 * @param blockingDependency Optional out-parameter receiving the id that blocked the
	 *        check: for a broken graph the first missing id (or, if there is none, the first
	 *        id of the cycle); otherwise the first dependency that is neither `Resident` nor
	 *        `Evictable`. Untouched when the check succeeds.
	 * @return `true` when every transitive dependency is `Resident` or `Evictable`; `false`
	 *         when one is not, or when the graph itself is broken.
	 * @note Despite the name it checks the *transitive* closure, and it treats `Evictable` as
	 *       satisfied: an evictable asset still has its payload.
	 */
	bool dependenciesResident(AssetId id, const ResourceRegistry& registry, AssetId* blockingDependency = nullptr) const;
};

/**
 * @brief Budget counters maintained by the CPU-side asset runtime.
 *
 * `BudgetKind` splits "how many bytes do we hold" into four independent buckets, so a caller
 * can limit resident memory and in-flight work separately. The registry stores one cost value
 * per kind and per entry; @ref Euph::Asset::ResidencyManager sums them into usage and compares
 * that against the configured budgets.
 *
 * Which bucket is written by whom:
 *
 * - @ref BudgetKind::CpuBytes - `ResourceRegistry::setPayload` and
 *   `StreamingScheduler::completeDecode`: resident decoded bytes of the entry.
 * - @ref BudgetKind::AudioBytes - `ResourceRegistry::setCost` and
 *   `StreamingScheduler::completeDecode`: resident audio bytes, counted *in addition to* the CPU
 *   figure, so audio is normally accounted twice.
 * - @ref BudgetKind::IoInflightBytes - `StreamingScheduler::startQueuedRequests`: the record's
 *   `storedSize` while its read runs; cleared on completion or failure.
 * - @ref BudgetKind::DecodeInflightBytes - `StreamingScheduler::completeIo`: the raw byte count
 *   while the decode runs; cleared on completion or failure.
 *
 * The in-flight buckets are what makes "we are over budget because of work that is about to
 * finish" distinguishable from "we are over budget because we kept too much". Note that the
 * scheduler only *records* them; nothing throttles on them - see
 * @ref euph_asset_runtime_limits.
 *
 * `BudgetKind::Count` is a sentinel used to size the registry's per-entry cost array and the
 * manager's budget array; it is never a valid kind.
 */
enum class BudgetKind : std::uint8_t {
	CpuBytes,            /**< Resident CPU payload bytes. */
	AudioBytes,          /**< Resident audio payload bytes. */
	IoInflightBytes,     /**< Bytes currently being loaded from I/O. */
	DecodeInflightBytes, /**< Bytes currently being decoded. */
	Count                /**< Sentinel used for fixed-size arrays. */
};

/**
 * @brief Type-erased CPU resident payload retained by ResourceRegistry.
 *
 * The type string is intentionally lightweight. Higher layers may agree on strings such as
 * `"bytes"` or `"decode-target"` without making Euphemy depend on renderer or
 * backend-specific payload classes.
 *
 * @section euph_asset_payload_ownership Ownership semantics
 *
 * @ref data is a `shared_ptr<void>`, so a payload is trivially copyable and every copy keeps
 * the object alive. This is the mechanism behind the GPU hand-off: copy the payload, let the
 * registry entry be evicted, and keep using the object until the last copy is dropped. The
 * shared pointer was created with a concrete deleter, so the object is destroyed with its real
 * type even though the pointer is `void`.
 *
 * @section euph_asset_payload_bytes What `bytes` is (and is not)
 *
 * @ref bytes is the *accounted* cost, not a measurement: `setPayload` copies it into
 * @ref BudgetKind::CpuBytes, and the scheduler overrides that with the decoder's own figure
 * when it reports one. It is therefore allowed to be an estimate (for example a decoded image's
 * expected size before it is fully materialised), but keep it stable per payload, or the
 * eviction order will look arbitrary.
 *
 * @section euph_asset_payload_type The type tag
 *
 * The tag is a plain string; Euphemy never compares it. It exists so that an uploader can
 * decide how to interpret `data` (for example "cast to `Euph::Media::Image::DecodeTarget`")
 * without a registry of polymorphic base classes. `makeBytePayload` produces `"bytes"`.
 */
struct ResidentPayload {
	/** @brief Shared ownership of the payload object. */
	std::shared_ptr<void> data;
	/** @brief Byte cost or payload size used by budgets and upload bridges. */
	std::uint64_t bytes = 0;
	/** @brief Free-form payload type tag. */
	std::string type;

	/**
	 * @brief Returns true when the payload owns data.
	 * @return `true` when @ref data is non-null.
	 */
	explicit operator bool() const noexcept { return data != nullptr; }
};

/**
 * @brief Wraps a byte vector in a ResidentPayload with a type tag.
 * @param bytes Byte buffer to take ownership of; moved into a shared vector.
 * @param type Payload type tag, by convention `"bytes"` for opaque byte blobs.
 * @return Payload whose `bytes` field equals the vector size and whose `data` owns the vector
 *         as `std::shared_ptr<std::vector<std::byte>>`.
 * @note The buffer is moved, not copied; pass `std::move(vector)` when the caller does not need
 *       it any more.
 * @note Reading it back requires the concrete type:
 *       `static_cast<std::vector<std::byte>*>(payload.data.get())`.
 */
MH_EUPH_API ResidentPayload makeBytePayload(std::vector<std::byte> bytes, std::string type = "bytes");

/**
 * @brief Owns resource lifetime state, typed handle validation, costs, and payloads.
 *
 * ResourceRegistry is the central CPU-side table used by the streaming runtime.
 * It does not perform I/O, decoding, or GPU work by itself.
 *
 * @section euph_asset_registry_slots Slots, generations and handles
 *
 * Entries live in a `std::vector` slot array; the registry maps @ref Euph::Asset::AssetId to a
 * slot index and recycles freed slots. Every slot carries an @ref Entry::generation that is
 * incremented when the slot is destroyed, so a handle which captured the old generation no longer
 * resolves - that is the entire staleness mechanism, and it is why handles are re-validated on
 * every access instead of being trusted.
 *
 * @code
 * const AssetId id = 7;
 * const auto handle = registry.createHandle<Euph::Asset::ImageResource>(id);  // slot 0, gen 1
 * registry.destroy(id);                                                      // gen becomes 2
 * registry.createHandle<Euph::Asset::ImageResource>(id);                     // slot 0, gen 2
 * registry.isAlive(handle);                                                  // false
 * @endcode
 *
 * @section euph_asset_registry_kinds Kinds and placeholders
 *
 * Each entry has one @ref Euph::Asset::ResourceKind. A typed handle
 * (`ResourceHandle<TTag>`) only validates when the tag's kind matches the entry's kind, so one id
 * cannot accidentally be used as a texture and as a buffer. The registry also keeps **one
 * placeholder slot per kind**: a stand-in handle that policy code can return when the real
 * resource is not resident (see @ref setPlaceholder and @ref resolveOrPlaceholder).
 *
 * @section euph_asset_registry_ledger What is tracked per entry
 *
 * | Group | Field | Updated by |
 * |---|---|---|
 * | identity | `id`, `kind`, `generation`, `active` | create / destroy |
 * | lifecycle | `state` | @ref setState, @ref fail |
 * | diagnostics | `failureReason`, `failureMessage` | @ref fail |
 * | policy | `flags`, `priority`, `lastTouchedFrame` | @ref setFlag, @ref setPriority, @ref touch |
 * | usage | `strongRefs`, `weakRefs` | @ref retain, @ref release, `retainWeak`, `releaseWeak` |
 * | budgets | `costs` | @ref setCost, @ref setPayload, @ref clearPayload, @ref clearCosts |
 * | data | `payload` | @ref setPayload, @ref clearPayload |
 *
 * @section euph_asset_registry_threading Threading and pointer lifetime
 *
 * Not thread-safe. @ref inspect and @ref payload hand out pointers into a vector that grows and
 * recycles slots, so an `Entry*` or `ResidentPayload*` is invalidated by any @ref create,
 * @ref createHandle or @ref destroy. Read what you need and let the pointer go.
 *
 * @section euph_asset_registry_example Typical use
 *
 * @code
 * Euph::Asset::ResourceRegistry registry;
 * const auto handle = registry.createHandle<Euph::Asset::AudioResource>(9);
 * registry.setPriority(9, Euph::Asset::StreamPriority { 5 });
 * registry.retain(9);                                          // protect against eviction
 * registry.setPayload(9, Euph::Asset::makeBytePayload(bytes)); // implies CpuResident
 * registry.setState(9, Euph::Asset::ResidencyState::Resident);
 * // ... use the payload ...
 * registry.setFlag(9, Euph::Asset::ResidencyFlag::Reloadable, true);
 * registry.release(9);
 * @endcode
 */
class MH_EUPH_API ResourceRegistry {
public:
	/**
	 * @brief Internal resource slot exposed for diagnostics and tests.
	 *
	 * The layout is deliberately public so that tests and debug tooling can read an entry
	 * wholesale through @ref ResourceRegistry::inspect. It is *not* meant to be written by
	 * hand: every field has an updating function that also maintains the invariants listed in
	 * @ref euph_asset_registry_ledger.
	 */
	struct Entry {
		/** @brief Asset id owned by this entry. */
		AssetId id = 0;
		/** @brief Resource kind used for typed handle checks. */
		ResourceKind kind = ResourceKind::Blob;
		/** @brief Generation incremented when a slot is reused. */
		std::uint32_t generation = 1;
		/** @brief True when this slot currently owns a resource. */
		bool active = false;
		/** @brief Current residency state. */
		ResidencyState state = ResidencyState::Missing;
		/** @brief Policy/diagnostic flags. */
		ResidencyFlag flags = ResidencyFlag::None;
		/** @brief Last recorded failure category. */
		FailureReason failureReason = FailureReason::None;
		/** @brief Optional human-readable failure details. */
		std::string failureMessage;
		/** @brief Strong references that block destruction/eviction. */
		std::uint32_t strongRefs = 0;
		/** @brief Weak references recorded for diagnostics/policy. */
		std::uint32_t weakRefs = 0;
		/** @brief Current streaming/eviction priority. */
		StreamPriority priority {};
		/** @brief Last frame index that touched this resource. */
		std::uint64_t lastTouchedFrame = 0;
		/** @brief Per-budget byte costs. */
		std::array<std::uint64_t, static_cast<std::size_t>(BudgetKind::Count)> costs {};
		/** @brief Optional resident CPU payload. */
		ResidentPayload payload;
	};
private:
	/** @brief Slot storage; the indices into this vector are what handles capture. */
	std::vector<Entry> entries;
	/** @brief Indices of destroyed slots, reused before the vector grows. */
	std::vector<std::uint32_t> freeSlots;
	/** @brief Asset id to slot index map; the primary lookup for every operation. */
	std::unordered_map<AssetId, std::uint32_t> slotByAsset;
	/** @brief One placeholder slot per @ref Euph::Asset::ResourceKind. */
	std::array<std::optional<std::uint32_t>, static_cast<std::size_t>(ResourceKind::Count)> placeholders;

	/**
	 * @brief Maps a resource kind to its index in the fixed-size per-kind arrays.
	 * @param kind Kind to index.
	 * @return `static_cast<std::size_t>(kind)`.
	 */
	static std::size_t kindIndex(ResourceKind kind);
	/**
	 * @brief Finds the live entry for an id.
	 * @param id Asset id to look up.
	 * @return Entry pointer, or `nullptr` when the id is unknown, the slot was recycled, or
	 *         the entry is inactive.
	 */
	Entry* entryFor(AssetId id);
	/** @brief Const overload of @ref entryFor. */
	const Entry* entryFor(AssetId id) const;
	/**
	 * @brief Validates a typed handle and returns its entry.
	 * @tparam TTag Handle tag type.
	 * @param handle Handle captured by the caller.
	 * @return Entry pointer, or `nullptr` when the slot is out of range, inactive, the id
	 *         differs, the generation differs, or the kind does not match `TTag::kind`.
	 */
	template<typename TTag> const Entry* entryFor(ResourceHandle<TTag> handle) const;
	/** @brief Non-const overload of the typed @ref entryFor. */
	template<typename TTag> Entry* entryFor(ResourceHandle<TTag> handle);
	/**
	 * @brief Reports whether a state transition is legal.
	 * @param from Current state.
	 * @param to Requested state.
	 * @return `true` when the transition is allowed; same-state and `-> Failed` are always
	 *         allowed. The table itself is documented on
	 *         @ref Euph::Asset::ResidencyState.
	 */
	static bool legalTransition(ResidencyState from, ResidencyState to);
	/**
	 * @brief Creates or finds the entry for an id and returns a blob-typed handle.
	 * @param id Asset id to create or find.
	 * @return Handle for the entry; a new entry starts `active` and in
	 *         @ref Euph::Asset::ResidencyState::Known.
	 */
	ResourceHandle<GenericBlobResource> createBlobHandle(AssetId id);
public:
	/** @brief Creates an empty registry. */
	ResourceRegistry();

	/**
	 * @brief Creates or finds a resource entry for id and kind.
	 * @param kind Kind to stamp on the entry. An existing entry is **re-typed** to this kind,
	 *        which invalidates typed handles of the previous kind.
	 * @param id Asset id to create or find.
	 * @return Untyped (blob-tagged) handle for the entry.
	 * @note Creating an id that already exists changes nothing else: state, refs, costs and
	 *       payload are preserved.
	 * @warning The returned handle is `ResourceHandle<GenericBlobResource>` whatever @p kind
	 *          is, so it always validates. Use @ref createHandle for a checked handle.
	 */
	ResourceHandle<GenericBlobResource> create(ResourceKind kind, AssetId id);
	/**
	 * @brief Creates or finds a typed resource handle for id.
	 * @tparam TTag Handle tag type; its `kind` is stamped on the entry.
	 * @param id Asset id to create or find.
	 * @return Handle that validates against the entry, or a default (falsy) handle when the
	 *         entry could not be found or re-typed.
	 * @note Prefer this over @ref create - it is the only way to get kind checking out of the
	 *       registry.
	 */
	template<typename TTag> ResourceHandle<TTag> createHandle(AssetId id);
	/**
	 * @brief Destroys an entry if it is not strongly referenced.
	 * @param id Asset id whose entry should be destroyed.
	 * @return `true` when the entry was destroyed, `false` when the id is unknown or
	 *         @ref Entry::strongRefs is non-zero.
	 * @warning Destruction does not consult the state: it clears flags, costs, payload and
	 *          diagnostics and frees the slot, so a `Resident` entry (and every payload copy
	 *          except outstanding `shared_ptr` copies) is dropped. Handles and placeholders
	 *          for that slot stop validating because the generation is bumped.
	 */
	bool destroy(AssetId id);

	/**
	 * @brief Returns true when a typed handle still matches a live entry.
	 * @tparam TTag Handle tag type.
	 * @param handle Handle to validate.
	 * @return `true` when slot, id, generation and kind all still match.
	 */
	template<typename TTag> bool isAlive(ResourceHandle<TTag> handle) const;
	/**
	 * @brief Returns true when id maps to a live entry.
	 * @param id Asset id to test.
	 * @return `true` when an active entry exists for @p id.
	 */
	bool isAlive(AssetId id) const;
	/**
	 * @brief Returns current state for id, or Missing when absent.
	 * @param id Asset id to query.
	 * @return The entry's @ref Euph::Asset::ResidencyState, or
	 *         @ref Euph::Asset::ResidencyState::Missing when there is no live entry - a
	 *         destroyed resource and a never-created one are indistinguishable here.
	 */
	ResidencyState state(AssetId id) const;
	/**
	 * @brief Returns current state for a typed handle, or Missing when invalid.
	 * @tparam TTag Handle tag type.
	 * @param handle Handle to query.
	 * @return The entry's state, or @ref Euph::Asset::ResidencyState::Missing when the handle
	 *         does not validate.
	 */
	template<typename TTag> ResidencyState state(ResourceHandle<TTag> handle) const;

	/**
	 * @brief Applies a legal state transition to a resource.
	 * @param id Asset id to transition.
	 * @param newState Target state.
	 * @return `true` when the transition happened (or was a no-op to the same state); `false`
	 *         when the id is unknown or the transition is illegal.
	 * @note A rejected transition records @ref Euph::Asset::FailureReason::InvalidTransition on
	 *       the entry and leaves the state unchanged.
	 * @note A successful transition to anything but @ref Euph::Asset::ResidencyState::Failed
	 *       clears the recorded reason and message, so "state is not Failed" and "nothing is
	 *       recorded" always agree after a transition.
	 */
	bool setState(AssetId id, ResidencyState newState);
	/**
	 * @brief Marks a resource Failed with diagnostic information.
	 * @param id Asset id to fail.
	 * @param reason Failure category; @ref Euph::Asset::FailureReason::None is promoted to
	 *        @ref Euph::Asset::FailureReason::Unknown.
	 * @param message Optional human-readable detail, stored verbatim.
	 * @return `true` when the entry exists and was marked; `false` for unknown ids.
	 * @note This bypasses the transition table on purpose: a failure must be recordable from
	 *       any state, including states with no legal successors. It also zeroes the two
	 *       in-flight budget costs, because a failed transfer no longer occupies them.
	 */
	bool fail(AssetId id, FailureReason reason, std::string message = {});
	/**
	 * @brief Returns the last failure reason for id.
	 * @param id Asset id to query.
	 * @return The recorded reason; @ref Euph::Asset::FailureReason::None for a live entry that
	 *         never failed, and @ref Euph::Asset::FailureReason::AssetNotFound when there is no
	 *         live entry at all.
	 */
	FailureReason failureReason(AssetId id) const;
	/**
	 * @brief Returns the last failure message for id.
	 * @param id Asset id to query.
	 * @return Reference to the stored message, or a shared empty string when there is no live
	 *         entry. The reference is invalidated by @ref fail on the same entry.
	 */
	const std::string& failureMessage(AssetId id) const;

	/**
	 * @brief Updates the last-touched frame through a typed handle.
	 * @tparam TTag Handle tag type.
	 * @param handle Handle identifying the entry.
	 * @param frameIndex Frame counter to store; the caller defines the unit.
	 * @return `true` when the handle validated and the frame was stored.
	 * @note Eviction orders candidates by *ascending* @ref Entry::lastTouchedFrame, so touching
	 *       everything used in a frame is what protects the working set.
	 */
	template<typename TTag> bool touch(ResourceHandle<TTag> handle, std::uint64_t frameIndex);
	/**
	 * @brief Updates the last-touched frame for id.
	 * @param id Asset id to touch.
	 * @param frameIndex Frame counter to store.
	 * @return `true` when the entry exists.
	 */
	bool touch(AssetId id, std::uint64_t frameIndex);
	/**
	 * @brief Sets the streaming/eviction priority for id.
	 * @param id Asset id to update.
	 * @param priority New priority; larger means more important.
	 * @note Silently does nothing for unknown ids.
	 */
	void setPriority(AssetId id, StreamPriority priority);
	/**
	 * @brief Returns the priority for id.
	 * @param id Asset id to query.
	 * @return The stored priority, or a default-constructed (value `0`) priority when the id is
	 *         unknown.
	 */
	StreamPriority priority(AssetId id) const;

	/**
	 * @brief Adds a strong reference that blocks destruction/eviction.
	 * @param id Asset id to retain.
	 * @return `true` when the entry exists and the count was incremented.
	 * @note The counter only saturates at `uint32_t` overflow; releasing a reference that was
	 *       never taken is rejected by @ref release instead of underflowing.
	 */
	bool retain(AssetId id);
	/**
	 * @brief Releases one strong reference.
	 * @param id Asset id to release.
	 * @return `true` when a reference was released, `false` when the id is unknown or the count
	 *         was already zero.
	 */
	bool release(AssetId id);
	/**
	 * @brief Adds a weak reference counter for diagnostics/policy.
	 * @param id Asset id to retain weakly.
	 * @return `true` when the entry exists.
	 * @note Weak references are *not* consulted by eviction or destruction; they exist so that a
	 *       cache or a listener can record interest without pinning the resource.
	 */
	bool retainWeak(AssetId id);
	/**
	 * @brief Releases one weak reference counter.
	 * @param id Asset id to release.
	 * @return `true` when a weak reference was released, `false` when the id is unknown or the
	 *         count was already zero.
	 */
	bool releaseWeak(AssetId id);
	/** @brief Returns the strong reference count for id, or `0` for unknown ids. */
	std::uint32_t strongRefs(AssetId id) const;
	/** @brief Returns the weak reference count for id, or `0` for unknown ids. */
	std::uint32_t weakRefs(AssetId id) const;

	/**
	 * @brief Sets one budget cost counter for id.
	 * @param id Asset id to update.
	 * @param kind Budget bucket to write.
	 * @param bytes Cost in bytes; stored verbatim, never derived from the payload.
	 * @note Silently does nothing for unknown ids.
	 */
	void setCost(AssetId id, BudgetKind kind, std::uint64_t bytes);
	/** @brief Returns one budget cost counter for id, or `0` when the id is unknown. */
	std::uint64_t cost(AssetId id, BudgetKind kind) const;
	/** @brief Returns the CPU byte cost for id (convenience wrapper over @ref cost). */
	std::uint64_t cpuCost(AssetId id) const;
	/** @brief Returns the audio byte cost for id (convenience wrapper over @ref cost). */
	std::uint64_t audioCost(AssetId id) const;
	/** @brief Clears all budget cost counters for id. */
	void clearCosts(AssetId id);

	/**
	 * @brief Enables or disables one ResidencyFlag.
	 * @param id Asset id to update.
	 * @param flag Flag to change.
	 * @param enabled `true` to set, `false` to clear.
	 * @note Silently does nothing for unknown ids, so it cannot double as an existence check.
	 */
	void setFlag(AssetId id, ResidencyFlag flag, bool enabled);
	/**
	 * @brief Returns true when a ResidencyFlag is set for id.
	 * @param id Asset id to query.
	 * @param flag Flag to test; a union of flags tests for "all of them".
	 * @return `false` for unknown ids.
	 */
	bool hasFlag(AssetId id, ResidencyFlag flag) const;

	/**
	 * @brief Stores a resident CPU payload for id.
	 * @param id Asset id to update.
	 * @param payload Payload to store, moved in; copying it beforehand is how a caller keeps the
	 *        bytes alive independently of the entry.
	 * @note Side effects: @ref BudgetKind::CpuBytes is set to `payload.bytes`, and
	 *       @ref Euph::Asset::ResidencyFlag::CpuResident is set or cleared depending on whether
	 *       `payload.data` is non-null.
	 * @note Storing a payload does not change the residency state; call @ref setState (or let
	 *       the scheduler do it) to reach
	 *       @ref Euph::Asset::ResidencyState::Resident.
	 */
	void setPayload(AssetId id, ResidentPayload payload);
	/**
	 * @brief Clears the resident CPU payload for id.
	 * @param id Asset id to update.
	 * @note Also zeroes @ref BudgetKind::CpuBytes and clears
	 *       @ref Euph::Asset::ResidencyFlag::CpuResident, but leaves the residency state alone -
	 *       eviction is what moves the entry back to `Known`, so a payload-free entry can still
	 *       read as `Resident` until policy changes it.
	 */
	void clearPayload(AssetId id);
	/**
	 * @brief Returns the resident payload for id.
	 * @param id Asset id to query.
	 * @return Pointer to the stored payload (which may hold a null `data`), or `nullptr` when
	 *         the id is unknown.
	 * @warning Invalidated by any operation that creates or destroys an entry.
	 */
	const ResidentPayload* payload(AssetId id) const;

	/**
	 * @brief Registers a typed fallback handle for nonresident resources.
	 * @tparam TTag Handle tag type.
	 * @param handle Handle to use as the placeholder for `TTag::kind`.
	 * @note Each kind has exactly one placeholder slot, so a second call replaces the first
	 *       registration.
	 * @note Ignored when the handle does not validate, so a placeholder is always a live entry.
	 *       Registering a placeholder neither pins nor retains it.
	 */
	template<typename TTag> void setPlaceholder(ResourceHandle<TTag> handle);
	/**
	 * @brief Returns the typed fallback handle, if one is registered and live.
	 * @tparam TTag Handle tag type.
	 * @return The placeholder handle, or `std::nullopt` when nothing is registered for that kind
	 *         or the registered entry has since been destroyed.
	 */
	template<typename TTag> std::optional<ResourceHandle<TTag> > placeholder() const;
	/**
	 * @brief Returns handle when resident, otherwise a live typed placeholder.
	 * @tparam TTag Handle tag type.
	 * @param handle Handle requested by the caller.
	 * @return @p handle when it validates *and* its state is
	 *         @ref Euph::Asset::ResidencyState::Resident; otherwise the placeholder registered
	 *         for `TTag::kind`; otherwise a default (falsy) handle.
	 * @note Only `Resident` counts as usable here. `Evictable` and every in-flight state fall
	 *       through to the placeholder even though a payload may still be present.
	 */
	template<typename TTag> ResourceHandle<TTag> resolveOrPlaceholder(ResourceHandle<TTag> handle) const;

	/**
	 * @brief Returns all live asset ids currently known to the registry.
	 * @return Sorted vector of ids, so policy passes are deterministic across runs.
	 */
	std::vector<AssetId> activeAssetIds() const;
	/**
	 * @brief Returns a diagnostic view of an entry.
	 * @param id Asset id to inspect.
	 * @return Pointer to the live entry, or `nullptr` when absent.
	 * @warning Invalidated by any operation that creates or destroys an entry.
	 */
	const Entry* inspect(AssetId id) const;
};

/**
 * @brief RAII strong reference for a ResourceRegistry entry.
 *
 * The lease retains a resource on construction and releases it on destruction, making short-lived
 * CPU-side use explicit.
 *
 * @tparam TTag Handle tag type; it only shapes the stored handle, the registry reference counting
 *         itself is keyed by @ref Euph::Asset::AssetId.
 *
 * @section euph_asset_lease_scope What a lease protects
 *
 * A lease increments @ref Euph::Asset::ResourceRegistry::Entry::strongRefs, which
 * @ref Euph::Asset::ResidencyManager treats as "do not evict" and
 * @ref Euph::Asset::ResourceRegistry::destroy treats as "do not destroy". It does **not** own the
 * payload - for that, copy the @ref Euph::Asset::ResidentPayload - and it does not keep the
 * resource loaded: a lease taken on a `Known` entry protects an empty entry.
 *
 * @section euph_asset_lease_pitfalls Two consequences of the implementation
 *
 * - The constructor retains by **id only**. It does not validate the handle's slot or generation,
 *   so a stale handle with a live id still produces a successful lease. Validate with
 *   `ResourceRegistry::isAlive` first when that matters.
 * - If the retain fails (unknown id, or an id of `0` from a default handle) the lease becomes
 *   *inactive*: `operator bool()` returns `false` and nothing is released later. Construction never
 *   throws.
 *
 * @code
 * {
 *     Euph::Asset::ResourceLease<Euph::Asset::ImageResource> lease(registry, handle);
 *     if (!lease) {
 *         return;                     // the id was unknown when the lease was taken
 *     }
 *     const Euph::Asset::ResidentPayload* payload = registry.payload(lease.get().id);
 *     // ... upload/read ...
 * }                                   // strongRefs is decremented here
 * @endcode
 */
template<typename TTag> class ResourceLease {
private:
	/** @brief Registry that owns the retained entry, or `nullptr` for an inactive lease. */
	ResourceRegistry* registry = nullptr;
	/** @brief Handle the lease was created from. */
	ResourceHandle<TTag> handle {};
public:
	/** @brief Creates an empty lease. */
	ResourceLease() = default;
	/**
	 * @brief Attempts to retain resourceHandle in resourceRegistry.
	 * @param resourceRegistry Registry to retain in; must outlive the lease.
	 * @param resourceHandle Handle identifying the resource to retain.
	 * @note When the retain fails the lease is constructed in the inactive state instead of
	 *       reporting an error; test with `operator bool()`.
	 */
	ResourceLease(ResourceRegistry& resourceRegistry, ResourceHandle<TTag> resourceHandle)
		: registry(&resourceRegistry), handle(resourceHandle)
	{
		if (!registry->retain(handle.id))
			registry = nullptr;
	}
	ResourceLease(const ResourceLease&) = delete;
	ResourceLease& operator=(const ResourceLease&) = delete;
	/**
	 * @brief Moves a lease without changing the reference count.
	 * @param other Lease to take over; left inactive.
	 */
	ResourceLease(ResourceLease&& other) noexcept
		: registry(other.registry), handle(other.handle)
	{
		other.registry = nullptr;
		other.handle = {};
	}
	/**
	 * @brief Releases any current lease, then moves another lease into this one.
	 * @param other Lease to take over; left inactive.
	 * @return Reference to this lease.
	 */
	ResourceLease& operator=(ResourceLease&& other) noexcept
	{
		if (this != &other) {
			reset();
			registry = other.registry;
			handle = other.handle;
			other.registry = nullptr;
			other.handle = {};
		}
		return *this;
	}
	/** @brief Releases the retained resource, if any. */
	~ResourceLease() { reset(); }

	/**
	 * @brief Releases the retained resource and clears this lease.
	 * @note Idempotent: calling it twice releases only once, and it is safe on an inactive
	 *       lease.
	 */
	void reset()
	{
		if (registry) {
			registry->release(handle.id);
			registry = nullptr;
			handle = {};
		}
	}
	/** @brief Returns the handle retained by this lease. */
	ResourceHandle<TTag> get() const noexcept { return handle; }
	/** @brief Returns true when this lease successfully retained a resource. */
	explicit operator bool() const noexcept { return registry != nullptr; }
};

/**
 * @brief Detailed result from budget eviction.
 *
 * Returned by @ref Euph::Asset::ResidencyManager::evictUntilWithinBudgetDetailed, which needs
 * to report *why* it may have stopped early. The two lists answer different questions:
 * @ref evicted is what actually happened, @ref blocked is what would have happened next if the
 * blockers were removed.
 */
struct EvictionResult {
	/** @brief Asset ids actually evicted, in the order they were removed. */
	std::vector<AssetId> evicted;
	/**
	 * @brief Asset ids considered but protected by policy or references.
	 *
	 * Collected on every pass through the candidate list and then sorted and de-duplicated, so
	 * an id appears once however many passes it survived. Membership means at least one of:
	 * pinned, strongly referenced, or still required by a resident dependent.
	 */
	std::vector<AssetId> blocked;
	/** @brief True when all configured budgets are satisfied after eviction. */
	bool withinBudget = true;
};

/**
 * @brief Applies CPU-side budget policy and conservative eviction.
 *
 * The manager evicts only resources that have been explicitly marked Evictable, and it respects
 * pins, strong references, and resident dependents.
 *
 * @section euph_asset_residency_model The model in one paragraph
 *
 * The manager owns no state except the budgets and an optional catalog pointer: every usage
 * figure is *summed on demand* from the registry's per-entry costs. It never decides that
 * something should be loaded (that is the scheduler) and never removes the payload of a
 * resource that is still `Resident`; the application says "this is discardable now" with
 * @ref makeEvictable and the manager picks victims among those entries.
 *
 * @section euph_asset_residency_order Victim ordering
 *
 * When a budget is exceeded, the pass loops: collect candidates, sort, evict the first, repeat.
 * Candidates must be `Evictable`; a candidate is skipped and reported as blocked when it is
 * pinned, strongly referenced, or (with a catalog attached) still needed by another resident
 * entry. The survivors are ordered by:
 *
 * 1. lowest @ref Euph::Asset::StreamPriority wins (least important first);
 * 2. then the oldest @ref Euph::Asset::ResourceRegistry::Entry::lastTouchedFrame;
 * 3. then the largest total cost (all buckets summed) - one big victim frees more than a small
 *    one;
 * 4. then the lowest @ref Euph::Asset::AssetId, so equal candidates stay deterministic.
 *
 * Evicting one asset performs `Evictable -> Evicting -> Known`, clears the payload, all costs
 * and the @ref Euph::Asset::ResidencyFlag::EvictionRequested flag, and leaves the entry (and its
 * handle generation) alive so a later request can reuse it.
 *
 * @section euph_asset_residency_example Wiring it up
 *
 * @code
 * Euph::Asset::ResidencyManager residency(registry, &catalog);
 * residency.setBudget(Euph::Asset::BudgetKind::CpuBytes, 256ull << 20);
 * // ... frames go by, the registry accumulates residents ...
 * for (Euph::Asset::AssetId id : recentlyUnused) {
 *     residency.makeEvictable(id);            // Resident (or Evictable) -> Evictable
 * }
 * const Euph::Asset::EvictionResult result = residency.evictUntilWithinBudgetDetailed();
 * if (!result.withinBudget) {
 *     // everything evictable is pinned/retained/depended upon: raise the budget or relax policy
 * }
 * @endcode
 *
 * @section euph_asset_residency_threading Threading
 *
 * Not thread-safe, and it calls back into the registry on every step, so it must run on the
 * thread that owns both. With a catalog attached, every candidate scan resolves dependencies per
 * entry, so it is noticeably more expensive than plain budget arithmetic.
 */
class MH_EUPH_API ResidencyManager {
private:
	/** @brief Registry whose entries are measured and evicted. */
	ResourceRegistry& registry;
	/** @brief Optional catalog used to protect assets that resident entries depend on. */
	const AssetCatalog* catalog = nullptr;
	/** @brief Configured byte budget per @ref Euph::Asset::BudgetKind. */
	std::array<std::uint64_t, static_cast<std::size_t>(BudgetKind::Count)> budgets;

	/**
	 * @brief Tests whether every budget is currently satisfied.
	 * @return `true` when usage is less than or equal to the budget for every kind.
	 * @note Usage *equal* to the budget is fine; only strictly greater values count as over.
	 */
	bool withinBudgets() const;
	/**
	 * @brief Sums every cost bucket of one entry.
	 * @param entry Entry to measure.
	 * @return Total over all @ref Euph::Asset::BudgetKind values, used as the eviction
	 *         tie-breaker.
	 */
	static std::uint64_t totalReleasableCost(const ResourceRegistry::Entry& entry);
	/**
	 * @brief Checks whether another resident entry depends on id.
	 * @param id Dependency candidate.
	 * @return `true` when some other `Resident`/`Evictable` entry has @p id in its transitive
	 *         dependencies.
	 * @note Without a catalog this always returns `false`, so dependency protection is opt-in.
	 */
	bool hasResidentDependent(AssetId id) const;
public:
	/**
	 * @brief Creates a manager for a registry and optional dependency catalog.
	 * @param resourceRegistry Registry to manage; must outlive the manager.
	 * @param assetCatalog Optional catalog used to protect dependencies during eviction; may be
	 *        `nullptr`, which disables dependency protection.
	 * @note All budgets start at `UINT64_MAX`, i.e. effectively unlimited; an unlimited budget
	 *       makes @ref evictUntilWithinBudget a no-op.
	 */
	explicit ResidencyManager(ResourceRegistry& resourceRegistry, const AssetCatalog* assetCatalog = nullptr);
	/**
	 * @brief Updates the dependency catalog used for eviction protection.
	 * @param assetCatalog New catalog, or `nullptr` to disable dependency protection.
	 */
	void setCatalog(const AssetCatalog* assetCatalog);
	/**
	 * @brief Sets the byte budget for one budget kind.
	 * @param kind Bucket to configure.
	 * @param bytes Maximum allowed usage; usage above this value triggers eviction.
	 */
	void setBudget(BudgetKind kind, std::uint64_t bytes);
	/** @brief Returns the configured byte budget for one kind. */
	std::uint64_t budget(BudgetKind kind) const;
	/**
	 * @brief Returns current registry usage for one budget kind.
	 * @param kind Bucket to sum.
	 * @return Sum of that cost over every live registry entry - including entries that are not
	 *         `Resident`, so it is exactly the figure compared against the budget.
	 */
	std::uint64_t currentUsage(BudgetKind kind) const;
	/**
	 * @brief Marks a resident resource as eligible for eviction.
	 * @param id Asset id to mark.
	 * @return `true` when the transition `Resident -> Evictable` (or `Evictable -> Evictable`)
	 *         was legal and applied.
	 * @note This is the only way an asset becomes an eviction candidate; nothing else in the
	 *       runtime calls it.
	 */
	bool makeEvictable(AssetId id);
	/**
	 * @brief Pins a resource so budget eviction will not remove it.
	 * @param id Asset id to pin.
	 * @note Sets @ref Euph::Asset::ResidencyFlag::Pinned; it does not retain the resource and
	 *       does not stop @ref Euph::Asset::ResourceRegistry::destroy.
	 */
	void pin(AssetId id);
	/** @brief Clears a resource pin. */
	void unpin(AssetId id);
	/**
	 * @brief Evicts deterministic candidates until budgets are satisfied or blocked.
	 * @return Full report: what was evicted, what was blocked, and whether the budgets ended up
	 *         satisfied.
	 * @note Termination: the loop stops as soon as the budgets are met, or as soon as a pass
	 *       yields no candidates. It cannot spin forever, because every iteration either removes
	 *       an entry or breaks out.
	 */
	EvictionResult evictUntilWithinBudgetDetailed();
	/**
	 * @brief Evicts candidates and returns only the evicted ids.
	 * @return Ids of the entries that were evicted, in eviction order.
	 * @note Convenience wrapper over @ref evictUntilWithinBudgetDetailed for callers that do not
	 *       care why the pass stopped.
	 */
	std::vector<AssetId> evictUntilWithinBudget();
};

/**
 * @brief Output produced by a StreamingScheduler decoder callback.
 *
 * The decoder is the only place where asset-specific knowledge lives, so this struct is its whole
 * output surface: what to store, what it costs, and what kind of resource it turned out to be.
 *
 * @code
 * Euph::Asset::DecodedAsset decodeImage(const Euph::Asset::AssetRecord& record,
 *                                       std::span<const std::byte> bytes)
 * {
 *     Euph::Asset::DecodedAsset decoded;
 *     auto target = std::make_shared<Euph::Media::Image::DecodeTarget>();
 *     decodePNGInto(*target, bytes);                          // your decoder of choice
 *     decoded.payload = { target, target->totalBytes(), "decode-target" };
 *     decoded.cpuBytes = target->totalBytes();
 *     decoded.kind = Euph::Asset::ResourceKind::Image;        // re-types the entry
 *     return decoded;
 * }
 * @endcode
 *
 * @warning Throwing from the decoder is the supported way to report failure: the scheduler catches
 *          it and calls `ResourceRegistry::fail` with
 *          @ref Euph::Asset::FailureReason::DecodeError and `what()` as the message.
 */
struct DecodedAsset {
	/** @brief Optional resident CPU payload to store in ResourceRegistry. */
	ResidentPayload payload;
	/**
	 * @brief CPU byte cost to account after decode.
	 *
	 * When non-zero this **overrides** the cost that `setPayload` derived from `payload.bytes`;
	 * when zero, the payload's own byte count stands. Use it when the interesting cost is not the
	 * payload object's size (for example when the payload shares storage).
	 */
	std::uint64_t cpuBytes = 0;
	/** @brief Audio byte cost to account after decode; always written, `0` is meaningful. */
	std::uint64_t audioBytes = 0;
	/**
	 * @brief Resource kind produced by the decode step.
	 *
	 * The scheduler re-types the registry entry when this differs from the stored kind and is not
	 * @ref Euph::Asset::ResourceKind::Blob. Leaving it at `Blob` therefore means "do not touch the
	 * kind", not "the result is a blob".
	 */
	ResourceKind kind = ResourceKind::Blob;
};

/**
 * @brief Tunable behavior for StreamingScheduler.
 *
 * Passed by value to the constructor and stored as a copy; changing the caller's copy afterwards
 * has no effect.
 */
struct StreamingSchedulerOptions {
	/**
	 * @brief Maximum number of I/O jobs running at once.
	 *
	 * This caps *jobs*, not threads: it limits how many assets are between `LoadingIO`/`Decoding`
	 * and completion, and therefore how much in-flight memory the queue can accumulate. The pool
	 * needs at least this many threads to use the limit fully. `0` is clamped to `1` by the
	 * scheduler constructor.
	 */
	std::size_t maxIoJobs = 2;
	/**
	 * @brief When true, requests dependency assets before the parent asset.
	 *
	 * Enabled, a request for an asset whose dependencies are not resident recursively requests them
	 * with the same priority and parks the parent in
	 * @ref Euph::Asset::ResidencyState::WaitingDependencies. Disabled, the parent still waits, but
	 * the caller is responsible for requesting the dependencies.
	 */
	bool autoRequestDependencies = true;
	/**
	 * @brief When true, failed assets may be requested again.
	 *
	 * Enabled, `request` resets a `Failed` entry to `Known` before trying again. Disabled (the
	 * default), a failed entry stays failed and later requests fail immediately, which is usually
	 * what you want for a genuinely missing asset.
	 */
	bool retryFailedRequests = false;
};

/**
 * @brief Coordinates dependency checks, async byte reads, decode callbacks, and commits.
 *
 * StreamingScheduler uses the caller-provided ThreadPool and loader/decoder callbacks. It never
 * talks to Kaldi or renderer APIs; decoded payloads remain CPU-side in ResourceRegistry.
 *
 * @section euph_asset_sched_pipeline The pipeline in detail
 *
 * One request walks through five stages. Every stage runs on the polling thread except the two
 * marked *(worker)*, which run on the thread pool:
 *
 * 1. **request** - `requestInternal` resolves the transitive dependencies. A missing record or a
 *    broken graph fails the entry immediately. If dependencies are not resident yet, each
 *    non-resident one is requested in turn when
 *    @ref Euph::Asset::StreamingSchedulerOptions::autoRequestDependencies is set, and the parent is
 *    parked in `WaitingDependencies`. Otherwise the parent is queued.
 * 2. **queue** - queued ids wait for a free I/O slot. `startQueuedRequests` starts the
 *    highest-priority one first, breaks ties by the *lower* asset id, and asks the Loader for a
 *    device. Errors here (unknown record, no loader, throwing loader, null device) fail the entry
 *    with `IoError`.
 * 3. **read** *(worker)* - the device, the record and a stop token are captured by value into a
 *    pool task that reads exactly `storedSize` bytes. The entry is `LoadingIO` and its
 *    @ref Euph::Asset::BudgetKind::IoInflightBytes is `storedSize`.
 * 4. **decode** *(worker)* - `completeIo` moves the bytes into a second pool task that invokes the
 *    Decoder. The entry is `Decoding`, `IoInflightBytes` is zero, and
 *    @ref Euph::Asset::BudgetKind::DecodeInflightBytes holds the byte count.
 * 5. **commit** - `completeDecode` stores the payload, accounts the costs, re-types the entry when
 *    the decoder said so, and moves it to `Resident`.
 *
 * @section euph_asset_sched_poll Polling, wake-up and bookkeeping
 *
 * `pollCompletions` is the heartbeat. Each call drains every finished stage, drops the jobs whose
 * entry reached `Resident` or `Failed`, wakes waiting parents whose dependencies became resident,
 * and fills the freed I/O slots from the queue. It never blocks, so it is safe to call once per
 * frame; a busy-wait loop should add a short sleep so the worker threads get CPU time.
 *
 * The three private containers mirror the three pipeline stages behind I/O:
 *
 * - `queued` - requests eligible to start I/O, waiting for a slot;
 * - `waiting` - parents blocked on dependencies (woken by `wakeWaitingRequests`);
 * - `jobs` - requests that own a running future; at most one per asset.
 *
 * @section euph_asset_sched_errors Errors and cancellation
 *
 * Every failure path ends in `ResourceRegistry::fail` with a reason and a message, and clears both
 * in-flight costs. Cancellation is cooperative; see @ref euph_asset_runtime_limits for how a
 * cancelled read is reported.
 *
 * @section euph_asset_sched_threading Threading summary
 *
 * The scheduler itself is not thread-safe; only the read and decode tasks are concurrent, and they
 * touch nothing but their captured values. See @ref euph_asset_runtime_threading for the full
 * contract and @ref euph_asset_runtime_lifetime for destruction order.
 */
class MH_EUPH_API StreamingScheduler {
public:
	/**
	 * @brief Opens an I/O device for one asset record.
	 *
	 * Runs on the polling thread, inside `pollCompletions`. The returned device is kept alive by the
	 * read job (it is a shared, reference-counted `Elv::Io::sDevice`), so it may be created and
	 * dropped freely.
	 *
	 * A typical implementation looks at `record.path` and returns an opened `Euph::Io::File`,
	 * `Euph::Io::PhysFS` reader or memory device. `record.offset` and `record.storedSize` are applied
	 * by the scheduler, not by the loader.
	 *
	 * @param record Record describing the asset to open.
	 * @return Open device, or an empty/null device to signal failure (the scheduler then records
	 *         @ref Euph::Asset::FailureReason::IoError).
	 */
	using Loader = std::function<Elv::Io::sDevice(const AssetRecord&)>;
	/**
	 * @brief Converts stored bytes into a decoded asset payload and costs.
	 *
	 * Runs on a pool thread. The span points at a buffer owned by the task, so it is valid for the
	 * duration of the call only - copy what you need to keep. Throwing reports
	 * @ref Euph::Asset::FailureReason::DecodeError with `what()` as the message.
	 *
	 * @param record Record that produced the bytes (useful for `type`, `compression`,
	 *        `decodedSize`).
	 * @param bytes Raw stored bytes, exactly `storedSize` long.
	 * @return Decoded payload and its accounted costs.
	 */
	using Decoder = std::function<DecodedAsset(const AssetRecord&, std::span<const std::byte>)>;
private:
	/**
	 * @brief One in-flight request and the future representing its current stage.
	 *
	 * Defined in the implementation file because no caller needs it. The two futures are never both
	 * active: `ioFuture` is valid while the stage is `Io`, `decodeFuture` while it is `Decode`.
	 */
	struct Job;

	/** @brief Catalog consulted for records; must outlive the scheduler. */
	const AssetCatalog& catalog;
	/** @brief Registry receiving every state change; must outlive the scheduler. */
	ResourceRegistry& registry;
	/** @brief Pool that executes reads and decodes; must outlive the scheduler. */
	Elv::Util::ThreadPool& executor;
	/** @brief Callback that opens devices; copied at construction. */
	Loader loader;
	/** @brief Callback that decodes bytes; copied at construction. */
	Decoder decoder;
	/** @brief Behaviour options, copied at construction. */
	StreamingSchedulerOptions options;
	/** @brief Requests eligible to start I/O, waiting for a free slot. */
	std::deque<AssetId> queued;
	/** @brief Parent requests parked until their dependencies are resident. */
	std::vector<AssetId> waiting;
	/** @brief Requests with a running read or decode. */
	std::vector<Job> jobs;

	/**
	 * @brief Shared implementation of a request.
	 * @param id Asset to request.
	 * @param priority Priority recorded on the entry and propagated to dependencies.
	 * @param dependencyRequest `true` when this call came from resolving another request's
	 *        dependencies; currently informational only.
	 * @return `true` when the request was accepted, is already satisfied, or is already in progress;
	 *         `false` when the record is unknown, the dependency graph is broken, or the entry could
	 *         not be moved to `Requested`.
	 * @note Unknown records create a blob entry and fail it with
	 *       @ref Euph::Asset::FailureReason::AssetNotFound, so the failure is observable through the
	 *       registry instead of only through the return value.
	 */
	bool requestInternal(AssetId id, StreamPriority priority, bool dependencyRequest);
	/**
	 * @brief Records priority and queues an id whose dependencies are satisfied.
	 * @param id Asset to queue.
	 * @param priority Priority to store on the entry.
	 * @return `true` when the asset is already resident, already queued or running, or was
	 *         successfully queued; `false` when the transition to `Requested` was rejected.
	 * @note Idempotent: requesting a resident or in-flight asset is a success, not an error.
	 */
	bool queueReadyRequest(AssetId id, StreamPriority priority);
	/**
	 * @brief Starts reads for queued requests while I/O slots are free.
	 *
	 * Called from `pollCompletions` only, so it always runs on the polling thread. Picks the
	 * highest-priority queued id (ties: lowest id), resolves the record, calls the Loader, marks the
	 * entry `LoadingIO`, accounts `IoInflightBytes`, and posts the read task. Any problem fails that
	 * entry and continues with the next candidate instead of aborting the loop.
	 */
	void startQueuedRequests();
	/**
	 * @brief Re-tests parked parents and queues the ones whose dependencies are now resident.
	 *
	 * A parent whose dependencies are still missing stays parked; see
	 * @ref euph_asset_runtime_limits for the consequence of a dependency that failed.
	 */
	void wakeWaitingRequests();
	/** @brief Returns true when id is in the queue awaiting an I/O slot. */
	bool hasQueued(AssetId id) const;
	/** @brief Returns true when id is parked waiting for dependencies. */
	bool hasWaiting(AssetId id) const;
	/** @brief Returns true when id owns a running read or decode job. */
	bool hasJob(AssetId id) const;
	/**
	 * @brief Finishes the read stage of a job and posts its decode.
	 * @param job Job whose `ioFuture` is ready.
	 *
	 * Retrieves the bytes (rethrowing a read failure as `IoError`), moves the in-flight cost from
	 * `IoInflightBytes` to `DecodeInflightBytes`, moves the entry to `Decoding`, and posts the decode
	 * task with the record and the decoder captured by value. A missing decoder is reported as
	 * `IoError` with a descriptive message, because it is detected here rather than inside the
	 * decoder.
	 */
	void completeIo(Job& job);
	/**
	 * @brief Finishes the decode stage of a job and commits its payload.
	 * @param job Job whose `decodeFuture` is ready.
	 *
	 * Retrieves the @ref Euph::Asset::DecodedAsset (rethrowing as `DecodeError`), clears
	 * `DecodeInflightBytes`, stores the payload, applies `cpuBytes`/`audioBytes`, re-types the entry
	 * when the decoder reported a different non-blob kind, and moves it to `Resident`.
	 */
	void completeDecode(Job& job);
	/**
	 * @brief Records a terminal failure for a job's asset.
	 * @param job Job that failed; only its id is used.
	 * @param reason Failure category to record.
	 * @param message Human-readable detail to record.
	 *
	 * Clears both in-flight costs before calling `ResourceRegistry::fail`, so a failed asset never
	 * keeps occupying the in-flight budgets.
	 */
	void failJob(Job& job, FailureReason reason, const std::string& message);
public:
	/**
	 * @brief Creates a scheduler over existing catalog, registry, executor, and callbacks.
	 * @param assetCatalog Catalog to resolve records and dependencies from; must outlive the
	 *        scheduler.
	 * @param resourceRegistry Registry that receives every state change; must outlive the scheduler.
	 * @param threadPool Pool used for reads and decodes; must outlive the scheduler.
	 * @param loaderCallback Callback that opens devices; copied.
	 * @param decoderCallback Callback that decodes bytes; copied.
	 * @param schedulerOptions Behaviour options; copied, with `maxIoJobs == 0` clamped to `1`.
	 * @note Construction performs no I/O and touches no entries: nothing starts until `request`
	 *       followed by `pollCompletions`.
	 */
	StreamingScheduler(const AssetCatalog& assetCatalog, ResourceRegistry& resourceRegistry, Elv::Util::ThreadPool& threadPool, Loader loaderCallback, Decoder decoderCallback, StreamingSchedulerOptions schedulerOptions = {});
	/**
	 * @brief Destroys the scheduler without waiting for pending jobs.
	 *
	 * @warning The destructor is defaulted, so it does **not** wait. Pending futures are destroyed
	 *          and their results are never delivered to the registry. Destroy the scheduler before
	 *          the thread pool (whose own destructor does wait for running tasks) and before the
	 *          registry and catalog.
	 */
	~StreamingScheduler();
	/**
	 * @brief Requests an asset and optionally its dependencies.
	 * @param id Asset to request.
	 * @param priority Priority to record; larger is more important. Propagated to dependencies.
	 * @return `true` when the request was accepted, already satisfied, or already in progress;
	 *         `false` when it was rejected (unknown record, broken dependency graph, illegal
	 *         transition).
	 * @note Requesting a resident asset is a cheap no-op that returns `true`; requesting one that is
	 *       already queued or running is likewise idempotent.
	 * @note This only enqueues work. Progress requires repeated @ref pollCompletions calls.
	 */
	bool request(AssetId id, StreamPriority priority = {});
	/**
	 * @brief Cancels a queued or waiting request that has not started I/O.
	 * @param id Asset to cancel.
	 * @return `true` when something was actually cancelled: an entry was removed from the queue or
	 *         the waiting list, or a running job was asked to stop.
	 * @note A cancelled request is failed with @ref Euph::Asset::FailureReason::Canceled and the
	 *       message `"Streaming request canceled"`, but the job itself stays in the scheduler until
	 *       the next @ref pollCompletions - and a read that observes the stop then overwrites the
	 *       reason with @ref Euph::Asset::FailureReason::IoError. See
	 *       @ref euph_asset_runtime_limits.
	 */
	bool cancel(AssetId id);
	/**
	 * @brief Polls I/O/decode futures and advances completed jobs.
	 *
	 * The only function that makes progress. It never blocks: every future is checked with a zero
	 * timeout, so the cost is proportional to the number of in-flight jobs. Call it once per frame,
	 * or in a loading loop.
	 */
	void pollCompletions();
	/**
	 * @brief Returns the current registry state for id.
	 * @param id Asset to query.
	 * @return The registry's state, i.e. @ref Euph::Asset::ResidencyState::Missing for unknown ids.
	 */
	ResidencyState state(AssetId id) const;
	/**
	 * @brief Returns the number of active background jobs.
	 * @return Size of the internal job list, i.e. how many assets are `LoadingIO` or `Decoding` (plus
	 *         finished jobs not yet retired by a poll).
	 */
	std::size_t pendingJobs() const noexcept;
	/**
	 * @brief Returns the number of requests waiting to start I/O.
	 * @return Size of the internal queue, i.e. how many assets are `Requested` and waiting for a
	 *         slot.
	 */
	std::size_t queuedRequests() const noexcept;
	/**
	 * @brief Returns the number of parent requests waiting on dependencies.
	 * @return Size of the internal waiting list, i.e. how many assets are in
	 *         `WaitingDependencies`.
	 */
	std::size_t waitingRequests() const noexcept;
};

/**
 * @brief Validates a typed handle and returns its const entry.
 * @note The @p parameters and the return value are documented on the in-class
 *       declaration above.
 */
template<typename TTag>
const ResourceRegistry::Entry* ResourceRegistry::entryFor(ResourceHandle<TTag> handle) const
{
	if (handle.slot >= entries.size())
		return nullptr;
	const Entry& entry = entries[handle.slot];
	if (!entry.active || entry.id != handle.id || entry.generation != handle.generation || entry.kind != TTag::kind)
		return nullptr;
	return &entry;
}

/**
 * @brief Validates a typed handle and returns its mutable entry.
 * @note The @p parameters and the return value are documented on the in-class
 *       declaration above.
 */
template<typename TTag>
ResourceRegistry::Entry* ResourceRegistry::entryFor(ResourceHandle<TTag> handle)
{
	return const_cast<Entry*>(static_cast<const ResourceRegistry&>(*this).entryFor(handle));
}

/**
 * @brief Creates or finds an entry and returns a handle of the requested tag.
 * @note The @p parameters and the return value are documented on the in-class
 *       declaration above.
 */
template<typename TTag>
ResourceHandle<TTag> ResourceRegistry::createHandle(AssetId id)
{
	ResourceHandle<GenericBlobResource> generic = create(TTag::kind, id);
	const Entry* entry = inspect(id);
	if (!entry || entry->kind != TTag::kind)
		return {};
	return { id, generic.generation, generic.slot };
}

/**
 * @brief Returns true when a typed handle still matches a live entry.
 * @note The @p parameters and the return value are documented on the in-class
 *       declaration above.
 */
template<typename TTag>
bool ResourceRegistry::isAlive(ResourceHandle<TTag> handle) const
{
	return entryFor(handle) != nullptr;
}

/**
 * @brief Returns the state behind a typed handle.
 * @note The @p parameters and the return value are documented on the in-class
 *       declaration above.
 */
template<typename TTag>
ResidencyState ResourceRegistry::state(ResourceHandle<TTag> handle) const
{
	const Entry* entry = entryFor(handle);
	return entry ? entry->state : ResidencyState::Missing;
}

/**
 * @brief Stores the last-touched frame through a typed handle.
 * @note The @p parameters and the return value are documented on the in-class
 *       declaration above.
 */
template<typename TTag>
bool ResourceRegistry::touch(ResourceHandle<TTag> handle, std::uint64_t frameIndex)
{
	Entry* entry = entryFor(handle);
	if (!entry)
		return false;
	entry->lastTouchedFrame = frameIndex;
	return true;
}

/**
 * @brief Registers a placeholder handle for the tag's kind.
 * @note The @p parameters and the return value are documented on the in-class
 *       declaration above.
 */
template<typename TTag>
void ResourceRegistry::setPlaceholder(ResourceHandle<TTag> handle)
{
	if (isAlive(handle))
		placeholders[kindIndex(TTag::kind)] = handle.slot;
}

/**
 * @brief Returns the registered placeholder for the tag's kind.
 * @note The @p parameters and the return value are documented on the in-class
 *       declaration above.
 */
template<typename TTag>
std::optional<ResourceHandle<TTag> > ResourceRegistry::placeholder() const
{
	const auto slot = placeholders[kindIndex(TTag::kind)];
	if (!slot || *slot >= entries.size())
		return std::nullopt;
	const Entry& entry = entries[*slot];
	if (!entry.active || entry.kind != TTag::kind)
		return std::nullopt;
	return ResourceHandle<TTag> { entry.id, entry.generation, *slot };
}

/**
 * @brief Returns the handle when it is resident, otherwise the registered placeholder.
 * @note The @p parameters and the return value are documented on the in-class
 *       declaration above.
 */
template<typename TTag>
ResourceHandle<TTag> ResourceRegistry::resolveOrPlaceholder(ResourceHandle<TTag> handle) const
{
	if (isAlive(handle) && state(handle) == ResidencyState::Resident)
		return handle;
	const auto fallback = placeholder<TTag>();
	return fallback ? *fallback : ResourceHandle<TTag> {};
}

} // namespace Asset
} // namespace Euph

#endif // EUPHASSETRUNTIME_HPP
