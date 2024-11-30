#include "EuphConfigurationElement.hpp"
#include <cstring>
#include <charconv>
namespace Euph {
namespace Conf {

SizedString::StringPool SizedString::string_pool;
void SizedString::unpack(ConfUint& startIndex, ConfUint& length) const
{
	startIndex = (container & INDEX_MASK) >> LENGTH_BITS;
	length = container & LENGTH_MASK;
}

void SizedString::pack(ConfUint startIndex, ConfUint length)
{
	container = startIndex;
	container <<= LENGTH_BITS;
	container |= (length & LENGTH_MASK);
}

std::string_view SizedString::asStringView() const
{
	ConfUint startIndex, length;
	unpack(startIndex, length);
	return std::string_view(&string_pool[startIndex],length);
}

std::string SizedString::copyIntoString() const
{
	ConfUint startIndex, length;
	unpack(startIndex, length);
	return std::string(&string_pool[startIndex], length);
}

const char* SizedString::c_str() const
{
	ConfUint startIndex, length;
	unpack(startIndex, length);
	return &string_pool[startIndex];
}
static ConfUint CURRENT_STRING_INDEX = 0;
void SizedString::fromStringView(const std::string_view& strView)
{
	ConfUint index = CURRENT_STRING_INDEX;
	ConfUint length = strView.length();
	std::memcpy(&string_pool[index],strView.data(),length);
	string_pool[index+length] = 0;
	CURRENT_STRING_INDEX += length+1;
	pack(index,length);
}

ConfigurationElement::ConfigurationElement(const ConfigurationElement& cpy)
	: type(cpy.type), storage(cpy.storage)
{

}

ConfigurationElement& ConfigurationElement::operator=(const ConfigurationElement& cpy)
{
	this->type = cpy.type;
	this->storage = cpy.storage;
	return *this;
}

ConfigurationElement::ConfigurationElement(ConfigurationElementType type, const std::string_view& str)
	: type(type)
{
	switch (type) {
		case ConfigurationElementType::CONF_FLOAT:
		{
			ConfFloat tmp;
			std::from_chars(str.data(),str.data() + str.size(),tmp);
			storage.f = tmp;
			break;
		}
		case ConfigurationElementType::CONF_INT:
		{
			ConfInt tmp;
			std::from_chars(str.data(),str.data() + str.size(),tmp);
			storage.i = tmp;
			break;
		}
		case ConfigurationElementType::CONF_UINT:
		{
			ConfUint tmp;
			std::from_chars(str.data(),str.data() + str.size(),tmp);
			storage.u = tmp;
			break;
		}
		case ConfigurationElementType::CONF_BOOL:
		{
			bool tmp;
			if(str == "true") tmp = true;
			else if(str == "TRUE") tmp = true;
			else if(str == "false") tmp = false;
			else if(str == "FALSE") tmp = false;
			else if(str == "yes") tmp = true;
			else if(str == "YES") tmp = true;
			else if(str == "no") tmp = false;
			else if(str == "NO") tmp = false;
			else {
				int tmpInt;
				std::from_chars(str.data(),str.data() + str.size(),tmpInt);
				tmp = static_cast<bool>(tmpInt);
			}
			storage.b = tmp;
			break;
		}
		case ConfigurationElementType::CONF_STRING:
		{
			SizedString tmp;
			tmp.fromStringView(str);
			storage.s = tmp;
			break;
		}
	}
}
ConfigurationElement::ConfigurationElement(ConfFloat flt)
	: type(ConfigurationElementType::CONF_FLOAT)
{
	storage.f = flt;
}
ConfigurationElement::ConfigurationElement(ConfInt nnt)
	: type(ConfigurationElementType::CONF_INT)
{
	storage.i = nnt;
}
ConfigurationElement::ConfigurationElement(ConfUint uint)
	: type(ConfigurationElementType::CONF_UINT)
{
	storage.u = uint;
}
ConfigurationElement::ConfigurationElement(bool boolean)
	: type(ConfigurationElementType::CONF_BOOL)
{
	storage.b = boolean;
}
ConfigurationElement::ConfigurationElement(const std::string_view& strview)
	: type(ConfigurationElementType::CONF_STRING)
{
	storage.s.fromStringView(strview);
}

}
}
