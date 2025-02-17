#ifndef ELVSTREXCEPTION_HPP
#define ELVSTREXCEPTION_HPP
#include <Elvavena/Util/ElvAllocatorBasic.hpp>
#include <exception>
#include <functional>
#include <sstream>
namespace Elv {
namespace Util {

/**
 * @brief A custom exception class that uses a custom allocator.
 *
 * This class extends the standard `std::exception` class and allows for the use
 * of a custom allocator for its internal string storage.
 *
 * @tparam Alloc The allocator type to use for the internal string storage.
 *               Defaults to `std::allocator<char>`.
 */
template <typename Alloc = std::allocator<char>> requires Allocator<Alloc, char>
class Exception : public std::exception
{
public:
	/**
	 * @brief Typedef for the string type using the specified allocator.
	 */
	typedef std::basic_string<char, std::char_traits<char>, Alloc> String;

	/**
	 * @brief Typedef for the string stream type using the specified allocator.
	 */
	typedef std::basic_stringstream<char, std::char_traits<char>, Alloc> StringStream;

	/**
	 * @brief Typedef for a function that fills a string stream.
	 *
	 * The function takes a `StringStream` reference and fills it with the
	 * desired exception message.
	 */
	typedef std::function<void(StringStream&)> StringStreamFiller;

private:
	/**
	 * @brief The internal string storage for the exception message.
	 */
	String str;

public:
	/**
	 * @brief Constructs an exception with a custom message filled by a filler function.
	 *
	 * @param filler A function that fills the string stream with the exception message.
	 */
	Exception(StringStreamFiller filler)
	{
		StringStream stream;
		filler(stream);
		str = stream.str();
	}

	/**
	 * @brief Returns the exception message.
	 *
	 * This method overrides the `what()` method from `std::exception` to return
	 * the custom exception message.
	 *
	 * @return A pointer to the exception message.
	 */
	const char* what() const noexcept override
	{
		return str.c_str();
	}
};

}
}

#endif // ELVSTREXCEPTION_HPP
