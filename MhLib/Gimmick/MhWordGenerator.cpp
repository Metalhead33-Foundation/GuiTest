#include <MhLib/Gimmick/MhWordGenerator.hpp>
#include <sstream>
#include <array>

namespace MH33 {
namespace Gimmick {

static const std::array<char,5> VOWELS = { 'a', 'e', 'o', 'i', 'u' };
static const std::array<char,21> CONSONANTS = { 'b', 'c', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
											 'm', 'n', 'p', 'q', 'r', 's', 't', 'v', 'w', 'x', 'y', 'z' };
static char getCharacter(Util::RNG& rng, unsigned& vowelCounter, unsigned& consonantCounter, bool isInitial) {
	bool willBeVowel;
	if(vowelCounter >= 2) willBeVowel = false;
	else if(consonantCounter >= 3 || isInitial) willBeVowel = true;
	else {
		willBeVowel = rng.generateFloat() >= 0.5f;
	}
	if(willBeVowel) {
		consonantCounter = 0;
		++vowelCounter;
		return VOWELS[rng.generate(VOWELS.size())];
	} else {
		vowelCounter = 0;
		++consonantCounter;
		return CONSONANTS[rng.generate(CONSONANTS.size())];
	}
}

WordGenerator::WordGenerator(uint32_t seed)
	: rng(seed)
{

}

WordGenerator::WordGenerator(const Util::RNG &nrng)
	: rng(nrng)
{

}

WordGenerator::WordGenerator(const WordGenerator &cpy)
	: rng(cpy.rng)
{

}

std::string WordGenerator::generate(unsigned minLength, unsigned maxLength, CapitalizationPolicy capitalizationPolicy)
{
	const unsigned len = (minLength == maxLength) ? minLength : rng.generate(minLength,maxLength);
	unsigned vowelCounter = 0;
	unsigned consonantCounter = 0;
	std::stringstream stream;
	for(unsigned i = 0; i < len; ++i) {
		stream << getCharacter(rng,vowelCounter,consonantCounter,i == 0);
	}
	switch (capitalizationPolicy) {
	case CapitalizationPolicy::DONT: return stream.str();
	case CapitalizationPolicy::ALL: {
		std::string tmp = stream.str();
		for(auto& it : tmp) it = toupper(it);
		return tmp;
	}
	case CapitalizationPolicy::START: {
		std::string tmp = stream.str();
		tmp[0] = toupper(tmp[0]);
		return tmp;
	}
	default: return std::string();
	}
	return std::string();
}

}
}
