#ifndef EUPHASSETREGISTRY_HPP
#define EUPHASSETREGISTRY_HPP

#include <Euphemy/Asset/EuphAssetTypes.hpp>
#include <Euphemy/Config/EuphLib.hpp>
#include <Euphemy/Config/EuphConfiguration.hpp>
#include <optional>
#include <unordered_map>

namespace Euph {
namespace Asset {

class MH_EUPH_API AssetRegistry {
private:
	std::unordered_map<AssetId, AssetRecord> records;

public:
	void registerAsset(const AssetRecord& record);
	void registerAsset(AssetRecord&& record);
	bool contains(AssetId id) const;
	const AssetRecord* find(AssetId id) const;
	std::optional<AssetRecord> get(AssetId id) const;
	void clear();
	std::size_t size() const;
	void loadFromConfiguration(const Euph::Conf::Configuration& config);
};

MH_EUPH_API AssetId parseAssetId(std::string_view text);
MH_EUPH_API std::string assetIdToHex(AssetId id);
MH_EUPH_API Compression parseCompression(std::string_view text);
MH_EUPH_API std::string compressionToString(Compression compression);

} // namespace Asset
} // namespace Euph

#endif // EUPHASSETREGISTRY_HPP
