#ifndef EUPHASSETTYPES_H
#define EUPHASSETTYPES_H
/**
 * @file EuphAssetTypes.hpp
 * @brief Defines the small value types used by Euphemy's asset streaming runtime.
 */
#include <cstdint>
#include <string>
#include <vector>
namespace Euph {
namespace Asset {

/** @brief Stable, catalog-level identifier for an asset record or resource entry. */
using AssetId = std::uint64_t;

/**
 * @brief High-level lifetime state for a CPU-side streamed asset.
 *
 * These states intentionally describe Euphemy's non-GPU resource lifecycle.
 * Renderer/backend upload state is represented outside Euphemy.
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
	Unloaded = Missing,
	Queued = Requested,
	WaitingUpload = WaitingCommit,
	Uploading = Committing
	///@}
};

/** @brief Machine-readable reason for a terminal or diagnostic resource failure. */
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
	UploadError = CommitError
};

/** @brief Policy and diagnostic flags attached to a resource registry entry. */
enum class ResidencyFlag : std::uint32_t {
	None = 0,                    /**< No flags are set. */
	Pinned = 1u << 0,            /**< The resource must not be evicted by policy. */
	PlaceholderAllowed = 1u << 1,/**< A typed placeholder may be substituted. */
	Reloadable = 1u << 2,        /**< The resource can be loaded again after eviction. */
	CpuResident = 1u << 3,       /**< The CPU-side payload is resident. */
	ExternalResident = 1u << 4,  /**< Some external system has a resident copy. */
	EvictionRequested = 1u << 5  /**< Eviction has been requested but not completed. */
};

/** @brief Compression format declared by an asset record's stored bytes. */
enum class Compression {
	None, /**< Stored bytes are uncompressed. */
	Zstd  /**< Stored bytes use Zstandard compression. */
};

/** @brief Integer priority used to order streaming and eviction decisions. */
struct StreamPriority {
	/** @brief Larger values are treated as more important by schedulers. */
	int value = 0;

	/** @brief Orders priorities by numeric value. */
	constexpr bool operator<(const StreamPriority& right) const {
		return value < right.value;
	}

	/** @brief Compares priority values. */
	constexpr bool operator==(const StreamPriority& right) const = default;
};

/**
 * @brief Metadata for locating, decoding, and dependency-ordering one asset.
 *
 * An AssetRecord is descriptive only; actual lifetime, resident payloads, and
 * budgets live in ResourceRegistry and ResidencyManager.
 */
struct AssetRecord {
	/** @brief Stable catalog identifier. */
	AssetId id = 0;
	/** @brief Optional device/path hint used by loaders. */
	std::string path;
	/** @brief Byte offset inside the loader-provided device. */
	std::uint64_t offset = 0;
	/** @brief Number of stored bytes to read from offset. */
	std::uint64_t storedSize = 0;
	/** @brief Expected decoded byte cost, if known. */
	std::uint64_t decodedSize = 0;
	/** @brief Compression declared for the stored bytes. */
	Compression compression = Compression::None;
	/** @brief Free-form type string used by decoders and integration layers. */
	std::string type;
	/** @brief Direct dependencies that must be resident before this asset. */
	std::vector<AssetId> dependencies;

	/** @brief Compares all catalog metadata fields. */
	bool operator==(const AssetRecord& right) const = default;
};

/** @brief Combines two ResidencyFlag bitmasks. */
constexpr ResidencyFlag operator|(ResidencyFlag left, ResidencyFlag right)
{
	return static_cast<ResidencyFlag>(static_cast<std::uint32_t>(left) | static_cast<std::uint32_t>(right));
}

/** @brief Intersects two ResidencyFlag bitmasks. */
constexpr ResidencyFlag operator&(ResidencyFlag left, ResidencyFlag right)
{
	return static_cast<ResidencyFlag>(static_cast<std::uint32_t>(left) & static_cast<std::uint32_t>(right));
}

/** @brief Adds flags to an existing ResidencyFlag bitmask. */
inline ResidencyFlag& operator|=(ResidencyFlag& left, ResidencyFlag right)
{
	left = left | right;
	return left;
}

/** @brief Returns true when a flag is present in a bitmask. */
constexpr bool hasFlag(ResidencyFlag flags, ResidencyFlag flag)
{
	return (flags & flag) != ResidencyFlag::None;
}

}
}
#endif // EUPHASSETTYPES_H
