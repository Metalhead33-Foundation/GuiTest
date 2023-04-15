#ifndef RTPROCESSORSTACK_HPP
#define RTPROCESSORSTACK_HPP
#include "RichTextProcessor.hpp"
#include <stack>

namespace TXT {
DEFINE_CLASS(RtProcessorStack)
class RtProcessorStack
{
private:
	std::stack<glm::fvec4> colourStack;
	std::stack<std::string> fontStack;
	std::stack<int> sizeStack;
	unsigned underscoreStack;
	unsigned boldStack;
	unsigned italicStack;
	unsigned strikethroughStack;
protected:
	RichTextProcessor* RTP;
public:
	RtProcessorStack();
	RtProcessorStack(RichTextProcessor* nRTP);
	RichTextProcessor* getRTP() const;
	void setRTP(RichTextProcessor* newRTP);
	// Push
	void pushColour(const glm::fvec4& rgba);
	void pushColour(const glm::fvec3& rgb);
	void pushColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF);
	void pushFont(const std::string& fontname);
	void pushSize(int size);
	void pushUnderscore();
	void pushBold();
	void pushItalic();
	void pushStrikethrough();
	// Pop
	void popColour();
	void popFont();
	void popSize();
	void popUnderscore();
	void popBold();
	void popItalic();
	void popStrikethrough();
};
}
#endif // RTPROCESSORSTACK_HPP
