#ifndef EUPHCONFIGURATION_H
#define EUPHCONFIGURATION_H
#include <Euphemy/Config/EuphConfigurationElement.hpp>
#include <map>
namespace Euph {
namespace Conf {

/**
 * @class ConfigurationSection
 * @brief Represents a section in a configuration file with key-value pairs
 *
 * This class provides storage and access methods for configuration elements
 * organized as key-value pairs within a section.
 */
class MH_EUPH_API ConfigurationSection {
public:
	/// Container type for configuration elements
	typedef std::map<std::string,ConfigurationElement,std::less<>> SectionContainer;
	/// Iterator type
	typedef SectionContainer::iterator iterator;
	/// Const iterator type
	typedef SectionContainer::const_iterator const_iterator;
	/// Reverse iterator type
	typedef SectionContainer::reverse_iterator reverse_iterator;
	/// Const reverse iterator type
	typedef SectionContainer::const_reverse_iterator const_reverse_iterator;

private:
	SectionContainer container; ///< Internal storage for configuration elements

public:
	/// Default constructor
	ConfigurationSection();
	/// Copy constructor
	ConfigurationSection(const ConfigurationSection& cpy);
	/// Move constructor
	ConfigurationSection(ConfigurationSection&& mov);
	/// Copy assignment operator
	ConfigurationSection& operator=(const ConfigurationSection& cpy);
	/// Move assignment operator
	ConfigurationSection operator=(ConfigurationSection&& mov);

	/**
	 * @brief Get the internal container (mutable version)
	 * @return Reference to the internal container
	 */
	SectionContainer& getContainer();

	/**
	 * @brief Get the internal container (const version)
	 * @return Const reference to the internal container
	 */
	const SectionContainer& getContainer() const;

	// Setters
	/**
	 * @brief Set a string value
	 * @param key Key for the value
	 * @param value String value to set
	 */
	void setString(const std::string_view& key, const std::string_view value);

	/**
	 * @brief Set a floating point value
	 * @param key Key for the value
	 * @param value Floating point value to set
	 */
	void setFloat(const std::string_view& key, ConfFloat value);

	/**
	 * @brief Set an integer value
	 * @param key Key for the value
	 * @param value Integer value to set
	 */
	void setInt(const std::string_view& key, ConfInt value);

	/**
	 * @brief Set an unsigned integer value
	 * @param key Key for the value
	 * @param value Unsigned integer value to set
	 */
	void setUint(const std::string_view& key, ConfUint value);

	/**
	 * @brief Set a boolean value
	 * @param key Key for the value
	 * @param value Boolean value to set
	 */
	void setBool(const std::string_view& key, ConfUint value);

	// Getters
	/**
	 * @brief Get a string value
	 * @param key Key to look up
	 * @param def Default value if key not found
	 * @return The string value or default if not found
	 */
	std::string_view getString(const std::string_view& key, const std::string_view& def="");

	/**
	 * @brief Get a floating point value
	 * @param key Key to look up
	 * @param def Default value if key not found
	 * @return The float value or default if not found
	 */
	ConfFloat getFloat(const std::string_view& key, ConfFloat def=static_cast<ConfFloat>(0));

	/**
	 * @brief Get an integer value
	 * @param key Key to look up
	 * @param def Default value if key not found
	 * @return The integer value or default if not found
	 */
	ConfInt getInteger(const std::string_view& key, ConfInt def=static_cast<ConfInt>(0));

	/**
	 * @brief Get an unsigned integer value
	 * @param key Key to look up
	 * @param def Default value if key not found
	 * @return The unsigned integer value or default if not found
	 */
	ConfUint getUInteger(const std::string_view& key, ConfUint def=static_cast<ConfUint>(0));

	/**
	 * @brief Get a boolean value
	 * @param key Key to look up
	 * @param def Default value if key not found
	 * @return The boolean value or default if not found
	 */
	bool getBool(const std::string_view& key, bool def=false);

	// Generic setter
	/**
	 * @brief Insert a key-value pair (string_view version)
	 * @param key Key to insert
	 * @param value Value to insert
	 */
	void insert(const std::string_view& key, const std::string_view value);

	/**
	 * @brief Insert a key-value pair (string version)
	 * @param key Key to insert
	 * @param value Value to insert
	 */
	void insert(const std::string_view& key, const std::string& value);

	// Convenience iterators
	iterator begin(); ///< Returns an iterator to the beginning
	const_iterator begin() const; ///< Returns a const iterator to the beginning
	iterator end(); ///< Returns an iterator to the end
	const_iterator end() const; ///< Returns a const iterator to the end
	reverse_iterator rbegin(); ///< Returns a reverse iterator to the beginning
	const_reverse_iterator rbegin() const; ///< Returns a const reverse iterator to the beginning
	reverse_iterator rend(); ///< Returns a reverse iterator to the end
	const_reverse_iterator rend() const; ///< Returns a const reverse iterator to the end

	/**
	 * @brief Find an element by key (mutable version)
	 * @param key Key to search for
	 * @return Iterator to the element or end() if not found
	 */
	iterator find(const std::string_view& key);

