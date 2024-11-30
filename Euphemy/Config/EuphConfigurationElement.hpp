#ifndef EUPHCONFIGURATIONELEMENT_H
#define EUPHCONFIGURATIONELEMENT_H
#include <Euphemy/Config/EuphLib.hpp>
#include <cstdint>
#include <string>
#include <array>
#include <sstream>
namespace Euph {
namespace Conf {
//#define LARGE_CONF_DATA
#ifdef LARGE_CONF_DATA
	typedef double ConfFloat;
	typedef int64_t ConfInt;
	typedef uint64_t ConfUint;
#else
	typedef float ConfFloat;
	typedef int32_t ConfInt;
	typedef uint32_t ConfUint;
#endif

class MH_EUPH_API SizedString {
public:
	static constexpr const ConfUint MAX_STRING_POOL_SIZE = 4 * 1024 * 1024;
#ifdef LARGE_CONF_DATA
	static constexpr const ConfUint LENGTH_BITS = 32;
#else
	static constexpr const ConfUint LENGTH_BITS = 12;
#endif
	static_assert (LENGTH_BITS <= sizeof(ConfUint)*4,"LENGTH_BITS must be half or less the size of ConfUint!");
	static constexpr const ConfUint INDEX_BITS = static_cast<ConfUint>(sizeof(ConfUint)*8) - LENGTH_BITS;
	static constexpr const ConfUint LENGTH_MASK = (static_cast<ConfUint>(1) << LENGTH_BITS) - 1;
	static constexpr const ConfUint INDEX_MASK = ~LENGTH_MASK;
	static constexpr const ConfUint STRING_POOL_SIZE = std::min((static_cast<ConfUint>(1) << INDEX_BITS), MAX_STRING_POOL_SIZE);
	typedef std::array<char,STRING_POOL_SIZE> StringPool;
	ConfUint container;
	static StringPool string_pool;
	void unpack(ConfUint& startIndex, ConfUint& length) const;
	void pack(ConfUint startIndex, ConfUint length);
	std::string_view asStringView() const;
	std::string copyIntoString() const;
	const char* c_str() const;
	void fromStringView(const std::string_view& strView);
};

enum class ConfigurationElementType : uint8_t {
	CONF_FLOAT,
	CONF_INT,
	CONF_UINT,
	CONF_BOOL,
	CONF_STRING
};

union ConfigurationElementUnion
{
	ConfFloat f;
	ConfInt i;
	ConfUint u;
	bool b;
	SizedString s;
};

class MH_EUPH_API ConfigurationElement {
public:
	ConfigurationElementType type;
	ConfigurationElementUnion storage;
	ConfigurationElement(const ConfigurationElement& cpy);
	ConfigurationElement& operator=(const ConfigurationElement& cpy);
	ConfigurationElement(ConfigurationElementType type, const std::string_view& str);
	explicit ConfigurationElement(ConfFloat flt);
	explicit ConfigurationElement(ConfInt nnt);
	explicit ConfigurationElement(ConfUint uint);
	explicit ConfigurationElement(bool boolean);
	explicit ConfigurationElement(const std::string_view& strview);
};

}
}
template<class CharT, class Traits = std::char_traits<CharT>>
std::basic_ostream<CharT,Traits>& operator<<(std::basic_ostream<CharT,Traits>& left, Euph::Conf::ConfigurationElement& right) {
	switch (right.type) {
		case Euph::Conf::ConfigurationElementType::CONF_FLOAT:
			left << right.storage.f;
			break;
		case Euph::Conf::ConfigurationElementType::CONF_INT:
			left << right.storage.i;
			break;
		case Euph::Conf::ConfigurationElementType::CONF_UINT:
			left << right.storage.u;
			break;
		case Euph::Conf::ConfigurationElementType::CONF_BOOL:
			left << right.storage.b;
			break;
		case Euph::Conf::ConfigurationElementType::CONF_STRING:
			left << right.storage.s.asStringView();
			break;
	}
	return left;
}

#endif // EUPHCONFIGURATIONELEMENT_H
