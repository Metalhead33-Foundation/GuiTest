#ifndef MMLPARSER_H
#define MMLPARSER_H
#include <MhLib/Text/MhRtProcessorStack.hpp>
#include <sstream>
namespace MH33 {
namespace TXT {
DEFINE_CLASS(MmlParser)
class MmlParser : public RtProcessorStack
{
private:
	std::stringstream commandStream;
	void onTagEnd();
	void processTag(const std::string& tag);
	void onEscapeEnd();
	void processEscape(const std::string& escape);
	bool isProcessingTag;
	bool isProcessingEscape;
public:
	MmlParser();
	MmlParser(RichTextProcessor* nRTP);
	void parse(char c);
	void parse(const char* cstr);
	void parse(const std::string& str);
};
}
}
#endif // MMLPARSER_H
