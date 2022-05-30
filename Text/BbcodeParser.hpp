#ifndef BBCODEPARSER_H
#define BBCODEPARSER_H
#include "RtProcessorStack.hpp"
#include <sstream>

class BbcodeParser : public RtProcessorStack
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
	BbcodeParser();
	BbcodeParser(RichTextProcessor* nRTP);
	void parse(char c);
	void parse(const char* cstr);
	void parse(const std::string& str);
};

#endif // BBCODEPARSER_H
