#ifndef EUPHASSETTYPES_H
#define EUPHASSETTYPES_H
#include <cstdint>
#include <string>
#include <vector>
namespace Euph {
namespace Asset {

using AssetId = std::uint64_t;
enum class ResidencyState : std::uint8_t {
	Missing,
	Known,
	Requested,
	WaitingDependencies,
	LoadingIO,
	Decoding,
	WaitingUpload,
	Uploading,
	Resident,
	Evictable,
	Evicting,
	Failed,

	Unloaded = Missing,
	Queued = Requested
};
enum class FailureReason : std::uint8_t {
	None,
	AssetNotFound,
	MissingDependency,
	DependencyCycle,
	InvalidTransition,
	IoError,
	DecodeError,
	UploadError,
	Canceled,
	BudgetExceeded,
	Unknown
};
enum class ResidencyFlag : std::uint32_t {
	None = 0,
	Pinned = 1u << 0,
	PlaceholderAllowed = 1u << 1,
	Reloadable = 1u << 2,
	CpuResident = 1u << 3,
	GpuResident = 1u << 4,
	EvictionRequested = 1u << 5
};
enum class Compression {
	None,
	Zstd
};

struct StreamPriority {
	int value = 0;

	constexpr bool operator<(const StreamPriority& right) const {
		return value < right.value;
	}

	constexpr bool operator==(const StreamPriority& right) const = default;
};

struct AssetRecord {
	AssetId id = 0;
	std::string path;
	std::uint64_t offset = 0;
	std::uint64_t storedSize = 0;
	std::uint64_t decodedSize = 0;
	Compression compression = Compression::None; // Do we really need this at all?
	std::string type;
	std::vector<AssetId> dependencies;

	bool operator==(const AssetRecord& right) const = default;
};

constexpr ResidencyFlag operator|(ResidencyFlag left, ResidencyFlag right)
{
	return static_cast<ResidencyFlag>(static_cast<std::uint32_t>(left) | static_cast<std::uint32_t>(right));
}

constexpr ResidencyFlag operator&(ResidencyFlag left, ResidencyFlag right)
{
	return static_cast<ResidencyFlag>(static_cast<std::uint32_t>(left) & static_cast<std::uint32_t>(right));
}

inline ResidencyFlag& operator|=(ResidencyFlag& left, ResidencyFlag right)
{
	left = left | right;
	return left;
}

constexpr bool hasFlag(ResidencyFlag flags, ResidencyFlag flag)
{
	return (flags & flag) != ResidencyFlag::None;
}

}
}
#endif // EUPHASSETTYPES_H
