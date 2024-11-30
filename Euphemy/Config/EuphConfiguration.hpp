#ifndef EUPHCONFIGURATION_H
#define EUPHCONFIGURATION_H
#include <Euphemy/Config/EuphConfigurationElement.hpp>
#include <map>
namespace Euph {
namespace Conf {

class MH_EUPH_API ConfigurationSection {
public:
	typedef std::map<std::string,ConfigurationElement,std::less<>> SectionContainer;
	typedef SectionContainer::iterator iterator;
	typedef SectionContainer::const_iterator const_iterator;
	typedef SectionContainer::reverse_iterator reverse_iterator;
	typedef SectionContainer::const_reverse_iterator const_reverse_iterator;
private:
	SectionContainer container;
public:
	ConfigurationSection();
	ConfigurationSection(const ConfigurationSection& cpy);
	ConfigurationSection(ConfigurationSection&& mov);
	ConfigurationSection& operator=(const ConfigurationSection& cpy);
	ConfigurationSection operator=(ConfigurationSection&& mov);
	SectionContainer& getContainer();
	const SectionContainer& getContainer() const;
	// Setters
	void setString(const std::string_view& key, const std::string_view value);
	void setFloat(const std::string_view& key, ConfFloat value);
	void setInt(const std::string_view& key, ConfInt value);
	void setUint(const std::string_view& key, ConfUint value);
	void setBool(const std::string_view& key, ConfUint value);
	// Getters
	std::string_view getString(const std::string_view& key, const std::string_view& def="");
	ConfFloat getFloat(const std::string_view& key, ConfFloat def=static_cast<ConfFloat>(0));
	ConfInt getInteger(const std::string_view& key, ConfInt def=static_cast<ConfInt>(0));
	ConfUint getUInteger(const std::string_view& key, ConfUint def=static_cast<ConfUint>(0));
	bool getBool(const std::string_view& key, bool def=false);
	// Generic setter
	void insert(const std::string_view& key, const std::string_view value);
	void insert(const std::string_view& key, const std::string& value);
	// Convenience iterators
	iterator begin();
	const_iterator begin() const;
	iterator end();
	const_iterator end() const;
	reverse_iterator rbegin();
	const_reverse_iterator rbegin() const;
	reverse_iterator rend();
	const_reverse_iterator rend() const;
	iterator find(const std::string_view& key);
	const_iterator find(const std::string_view& key) const;
};

class MH_EUPH_API Configuration
{
public:
	typedef std::map<std::string,ConfigurationSection,std::less<>> SectionMap;
	typedef SectionMap::iterator iterator;
	typedef SectionMap::const_iterator const_iterator;
	typedef SectionMap::reverse_iterator reverse_iterator;
	typedef SectionMap::const_reverse_iterator const_reverse_iterator;
private:
	SectionMap sections;
public:
	Configuration();
	Configuration(const Configuration& cpy);
	Configuration(Configuration&& mov);
	Configuration& operator=(const Configuration& cpy);
	Configuration operator=(Configuration&& mov);
	Configuration(std::istream& sstream);
	Configuration(const std::string& strng);
	SectionMap& getSections();
	const SectionMap& getSections() const;
	void fromString(std::istream& sstream);
	void fromString(const std::string& strng);
	// Convenience iterators
	iterator begin();
	const_iterator begin() const;
	iterator end();
	const_iterator end() const;
	reverse_iterator rbegin();
	const_reverse_iterator rbegin() const;
	reverse_iterator rend();
	const_reverse_iterator rend() const;
	iterator find(const std::string_view& key);
	const_iterator find(const std::string_view& key) const;
};

}
}

template<class CharT, class Traits = std::char_traits<CharT>>
std::basic_ostream<CharT,Traits>& operator<<(std::basic_ostream<CharT,Traits>& left, Euph::Conf::ConfigurationSection& right) {
	for(auto it = std::begin(right) ; it != std::end(right); ++it)
	{
		left << it->first << '=' << it->second << std::endl;
	}
	return left;
}
template<class CharT, class Traits = std::char_traits<CharT>>
std::basic_ostream<CharT,Traits>& operator<<(std::basic_ostream<CharT,Traits>& left, Euph::Conf::Configuration& right) {
	for(auto it = std::begin(right) ; it != std::end(right); ++it)
	{
		left << '[' << it->first << ']' << '\n' << it->second << std::endl;
	}
	return left;
}

#endif // EUPHCONFIGURATION_H
