#ifndef RTPROCESSORSTACK_HPP
#define RTPROCESSORSTACK_HPP
#include "RichTextProcessor.hpp"
#include <stack>

class RtProcessorStack
{
	std::stack<glm::fvec4> colourStack;
	std::stack<std::string> fontStack;
	int underscoreStack;
	int boldStack;
	int italicStack;
	int strikethroughStack;
public:
	RtProcessorStack();
};

#endif // RTPROCESSORSTACK_HPP
