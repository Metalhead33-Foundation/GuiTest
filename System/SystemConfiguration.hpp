#ifndef SYSTEMCONFIGURATION_HPP
#define SYSTEMCONFIGURATION_HPP
#include <string>
#include <map>
#include <fstream>
#include <functional>

namespace SYS {

struct ConfigurationSection {
	std::map<std::string,std::string> values;
	const std::string& getValueOrDefaultStr(const std::string& key, const std::string& defaultValue = "") const;
	int getValueOrDefaultInt(const std::string& key, int defaultValue = 0) const;
	double getValueOrDefaultDouble(const std::string& key, double defaultValue = 0.0) const;
	bool getValueOrDefaulBoolean(const std::string& key, bool defaultValue = false) const;
};
class Configuration
{
private:
	std::map<std::string,ConfigurationSection> sections;
	void insertDefaultSections();
public:
	Configuration();
	Configuration(std::ifstream& file);
	void iterateOverSections(const std::function<void(const std::string&)>& onSectionStart, const std::function<void(const std::string&,const std::string&,const std::string&)>& onValue);
	const std::map<std::string, ConfigurationSection>& getSections() const;
	std::map<std::string, ConfigurationSection>& getSections();
};

}
#endif // SYSTEMCONFIGURATION_HPP
