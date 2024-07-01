#include "JsException.hpp"
#include <sstream>

namespace JS {
StdException::StdException(JSContext* cx)
{
	bool isExecptionPending = JS_IsExceptionPending(cx);
	std::stringstream errorstream;
	errorstream << "Uh-oh, stinky!" << std::endl;
	if(isExecptionPending) {
		JS::RootedValue exception(cx);
		if(JS_GetPendingException(cx,&exception) && exception.isObject()) {
		   JS::Rooted<JSObject*> exceptionObject(cx,&exception.toObject());
		   JSErrorReport *what = JS_ErrorFromException(cx,exceptionObject);
		   if(what) {
			   errorstream << "Error on file \"" << what->filename << "\", line " << what->lineno << ", column " << what->column << std::endl;
			   errorstream << "Error message: " << what->message().c_str() << std::endl;
		   }
		}
	}
	JS_ClearPendingException(cx);
	err_str = errorstream.str();
}

StdException::StdException(JSContext *cx, JS::HandleValue error)
{
	std::stringstream errorstream;
	errorstream << "Uh-oh, stinky!" << std::endl;
	JS::Rooted<JSObject*> exceptionObject(cx,&error.toObject());
	JSErrorReport *what = JS_ErrorFromException(cx,exceptionObject);
	if(what) {
	   errorstream << "Error on file \"" << what->filename << "\", line " << what->lineno << ", column " << what->column << std::endl;
	   errorstream << "Error message: " << what->message().c_str() << std::endl;
	}
	err_str = errorstream.str();
}

StdException::StdException(JSContext *cx, const char *message, JSErrorReport *report)
{
	std::stringstream errorstream;
	errorstream << "Uh-oh, stinky!" << std::endl;
	if(report) {
		errorstream << "Error on file \"" << report->filename << "\", line " << report->lineno << ", column " << report->column << std::endl;
		errorstream << "Error message: " << report->message().c_str() << std::endl;
	}
	err_str = errorstream.str();
}

const char *StdException::what() const noexcept
{
	return err_str.c_str();
}
}