	/**
	 * @brief Find an element by key (const version)
	 * @param key Key to search for
	 * @return Const iterator to the element or end() if not found
	 */
	const_iterator find(const std::string_view& key) const;
};

/**
 * @class Configuration
 * @brief Represents a complete configuration with multiple sections
 *
 * This class provides storage and access methods for configuration data
 * organized in sections, each containing key-value pairs.
 */
class MH_EUPH_API Configuration
{
public:
	/// Container type for configuration sections
	typedef std::map<std::string,ConfigurationSection,std::less<>> SectionMap;
	/// Iterator type
	typedef SectionMap::iterator iterator;
	/// Const iterator type
	typedef SectionMap::const_iterator const_iterator;
	/// Reverse iterator type
	typedef SectionMap::reverse_iterator reverse_iterator;
	/// Const reverse iterator type
	typedef SectionMap::const_reverse_iterator const_reverse_iterator;

private:
	SectionMap sections; ///< Internal storage for configuration sections

public:
	/// Default constructor
	Configuration();
	/// Copy constructor
	Configuration(const Configuration& cpy);
	/// Move constructor
	Configuration(Configuration&& mov);
	/// Copy assignment operator
	Configuration& operator=(const Configuration& cpy);
	/// Move assignment operator
	Configuration operator=(Configuration&& mov);

	/**
	 * @brief Construct from input stream
	 * @param sstream Input stream to parse configuration from
	 */
	Configuration(std::istream& sstream);

	/**
	 * @brief Construct from string
	 * @param strng String containing configuration data
	 */
	Configuration(const std::string& strng);

	/**
	 * @brief Get the internal sections map (mutable version)
	 * @return Reference to the internal sections map
	 */
	SectionMap& getSections();

	/**
	 * @brief Get the internal sections map (const version)
	 * @return Const reference to the internal sections map
	 */
	const SectionMap& getSections() const;

	/**
	 * @brief Parse configuration from input stream
	 * @param sstream Input stream to parse
	 */
	void fromString(std::istream& sstream);

	/**
	 * @brief Parse configuration from string
	 * @param strng String containing configuration data
	 */
	void fromString(const std::string& strng);

	// Convenience iterators
	iterator begin(); ///< Returns an iterator to the beginning
	const_iterator begin() const; ///< Returns a const iterator to the beginning
	iterator end(); ///< Returns an iterator to the end
	const_iterator end() const; ///< Returns a const iterator to the end
	reverse_iterator rbegin(); ///< Returns a reverse iterator to the beginning
	const_reverse_iterator rbegin() const; ///< Returns a const reverse iterator to the beginning
	reverse_iterator rend(); ///< Returns a reverse iterator to the end
	const_reverse_iterator rend() const; ///< Returns a const reverse iterator to the end

	/**
	 * @brief Find a section by name (mutable version)
	 * @param key Section name to search for
	 * @return Iterator to the section or end() if not found
	 */
	iterator find(const std::string_view& key);

	/**
	 * @brief Find a section by name (const version)
	 * @param key Section name to search for
	 * @return Const iterator to the section or end() if not found
	 */
	const_iterator find(const std::string_view& key) const;

	// Getters
	/**
	 * @brief Get a string value from a section
	 * @param section Section name
	 * @param key Key to look up
	 * @param def Default value if key not found
	 * @return The string value or default if not found
	 */
	std::string_view getString(const std::string_view& section, const std::string_view& key, const std::string_view& def="");

	/**
	 * @brief Get a floating point value from a section
	 * @param section Section name
	 * @param key Key to look up
	 * @param def Default value if key not found
	 * @return The float value or default if not found
	 */
	ConfFloat getFloat(const std::string_view& section, const std::string_view& key, ConfFloat def=static_cast<ConfFloat>(0));

	/**
	 * @brief Get an integer value from a section
	 * @param section Section name
	 * @param key Key to look up
	 * @param def Default value if key not found
	 * @return The integer value or default if not found
	 */
	ConfInt getInteger(const std::string_view& section, const std::string_view& key, ConfInt def=static_cast<ConfInt>(0));

	/**
	 * @brief Get an unsigned integer value from a section
	 * @param section Section name
	 * @param key Key to look up
	 * @param def Default value if key not found
	 * @return The unsigned integer value or default if not found
	 */
	ConfUint getUInteger(const std::string_view& section, const std::string_view& key, ConfUint def=static_cast<ConfUint>(0));

	/**
	 * @brief Get a boolean value from a section
	 * @param section Section name
	 * @param key Key to look up
	 * @param def Default value if key not found
	 * @return The boolean value or default if not found
	 */
	bool getBool(const std::string_view& section, const std::string_view& key, bool def=false);
};

} // namespace Conf
} // namespace Euph

/**
 * @brief Output operator for ConfigurationSection
 * @tparam CharT Character type
 * @tparam Traits Character traits
 * @param left Output stream
 * @param right ConfigurationSection to output
 * @return Reference to the output stream
 */
template<class CharT, class Traits = std::char_traits<CharT>>
std::basic_ostream<CharT,Traits>& operator<<(std::basic_ostream<CharT,Traits>& left, Euph::Conf::ConfigurationSection& right) {
	for(auto it = std::begin(right) ; it != std::end(right); ++it)
	{
		left << it->first << '=' << it->second << std::endl;
	}
	return left;
}

/**
 * @brief Output operator for Configuration
 * @tparam CharT Character type
 * @tparam Traits Character traits
 * @param left Output stream
 * @param right Configuration to output
 * @return Reference to the output stream
 */
template<class CharT, class Traits = std::char_traits<CharT>>
std::basic_ostream<CharT,Traits>& operator<<(std::basic_ostream<CharT,Traits>& left, Euph::Conf::Configuration& right) {
	for(auto it = std::begin(right) ; it != std::end(right); ++it)
	{
		left << '[' << it->first << ']' << '\n' << it->second << std::endl;
	}
	return left;
}

#endif // EUPHCONFIGURATION_H
