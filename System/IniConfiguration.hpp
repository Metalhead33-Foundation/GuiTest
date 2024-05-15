#ifndef INICONFIGURATION_H
#define INICONFIGURATION_H
#include <string>
#include <sstream>
#include <map>

struct IniConfigurationSection {
	typedef std::map<std::string,std::string> Map;
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
	iterator insert(const std::string& key, std::string&& value);
	std::string& operator[](const std::string& key);
	const std::string& getValueOrDefault(const std::string& key, const std::string& defaultValue);
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
