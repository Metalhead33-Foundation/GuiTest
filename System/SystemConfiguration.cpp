#include "SystemConfiguration.hpp"
#include <algorithm>
#include <sstream>
#include <exception>
namespace SYS {

std::string& ltrim(std::string &s)
{
    auto it = std::find_if(s.begin(), s.end(),
                    [](char c) {
                        return !std::isspace<char>(c, std::locale::classic());
                    });
    s.erase(s.begin(), it);
    return s;
}

std::string& rtrim(std::string &s)
{
	auto it = std::find_if(s.rbegin(), s.rend(),
					[](char c) {
						return !std::isspace<char>(c, std::locale::classic());
					});
	s.erase(it.base(), s.end());
	return s;
}

std::string& trim(std::string &s) {
    return ltrim(rtrim(s));
}

const std::map<std::string, ConfigurationSection>& Configuration::getSections() const
{
	return sections;
}

std::map<std::string, ConfigurationSection>& Configuration::getSections()
{
	return sections;
}

const std::vector<std::string> DefaultSections = {
	"Renderer", "Audio"
};

void Configuration::insertDefaultSections()
{
	for(const auto& it : DefaultSections) {
		sections.insert_or_assign(it, ConfigurationSection());
	}
}

Configuration::Configuration()
{
	insertDefaultSections();
}

Configuration::Configuration(std::ifstream& file)
{
	insertDefaultSections();
	if(!file.good()) {
		return;
	}
	// Read file line by line
	std::string line;
	std::string section;
	while (std::getline(file, line)) {
		// Remove comments first
		size_t commentStart = line.find(';');
		if(commentStart != std::string::npos) {
			line = line.substr(0,commentStart);
		}
		// Extract section and key-value pair
		size_t section_start = line.find('[');
		size_t section_end = line.find(']');
		size_t key_start = line.find('=');
		if (section_start != std::string::npos && section_end != std::string::npos) {
		// Line defines a section
		section = line.substr(section_start + 1, section_end - section_start - 1);
		section = trim(section);
		if(!sections.contains(section)) sections[section] = ConfigurationSection(); // Create empty map for section
		} else if (key_start != std::string::npos) {
		// Line defines a key-value pair
		std::string key = line.substr(0, key_start);
		std::string value = line.substr(key_start + 1);
		key = trim(key);
		value = trim(value);
		sections[section].values[key] = value; // Store key-value pair in last section
		}
	}
}

void Configuration::iterateOverSections(const std::function<void (const std::string&)>& onSectionStart, const std::function<void (const std::string&, const std::string&, const std::string&)>& onValue)
{
	for(auto sect = std::begin(sections); sect != std::end(sections); ++sect) {
		onSectionStart(sect->first);
		for(auto kv = std::begin(sect->second.values); kv != std::end(sect->second.values); ++kv) {
			onValue(sect->first,kv->first,kv->second);
		}
	}
}

const std::string& ConfigurationSection::getValueOrDefaultStr(const std::string& key, const std::string& defaultValue) const
{
	try {
	auto kv = values.find(key);
	if(kv == std::end(values)) return defaultValue;
	else return kv->second;
	} catch (const std::exception& exp) {
		return defaultValue;
	}
}

int ConfigurationSection::getValueOrDefaultInt(const std::string& key, int defaultValue) const
{
	try {
	auto kv = values.find(key);
	int value;
	if(kv == std::end(values)) return defaultValue;
	std::stringstream(kv->second) >> value;
	return value;
	} catch (const std::exception& exp) {
		return defaultValue;
	}
}

double ConfigurationSection::getValueOrDefaultDouble(const std::string& key, double defaultValue) const
{
	try {
	auto kv = values.find(key);
	double value;
	if(kv == std::end(values)) return defaultValue;
	std::stringstream(kv->second) >> value;
	return value;
	} catch (const std::exception& exp) {
		return defaultValue;
	}
}

bool ConfigurationSection::getValueOrDefaulBoolean(const std::string& key, bool defaultValue) const
{
	try {
	auto kv = values.find(key);
	bool value;
	if(kv == std::end(values)) return defaultValue;
	std::stringstream(kv->second) >> value;
	return value;
	} catch (const std::exception& exp) {
		return defaultValue;
	}
}


}
