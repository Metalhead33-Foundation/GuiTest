#include "IniConfiguration.hpp"
#include <algorithm>
#include <cctype>
#include <locale>
#include <stdexcept>

const IniConfiguration::SectionMap &IniConfiguration::getSections() const
{
	return sections;
}

IniConfiguration::SectionMap &IniConfiguration::getSections()
{
	return sections;
}

IniConfiguration::iterator IniConfiguration::find(const std::string &sectionName)
{
	return sections.find(sectionName);
}

IniConfiguration::const_iterator IniConfiguration::find(const std::string &sectionName) const
{
	return sections.find(sectionName);
}

IniConfiguration::iterator IniConfiguration::begin()
{
	return sections.begin();
}

IniConfiguration::const_iterator IniConfiguration::begin() const
{
	return sections.begin();
}

IniConfiguration::iterator IniConfiguration::end()
{
	return sections.end();
}

IniConfiguration::const_iterator IniConfiguration::end() const
{
	return sections.end();
}

IniConfiguration::iterator IniConfiguration::insert(const std::string &sectionName, const IniConfigurationSection &sect)
{
	return sections.insert(std::make_pair(sectionName,sect)).first;
}

IniConfiguration::iterator IniConfiguration::insert(const std::string &sectionName, IniConfigurationSection &&sect)
{
	return sections.insert(std::make_pair(sectionName,std::move(sect))).first;
}

IniConfigurationSection &IniConfiguration::operator [](const std::string &sectionName)
{
	return sections[sectionName];
}

// trim from start (in place)
inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
}

// trim from end (in place)
inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

inline void trim(std::string &s) {
	rtrim(s);
	ltrim(s);
}

inline size_t getStringCommentStartPos(const std::string& str) {
	bool isInString = false;
	for(size_t i = 0; i < str.size(); ++i) {
		if(str[i] == '\"') isInString = !isInString;
		else if(str[i] == ';' && !isInString) return i;
	}
	return std::string::npos;
}

inline void unescapeString(std::string& string) {
	if(string.empty()) return;
	if(string[0] != '\"' || string[string.size()-1] != '\"') return;
	string = string.substr(1,string.size()-2);
	std::stringstream sstrm;
	bool isEscaped = false;
	for(const auto& it : string) {
		if(isEscaped) {
			switch (it) {
			case '\\': sstrm << '\\'; isEscaped = false; break;
			case '\'': sstrm << '\''; isEscaped = false; break;
			case '\"': sstrm << '\"'; isEscaped = false; break;
			case '0': sstrm << '\0'; isEscaped = false; break;
			case 'n': sstrm << '\n'; isEscaped = false; break;
			case 'a': sstrm << '\a'; isEscaped = false; break;
			case 'b': sstrm << '\b'; isEscaped = false; break;
			case 't': sstrm << '\t'; isEscaped = false; break;
			case 'v': sstrm << '\v'; isEscaped = false; break;
			case 'r': sstrm << '\r'; isEscaped = false; break;
			case 'f': sstrm << '\f'; isEscaped = false; break;
			case '?': sstrm << '\?'; isEscaped = false; break;
			case ';': sstrm << ';'; isEscaped = false; break;
			case '#': sstrm << '#'; isEscaped = false; break;
			case ':': sstrm << ':'; isEscaped = false; break;
			}
		} else {
			if(it == '\\') isEscaped = true;
			else sstrm << it;
		}
	}
	string = sstrm.str();
	trim(string);
}

IniConfiguration::IniConfiguration(std::istream &sstream)
{
	std::string currLine;
	std::string sectionName;
	IniConfigurationSection currentSection;
	while( std::getline( sstream, currLine , '\n' ) ) {
		auto commentDelimiter = getStringCommentStartPos(currLine);

		if(commentDelimiter != std::string::npos) {
			currLine = currLine.substr(0, commentDelimiter - 1);
		}

		trim(currLine);
		if(currLine.empty()) continue;

		if(currLine[0] == '[') {
			auto indexForClosing = currLine.find(']');
			if(indexForClosing == std::string::npos) throw std::runtime_error("Malformed section starter!");
			if (!sectionName.empty()) {
				sections.emplace(sectionName,std::move(currentSection));
			}
			sectionName = currLine.substr(1, indexForClosing - 1);
			continue;
		}

		auto delimiterPos = currLine.find('=');
		if (delimiterPos == std::string::npos) throw std::runtime_error("Malformed line!");
		std::string key = currLine.substr(0, delimiterPos);
		std::string value = currLine.substr(delimiterPos + 1);
		trim(key);
		trim(value);
		unescapeString(value);
		currentSection.insert(key, value);
	}
	if (!sectionName.empty()) {
		sections.emplace(sectionName,std::move(currentSection));
	}
}

IniConfigurationSection::iterator IniConfigurationSection::find(const std::string &key)
{
	return map.find(key);
}

IniConfigurationSection::IniConfigurationSection()
{

}

IniConfigurationSection::IniConfigurationSection(const IniConfigurationSection &cpy)
	: map(cpy.map)
{

}

IniConfigurationSection::IniConfigurationSection(IniConfigurationSection &&mov)
	: map(std::move(mov.map))
{

}

IniConfigurationSection &IniConfigurationSection::operator=(const IniConfigurationSection &cpy)
{
	this->map = cpy.map;
	return *this;
}


IniConfigurationSection &IniConfigurationSection::operator=(IniConfigurationSection &&mov)
{
	this->map = std::move(mov.map);
	return *this;
}
IniConfigurationSection::const_iterator IniConfigurationSection::find(const std::string &key) const
{
	return map.find(key);
}

IniConfigurationSection::iterator IniConfigurationSection::begin()
{
	return map.begin();
}

IniConfigurationSection::const_iterator IniConfigurationSection::begin() const
{
	return map.begin();
}

IniConfigurationSection::iterator IniConfigurationSection::end()
{
	return map.end();
}

IniConfigurationSection::const_iterator IniConfigurationSection::end() const
{
	return map.end();
}

IniConfigurationSection::iterator IniConfigurationSection::insert(const std::string &key, const std::string &value)
{
	return map.emplace(key,value).first;
}

IniConfigurationSection::iterator IniConfigurationSection::insert(const std::string &key, std::string &&value)
{
	return map.emplace(key,std::move(value)).first;
}

const std::string &IniConfigurationSection::getValueOrDefault(const std::string &key, const std::string &defaultValue)
{
	auto iter = map.find(key);
	if(iter == map.end()) {
		map.emplace(key,defaultValue);
		return defaultValue;
	} else return iter->second;
}
