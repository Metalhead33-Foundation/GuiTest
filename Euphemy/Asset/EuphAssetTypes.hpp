#ifndef EUPHASSETTYPES_H
#define EUPHASSETTYPES_H
#include <cstdint>
#include <string>
#include <vector>
namespace Euph {
namespace Asset {

using AssetId = std::uint64_t;
enum class ResidencyState {
	Unloaded,
	Queued,
	LoadingIO,
	Decoding,
	Resident,
	Evicting,
	Failed
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

}
}
#endif // EUPHASSETTYPES_H
