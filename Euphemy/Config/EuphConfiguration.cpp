#include "EuphConfiguration.hpp"

namespace Euph {
namespace Conf {

inline void ltrim(std::string_view &s) {
	size_t start = s.find_first_not_of(" \t\n\r\f\v");
	s = (start == std::string_view::npos) ? std::string_view() : s.substr(start);
}
inline void ltrim(std::string &s) {
	size_t start = s.find_first_not_of(" \t\n\r\f\v");
	s = (start == std::string::npos) ? std::string() : s.substr(start);
}

inline void rtrim(std::string_view &s) {
	size_t end = s.find_last_not_of(" \t\n\r\f\v");
	s = (end == std::string_view::npos) ? std::string_view() : s.substr(0, end + 1);
}

inline void rtrim(std::string &s) {
	size_t end = s.find_last_not_of(" \t\n\r\f\v");
	s = (end == std::string::npos) ? std::string() : s.substr(0, end + 1);
}

inline void trim(std::string_view &s) {
	ltrim(s);
	rtrim(s);
}

inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}

inline size_t getStringCommentStartPos(std::string_view str) {
	bool isInString = false;
	for (size_t i = 0; i < str.size(); ++i) {
		if (str[i] == '\"') isInString = !isInString;
		else if (str[i] == ';' && !isInString) return i;
	}
	return std::string_view::npos;
}

inline std::string unescapeString(std::string_view string) {
	if (string.empty() || string.front() != '\"' || string.back() != '\"')
		return std::string(string);

	string = string.substr(1, string.size() - 2);
	std::string result;
	result.reserve(string.size()+1);
	bool isEscaped = false;

	for (char ch : string) {
		if (isEscaped) {
			switch (ch) {
				case '\\': result += '\\'; break;
				case '\'': result += '\''; break;
				case '\"': result += '\"'; break;
				case '0': result += '\0'; break;
				case 'n': result += '\n'; break;
				case 'a': result += '\a'; break;
				case 'b': result += '\b'; break;
				case 't': result += '\t'; break;
				case 'v': result += '\v'; break;
				case 'r': result += '\r'; break;
				case 'f': result += '\f'; break;
				case '?': result += '\?'; break;
				case ';': result += ';'; break;
				case '#': result += '#'; break;
				case ':': result += ':'; break;
				default: result += ch; break;
			}
			isEscaped = false;
		} else if (ch == '\\') {
			isEscaped = true;
		} else {
			result += ch;
		}
	}
	trim(result);
	return result;
}

Configuration::SectionMap& Configuration::getSections()
{
	return sections;
}

const Configuration::SectionMap& Configuration::getSections() const
{
	return sections;
}

void Configuration::fromString(std::istream &sstream)
{
	std::string currLine;
	std::string sectionName;
	ConfigurationSection currentSection;
	while (std::getline(sstream, currLine, '\n')) {
		std::string_view lineView = currLine;
		auto commentDelimiter = getStringCommentStartPos(lineView);
		if (commentDelimiter != std::string_view::npos) {
			lineView = lineView.substr(0, commentDelimiter);
		}

		trim(lineView);
		if (lineView.empty()) continue;
		if (lineView.front() == '[') {
			auto indexForClosing = lineView.find(']');
			if (indexForClosing == std::string_view::npos)
				throw std::runtime_error("Malformed section starter!");

			if (!sectionName.empty()) {
				sections.emplace(std::move(sectionName), std::move(currentSection));
			}

			sectionName = std::string(lineView.substr(1, indexForClosing - 1));
			continue;
		}

		auto delimiterPos = lineView.find('=');
		if (delimiterPos == std::string_view::npos)
			throw std::runtime_error("Malformed line!");

		std::string key = std::string(lineView.substr(0, delimiterPos));
		std::string value = std::string(lineView.substr(delimiterPos + 1));
		trim(key);
		trim(value);
		value = unescapeString(value);
		currentSection.insert(std::move(key), std::move(value));
	}

	if (!sectionName.empty()) {
		sections.emplace(std::move(sectionName), std::move(currentSection));
	}
}

void Configuration::fromString(const std::string& strng)
{
	std::stringstream sstrm;
	sstrm.str(strng);
	fromString(sstrm);
}

Configuration::iterator Configuration::begin()
{
	return sections.begin();
}

Configuration::const_iterator Configuration::begin() const
{
	return sections.begin();
}

Configuration::iterator Configuration::end()
{
	return sections.end();
}

Configuration::const_iterator Configuration::end() const
{
	return sections.end();
}

Configuration::reverse_iterator Configuration::rbegin()
{
	return sections.rbegin();
}

Configuration::const_reverse_iterator Configuration::rbegin() const
{
	return sections.rbegin();
}

Configuration::reverse_iterator Configuration::rend()
{
	return sections.rend();
}

Configuration::const_reverse_iterator Configuration::rend() const
{
	return sections.rend();
}

Configuration::iterator Configuration::find(const std::string_view& key)
{
	return sections.find(key);
}

Configuration::const_iterator Configuration::find(const std::string_view& key) const
{
	return sections.find(key);
}

Configuration::Configuration()
{

}
Configuration::Configuration(const Configuration& cpy)
	: sections(cpy.sections)
{

}

Configuration::Configuration(Configuration&& mov)
	: sections(std::move(mov.sections))
{

}

Configuration& Configuration::operator=(const Configuration& cpy)
{
	this->sections = cpy.sections;
	return *this;
}

Configuration::Configuration(std::istream& sstream)
{
	fromString(sstream);
}

Configuration::Configuration(const std::string& strng)
{
	fromString(strng);
}
Configuration Configuration::operator=(Configuration&& mov)
{
	this->sections = std::move(mov.sections);
	return *this;
}

ConfigurationSection::SectionContainer& ConfigurationSection::getContainer()
{
	return container;
}

ConfigurationSection::ConfigurationSection()
{

}

ConfigurationSection::ConfigurationSection(const ConfigurationSection& cpy)
	: container(cpy.container)
{

}

ConfigurationSection::ConfigurationSection(ConfigurationSection&& mov)
	: container(std::move(mov.container))
{

}

ConfigurationSection& ConfigurationSection::operator=(const ConfigurationSection& cpy)
{
	this->container = cpy.container;
	return *this;
}
ConfigurationSection ConfigurationSection::operator=(ConfigurationSection&& mov)
{
	this->container = std::move(mov.container);
	return *this;
}

const ConfigurationSection::SectionContainer& ConfigurationSection::getContainer() const
{
	return container;
}

void ConfigurationSection::setString(const std::string_view& key, const std::string_view value)
{
	auto it = container.find(key);
	if(it == std::end(container)) {
		container.emplace(key, value);
	} else {
		auto& ref = it->second;
		ref.type = ConfigurationElementType::CONF_STRING;
		ref.storage.s.fromStringView(value);
	}
}

void ConfigurationSection::setFloat(const std::string_view& key, ConfFloat value)
{
	auto it = container.find(key);
	if(it == std::end(container)) {
		container.emplace(key, value);
	} else {
		auto& ref = it->second;
		ref.type = ConfigurationElementType::CONF_FLOAT;
		ref.storage.f = value;
	}

}

void ConfigurationSection::setInt(const std::string_view& key, ConfInt value)
{
	auto it = container.find(key);
	if(it == std::end(container)) {
		container.emplace(key, value);
	} else {
		auto& ref = it->second;
		ref.type = ConfigurationElementType::CONF_INT;
		ref.storage.i = value;
	}
}

void ConfigurationSection::setUint(const std::string_view& key, ConfUint value)
{
	auto it = container.find(key);
	if(it == std::end(container)) {
		container.emplace(key, value);
	} else {
		auto& ref = it->second;
		ref.type = ConfigurationElementType::CONF_UINT;
		ref.storage.u = value;
	}
}

void ConfigurationSection::setBool(const std::string_view& key, ConfUint value)
{
	auto it = container.find(key);
	if(it == std::end(container)) {
		container.emplace(key, value);
	} else {
		auto& ref = it->second;
		ref.type = ConfigurationElementType::CONF_BOOL;
		ref.storage.b = value;
	}

}

std::string_view ConfigurationSection::getString(const std::string_view& key, const std::string_view& def)
{
	auto it = container.find(key);
	if(it == std::end(container)) {
		container.emplace(key, def);
		return def;
	} else {
		auto& ref = it->second;
		if(ref.type == ConfigurationElementType::CONF_STRING) return ref.storage.s.asStringView();
		else throw std::runtime_error("The given element is NOT string type!");
	}
	return def;
}

ConfFloat ConfigurationSection::getFloat(const std::string_view& key, ConfFloat def)
{
	auto it = container.find(key);
	if(it == std::end(container)) {
		container.emplace(key, def);
		return def;
	} else {
		auto& ref = it->second;
		switch (ref.type) {
			case ConfigurationElementType::CONF_FLOAT:
				return ref.storage.f;
			case ConfigurationElementType::CONF_INT:
				return static_cast<ConfFloat>(ref.storage.i);
			case ConfigurationElementType::CONF_UINT:
				return static_cast<ConfFloat>(ref.storage.u);
			case ConfigurationElementType::CONF_BOOL:
				return static_cast<ConfFloat>(ref.storage.b);
			case ConfigurationElementType::CONF_STRING:
				throw std::runtime_error("The given element is NOT float type!");
		}
	}
	return def;
}

ConfInt ConfigurationSection::getInteger(const std::string_view& key, ConfInt def)
{
	auto it = container.find(key);
	if(it == std::end(container)) {
		container.emplace(key, def);
		return def;
	} else {
		auto& ref = it->second;
		switch (ref.type) {
			case ConfigurationElementType::CONF_FLOAT:
				return static_cast<ConfInt>(ref.storage.f);
			case ConfigurationElementType::CONF_INT:
				return ref.storage.i;
			case ConfigurationElementType::CONF_UINT:
				return static_cast<ConfInt>(ref.storage.u);
			case ConfigurationElementType::CONF_BOOL:
				return static_cast<ConfInt>(ref.storage.b);
			case ConfigurationElementType::CONF_STRING:
				throw std::runtime_error("The given element is NOT float type!");
		}
	}
	return def;
}

ConfUint ConfigurationSection::getUInteger(const std::string_view& key, ConfUint def)
{
	auto it = container.find(key);
	if(it == std::end(container)) {
		container.emplace(key, def);
		return def;
	} else {
		auto& ref = it->second;
		switch (ref.type) {
			case ConfigurationElementType::CONF_FLOAT:
				return static_cast<ConfUint>(ref.storage.f);
			case ConfigurationElementType::CONF_INT:
				return static_cast<ConfUint>(ref.storage.i);
			case ConfigurationElementType::CONF_UINT:
				return ref.storage.u;
			case ConfigurationElementType::CONF_BOOL:
				return static_cast<ConfUint>(ref.storage.b);
			case ConfigurationElementType::CONF_STRING:
				throw std::runtime_error("The given element is NOT float type!");
		}
	}
	return def;
}

bool ConfigurationSection::getBool(const std::string_view& key, bool def)
{
	auto it = container.find(key);
	if(it == std::end(container)) {
		container.emplace(key, def);
		return def;
	} else {
		auto& ref = it->second;
		switch (ref.type) {
			case ConfigurationElementType::CONF_FLOAT:
				return static_cast<bool>(ref.storage.f);
			case ConfigurationElementType::CONF_INT:
				return static_cast<bool>(ref.storage.i);
			case ConfigurationElementType::CONF_UINT:
				return static_cast<bool>(ref.storage.u);
			case ConfigurationElementType::CONF_BOOL:
				return ref.storage.b;
			case ConfigurationElementType::CONF_STRING:
				throw std::runtime_error("The given element is NOT float type!");
		}
	}
	return def;
}

void ConfigurationSection::insert(const std::string_view& key, const std::string_view value)
{
	insert(key, std::string(value));
}

void ConfigurationSection::insert(const std::string_view& key, const std::string& value)
{
	ConfigurationElementType type;
	switch (key[0]) {
		case 'S':
		case 's': type = ConfigurationElementType::CONF_STRING; break;
		case 'B':
		case 'b': type = ConfigurationElementType::CONF_BOOL; break;
		case 'I':
		case 'i': type = ConfigurationElementType::CONF_INT; break;
		case 'U':
		case 'u': type = ConfigurationElementType::CONF_UINT; break;
		case 'F':
		case 'f': type = ConfigurationElementType::CONF_FLOAT; break;
		default: throw std::runtime_error("Invalid INI configuration value name! Must begin with 's', 'b', 'f', 'i' or 'u'!");
	}
	container.emplace(key,ConfigurationElement(type,value));
}

ConfigurationSection::iterator ConfigurationSection::begin()
{
	return container.begin();
}

ConfigurationSection::const_iterator ConfigurationSection::begin() const
{
	return container.begin();
}

ConfigurationSection::iterator ConfigurationSection::end()
{
	return container.end();
}

ConfigurationSection::const_iterator ConfigurationSection::end() const
{
	return container.end();
}

ConfigurationSection::reverse_iterator ConfigurationSection::rbegin()
{
	return container.rbegin();
}

ConfigurationSection::const_reverse_iterator ConfigurationSection::rbegin() const
{
	return container.rbegin();
}

ConfigurationSection::reverse_iterator ConfigurationSection::rend()
{
	return container.rend();
}

ConfigurationSection::const_reverse_iterator ConfigurationSection::rend() const
{
	return container.rend();
}

ConfigurationSection::iterator ConfigurationSection::find(const std::string_view& key)
{
	return container.find(key);
}

ConfigurationSection::const_iterator ConfigurationSection::find(const std::string_view& key) const
{
	return container.find(key);
}

}
}
