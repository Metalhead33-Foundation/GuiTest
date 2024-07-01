#ifndef JSEXCEPTION_MH33_HPP
#define JSEXCEPTION_MH33_HPP
#include <exception>
#include <jsapi.h>

namespace JS {
class Exception : public std::exception
{
private:
	std::string err_str;
public:
	Exception(JSContext *cx);
	Exception(JSContext* cx, JS::HandleValue error);
	Exception(JSContext* cx, const char* message, JSErrorReport* report);
	const char *what() const noexcept;
};

}

#endif // JSEXCEPTION_MH33_HPP
