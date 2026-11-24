#include "JsException.hpp"
#include "JsUtil.hpp"
#include <sstream>

namespace JS {
Exception::Exception(JSContext* cx)
{
	JS::BorrowedErrorReport borrowed(cx);
	bool isExecptionPending = JS_IsExceptionPending(cx);
	std::stringstream errorstream;
	errorstream << "Uh-oh, stinky!" << std::endl;
	if(isExecptionPending) {
		JS::RootedValue exception(cx);
		if(JS_GetPendingException(cx,&exception) && exception.isObject()) {
		   JS::Rooted<JSObject*> exceptionObject(cx,&exception.toObject());
		   bool expExtracted = JS_ErrorFromException(cx,exceptionObject,borrowed);
		   if(expExtracted) {
			   auto what = borrowed->message();
			   errorstream << "Error on file \"" << borrowed->filename.c_str() << "\", line " << borrowed->lineno << ", column " << borrowed->column.oneOriginValue() << std::endl;
			   errorstream << "Error message: " << what.c_str() << std::endl;
		   }
		}
	}
	JS_ClearPendingException(cx);
	err_str = errorstream.str();
}

Exception::Exception(JSContext *cx, JS::HandleValue error)
{
	JS::BorrowedErrorReport borrowed(cx);
	std::stringstream errorstream;
	errorstream << "Uh-oh, stinky!" << std::endl;
	JS::Rooted<JSObject*> exceptionObject(cx,&error.toObject());
	bool expExtracted = JS_ErrorFromException(cx,exceptionObject,borrowed);
	if(expExtracted) {
		auto what = borrowed->message();
		errorstream << "Error on file \"" << borrowed->filename.c_str() << "\", line " << borrowed->lineno << ", column " << borrowed->column.oneOriginValue() << std::endl;
		errorstream << "Error message: " << what.c_str() << std::endl;
	}
	err_str = errorstream.str();
}

Exception::Exception(JSContext *cx, const char *message, JSErrorReport *report)
{
	std::stringstream errorstream;
	errorstream << "Uh-oh, stinky!" << std::endl;
	if(report) {
		errorstream << "Error on file \"" << report->filename.c_str() << "\", line " << report->lineno << ", column " << report->column.oneOriginValue() << std::endl;
		errorstream << "Error message: " << report->message().c_str() << std::endl;
	}
	err_str = errorstream.str();
}

const char *Exception::what() const noexcept
{
	return err_str.c_str();
}
}
