#ifndef ITEXTRECEIVER_H
#define ITEXTRECEIVER_H
#include <string>
#include <string_view>
#include <MhLib/Util/MhGlobals.hpp>

DEFINE_CLASS(ITextReceiver)
class ITextReceiver {
public:
	virtual ~ITextReceiver() = default;
	virtual bool onText(const std::string& str) = 0;
	virtual bool onText(const std::string_view& strview) = 0;
	virtual bool onEnter() = 0;
	virtual bool onBackspace() = 0;
	virtual std::string getString() const = 0;
	virtual std::string_view getStringView() const = 0;
};

#endif // ITEXTRECEIVER_H
