#ifndef INICONFIGURATION_H
#define INICONFIGURATION_H
#include <string>
#include <sstream>
#include <map>
#include <cstdint>
#include <vector>
#ifdef INI_BIGSTRING
#define INI_STRING_STORAGE_SIZE 0x1FFFFF
typedef uint32_t IniStringPtr;
#else
#define INI_STRING_STORAGE_SIZE 0xFFFF
typedef uint16_t IniStringPtr;
#endif
struct SizedString {
	IniStringPtr ptr;
	IniStringPtr size;
};
enum class IniType : uint8_t {
	INI_STRING,
	INI_INTEGER,
	INI_UINTEGER,
	INI_FLOAT,
	INI_BOOLEAN
};

union IniValue {
	SizedString i_str;
	bool i_bool;
#ifdef INI_BIGNUMBERS
	double i_float;
	int64_t i_int;
	uint64_t i_uint;
#else
	float i_float;
	int32_t i_int;
	uint32_t i_uint;
#endif
};

struct IniConfigurationData {
	IniType type;
	IniValue value;
	IniConfigurationData();
	IniConfigurationData(IniType type, const std::string& value);
	IniConfigurationData(const IniConfigurationData& cpy);
	IniConfigurationData& operator=(const IniConfigurationData& cpy);
	static std::vector<char> INI_STRING_STORAGE;
	static IniStringPtr INI_STRING_STORAGE_PTR;
	std::string_view asStringView() const;
	std::string toString() const;
};
template <typename T> std::basic_ostream<T>& operator<<(std::basic_ostream<T>& left, const IniConfigurationData& right) {
	switch (right.type) {
	case IniType::INI_STRING:
		return left << std::string_view(&IniConfigurationData::INI_STRING_STORAGE[right.value.i_str.ptr], right.value.i_str.size - 1);
	case IniType::INI_INTEGER:
		return left << right.value.i_int;
	case IniType::INI_UINTEGER:
		return left << right.value.i_uint;
	case IniType::INI_FLOAT:
		return left << right.value.i_float;
	case IniType::INI_BOOLEAN:
		return left << right.value.i_bool;
	}
}

struct IniConfigurationSection {
	typedef std::map<std::string,IniConfigurationData> Map;
	typedef Map::iterator iterator;
	typedef Map::const_iterator const_iterator;
	Map map;
	IniConfigurationSection();
	IniConfigurationSection(const IniConfigurationSection& cpy);
	IniConfigurationSection(IniConfigurationSection&& mov);
	IniConfigurationSection& operator=(const IniConfigurationSection& cpy);
	IniConfigurationSection& operator=(IniConfigurationSection&& mov);
	iterator find(const std::string& key);
	const_iterator find(const std::string& key) const;
	iterator begin();
	const_iterator begin() const;
	iterator end();
	const_iterator end() const;
	iterator insert(const std::string& key, const std::string& value);
	IniConfigurationData& operator[](const std::string& key);
	iterator getValueOrDefault(const std::string& key, const std::string& defaultValue);
};

class IniConfiguration
{
public:
	typedef std::map<std::string,IniConfigurationSection> SectionMap;
	typedef SectionMap::iterator iterator;
	typedef SectionMap::const_iterator const_iterator;
private:
	SectionMap sections;
public:
	IniConfiguration(std::istream& sstream);
	const SectionMap &getSections() const;
	SectionMap &getSections();
	iterator find(const std::string& sectionName);
	const_iterator find(const std::string& sectionName) const;
	iterator begin();
	const_iterator begin() const;
	iterator end();
	const_iterator end() const;
	iterator insert(const std::string& sectionName, const IniConfigurationSection& sect);
	iterator insert(const std::string& sectionName, IniConfigurationSection&& sect);
	IniConfigurationSection& operator[](const std::string& sectionName);
};

#endif // INICONFIGURATION_H
