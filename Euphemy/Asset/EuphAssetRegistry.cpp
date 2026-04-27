#include "EuphAssetRegistry.hpp"
#include <algorithm>
#include <charconv>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace Euph {
namespace Asset {

namespace {

std::string getValue(const Euph::Conf::ConfigurationSection& section, std::string_view key, std::string_view def = "")
{
	const auto& container = section.getContainer();
	auto it = container.find(key);
	std::string typedKey;
	if(it == container.end()) {
		for(char prefix : { 's', 'u', 'i', 'b', 'f' }) {
			typedKey.assign(1, prefix);
			typedKey.append(key);
			it = container.find(typedKey);
			if(it != container.end()) {
				break;
			}
		}
	}
	if(it == container.end()) {
		return std::string(def);
	}
	const auto& element = it->second;
	switch(element.type) {
		case Euph::Conf::ConfigurationElementType::CONF_FLOAT:
			return std::to_string(element.storage.f);
		case Euph::Conf::ConfigurationElementType::CONF_INT:
			return std::to_string(element.storage.i);
		case Euph::Conf::ConfigurationElementType::CONF_UINT:
			return std::to_string(element.storage.u);
		case Euph::Conf::ConfigurationElementType::CONF_BOOL:
			return element.storage.b ? "true" : "false";
		case Euph::Conf::ConfigurationElementType::CONF_STRING:
			return element.storage.s.copyIntoString();
	}
	return std::string(def);
}

std::uint64_t parseUint64(std::string_view text)
{
	std::uint64_t value = 0;
	auto begin = text.data();
	auto end = text.data() + text.size();
	if(text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
		begin += 2;
		auto result = std::from_chars(begin, end, value, 16);
		if(result.ec == std::errc() && result.ptr == end) {
			return value;
		}
	} else {
		auto result = std::from_chars(begin, end, value, 10);
		if(result.ec == std::errc() && result.ptr == end) {
			return value;
		}
	}
	throw std::runtime_error("Invalid unsigned integer in asset manifest");
}

std::vector<AssetId> parseDependencies(std::string_view text)
{
	std::vector<AssetId> dependencies;
	std::size_t start = 0;
	while(start < text.size()) {
		std::size_t comma = text.find(',', start);
		std::string_view part = comma == std::string_view::npos
			? text.substr(start)
			: text.substr(start, comma - start);
		while(!part.empty() && std::isspace(static_cast<unsigned char>(part.front()))) part.remove_prefix(1);
		while(!part.empty() && std::isspace(static_cast<unsigned char>(part.back()))) part.remove_suffix(1);
		if(!part.empty()) {
			dependencies.push_back(parseAssetId(part));
		}
		if(comma == std::string_view::npos) {
			break;
		}
		start = comma + 1;
	}
	return dependencies;
}

} // namespace

void AssetRegistry::registerAsset(const AssetRecord& record)
{
	records[record.id] = record;
}

void AssetRegistry::registerAsset(AssetRecord&& record)
{
	const AssetId id = record.id;
	records[id] = std::move(record);
}

bool AssetRegistry::contains(AssetId id) const
{
	return records.find(id) != records.end();
}

const AssetRecord* AssetRegistry::find(AssetId id) const
{
	auto it = records.find(id);
	return it == records.end() ? nullptr : &it->second;
}

std::optional<AssetRecord> AssetRegistry::get(AssetId id) const
{
	auto it = records.find(id);
	if(it == records.end()) {
		return std::nullopt;
	}
	return it->second;
}

void AssetRegistry::clear()
{
	records.clear();
}

std::size_t AssetRegistry::size() const
{
	return records.size();
}

void AssetRegistry::loadFromConfiguration(const Euph::Conf::Configuration& config)
{
	for(const auto& sectionPair : config.getSections()) {
		const std::string& sectionName = sectionPair.first;
		constexpr std::string_view prefix = "asset:";
		if(sectionName.rfind(prefix, 0) != 0) {
			continue;
		}
		AssetRecord record;
		record.id = parseAssetId(std::string_view(sectionName).substr(prefix.size()));
		record.path = getValue(sectionPair.second, "path");
		record.offset = parseUint64(getValue(sectionPair.second, "offset", "0"));
		record.storedSize = parseUint64(getValue(sectionPair.second, "storedSize", "0"));
		record.decodedSize = parseUint64(getValue(sectionPair.second, "decodedSize", "0"));
		record.compression = parseCompression(getValue(sectionPair.second, "compression", "none"));
		record.type = getValue(sectionPair.second, "type", "blob");
		record.dependencies = parseDependencies(getValue(sectionPair.second, "dependencies"));
		registerAsset(std::move(record));
	}
}

AssetId parseAssetId(std::string_view text)
{
	if(text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
		text.remove_prefix(2);
	}
	AssetId id = 0;
	auto result = std::from_chars(text.data(), text.data() + text.size(), id, 16);
	if(result.ec == std::errc() && result.ptr == text.data() + text.size()) {
		return id;
	}
	throw std::runtime_error("Invalid asset id");
}

std::string assetIdToHex(AssetId id)
{
	std::ostringstream stream;
	stream << std::hex << std::setw(16) << std::setfill('0') << id;
	return stream.str();
}

Compression parseCompression(std::string_view text)
{
	std::string lower(text);
	std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	if(lower.empty() || lower == "none") {
		return Compression::None;
	}
	if(lower == "zstd") {
		return Compression::Zstd;
	}
	throw std::runtime_error("Unknown asset compression: " + lower);
}

std::string compressionToString(Compression compression)
{
	switch(compression) {
		case Compression::None: return "none";
		case Compression::Zstd: return "zstd";
	}
	return "none";
}

} // namespace Asset
} // namespace Euph
