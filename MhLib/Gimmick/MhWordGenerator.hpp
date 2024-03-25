#ifndef WORDGENERATOR_HPP
#define WORDGENERATOR_HPP
#include <MhLib/Util/MhRNG.hpp>
#include <MhLib/Gimmick/MhGimmick.hpp>

namespace MH33 {
namespace Gimmick {

/*! Capitalization policy for the Word Generator.*/
enum class CapitalizationPolicy {
	/*! All letters are minuscule.*/
	DONT,
	/*! All letters are capitalized.*/
	ALL,
	/*! Only the first letter is capitalized.*/
	START
};

DEFINE_CLASS(WordGenerator)
/*! A class for generating random words.*/
class MH_GIMMICK_API WordGenerator
{
private:
	/*! The random number generator used for generating random words.*/
	Util::RNG rng;
public:
	//! Constructor.
	/*!
	  \param seed RNG seed.
	*/
	WordGenerator(uint32_t seed = 0);
	//! Constructor.
	/*!
	  \param nrng A reference to a random-number generator.
	*/
	WordGenerator(const Util::RNG& nrng);
	//! Copy-constructor.
	/*!
	  \param nrng A reference to another word generator.
	*/
	WordGenerator(const WordGenerator& cpy);
	//! Generates a random word.
	/*!
	  \param minLength Minimum word length.
	  \param maxLength Maximum word length.
	  \param capitalizationPolicy Capitalization policy.
	  \return The generated word.
	*/
	std::string generate(unsigned minLength, unsigned maxLength, CapitalizationPolicy capitalizationPolicy = CapitalizationPolicy::DONT);
};

}
}

#endif // WORDGENERATOR_HPP
