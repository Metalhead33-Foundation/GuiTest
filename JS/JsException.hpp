#ifndef JSEXCEPTION_MH33_HPP
#define JSEXCEPTION_MH33_HPP
#include <exception>
#include <jsapi.h>

namespace JS {
class StdException : public std::exception
{
private:
	std::string err_str;
public:
	StdException(JSContext *cx);
	StdException(JSContext* cx, JS::HandleValue error);
	StdException(JSContext* cx, const char* message, JSErrorReport* report);
	const char *what() const noexcept;
};

}

#endif // JSEXCEPTION_MH33_HPP
