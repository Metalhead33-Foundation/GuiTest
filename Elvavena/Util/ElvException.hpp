#ifndef ELVSTREXCEPTION_HPP
#define ELVSTREXCEPTION_HPP
#include <Elvavena/Util/ElvAllocatorBasic.hpp>
#include <exception>
#include <functional>
#include <sstream>
namespace Elv {
namespace Util {

template <typename Alloc = std::allocator<char>> requires Allocator<Alloc, char>
class Exception : public std::exception
{
public:
	typedef std::basic_string<char, std::char_traits<char>, Alloc> String;
	typedef std::basic_stringstream<char, std::char_traits<char>, Alloc> StringStream;
	typedef std::function<void(StringStream&)> StringStreamFiller;
private:
	String str;
public:
	Exception(StringStreamFiller filler)
	{
		StringStream stream;
		filler(stream);
		str = stream.str();
	}
	const char* what() const noexcept override
	{
		return str.c_str();
	}
};

}
}

#endif // ELVSTREXCEPTION_HPP
